#include "fsm.h"

#include <stdio.h>
#include <string.h>

#include "timer.h"

#include "driver/elevio.h"
#include "lib/log.h"

#define TOTAL_BUTTONS (N_FLOORS * N_BUTTONS) 

// FSM private singleton state
static ElevatorState  currentState = STATE_INIT;
static int            currentFloor = 0;
static int            prevFloor    = 0;
static MotorDirection currentDir   = 0;

// Queue order array
bool orderArr[N_FLOORS][N_BUTTONS] = { false };

// Timers 
timerAlarm doorOpenTimer;
timerAlarm btnQueryTimer;
timerAlarm stopDebouncerTimer;

// Btn states
bool btn_states[N_FLOORS][N_BUTTONS]      = { false };
bool prev_btn_states[N_FLOORS][N_BUTTONS] = { false };


bool are_all_zeros(int n_a, int n_b, bool arr[n_a][n_b]) {
    bool *ptr = &arr[0][0]; // Pointer to the first element
    int total_elements = n_a * n_b;

    for (int i = 0; i < total_elements; i++) {
        if (ptr[i]) {
            return false;
        }
    }
    return true;
}

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
}

void fsm_onMoving(void) {
  /* LOOK algo (modified SCAN)
  1. Maintain direction - continue moving e.g. up if there are requests from the floors above
  2. Ignore opposing requests - if moving e.g. up do not stop for a hall down request at f_0 
                                (unless about to turn around)
  3. Go to IDLE if no reqs
  */
  
  // TODO: [DONE?] if a button is pressed during doorOpen elevator immediately starts moving
  // TODO: [DONE?] each button pressed on the same floor holds the door open for 3 seconds

  // Return if between floors
  if (currentFloor == -1) return;

  // Query order list
  for(int b = 0; b < N_BUTTONS; b++){
    // If order is at current floor, complete the order
    if (orderArr[currentFloor][b]) {
      // If moving up, ignore down hall calls; if moving down, ignore up hall calls (PRD: H2)
      if ((currentDir == DIRN_UP && b == BUTTON_HALL_DOWN) || (currentDir == DIRN_DOWN && b == BUTTON_HALL_UP)) continue;

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
        if ((currentDir == DIRN_UP && b == BUTTON_HALL_DOWN) || (currentDir == DIRN_DOWN && b == BUTTON_HALL_UP)) continue;

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
  elevio_motorDirection(currentDir);
}

void fsm_onDoorOpen(void) {
  if (timer_isTimeout(&doorOpenTimer) && !elevio_obstruction()) {
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
    if (are_all_zeros(N_FLOORS, N_BUTTONS, orderArr)) currentState = STATE_IDLE;
    else currentState = STATE_MOVING; // More orders await, move more

    // Just finished opening the door, return
    return;
  }
  elevio_doorOpenLamp(true);
#ifdef DEBUG
  log_debug("Opening door for 3 seconds");
#endif // DEBUG

  // Restart timer if obstruction is present (PRD: D4)
  if (elevio_obstruction()) timer_stop(&doorOpenTimer);
  timer_start(&doorOpenTimer, 3);
}

void fsm_onStop(void) {
    // TODO: Do not set direction stop in idle and stop states if current floor is -1, this way we can figure which floors the elev is between using (prevFloor + currentDir)
    // TODO: Moving state has to have logic to start moving to the closest floor after stop if inbetween floors (after an order is received)

    /* Stop the elevator, clear queue, reset state */

    // Set the the current direction to stop and stop motor
    currentDir = DIRN_STOP;
    elevio_motorDirection(currentDir);

    // Clear the order queue and btn states
    memset(orderArr,        false, sizeof(orderArr)); 
    memset(btn_states,      false, sizeof(btn_states)); 
    memset(prev_btn_states, false, sizeof(prev_btn_states)); 

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
}

void fsm_spin(void) {
    // Update floor reading once per spin
    currentFloor = elevio_floorSensor();
    if (currentFloor != -1) elevio_floorIndicator(currentFloor);

    // Start querying the btns after init
    if (currentState != STATE_INIT) {
      // Query slower than the spin loop
      timer_start(&btnQueryTimer, 0.01);
      
      // Check stop btn
      if (elevio_stopButton()) {
        elevio_stopLamp(true);
        timer_stop(&stopDebouncerTimer);
        timer_start(&stopDebouncerTimer, 0.5);
        currentState = STATE_STOP;
      }

      // Check for button presses
      if (timer_isTimeout(&btnQueryTimer)) {
        timer_stop(&btnQueryTimer);
        // Only take orders if the stop btn is NOT pressed
        if (timer_isTimeout(&stopDebouncerTimer)) {
          // Clear stop lamp if stop btn no longer pressed
          elevio_stopLamp(elevio_stopButton());
        }

        // Check order btns
        for(int f = 0; f < N_FLOORS; f++){
          for(int b = 0; b < N_BUTTONS; b++){
            // Get btn state
            bool fb_btn_state = elevio_callButton(f, b);
            btn_states[f][b] = fb_btn_state;

            // Debounce the button
            if (btn_states[f][b] != prev_btn_states[f][b]) {
              prev_btn_states[f][b] = btn_states[f][b];
              
              if (fb_btn_state && !elevio_stopButton()) {
                elevio_buttonLamp(f, b, true);

                // Add new order
                orderArr[f][b]  = true;
                currentState    = STATE_MOVING;
              }
            }
          }
        }
      }
    }

    printf("ORDER QUEUE:\n"); 
    for(int f = 0; f < N_FLOORS; f++){
      for(int b = 0; b < N_BUTTONS; b++){
        if (orderArr[f][b]) {
          if (b == 0) printf("UP button pressed on floor number %d.\n", f);
          else if (b == 1) printf("DOWN button pressed on floor number %d.\n", f);
          else if (b == 2) printf("CAB button pressed to go to floor number %d.\n", f);
        }
      }
    }

    switch (currentState) {
        case STATE_INIT:
            fsm_onInit();
            break;
        case STATE_IDLE:
            fsm_onIdle();
            break;
        case STATE_MOVING:
            fsm_onMoving();
            break;
        case STATE_DOOR_OPEN:
            fsm_onDoorOpen();
            break;
        case STATE_STOP:
            fsm_onStop();
            break;
    }

    // Set the previous floor if not between floors
    if (currentFloor != -1) prevFloor = currentFloor;
}
