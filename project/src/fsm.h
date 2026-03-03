/**
 * @file fsm.h
 * @brief Main finite state machine (FSM) loop for the elevator.
 *
 * This module orchestrates the elevator's behavior. It acts as the central 
 * nervous system, pulling in hardware readings from the HMI module, routing 
 * decisions through the movement module, and dispatching execution to 
 * internal state handlers (Init, Idle, Moving, Door Open, Stop).
 */
#pragma once

/**
 * @brief Executes one single tick of the elevator's state machine.
 *
 * @details This function should be called continuously in the main program loop 
 * (e.g., inside `main.c`'s `while(1)`). During each spin, it polls the hardware 
 * for floor updates and stop button presses, conditionally queries for new orders, 
 * and then triggers the appropriate internal state handler based on `elevator.currentState`.
 * Finally, it updates the previous floor tracking before yielding.
 */
void fsm_spin(void);
