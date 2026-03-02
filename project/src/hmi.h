/**
 * @file hmi.h
 * @brief Human-Machine Interface (HMI) module for the elevator.
 *
 * This module defines the interface for polling physical hardware inputs 
 * (buttons, switches, sensors) and updating the central elevator state 
 * and hardware lamps accordingly.
 */
#pragma once

#include "elevatorState.h"

/**
 * @brief Polls the emergency stop button and manages stop state transitions.
 *
 * Checks the physical state of the stop button, handles the necessary 
 * debouncing, and transitions the elevator into the emergency stop state 
 * if activated. It also controls the stop button illumination.
 * * @param[in,out] elev Pointer to the central elevator state structure.
 * @pre The elevator state must be initialized.
 */
void hmi_stopBtnHandler(ElevatorState* elev);

/**
 * @brief Reads the current floor sensor and updates the hardware indicator.
 *
 * Polls the hardware to see if the elevator is currently at a valid floor. 
 * If it is, it updates the central state and changes the physical floor 
 * indicator lamp on the elevator panel.
 *
 * @param[in,out] elev Pointer to the central elevator state structure.
 */
void hmi_floorHandler(ElevatorState* elev);

/**
 * @brief Polls all request buttons and registers new elevator orders.
 *
 * Iterates through all possible floor and cabin buttons. If a valid new 
 * press is detected, it registers the order in the state matrix and 
 * illuminates the corresponding button lamp.
 *
 * @param[in,out] elev Pointer to the central elevator state structure.
 * @note Orders are ignored if the elevator is currently stopped or debouncing 
 * from a stop command.
 */
void hmi_orderHandler(ElevatorState* elev);
