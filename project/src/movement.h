/**
 * @file movement.h
 * @brief Handles elevator routing, stopping logic, and direction calculation.
 *
 * This module implements the core elevator algorithm (the LOOK algorithm). 
 * It determines when the elevator should stop, which direction 
 * it should travel next based on active orders, and how to recover if halted 
 * between floors.
 */
#pragma once

#include "elevatorState.h"

/**
 * @brief Checks if there are any active orders above a specific floor.
 * * @param[in] elev Pointer to the central elevator state structure.
 * @param[in] floor The reference floor to check above.
 * @return true if there is at least one active order on a higher floor.
 * @return false if the order array is clear for all floors above.
 * * @details Iterates through the order matrix from `floor + 1` up to the highest 
 * possible floor, checking cabin, hall up, and hall down buttons.
 */
bool movement_requestsAbove(ElevatorState* elev, int floor);

/**
 * @brief Checks if there are any active orders below a specific floor.
 * * @param[in] elev Pointer to the central elevator state structure.
 * @param[in] floor The reference floor to check below.
 * @return true if there is at least one active order on a lower floor.
 * @return false if the order array is clear for all floors below.
 * * @details Iterates through the order matrix from floor `0` up to `floor - 1`, 
 * checking cabin, hall up, and hall down buttons.
 */
bool movement_requestsBelow(ElevatorState* elev, int floor);



/**
 * @brief Determines the initial direction if the elevator is halted between floors.
 * * @param[in,out] elev Pointer to the central elevator state structure.
 * @return true if a recovery was necessary and handled.
 * @return false if the elevator is not in a recovery state.
 * * @details If the elevator is stopped between floors (sensor reads -1) and a new 
 * order is placed, this function uses the previous direction and previous floor 
 * to calculate whether it is "hovering" above or below a floor. It then routes 
 * the elevator safely to the nearest requested floor without skipping orders.
 */
bool movement_recoveryHandler(ElevatorState* elev);

/**
 * @brief Evaluates whether the elevator should halt at the current floor.
 * * @param[in] elev Pointer to the central elevator state structure.
 * @return true if the elevator should stop at this floor.
 * @return false if the elevator should bypass this floor.
 * * @details Implements the core stopping logic: Always stops for cabin calls. 
 * If moving UP, it stops for UP hall calls, or DOWN hall calls if it is the 
 * highest active request. If moving DOWN, it stops for DOWN hall calls, or 
 * UP hall calls if it is the lowest active request.
 */
bool movement_stopHandler(ElevatorState* elev);

/**
 * @brief Calculates and applies the next motor direction based on active orders.
 * * @param[in,out] elev Pointer to the central elevator state structure.
 * * @details Checks for remaining requests in the current direction of travel. 
 * If orders remain, it continues in that direction. If the path is clear, it 
 * reverses direction to serve orders behind it. If the entire queue is empty, 
 * it stops the motor and transitions the system into STATE_IDLE.
 */
void movement_directionHandler(ElevatorState* elev);
