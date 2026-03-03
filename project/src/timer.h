/**
 * @file timer.h
 * @brief Provides non-blocking timers and blocking sleep functions.
 */
#pragma once

#include <stdbool.h>
#include <sys/time.h>
#include <stddef.h>
#include <time.h>

/**
 * @brief Represents a non-blocking timer instance.
 *
 * @details Tracks the absolute wall time when the timer is scheduled to finish, 
 * alongside a flag indicating if the timer is currently running.
 */
typedef struct {
  double          timerEndTime; /**< The absolute system time when the timer expires. */
  bool            timerActive;  /**< True if the timer is currently running, false otherwise. */
} timerAlarm;

/**
 * @brief Retrieves the current system wall time in seconds.
 * * @return The current time in seconds as a double-precision float.
 * * @details Fetches the time using gettimeofday. The seconds and microseconds are combined into a single value. The mathematical conversion used is $t_{sec} = s + \mu s \times 10^{-6}$ .
 */
double timer_getWallTime(void);

/**
 * @brief Initializes a timer instance to a safe default state.
 * * @param[out] timerAlarmInstance Pointer to the timer instance to initialize.
 * * @details Forces the timer to an inactive state and clears the end time. This should 
 * be called once before attempting to start or check the timer.
 */
void timer_init(timerAlarm* timerAlarmInstance);

/**
 * @brief Starts a timer for a specified duration.
 * @param[in,out] timerAlarmInstance Pointer to the timer instance.
 * @param[in] duration The amount of time to run the timer for, in seconds.
 *
 * @details Calculates the absolute end time based on the current wall time plus the 
 * requested duration. If the timer is already active, this function returns immediately 
 * without overwriting the running timer.
 */
void timer_start(timerAlarm* timerAlarmInstance, double duration);

/**
 * @brief Forcibly stops a running timer.
 * * @param[in,out] timerAlarmInstance Pointer to the timer instance to stop.
 * * @details Marks the timer as inactive and resets its end time to the current wall time.
 */
void timer_stop(timerAlarm* timerAlarmInstance);

/**
 * @brief Checks if an active timer has expired.
 * * @param[in] timerAlarmInstance Pointer to the timer instance to check.
 * * @return true if the timer is active AND the current wall time has passed the end time.
 * @return false if the timer is inactive or the time has not yet expired.
 * * @details This is a non-blocking check. It relies on polling the current wall time 
 * against the stored expiration time.
 */
bool timer_isTimeout(timerAlarm* timerAlarmInstance);

/**
 * @brief Blocks program execution for a specified duration.
 * * @param[in] ms The number of milliseconds to sleep.
 * * @details Uses nanosleep to suspend the thread. This is a blocking delay and will halt 
 * the main execution loop, unlike the non-blocking timerAlarm functions.
 */
void timer_msSleep(long ms);
