#pragma once

#include "../core/field.hpp"
#include "data.hpp"

// ランダムシミュレーション

extern int simulation(Field& field, SharedData *const pshared, ThreadTools *const ptools);
extern int startPlaySimulation(Field& field, Move mv,
                               SharedData *const pshared, ThreadTools *const ptools);
extern int startChangeSimulation(Field& field, int p, Cards c,
                                 SharedData *const pshared, ThreadTools *const ptools);
extern int startAllSimulation(Field& field, SharedData *const pshared, ThreadTools *const ptools);