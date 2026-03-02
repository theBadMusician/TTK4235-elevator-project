/**
 * @file hmi.c
 * @brief Implementation of the Human-Machine Interface logic.
 */

#include "hmi.h"

/**
 * @details If the elevator is in STATE_INIT, stop inputs are ignored to prevent 
 * startup anomalies. When the physical stop button is pressed, the stop lamp is 
 * activated, a 0.5 second debounce timer is started, and the system is immediately 
 * forced into STATE_STOP. The system remains in a debouncing state until the 
 * physical button is released and the timer expires, at which point the lamp 
 * clears and normal operation can resume.
 */
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

/**
 * @details Retrieves the sensor reading via elevio_floorSensor(). If the 
 * elevator is between floors, the sensor returns -1, and the floor indicator 
 * is left at its previous valid value. Updates to the indicator lamp are 
 * suppressed during STATE_INIT.
 */
void hmi_floorHandler(ElevatorState* elev) {
  // Update floor reading
  elev->currentFloor = elevio_floorSensor();

  // Update floor indicator if not in init state
  if (elev->currentState != STATE_INIT && elev->currentFloor != -1) 
    elevio_floorIndicator(elev->currentFloor);
}



/**
 * @details Iterates through the N_FLOORS x N_BUTTONS matrix. To prevent a single 
 * long button press from registering continuously, this function uses rising edge 
 * detection: an order is only registered if the button is currently pressed AND 
 * was not pressed in the previous polling cycle. Active inputs are strictly 
 * ignored if the stop button is active or still debouncing.
 */
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
