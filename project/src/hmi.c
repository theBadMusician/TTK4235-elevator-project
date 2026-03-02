/**
 * @file hmi.c
 * @brief Implementation of the Human-Machine Interface logic.
 */

#include "hmi.h"


void hmi_stopBtnHandler(ElevatorState* elev) {
  if (elev->currentState == STATE_INIT) {
    elev->isStopPressed = false;
    return;
  }

  elev->isStopPressed = elevio_stopButton();
  
  if (!elev->isStopPressed && timer_isTimeout(&elev->stopDebouncerTimer)) {
    elevio_stopLamp(false);
 
    // Force reset the timer using stop
    timer_stop(&elev->stopDebouncerTimer);

    // Debounce is finished
    elev->isStopDebouncing = false;
  }
  else if (elev->isStopPressed) {
    elevio_stopLamp(true);

    // Reset the timer using stop and debounce
    elev->isStopDebouncing = true;
    timer_stop(&elev->stopDebouncerTimer);
    timer_start(&elev->stopDebouncerTimer, 0.5);

    // Transition to stop state immediately
    elev->currentState = STATE_STOP; 
  }
}


void hmi_floorHandler(ElevatorState* elev) {
  // Update floor reading
  elev->currentFloor = elevio_floorSensor();

  // Update floor indicator if not in init state
  if (elev->currentState != STATE_INIT && elev->currentFloor != -1) 
    elevio_floorIndicator(elev->currentFloor);
}


void hmi_orderHandler(ElevatorState* elev) {
  bool isStopClear = !elev->isStopPressed && !elev->isStopDebouncing;
  // Check for button presses
  for(int f = 0; f < N_FLOORS; f++) {
    for(int b = 0; b < N_BUTTONS; b++) {
      bool isPressed = elevio_callButton(f, b);

      // Rising Edge Detection
      if (isPressed && !elev->prevBtnStates[f][b] && isStopClear) {
        elevio_buttonLamp(f, b, true);
        elev->orderArr[f][b] = true;
      }
      // Save current state for the next loop
      elev->prevBtnStates[f][b] = isPressed; 
    }
  }
}
