/**
 * @file syscalls.h
 * @brief System Call (SVC) routing and execution interface.
 */
#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../../inc/definitions.h"

typedef enum {
	SYSCALL_SUCCESS = 0, /**< Syscall completed, continue execution. */
	SYSCALL_UNKNOWN = 1, /**< Invalid syscall code requested. */
	SYSCALL_HALT    = 2, /**< Process requested termination. Hardware must stop it. */
	SYSCALL_BLOCK   = 3  /**< Process blocked. Hardware must yield the CPU. */
} SyscallStatus_t;

/**
 * @brief Routes and executes the requested system service based on AC register.
 */
SyscallStatus_t handleSyscall(void);

#endif // SYSCALLS_H
