
#include "elevatorState.h"

void elevatorState_init(ElevatorState* state) {
  // Init state variables
  state->currentState = STATE_INIT;
  state->currentFloor = 0;
  state->currentDir   = DIRN_STOP;

  // Init previous state variables
  state->prevFloor  = 0;
  state->prevDir    = DIRN_STOP;

  // Init stop button states
  state->isStopped        = false;
  state->isStopPressed    = false;
  state->isStopDebouncing = false;

  // Clear order array and previous button states
  for (int f = 0; f < N_FLOORS; f++) {
    for (int b = 0; b < N_BUTTONS; b++) {
      state->orderArr[f][b]       = false;
      state->prevBtnStates[f][b]  = false;
    }
  }

  // Initialize timers
  timer_init(&state->doorOpenTimer);
  timer_init(&state->btnQueryTimer);
  timer_init(&state->stopDebouncerTimer);
}
