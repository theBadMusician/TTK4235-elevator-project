#include "fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"
#include "utils.h"

#include "driver/elevio.h"
#include "lib/log.h"

#define TOTAL_BUTTONS (N_FLOORS * N_BUTTONS) 

// FSM private singleton state
static ElevatorState  currentState  = STATE_INIT;
static int            currentFloor  = 0;
static int            prevFloor     = 0;
static MotorDirection currentDir    = DIRN_STOP;
static MotorDirection prevDir       = DIRN_STOP;

static bool           isStopped     = false;
static bool           isStopPressed = false;

static bool isStopDebouncing = false;


// Queue order array
bool orderArr[N_FLOORS][N_BUTTONS] = { false };

// Timers 
timerAlarm doorOpenTimer;
timerAlarm btnQueryTimer;
timerAlarm stopDebouncerTimer;

// Btn states
bool prevBtnStates[N_FLOORS][N_BUTTONS] = { false };


void fsm_onInit(void) {
  // Driver init 
  #ifdef DEBUG
    log_debug("Initializing elevator...");
  #endif // DEBUG
  elevio_init();

  // Init timers
  timer_init(&doorOpenTimer);
  timer_init(&btnQueryTimer);
  timer_init(&stopDebouncerTimer);

  // Clear button lights
  for(int f = 0; f < N_FLOORS; f++){
    for(int b = 0; b < N_BUTTONS; b++){
      elevio_buttonLamp(f, b, false);
    }
  }

  // Localize a floor (PRD: O1)
  // Blocking, ignores everything else (PRD: O2)
  while (elevio_floorSensor() == -1) {
    elevio_motorDirection(DIRN_DOWN);
    timer_msSleep(1);
  }
  elevio_motorDirection(DIRN_STOP);
  
  // Init previous floor state
  prevFloor = elevio_floorSensor();

  // State transition T_0: STATE_INIT -> STATE_IDLE
  currentState = STATE_IDLE;
}


void fsm_onIdle(void) {
  // Set the the current direction (movement moment) to stop 
  currentDir = DIRN_STOP;

  // Motor is stopped
  elevio_motorDirection(DIRN_STOP);
  
  // Door is closed
  elevio_doorOpenLamp(false);

  if (!areAllElementsFalse(orderArr)) currentState = STATE_MOVING;
}

static bool _requests_above(int floor) {
    for (int f = floor + 1; f < N_FLOORS; f++) {
        for (int b = 0; b < N_BUTTONS; b++) {
            if (orderArr[f][b]) return true;
        }
    }
    return false;
}

static bool _requests_below(int floor) {
    for (int f = 0; f < floor; f++) {
        for (int b = 0; b < N_BUTTONS; b++) {
            if (orderArr[f][b]) return true;
        }
    }
    return false;
}

void fsm_onMoving(void) {
    /* Recovery when stopped between floors */
    if (currentFloor == -1 && isStopped) {
      // If there are no orders, wait
      if (!_requests_above(-1)) { 
          return; 
      }

      // Check if there is an order specifically at the floor we just left
      bool orderAtPrevFloor = orderArr[prevFloor][BUTTON_CAB] || 
                              orderArr[prevFloor][BUTTON_HALL_UP] || 
                              orderArr[prevFloor][BUTTON_HALL_DOWN];

      // Route directly towards the requested floor
      if (prevDir == DIRN_UP) {
        // Hovering above prevFloor
        if (_requests_above(prevFloor)) {
          currentDir = DIRN_UP;
        } else if (orderAtPrevFloor || _requests_below(prevFloor)) {
          currentDir = DIRN_DOWN;
        }
      } 
      else if (prevDir == DIRN_DOWN) {
        // Hovering below prevFloor
        if (_requests_below(prevFloor)) {
          currentDir = DIRN_DOWN;
        } else if (orderAtPrevFloor || _requests_above(prevFloor)) {
          currentDir = DIRN_UP;
        }
      }

      elevio_motorDirection(currentDir);
      isStopped = false;
      return;

      // // If there are orders anywhere, move in the previous direction to find a floor
      // if (_requests_above(-1)) { // Passing -1 checks the whole array
      //     currentDir = prevDir; //(prevDir == DIRN_UP) ? DIRN_DOWN : DIRN_UP; 
      //     elevio_motorDirection(currentDir);
      //     isStopped = false;
      // }
      // return; 
    }

    // Normal operation requires a valid floor reading
    if (currentFloor == -1) return;


    /* Stop logic */
    bool shouldStop = false;
    
    // Always stop for cab calls at the current floor
    if (orderArr[currentFloor][BUTTON_CAB]) shouldStop = true;

    if (currentDir == DIRN_UP) {
        // Stop for UP hall calls, or for DOWN calls if it is the highest request
        if (orderArr[currentFloor][BUTTON_HALL_UP] || 
           (!_requests_above(currentFloor) && orderArr[currentFloor][BUTTON_HALL_DOWN])) {
            shouldStop = true;
        }
    } 
    else if (currentDir == DIRN_DOWN) {
        // Stop for DOWN hall calls, or for UP calls if it is the lowest request
        if (orderArr[currentFloor][BUTTON_HALL_DOWN] || 
           (!_requests_below(currentFloor) && orderArr[currentFloor][BUTTON_HALL_UP])) {
            shouldStop = true;
        }
    }

    if (shouldStop) {
        elevio_motorDirection(DIRN_STOP);
        currentState = STATE_DOOR_OPEN;
        return;
    }

    /* Movement logic */
    if (currentDir == DIRN_UP) {
        if (_requests_above(currentFloor)) {
            currentDir = DIRN_UP;
        } else if (_requests_below(currentFloor)) {
            currentDir = DIRN_DOWN;
        } else {
            currentDir = DIRN_STOP;
            currentState = STATE_IDLE;
        }
    } 
    else if (currentDir == DIRN_DOWN) {
        if (_requests_below(currentFloor)) {
            currentDir = DIRN_DOWN;
        } else if (_requests_above(currentFloor)) {
            currentDir = DIRN_UP;
        } else {
           currentDir = DIRN_STOP;
           currentState = STATE_IDLE;
        }
    }
    // Wake up from a stopped state
    else if (currentDir == DIRN_STOP) {
        if (_requests_above(currentFloor)) {
            currentDir = DIRN_UP;
        } else if (_requests_below(currentFloor)) {
            currentDir = DIRN_DOWN;
        } else {
            currentState = STATE_IDLE;
        }
    }

    // Apply the direction
    if (currentDir != DIRN_STOP) {
        prevDir = currentDir;
    }
    elevio_motorDirection(currentDir);
}

void fsm_onDoorOpen(void) {
  if (timer_isTimeout(&doorOpenTimer) && !elevio_obstruction() && !elevio_stopButton()) {
    timer_stop(&doorOpenTimer);
    elevio_doorOpenLamp(false);
#ifdef DEBUG
    log_debug("Door closed!");
#endif // DEBUG

    // Clear orders for this floor
    for (int b = 0; b < N_BUTTONS; b++) {
      orderArr[currentFloor][b] = false;
      elevio_buttonLamp(currentFloor, b, false);
    }

    // No more orders, transition to idle
    if (areAllElementsFalse(orderArr)) currentState = STATE_IDLE;
    // More orders await, move more
    else currentState = STATE_MOVING;

    // Just finished opening the door, return
    return;
  }
  elevio_doorOpenLamp(true);
#ifdef DEBUG
  log_debug("Opening door for 3 seconds");
#endif // DEBUG

  // Restart timer if obstruction is present (PRD: D4)
  // Restart timer if stop button is pressed (PRD: D3)
  if (elevio_obstruction() || elevio_stopButton()) timer_stop(&doorOpenTimer);
  timer_start(&doorOpenTimer, 3);
}

void fsm_onStop(void) {
    /* Stop the elevator, clear queue, reset state */

    // Set the the current direction to stop and stop motor
    currentDir = DIRN_STOP;
    elevio_motorDirection(currentDir);

    // Clear the order queue and btn states
    memset(orderArr,      false, sizeof(orderArr)); 
    memset(prevBtnStates, false, sizeof(prevBtnStates)); 

    // Clear button lights
    for(int f = 0; f < N_FLOORS; f++){
      for(int b = 0; b < N_BUTTONS; b++){
        elevio_buttonLamp(f, b, false);
      }
    }

    // state transition
    if (currentFloor != -1) {
      elevio_stopLamp(true);
      currentState = STATE_DOOR_OPEN;
    } else currentState = STATE_IDLE;

    // Set stopped param for undefined floor escape
    if (currentFloor == -1) isStopped = true;
}


static void _stopBtnHandler(void) {
  if (currentState == STATE_INIT) {
    isStopPressed = false;
    return;
  }

  isStopPressed = elevio_stopButton();
  
  if (!isStopPressed && timer_isTimeout(&stopDebouncerTimer)) {
    elevio_stopLamp(false);
 
    // Force reset the timer using stop
    timer_stop(&stopDebouncerTimer);

    // Debounce is finished
    isStopDebouncing = false;
  }
  else if (isStopPressed) {
    elevio_stopLamp(true);

    // Reset the timer using stop and debounce
    isStopDebouncing = true;
    timer_stop(&stopDebouncerTimer);
    timer_start(&stopDebouncerTimer, 0.5);

    // Transition to stop state immediately
    currentState = STATE_STOP; 
  }
}

static void _floorHandler(void) {
  // Update floor reading
  currentFloor = elevio_floorSensor();

  // Update floor indicator if not in init state
  if (currentState != STATE_INIT && currentFloor != -1) 
    elevio_floorIndicator(currentFloor);
}

static void _orderHandler(void) {
  bool isStopClear = !isStopPressed && !isStopDebouncing;
  // Check for button presses
  for(int f = 0; f < N_FLOORS; f++) {
    for(int b = 0; b < N_BUTTONS; b++) {
      bool isPressed = elevio_callButton(f, b);

      // Rising Edge Detection
      if (isPressed && !prevBtnStates[f][b] && isStopClear) {
        elevio_buttonLamp(f, b, true);
        orderArr[f][b] = true;
      }
      // Save current state for the next loop
      prevBtnStates[f][b] = isPressed; 
    }
  }
}

void fsm_spin(void) {
    // Update floor once per spin
    _floorHandler();
 
    // Check stop btn
    _stopBtnHandler();
   
    // Start querying after init
    if (currentState != STATE_INIT) {

      // Query slower than the spin loop
      timer_start(&btnQueryTimer, 0.01);
      if (timer_isTimeout(&btnQueryTimer)) {
        // Reset the debounce timer
        timer_stop(&btnQueryTimer);

        // Handle btns and set orders
        _orderHandler();
      }
    }

    switch (currentState) {
      case STATE_INIT:      fsm_onInit();     break;
      case STATE_IDLE:      fsm_onIdle();     break;
      case STATE_MOVING:    fsm_onMoving();   break;
      case STATE_DOOR_OPEN: fsm_onDoorOpen(); break;
      case STATE_STOP:      fsm_onStop();     break;
    }

    // Set the previous floor if not between floors
    if (currentFloor != -1) prevFloor = currentFloor;
}
