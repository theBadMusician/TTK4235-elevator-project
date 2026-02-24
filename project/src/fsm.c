#include "fsm.h"

#include <stdio.h>
#include <string.h>

#include "timer.h"

#include "driver/elevio.h"
#include "lib/log.h"

#define QUEUE_SIZE 10

// FSM private singleton state
static ElevatorState  currentState = STATE_INIT;
static int            currentFloor = 0;
static MotorDirection currentDir   = 0;

// Queue order array
bool orderArr[N_FLOORS][N_BUTTONS] = { false };

// Timers 
timerAlarm doorOpenTimer;
timerAlarm btnQueryTimer;

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
  
  // State transition T_0: STATE_INIT -> STATE_IDLE
  currentState = STATE_IDLE;
}


void fsm_onIdle(void) {
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
  
  // TODO: if a button is pressed during doorOpen elevator immediately starts moving
  // TODO: [DONE?] each button pressed on the same floor holds the door open for 3 seconds

  // Return if between floors
  if (currentFloor == -1) return;

  // Query order list
  for(int b = 0; b < N_BUTTONS; b++){
    // If order is at current floor, complete the order
    if (orderArr[currentFloor][b]) {
      elevio_motorDirection(DIRN_STOP);
      currentState = STATE_DOOR_OPEN;
      return;
    }

    for (int f_offset = 0; f_offset < N_FLOORS - 1; f_offset++) { 
      int next_lower_floor_to_check = currentFloor - f_offset;
      int next_upper_floor_to_check = currentFloor + f_offset;
      if (orderArr[next_lower_floor_to_check][b] && next_lower_floor_to_check > -1) {
        currentDir = DIRN_DOWN;
        break;
      }
      else if (orderArr[next_upper_floor_to_check][b] && next_upper_floor_to_check < N_FLOORS) {
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
    // Stop the elevator, clear queue, reset state
    elevio_motorDirection(DIRN_STOP);

    // TODO: Clear the order queue
    
    // TODO: Check state transition conditions for stop state
    currentState = STATE_IDLE;
}

void fsm_spin(void) {
    // Update floor reading once per spin
    currentFloor = elevio_floorSensor();
    if (currentFloor != -1) elevio_floorIndicator(currentFloor);

    // Start querying the btns after init
    if (currentState != STATE_INIT) {
      // Query slower than the spin loop
      timer_start(&btnQueryTimer, 0.01);

      // Check for button presses
      if (timer_isTimeout(&btnQueryTimer)) {
        timer_stop(&btnQueryTimer);
        for(int f = 0; f < N_FLOORS; f++){
          for(int b = 0; b < N_BUTTONS; b++){
            // Get btn state
            bool fb_btn_state = elevio_callButton(f, b);
            btn_states[f][b] = fb_btn_state;

            // Debounce the button
            if (btn_states[f][b] != prev_btn_states[f][b]) {
              prev_btn_states[f][b] = btn_states[f][b];
              
              if (fb_btn_state) {
                if (f == currentFloor) {
                  currentState = STATE_DOOR_OPEN;
                } else {
                  // Turn on requested floor lights
                  elevio_buttonLamp(f, b, fb_btn_state);

                  // Add new order
                  orderArr[f][b]  = true;
                  currentState    = STATE_MOVING;
                }
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
}
