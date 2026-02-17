#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "driver/elevio.h"
#include "lib/log.h"

#include "timer.h"
#include "fsm.h"

int main(){
  log_trace("Starting elevator control system...");

  while(1){
    fsm_spin();
    nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
  }
  return 0;
}
