/**
 * @file scheduler.h
 * @brief Process Scheduler for the Lucario OS (Round Robin).
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * @brief Handles the timer interrupt (Quantum expiration).
 * Will be responsible for context switching between READY processes.
 */
void schedulerTick(void);

#endif // SCHEDULER_H
