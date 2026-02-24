#pragma once

#include <stdbool.h>
#include <sys/time.h>
#include <stddef.h>
#include <time.h>

typedef struct {
  double          timerEndTime;
  bool            timerActive;
} timerAlarm;


double timer_getWallTime(void);

void timer_init(timerAlarm* timerAlarmInstance);

void timer_start(timerAlarm* timerAlarmInstance, double duration);

void timer_stop(timerAlarm* timerAlarmInstance);

bool timer_isTimeout(timerAlarm* timerAlarmInstance);

void timer_msSleep(long ms);

