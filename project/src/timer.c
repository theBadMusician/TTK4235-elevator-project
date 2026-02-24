#include "timer.h"

double timer_getWallTime() {
  struct timeval timecheck;
  gettimeofday(&timecheck, NULL);

  return (double)timecheck.tv_sec + (double)timecheck.tv_usec * 1e-6;
}

void timer_init(timerAlarm* timerAlarmInstance) {
  timerAlarmInstance->timerActive = false;
  timerAlarmInstance->timerEndTime = 0.0f;
}

void timer_start(timerAlarm* timerAlarmInstance, double duration) {
  // Do not overwrite a running timer
  if (timerAlarmInstance->timerActive) return;

  timerAlarmInstance->timerEndTime = timer_getWallTime() + duration;
  timerAlarmInstance->timerActive = true;
}

void timer_stop(timerAlarm* timerAlarmInstance) {
  timerAlarmInstance->timerEndTime = timer_getWallTime();
  timerAlarmInstance->timerActive = false;
}

bool timer_isTimeout(timerAlarm* timerAlarmInstance) {
  return (timerAlarmInstance->timerActive && (timer_getWallTime() > timerAlarmInstance->timerEndTime));
}

void timer_msSleep(long ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);
}
