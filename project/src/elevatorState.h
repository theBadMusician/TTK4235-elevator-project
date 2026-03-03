/**
 * @file elevatorState.h
 * @brief Defines the state machine and data structures for the elevator.
 *
 * This file contains the definitions for the elevator's operating states
 * and the main state structure tracking current position, direction, 
 * orders, and timers.
 */
#pragma once

#include <stdbool.h>
#include "driver/elevio.h"
#include "timer.h"

/**
 * @brief Represents the current operational state of the elevator.
 */
typedef enum {
    STATE_INIT,         /**< Initialization state upon startup. */
    STATE_IDLE,         /**< Elevator is stationary and awaiting orders. */
    STATE_MOVING,       /**< Elevator is currently moving between floors. */
    STATE_DOOR_OPEN,    /**< Elevator has reached a target floor and doors are open. */
    STATE_STOP          /**< Emergency stop state triggered by the stop button. */
} State;

/**
 * @brief Central structure containing all state information for the elevator.
 * This structure encapsulates the physical state (floor, direction), 
 * the internal logic state (current operation, active orders), and the required 
 * timers for asynchronous events like door operations and switch debouncing.
 */
typedef struct {
    // States
    State           currentState;       /**< The active operational state of the elevator. */
    int             currentFloor;       /**< Most recently registered floor sensor. */
    MotorDirection  currentDir;         /**< Current direction of motor movement. */

    // Previous states
    int             prevFloor;          /**< The floor the elevator was at previously. */
    MotorDirection  prevDir;            /**< The previous direction of movement. */
    
    // Queue order array
    bool            orderArr[N_FLOORS][N_BUTTONS]; /**< Matrix tracking all active floor and cabin orders. */

    // Btn states
    bool            prevBtnStates[N_FLOORS][N_BUTTONS]; /**< Matrix tracking previous button states for edge detection. */

    // Stop button
    bool            isStopped;          /**< Flag indicating if the elevator is currently halted by the stop button. */
    bool            isStopPressed;      /**< Flag indicating if the physical stop button is currently held down. */
    bool            isStopDebouncing;   /**< Flag indicating if the stop button release is currently being debounced. */

    // Timers
    timerAlarm      doorOpenTimer;      /**< Timer regulating how long the doors remain open. */
    timerAlarm      btnQueryTimer;      /**< Timer regulating the polling rate for button presses. */
    timerAlarm      stopDebouncerTimer; /**< Timer handling the debounce delay for the stop button. */
} ElevatorState;

/**
 * @brief Initializes the elevator state structure with default values.
 *
 * @param[out] state Pointer to the ElevatorState structure to be initialized.
 *
 * @pre The `state` pointer must not be NULL.
 * @post All internal variables are set to their safe startup defaults (e.g., STATE_INIT, 
 * order arrays cleared, timers initialized).
 *
 * @details Sets the initial state to STATE_INIT and defaults the floor tracking 
 * to 0. Motor directions are explicitly halted using DIRN_STOP. The order queue 
 * and previous button state arrays are iteratively cleared to ensure no "ghost" 
 * inputs exist in memory upon startup. Finally, the required hardware/software 
 * timers are initialized for later use.
 */
void elevatorState_init(ElevatorState* state);
