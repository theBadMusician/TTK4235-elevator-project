#include "timer.h"
#include <sys/time.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

static double _timerEndTime;
static bool   _timerActive = false;

double timer_getWallTime() {
  struct timeval timecheck;
  gettimeofday(&timecheck, NULL);

  return (double)timecheck.tv_sec + (double)timecheck.tv_usec * 1e-6;
}

void timer_start(double duration) {
  // Do not overwrite a running timer
  if (_timerActive) return;

  _timerEndTime = timer_getWallTime() + duration;
  _timerActive = true;
}

void timer_stop(void) {
  _timerEndTime = timer_getWallTime();
  _timerActive = false;
}

bool timer_isTimeout(void) {
  return (_timerActive && (timer_getWallTime() > _timerEndTime));
}

void timer_msSleep(long ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);
}
