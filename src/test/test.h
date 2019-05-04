#pragma once

#include "../core/daifugo.hpp"
#include "../UECda.h"

bool CardsTest();
bool MovegenTest(const std::vector<std::string>& recordFiles);
bool DominanceTest(const std::vector<std::string>& recordFiles);
bool MateTest(const std::vector<std::string>& recordFiles);
bool L2Test(const std::vector<std::string>& recordFiles);