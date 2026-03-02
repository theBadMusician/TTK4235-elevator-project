#include "fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"
#include "utils.h"

#include "driver/elevio.h"
#include "lib/log.h"

#include "elevatorState.h"
#include "movement.h"
#include "hmi.h"

static ElevatorState elevator;

void fsm_onInit(void) {
  // Driver and state init 
  log_info("Initializing elevator...");
  elevio_init();
  elevatorState_init(&elevator);

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
  
  // Init floor state
  elevator.prevFloor    = elevio_floorSensor();
  elevator.currentFloor = elevio_floorSensor();

  // State transition T_0: STATE_INIT -> STATE_IDLE
  elevator.currentState = STATE_IDLE;
}


void fsm_onIdle(void) {
  // Set the the current direction (movement moment) to stop 
  elevator.currentDir = DIRN_STOP;

  // Motor is stopped
  elevio_motorDirection(DIRN_STOP);
  
  // Door is closed
  elevio_doorOpenLamp(false);

  if (!areAllElementsFalse(elevator.orderArr)) elevator.currentState = STATE_MOVING;
}


void fsm_onMoving(void) {
    // Attempt recovery if stopped mid-shaft
    if (movement_recoveryHandler(&elevator)) {
        return;
    }

    // Normal operation requires a valid floor reading
    if (elevator.currentFloor == -1) {
        return;
    }

    // Evaluate if there is an order to stop at this floor
    if (movement_stopHandler(&elevator)) {
        elevio_motorDirection(DIRN_STOP);
        elevator.currentState = STATE_DOOR_OPEN;
        return;
    }

    // Determine and apply the next movement direction
    movement_directionHandler(&elevator);
}

void fsm_onDoorOpen(void) {
  if (timer_isTimeout(&elevator.doorOpenTimer) && !elevio_obstruction() && !elevio_stopButton()) {
    timer_stop(&elevator.doorOpenTimer);
    elevio_doorOpenLamp(false);
#ifdef DEBUG
    log_debug("Door closed!");
#endif // DEBUG

    // Clear orders for this floor
    for (int b = 0; b < N_BUTTONS; b++) {
      elevator.orderArr[elevator.currentFloor][b] = false;
      elevio_buttonLamp(elevator.currentFloor, b, false);
    }

    // No more orders, transition to idle
    if (areAllElementsFalse(elevator.orderArr)) elevator.currentState = STATE_IDLE;
    // More orders await, move more
    else elevator.currentState = STATE_MOVING;

    // Just finished opening the door, return
    return;
  }
  elevio_doorOpenLamp(true);
#ifdef DEBUG
  log_debug("Opening door for 3 seconds");
#endif // DEBUG

  // Restart timer if obstruction is present (PRD: D4)
  // Restart timer if stop button is pressed (PRD: D3)
  if (elevio_obstruction() || elevio_stopButton()) timer_stop(&elevator.doorOpenTimer);
  timer_start(&elevator.doorOpenTimer, 3);
}

void fsm_onStop(void) {
    /* Stop the elevator, clear queue, reset state */

    // Set the the current direction to stop and stop motor
    elevator.currentDir = DIRN_STOP;
    elevio_motorDirection(elevator.currentDir);

    // Clear the order queue and btn states
    memset(elevator.orderArr,      false, sizeof(elevator.orderArr)); 
    memset(elevator.prevBtnStates, false, sizeof(elevator.prevBtnStates)); 

    // Clear button lights
    for(int f = 0; f < N_FLOORS; f++){
      for(int b = 0; b < N_BUTTONS; b++){
        elevio_buttonLamp(f, b, false);
      }
    }

    // state transition
    if (elevator.currentFloor != -1) {
      elevio_stopLamp(true);
      elevio_doorOpenLamp(true); // Open door right away
      elevator.currentState = STATE_DOOR_OPEN;
    } else elevator.currentState = STATE_IDLE;

    // Set stopped param for undefined floor escape
    if (elevator.currentFloor == -1) elevator.isStopped = true;
}


void fsm_spin(void) {
    // Update floor once per spin
    hmi_floorHandler(&elevator);
 
    // Check stop btn
    hmi_stopBtnHandler(&elevator);
   
    // Start querying after init
    if (elevator.currentState != STATE_INIT) {

      // Query slower than the spin loop
      timer_start(&elevator.btnQueryTimer, 0.01);
      if (timer_isTimeout(&elevator.btnQueryTimer)) {
        // Reset the debounce timer
        timer_stop(&elevator.btnQueryTimer);

        // Handle btns and set orders
        hmi_orderHandler(&elevator);
      }
    }

    switch (elevator.currentState) {
      case STATE_INIT:      fsm_onInit();     break;
      case STATE_IDLE:      fsm_onIdle();     break;
      case STATE_MOVING:    fsm_onMoving();   break;
      case STATE_DOOR_OPEN: fsm_onDoorOpen(); break;
      case STATE_STOP:      fsm_onStop();     break;
    }

    // Set the previous floor if not between floors
    if (elevator.currentFloor != -1) elevator.prevFloor = elevator.currentFloor;
}
