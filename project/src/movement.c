#include "movement.h"

bool movement_requestsAbove(ElevatorState* elev, int floor) {
  for (int f = floor + 1; f < N_FLOORS; f++) {
    for (int b = 0; b < N_BUTTONS; b++) {
      if (elev->orderArr[f][b]) return true;
    }
  }
  return false;
}
bool movement_requestsBelow(ElevatorState* elev, int floor) {
  for (int f = 0; f < floor; f++) {
    for (int b = 0; b < N_BUTTONS; b++) {
      if (elev->orderArr[f][b]) return true;
    }
  }
  return false;
}

/* Recovery when stopped between floors */
bool movement_recoveryHandler(ElevatorState* elev) {
  if (elev->currentFloor == -1 && elev->isStopped) {
    // If there are no orders, wait
    if (!movement_requestsAbove(elev, -1)) { 
      return true; 
    }

    // Check if there is an order specifically at the last floor
    bool orderAtPrevFloor = elev->orderArr[elev->prevFloor][BUTTON_CAB] || 
                            elev->orderArr[elev->prevFloor][BUTTON_HALL_UP] || 
                            elev->orderArr[elev->prevFloor][BUTTON_HALL_DOWN];

    // Route directly towards the requested floor
    if (elev->prevDir == DIRN_UP) {
      // Hovering above prevFloor
      if (movement_requestsAbove(elev, elev->prevFloor)) {
        elev->currentDir = DIRN_UP;
      } else if (orderAtPrevFloor || movement_requestsBelow(elev, elev->prevFloor)) {
        elev->currentDir = DIRN_DOWN;
      }
    } 
    else if (elev->prevDir == DIRN_DOWN) {
      // Hovering below prevFloor
      if (movement_requestsBelow(elev, elev->prevFloor)) {
        elev->currentDir = DIRN_DOWN;
      } else if (orderAtPrevFloor || movement_requestsAbove(elev, elev->prevFloor)) {
        elev->currentDir = DIRN_UP;
      }
    }

    elevio_motorDirection(elev->currentDir);
    elev->isStopped = false;
    return true; 
  }
  
  // Not in a recovery state
  return false;
}

/* Check if we should stop at the current floor */
bool movement_stopHandler(ElevatorState* elev) {
  // Always stop for cab calls at the current floor
  if (elev->orderArr[elev->currentFloor][BUTTON_CAB]) return true;

  if (elev->currentDir == DIRN_UP) {
    // Stop for UP hall calls, or for DOWN calls if it is the highest request
    if (elev->orderArr[elev->currentFloor][BUTTON_HALL_UP] || (!movement_requestsAbove(elev, elev->currentFloor) && elev->orderArr[elev->currentFloor][BUTTON_HALL_DOWN])) {
      return true;
    }
  } 
  else if (elev->currentDir == DIRN_DOWN) {
    // Stop for DOWN hall calls, or for UP calls if it is the lowest request
    if (elev->orderArr[elev->currentFloor][BUTTON_HALL_DOWN] || (!movement_requestsBelow(elev, elev->currentFloor) && elev->orderArr[elev->currentFloor][BUTTON_HALL_UP])) {
      return true;
    }
  }
  return false;
}

void movement_directionHandler(ElevatorState* elev) {
  if (elev->currentDir == DIRN_UP) {
    if (movement_requestsAbove(elev, elev->currentFloor)) {
      elev->currentDir = DIRN_UP;
    } else if (movement_requestsBelow(elev, elev->currentFloor)) {
      elev->currentDir = DIRN_DOWN;
    } else {
      elev->currentDir = DIRN_STOP;
      elev->currentState = STATE_IDLE;
    }
  } 
  else if (elev->currentDir == DIRN_DOWN) {
    if (movement_requestsBelow(elev, elev->currentFloor)) {
      elev->currentDir = DIRN_DOWN;
    } else if (movement_requestsAbove(elev, elev->currentFloor)) {
      elev->currentDir = DIRN_UP;
    } else {
      elev->currentDir = DIRN_STOP;
      elev->currentState = STATE_IDLE;
    }
  }
  // Wake up from a stopped state
  else if (elev->currentDir == DIRN_STOP) {
    if (movement_requestsAbove(elev, elev->currentFloor)) {
      elev->currentDir = DIRN_UP;
    } else if (movement_requestsBelow(elev, elev->currentFloor)) {
      elev->currentDir = DIRN_DOWN;
    } else {
      elev->currentState = STATE_IDLE;
    }
  }

  // Apply the direction
  if (elev->currentDir != DIRN_STOP) {
    elev->prevDir = elev->currentDir;
  }
  elevio_motorDirection(elev->currentDir);
}

