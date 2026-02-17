#pragma once

#include <stdbool.h>

double timer_getWallTime(void);

void timer_start(double duration);

void timer_stop(void);

bool timer_isTimeout(void);

void timer_msSleep(long ms);

