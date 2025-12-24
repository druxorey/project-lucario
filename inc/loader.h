#ifndef MOCK_LOADER_H
#define MOCK_LOADER_H

#include "../inc/definitions.h"

typedef enum {
    LOADER_SUCCESS  = 0,
    LOADER_ERROR    = 1,
} LoaderStatus_t;

LoaderStatus_t loaderLoadProgram(const char* fileName);

#endif // LOGGER_H
