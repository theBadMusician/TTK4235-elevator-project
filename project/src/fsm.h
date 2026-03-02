#pragma once

typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_MOVING,
    STATE_DOOR_OPEN,
    STATE_STOP
} ElevatorState;

void fsm_spin(void);
