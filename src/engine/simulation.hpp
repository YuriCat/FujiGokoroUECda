#pragma once

#include "../core/field.hpp"
#include "data.hpp"

// ランダムシミュレーション

namespace Simulation {
    extern void init();
    extern void stats();
}

extern int simulation(Field& field, SharedData *const pshared, ThreadTools *const ptools);
extern int startPlaySimulation(Field& field, MoveInfo mv,
                               SharedData *const pshared, ThreadTools *const ptools);
extern int startChangeSimulation(Field& field, int p, Cards c,
                                 SharedData *const pshared, ThreadTools *const ptools);
extern int startAllSimulation(Field& field, SharedData *const pshared, ThreadTools *const ptools);