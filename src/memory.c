#include <pthread.h>
#include <stdbool.h>

#include "../inc/memory.h"
#include "../inc/logger.h" 

word RAM[RAM_SIZE];
pthread_mutex_t BUS_LOCK;

void memoryInit(void) {
    pthread_mutex_init(&BUS_LOCK, NULL);
}

bool is_not_SO_memory(address addr) {
    return (addr >= OS_RESERVED_SIZE && addr < RAM_SIZE);
}

bool access_is_invalid(address addr) {
    return ((addr + CPU.RB) > CPU.RL || (addr + CPU.RB) < CPU.RB); 
}

word read_mem(address addr) {
    pthread_mutex_lock(&BUS_LOCK);
    if(!is_not_SO_memory(addr)){
        // Handle invalid access
        pthread_mutex_unlock(&BUS_LOCK);
        //loggerLog(LOG_ERROR, "Access to reserved OS memory.");
        printf(stderr, "Access to reserved OS memory");
        return -1;   
    }
    else {
        if((CPU.PSW.mode == MODE_USER) && access_is_invalid(addr)){
            // Invalid adressing interruption
            pthread_mutex_unlock(&BUS_LOCK);
            //loggerLog(LOG_ERROR, "Invalid memory access attempt in USER mode."); //REVISAR
            printf(stderr, "Memory Access Violation");
            return -1;
        }    
        word data = RAM[addr];
        pthread_mutex_unlock(&BUS_LOCK);
        return data;
    }
    
}

void write_mem(address addr, word data) {
    pthread_mutex_lock(&BUS_LOCK);
    if(data < MIN_MAGNITUDE || data > MAX_MAGNITUDE) {
        pthread_mutex_unlock(&BUS_LOCK);
        //loggerLog(LOG_ERROR, "Data magnitude overflow on memory write.");
        printf(stderr, "Data magnitude overflow on memory write");
        return;
    }
    if(!is_not_SO_memory(addr)){
        // Handle invalid access
        pthread_mutex_unlock(&BUS_LOCK);
        //loggerLog(LOG_ERROR, "Access to reserved OS memory.");
        printf(stderr, "Access to reserved OS memory");
        return;
    }
    else {
        if((CPU.PSW.mode == MODE_USER) && access_is_invalid(addr)){
            // Invalid adressing interruption
            pthread_mutex_unlock(&BUS_LOCK);
            //loggerLog(LOG_ERROR, "Invalid memory write attempt in USER mode."); //REVISAR
            printf(stderr, "Memory Access Violation");
            return;
        }
        RAM[addr] = data;
        pthread_mutex_unlock(&BUS_LOCK);
        return;
    } 
}

