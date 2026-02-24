#include "fsm.h"

#include "timer.h"

#include "driver/elevio.h"
#include "lib/log.h"

static ElevatorState currentState = STATE_INIT;
static int           currentFloor = 0;

void fsm_onInit(void) {
  // Driver init 
  #ifdef DEBUG
    log_debug("Initializing elevator...");
  #endif // DEBUG
  elevio_init();

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
  // Do stuff
}

void fsm_onDoorOpen(void) {
  if (timer_isTimeout()) {
    timer_stop();
    elevio_doorOpenLamp(false);
    log_debug("Door closed!");
    currentState = STATE_IDLE;
    // Add state transition here
    return;
  }
  elevio_doorOpenLamp(true);
  log_debug("Opening door for 3 seconds");
  timer_start(3);
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

    // Start querying the btns after init
    if (currentState != STATE_INIT) {
      // Check for button presses
      for(int f = 0; f < N_FLOORS; f++){
        for(int b = 0; b < N_BUTTONS; b++){
          if (elevio_callButton(f, b)) {
            if (f == currentFloor) {
              currentState = STATE_DOOR_OPEN;
              return;
            }
          }
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
