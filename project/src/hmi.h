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
 *
 * @param [in,out] elev Pointer to the central elevator state structure.
 *
 * @pre The elevator state must be initialized.
 *
 * @details If the elevator is in STATE_INIT, stop inputs are ignored to prevent 
 * startup anomalies. When the physical stop button is pressed, the stop lamp is 
 * activated, a debounce timer is started, and the system is immediately 
 * forced into STATE_STOP. The system remains in a debouncing state until the 
 * physical button is released and the timer expires, at which point the lamp 
 * clears and normal operation can resume.
 */
void hmi_stopBtnHandler(ElevatorState* elev);

/**
 * @brief Reads the current floor sensor and updates the hardware indicator.
 *
 * Polls the hardware to see if the elevator is currently at a valid floor. 
 * If it is, it updates the central state and changes the physical floor 
 * indicator lamp on the elevator panel.
 *
 * @param [in,out] elev Pointer to the central elevator state structure.
 *
 * @details Retrieves the sensor reading via elevio_floorSensor(). If the 
 * elevator is between floors, the sensor returns -1, and the floor indicator 
 * is left at its previous valid value. Updates to the indicator lamp are 
 * suppressed during STATE_INIT.

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
 *
 * @details Iterates through the N_FLOORS x N_BUTTONS matrix. To prevent a single 
 * long button press from registering continuously, this function uses rising edge 
 * detection: an order is only registered if the button is currently pressed AND 
 * was not pressed in the previous polling cycle. Active inputs are strictly 
 * ignored if the stop button is active or still debouncing.
 */
void hmi_orderHandler(ElevatorState* elev);
