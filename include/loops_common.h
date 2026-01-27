#ifndef LOOPS_COMMON_H
#define LOOPS_COMMON_H

#include <cstddef>

// Shared constants
extern const int LIMIT;
extern const int OFF_LIMITS;
extern const int DATA_COMPLETE;
extern const size_t BUFFER_SIZE;

// Shared state used by loops
extern bool isLoopExecution;
extern bool tunnel_found;
extern const char *prefix;

// Shared utilities implemented in common.cpp
extern "C" void setPrefix(const char *_prefix);
extern "C" void queueToPcClear();
extern "C" void resetDac();

#endif // LOOPS_COMMON_H
