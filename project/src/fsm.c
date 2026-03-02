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

void fsm_onMoving(void) {
  /* LOOK algo (modified SCAN)
  1. Maintain direction - continue moving e.g. up if there are requests from the floors above
  2. Ignore opposing requests - if moving e.g. up do not stop for a hall down request at f_0 
                                (unless about to turn around)
  3. Go to IDLE if no reqs
  */

  // If between floors, calculate an escape direction
  if (currentFloor == -1 && isStopped) {
    int targetFloor = -1;

    // Find any pending order to serve as our target
    for (int f = 0; f < N_FLOORS; f++) {
      for (int b = 0; b < N_BUTTONS; b++) {
        if (orderArr[f][b]) {
          targetFloor = f;
          break;
        }
      }
      if (targetFloor != -1) break;
    }

    // If no orders exist, remain stopped
    if (targetFloor == -1) {
      currentDir = DIRN_STOP;
      elevio_motorDirection(currentDir);
      return; 
    }

    // Determine escape direction using positional memory
    if (targetFloor > prevFloor) {
      currentDir = DIRN_UP;
    } else if (targetFloor < prevFloor) {
      currentDir = DIRN_DOWN;
    } else {
      // The target is the floor we just left. 
      // Reverse the direction we used to leave it.
      if (prevDir == DIRN_UP) {
          currentDir = DIRN_DOWN;
      } else if (prevDir == DIRN_DOWN) {
          currentDir = DIRN_UP;
      }
    }

    prevDir = currentDir;
    elevio_motorDirection(currentDir);
    isStopped = false;
    return;
  }


  /* Normal Operation */

  // Return if for some reason still between floors
  if (currentFloor == -1) return;

  // If order is at current floor, complete the order
  for(int b = 0; b < N_BUTTONS; b++){
    if (orderArr[currentFloor][b]) {
      // If moving up, ignore down hall calls; if moving down, ignore up hall calls (PRD: H2)
      if (!isSingleElementTrue(orderArr) && ((currentDir == DIRN_UP && b == BUTTON_HALL_DOWN) || (currentDir == DIRN_DOWN && b == BUTTON_HALL_UP))) continue;

      // Else open the door at current floor
      elevio_motorDirection(DIRN_STOP);
      currentState = STATE_DOOR_OPEN;
      return;
    }
  }

  // Check for orders on the floors in the current direction
  // The order button type cannot be for the opposite direction
  bool found_order_in_current_dir = false;
  for (int f_offset = 1; f_offset < N_FLOORS; f_offset++) { 
    int next_floor_to_check = currentFloor + f_offset * currentDir;

    if (next_floor_to_check > -1 && next_floor_to_check < N_FLOORS) {
      for (int b = 0; b < N_BUTTONS; b++) {
        // If moving up, ignore down hall calls; if moving down, ignore up hall calls (PRD: H2)
        if (!isSingleElementTrue(orderArr) && ((currentDir == DIRN_UP && b == BUTTON_HALL_DOWN) || (currentDir == DIRN_DOWN && b == BUTTON_HALL_UP))) continue;

        if (orderArr[next_floor_to_check][b]) {
          found_order_in_current_dir = true;
          break;
        }
      }
    }
    if (found_order_in_current_dir) break;
  }
  if (found_order_in_current_dir) {
    elevio_motorDirection(currentDir);
    return;
  } 

  // else if current dir is stop, go to closest floor
  for (int f_offset = 1; f_offset < N_FLOORS; f_offset++) { 
    int next_lower_floor_to_check = currentFloor - f_offset;
    int next_upper_floor_to_check = currentFloor + f_offset;

    for (int b = 0; b < N_BUTTONS; b++) {
      if (next_lower_floor_to_check > -1 && orderArr[next_lower_floor_to_check][b]) {
        currentDir = DIRN_DOWN;
        break;
      }
      else if (next_upper_floor_to_check < N_FLOORS && orderArr[next_upper_floor_to_check][b]) {
        currentDir = DIRN_UP;
        break;
      }
    }
  }
  if (currentDir != DIRN_STOP) prevDir = currentDir;
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
    else currentState = STATE_MOVING; // More orders await, move more

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
