#pragma once

#include "../core/daifugo.hpp"
#include "../UECda.h"
#include "../engine/data.hpp"

extern SharedData shared;

bool CardsTest();
bool MovegenTest(const std::vector<std::string>& recordFiles);
bool DominanceTest(const std::vector<std::string>& recordFiles);
bool MateTest(const std::vector<std::string>& recordFiles);
bool Last2Test(const std::vector<std::string>& recordFiles);

bool PolicyTest(const std::vector<std::string>& recordFiles);