#pragma once

#include "../core/field.hpp"
#include "data.hpp"

// ランダムシミュレーション

namespace Settings {
    const bool L2SearchInSimulation = true;
    const bool MateSearchInSimulation = true;
    const double simulationTemperatureChange = 1.0;
    const double simulationTemperaturePlay = 1.1;
    const double simulationAmplifyCoef = 0.22;
    const double simulationAmplifyExponent = 2;
}

extern int simulation(Field& field, SharedData *const pshared, ThreadTools *const ptools);
extern int startPlaySimulation(Field& field, MoveInfo mv,
                               SharedData *const pshared, ThreadTools *const ptools);
extern int startChangeSimulation(Field& field, int p, Cards c,
                                 SharedData *const pshared, ThreadTools *const ptools);
extern int startAllSimulation(Field& field, SharedData *const pshared, ThreadTools *const ptools);