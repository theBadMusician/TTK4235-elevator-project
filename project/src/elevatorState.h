#pragma once

#include <stdbool.h>
#include "driver/elevio.h"
#include "timer.h"

typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_MOVING,
    STATE_DOOR_OPEN,
    STATE_STOP
} State;

typedef struct {
    // States
    State           currentState;
    int             currentFloor;
    MotorDirection  currentDir;

    // Previous states
    int             prevFloor;
    MotorDirection  prevDir;
   
    // Queue order array
    bool            orderArr[N_FLOORS][N_BUTTONS];

    // Btn states
    bool            prevBtnStates[N_FLOORS][N_BUTTONS];

    // Stop button
    bool            isStopped;
    bool            isStopPressed;
    bool            isStopDebouncing;

    // Timers
    timerAlarm      doorOpenTimer;
    timerAlarm      btnQueryTimer;
    timerAlarm      stopDebouncerTimer;
} ElevatorState;

void elevatorState_init(ElevatorState* state);

