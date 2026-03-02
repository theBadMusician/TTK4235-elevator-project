#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fsm.h"
#include "lib/log.h"

int main(){
  log_info("Starting elevator control system...\n");

  while(1){
    fsm_spin();
    nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
  }
  return 0;
}
