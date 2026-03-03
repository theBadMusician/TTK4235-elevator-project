/**
 * @file main.c
 * @brief Main entry point for the elevator control software.
 * * This file contains the root execution loop of the program. It initiates 
 * the continuous ticking of the core finite state machine 
 * at a fixed time interval.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fsm.h"
#include "lib/log.h"

/**
 * @brief Initializes the system and runs the main FSM loop.
 * * @details The function kicks off the elevator control system and enters an 
 * infinite loop, calling fsm_spin() to evaluate inputs and update the state 
 * machine. To prevent the program from consuming 100% of the CPU, the loop 
 * yields execution for 20 milliseconds between each spin, resulting in an 
 * effective system polling rate of 50 Hz.
 * * @return int Returns 0 upon successful execution (in practice, 
 * this loop runs infinitely until the process is manually terminated).
 */
int main(){
  log_info("Starting elevator control system...\n");

  while(1){
    fsm_spin();
    nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
  }
  return 0;
}
