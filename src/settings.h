#pragma once

// 藤心基本設定
// 設定パターンによっては動かなくなるかもしれないので注意

#include <string>
#include <cfloat>
#include <algorithm>

// プロフィール
#define MY_MC_NAME "lilovyy"
#define MY_POL_NAME "maLilovyy"
#define MY_POL_RL_NAME "maLilovyyRL"

#define MY_VERSION "20190504"
#define MY_COACH "KatsukiOhto"

// 重要な設定

//#define MINIMUM // 本番用
//#define MONITOR // 着手決定関連の表示
//#define BROADCAST // 試合進行実況
//#define DEBUG // デバッグ出力。プレイアウトの内容も出力するので、重すぎて試合にならない。バグチェック用

// 試合に必要でないリッチ設定 本番用では自動オフ

#define CHECK_ALL_MOVES // 自分のプレイにて、必勝や諦めの判定がなされた後も生成された全ての着手を検討する

//#define FIXED_N_PLAYOUTS (2000) // プレイアウト回数を固定(デバッグ、実験用)

// 戦略設定

// 思考レベル(0~＋∞だが、6以上の場合は計算時間解析が上手く行かないかも)
// 0だとMCに入らない(POLICY_ONLYをオンにするのと同じ)
#define THINKING_LEVEL (9)

// 並列スレッド数
#define N_THREADS (8)

// 末端報酬を階級リセットから何試合前まで計算するか
constexpr int N_REWARD_CALCULATED_GAMES = 32;

// ログ関連

// ログを取って外部ファイルに書き出す
#define LOGGING_FILE

// 置換表設定
#define USE_L2BOOK // ラスト2人置換表を使う

// ヒューリスティックの利用設定
#define PRUNE_ROOT_CHANGE_BY_HEURISTICS // ヒューリスティックなルート候補交換の枝刈り

// ルートでの方策関数利用設定
#define USE_POLICY_TO_ROOT

// 自分以外で通算順位の最高のプレーヤーの結果も考慮
//#define DEFEAT_RIVAL_MC // MCにて
#define DEFEAT_RIVAL_MATE // 必勝着手がある場合

// 方策の計算設定
using policy_value_t = float;

// シミュレーション中の選び方のアルゴリズム
enum Selector {
    NAIVE, THRESHOLD, POLY_BIASED, EXP_BIASED,
};
// 相手の手札の配り方のアルゴリズム
enum DealType {
    RANDOM, SBJINFO, BIAS, REJECTION,
};

// プレーヤー人数
#define N_NORMAL_PLAYERS (5)

namespace Settings {
    extern int numPlayThreads;
    extern int numChangeThreads;
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

#ifdef FIXED_N_PLAYOUTS
#undef FIXED_N_PLAYOUTS
#endif

#endif

// ルール設定
#ifndef _PLAYERS
// 標準人数に設定
#define _PLAYERS (N_NORMAL_PLAYERS)
#endif

// 方策関数そのままのクライアントとしてのビルド
#ifdef POLICY_ONLY

// 名前
#define MY_NAME MY_POL_NAME

// スレッド数は1ということにする
// (0にすると探索用バッファが無くなる)
#ifdef N_THREADS
#undef N_THREADS
#define N_THREADS (1)
#endif

#else // POLICY_ONLY

// モンテカルロありのとき

// 名前
#define MY_NAME MY_MC_NAME

#endif // POLICY_ONLY


// 本番用のとき、プレーに無関係なものは全て削除
// ただしバグ落ちにdefineが絡む可能性があるので、強制終了時の出力は消さない
#ifdef MINIMUM

// デバッグ
#ifdef DEBUG
#undef DEBUG
#endif

// 実況
#ifdef BROADCAST
#undef BROADCAST
#endif

#endif // MINIMUM

// スレッド数として2以上が指定された場合は、マルチスレッドフラグを立てる
#if N_THREADS >= (2)
#define MULTI_THREADING
#endif
