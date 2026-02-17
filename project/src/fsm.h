#pragma once

typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_MOVING,
    STATE_DOOR_OPEN,
    STATE_STOP
} ElevatorState;

// Initialization state
void fsm_onInit(void);

void fsm_onIdle(void);
void fsm_onMoving(void);
void fsm_onDoorOpen(void);
void fsm_onStop(void);
