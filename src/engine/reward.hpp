#pragma once

#include "../UECda.h"
#include "../core/field.hpp"

std::vector<std::array<double, N_PLAYERS>> standardReward(int games);

uint32_t rivalPlayers(const Field& field, int playerNum);
int positionPreferRevolution(const Field& field, int playerNum);