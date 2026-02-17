#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "driver/elevio.h"
#include "timer.h"


int main(){
    elevio_init();
    
    printf("=== Example Program ===\n");
    printf("Press the stop button on the elevator panel to exit\n");

    elevio_motorDirection(DIRN_UP);

    while(1){
        int floor = elevio_floorSensor();

        if (floor != -1)
          printf("Current floor: %d \n", floor);

        if (timer_isTimeout()) {
          printf("Timer has timed out! \n");
          timer_stop();
        }
        if(floor == 0){
            elevio_motorDirection(DIRN_UP);
            timer_start(5.0f);
        }

        if(floor == N_FLOORS-1){
            elevio_motorDirection(DIRN_DOWN);
        }


        for(int f = 0; f < N_FLOORS; f++){
            for(int b = 0; b < N_BUTTONS; b++){
                int btnPressed = elevio_callButton(f, b);
                elevio_buttonLamp(f, b, btnPressed);
            }
        }

        if(elevio_obstruction()){
            elevio_stopLamp(1);
        } else {
            elevio_stopLamp(0);
        }
        
        if(elevio_stopButton()){
            elevio_motorDirection(DIRN_STOP);
            break;
        }
        
        nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
    }

    return 0;
}
