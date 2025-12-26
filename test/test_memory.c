#include <stdlib.h>
#include <pthread.h>

#include "../lib/utest.h"
#include "../inc/memory.h"

UTEST_MAIN();

// Provide global CPU instance for linking
CPU_t CPU;

// Auxiliary function for threading test
void* thread_memory_action(void* arg) {
	int value = *((int*)arg);
    free(arg);
    for (int i = 0; i < 100; i++) {
        word data = value * 1000 + i;
        address addr = OS_RESERVED_SIZE + (value * 100) + i;
        write_mem(addr, data);
    }
	return NULL;
}

// Verify IsNotSOMemory boundaries
UTEST(Memory, IsNotSOMemory_bounds) {
    EXPECT_TRUE(isNotSOMemory(OS_RESERVED_SIZE));
    EXPECT_FALSE(isNotSOMemory(OS_RESERVED_SIZE - 1));
    EXPECT_FALSE(isNotSOMemory(RAM_SIZE));
}

// Verify AccessIsInvalid boundaries
UTEST(Memory, AccessIsInvalid_bounds) {
    CPU.RB = 300;
    CPU.RL = 350;
    EXPECT_FALSE(accessIsInvalid(0));     // 300 within [RB,RL]
    EXPECT_FALSE(accessIsInvalid(50));    // 350 within [RB,RL]
    EXPECT_TRUE(accessIsInvalid(51));     // 351 > RL
    EXPECT_TRUE(accessIsInvalid(-1));     // 299 < RB
}

// Verify read and write in valid user mode region
UTEST(Memory, ReadWriteValidKernelMode) {
    memoryInit();
    CPU.PSW.mode = MODE_KERNEL;

    address addr = 350; // valid user region
    word data = 1234567; // within magnitude

    write_mem(addr, data);
    word out = read_mem(addr);
    EXPECT_EQ(data, out);
}

// Verify read and write in SO region
UTEST(Memory, BlockOSReservedRegion) {
    memoryInit();
    CPU.PSW.mode = MODE_KERNEL; // even kernel should not access reserved region by these checks

    address addr = 100; // < OS_RESERVED_SIZE
    word data = 1234;

    write_mem(addr, data); // should be no-op
    word out = read_mem(addr); // should return -1 due to reserved region
    EXPECT_EQ(-1, out);
}

// Verify invalid access in user mode due to RB/RL
UTEST(Memory, UserModeInvalidAccess) {
    memoryInit();
    CPU.PSW.mode = MODE_USER;
    CPU.RB = 300;
    CPU.RL = 340;

    address addr = 50; // 350 > RL -> invalid in user mode
    word data = 4321;

    write_mem(addr, data); // no-op
    word out = read_mem(addr); // error
    EXPECT_EQ(-1, out);
}

// Verify magnitude overflow on write
UTEST(Memory, MagnitudeOverflowWrite) {
    memoryInit();
    CPU.PSW.mode = MODE_KERNEL;

    address addr = 360;
    word tooBig = 10000000; // > MAX_MAGNITUDE

    write_mem(addr, tooBig); // should be blocked
    word out = read_mem(addr);
    EXPECT_EQ(0, out); // default zero since no write occurred
}

// Verify that in kernel mode, RB/RL checks are ignored
UTEST(Memory, KernelModeIgnoresRB_RL) {
    memoryInit();
    CPU.PSW.mode = MODE_KERNEL;
    CPU.RB = 300;
    CPU.RL = 310; // very small range

    address addr = 1999; // top of RAM, still >= OS_RESERVED_SIZE
    word data = 42;

    write_mem(addr, data);
    EXPECT_EQ(data, read_mem(addr));
}
// Verify thread-safety of memory operations
UTEST(Memory, ThreadSafety) {
    memoryInit();
    CPU.PSW.mode = MODE_KERNEL;

    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i + 1;
        pthread_create(&threads[i], NULL, thread_memory_action, arg);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    // Verify data written by threads
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 100; j++) {
            address addr = OS_RESERVED_SIZE + ((i + 1) * 100) + j;
            word expected = (i + 1) * 1000 + j;
            EXPECT_EQ(expected, read_mem(addr));
        }
    }
}
