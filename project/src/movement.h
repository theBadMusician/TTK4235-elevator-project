#pragma once

#include "elevatorState.h"

bool movement_requestsAbove(ElevatorState* elev, int floor);
bool movement_requestsBelow(ElevatorState* elev, int floor);

bool movement_recoveryHandler(ElevatorState* elev);
bool movement_stopHandler(ElevatorState* elev);
void movement_directionHandler(ElevatorState* elev);


