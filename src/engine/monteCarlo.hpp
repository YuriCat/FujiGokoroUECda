#pragma once

#include "data.hpp"

namespace MonteCarlo {
    extern void init();
    extern void stats();
}

// マルチスレッディングのときはスレッド、
// シングルの時は関数として呼ぶ

extern void MonteCarloThread(const int threadId, const int numThreads,
                             RootInfo *const proot, const Field *const pfield,
                             SharedData *const pshared, ThreadTools *const ptools);