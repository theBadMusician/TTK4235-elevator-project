/**
 * @file elevatorState.c
 * @brief Implementation of the elevator state tracking and initialization.
 */

#include "elevatorState.h"

/**
 * @details Sets the initial state to STATE_INIT and defaults the floor tracking 
 * to 0. Motor directions are explicitly halted using DIRN_STOP. The order queue 
 * and previous button state arrays are iteratively cleared to ensure no "ghost" 
 * inputs exist in memory upon startup. Finally, the required hardware/software 
 * timers are initialized for later use.
 */
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
