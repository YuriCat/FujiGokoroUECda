#pragma once

// 藤心基本設定
// 設定パターンによっては動かなくなるかもしれないので注意

#include <string>
#include <cfloat>
#include <algorithm>

// プロフィール
const std::string MY_NAME = "lilovyy";
const std::string MY_POL_NAME = "maLilovyy";
const std::string MY_VERSION = "20231013";

// 戦略設定

// 思考レベル(0~＋∞)
#define THINKING_LEVEL (9)

// 最大並列スレッド数
#define N_THREADS (8)

// 末端報酬を階級リセットから何試合前まで計算するか
constexpr int N_REWARD_CALCULATED_GAMES = 32;

// 方策の計算設定
using policy_value_t = float;

// プレーヤー人数
#define N_NORMAL_PLAYERS (5)

namespace Settings {
    extern bool policyMode;
    extern int numPlayThreads;
    extern int numChangeThreads;
    extern int fixedSimulationCount;
    extern bool maximizePosition;
}

extern std::string DIRECTORY_PARAMS_IN;
extern std::string DIRECTORY_PARAMS_OUT;
extern std::string DIRECTORY_LOGS;

struct ConfigReader {
    ConfigReader(std::string cfile);
};
extern ConfigReader configReader;

/**************************以下は直接変更しない**************************/

// 教師用ビルドでは1スレッドでルートでの方策の利用はなし
#ifdef TEACHER

#ifdef N_THREADS
#undef N_THREADS
#endif
#define N_THREADS (1)

#undef THINKING_LEVEL
#define THINKING_LEVEL (40)

#ifdef USE_POLICY_TO_ROOT
#undef USE_POLICY_TO_ROOT
#endif

#endif

// ルール設定
#ifndef _PLAYERS
// 標準人数に設定
#define _PLAYERS (N_NORMAL_PLAYERS)
#endif