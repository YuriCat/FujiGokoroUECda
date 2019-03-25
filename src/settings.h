/*
 settings.h
 Katsuki Ohto
 */

// 藤心基本設定
// 設定パターンによっては動かなくなるかもしれないので注意

#ifndef UECDA_SETTINGS_H_
#define UECDA_SETTINGS_H_

#include <cfloat>

// プロフィール
#define MY_MC_NAME "lilovyy"
#define MY_POL_NAME "maLilovyy"
#define MY_POL_RL_NAME "maLilovyyRL"

#define MY_VERSION "20190325"
#define MY_COACH "KatsukiOhto"

// 重要な設定

//#define MINIMUM // 本番用
//#define MONITOR // 着手決定関連の表示
//#define BROADCAST // 試合進行実況
//#define DEBUG // デバッグ出力。プレイアウトの内容も出力するので、重すぎて試合にならない。バグチェック用

// 試合に必要でないリッチ設定 本番用では自動オフ
// メソッド解析
#define USE_ANALYZER

#define CHECK_ALL_MOVES // 自分のプレイにて、必勝や諦めの判定がなされた後も生成された全ての着手を検討する

//#define FIXED_N_PLAYOUTS (8000) // プレイアウト回数を固定(デバッグ、実験用)

// 戦略設定

// 思考レベル(0~＋∞だが、6以上の場合は計算時間解析が上手く行かないかも)
// 0だとMCに入らない(POLICY_ONLYをオンにするのと同じ)
#define THINKING_LEVEL (9)

// 並列化
// スレッド数(ビルド時決定)
// 0以下を設定すると勝手に1になります
#define N_THREADS (8)

// 末端報酬を階級リセットから何試合前まで計算するか
constexpr int N_REWARD_CALCULATED_GAMES = 32;

// ログ関連

// ログを取って外部ファイルに書き出す
#define LOGGING_FILE

// 置換表設定
#define USE_L2BOOK // ラスト2人置換表を使う
//#define USE_LNCIBOOK // 3人以上の完全情報置換表を使う

// プレー関数メインの設定
#define SEARCH_ROOT_MATE // 必勝探索を行う
#define SEARCH_ROOT_L2 // ラスト2人の全読みを行う

// プレイアウト中の設定(学習時にも関係するので注意)
#define SEARCH_LEAF_MATE // プレイアウト末端で必勝探索を行う
#define SEARCH_LEAF_L2 // プレイアウト末端でラスト2人の全読みを行う
//#define SEARCH_LEAF_LNCI // プレイアウト末端でMaxN探索を行う

// 局面推定設定
//#define ESTIMATION_BY_TIME // 相手の計算量を利用した手札推定を行う

// 相手モデル解析設定
#define MODELING_PLAY // 相手の着手をモデリング
//#define MODELING_TIME // 相手の計算量をモデリング

// ヒューリスティックの利用設定
// #define PRUNE_ROOT_PLAY_BY_HEURISTICS // ヒューリスティックなルート候補着手の枝刈り
#define PRUNE_ROOT_CHANGE_BY_HEURISTICS // ヒューリスティックなルート候補交換の枝刈り

// ルートでの方策関数利用設定
#define USE_POLICY_TO_ROOT

// 自分以外で通算順位の最高のプレーヤーの結果も考慮
//#define DEFEAT_RIVAL_MC // MCにて
#define DEFEAT_RIVAL_MATE // 必勝着手がある場合

// レアプレー(有効である可能性は低いが、見せプレーとして行うプレー)
// オンにして自己対戦するとバグが発見出来るかも
//#define RARE_PLAY

// 方策の計算設定
using policy_value_t = float;
//using policy_value_t = double;

// softmax方策の温度設定
constexpr double SIMULATION_TEMPERATURE_CHANGE = 1.0;//DBL_MAX;
constexpr double SIMULATION_TEMPERATURE_PLAY = 1.1;//DBL_MAX;
// softmaxの増幅パラメータ
constexpr double SIMULATION_AMPLIFY_COEF = 0.22;
constexpr double SIMULATION_AMPLIFY_EXPONENT = 2;
// 方策そのままでプレーするとき
constexpr double TEMPERATURE_CHANGE = 1;//DBL_MAX;
constexpr double TEMPERATURE_PLAY = 0;//DBL_MAX;

// シミュレーション中の選び方のアルゴリズム
enum Selector{
    NAIVE,
    THRESHOLD,
    POLY_BIASED,
    EXP_BIASED,
};

// 相手の手札の配り方のアルゴリズム
enum DealType{
    RANDOM,
    SBJINFO,
    BIAS,
    REJECTION,
};

constexpr Selector SIMULATION_SELECTOR = Selector::POLY_BIASED;
constexpr DealType MONTECARLO_DEAL_TYPE = DealType::REJECTION;

// プレーヤー人数
#define N_NORMAL_PLAYERS (5)

/**************************以下は直接変更しない**************************/

// 大会用ビルドでは MATCH_CONST変数 は定数とする
#ifdef MATCH
#define MATCH_CONST constexpr
#else
#define MATCH_CONST
#endif

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

// スレッド数
#ifdef N_THREADS
#if N_THREADS <= 0
#undef N_THREADS
#define N_THREADS (1)
#endif
#endif // N_THREADS


// 価値関連(現在未使用)
#if THINKING_LEVEL <= 0
// レベル0は方策関数のみのモードとする。便宜上、時間の価値を大きい値に設定しておく
#define POLICY_ONLY // 方策関数のみで着手を決定する
#define VALUE_PER_CLOCK (9999999999.9)
#else
// 時間の価値をレベルの２乗に反比例させる。計算量がレベルに対して線形に変化する事を期待
// ...しているのだがそうなっていないようだ
#define VALUE_PER_CLOCK (5.0 / (THINKING_LEVEL * THINKING_LEVEL) / pow(10.0, 8))
#endif
#define VALUE_PER_SEC (VALUE_PER_CLOCK * 3191 * pow(10.0, 6)) // 時間の価値(1秒あたり),3191はうちのPCのクロック周波数(/microsec)

// 方策関数そのままのクライアントとしてのビルド
#ifdef POLICY_ONLY

// 名前
#ifdef RL_POLICY
#define MY_NAME MY_POL_RL_NAME
#else
#define MY_NAME MY_POL_NAME
#endif

// スレッド数は1ということにする
// (0にすると探索用バッファが無くなる)
#ifdef N_THREADS
#undef N_THREADS
#define N_THREADS (1)
#endif

// ログ取りをするかどうかは、
// ファイル出力を行うかどうかに依存
#ifdef LOGGING_FILE
#ifndef LOGGING
#define LOGGING
#endif
#endif

#else // POLICY_ONLY

// モンテカルロありのとき

// 名前
#define MY_NAME MY_MC_NAME

// ログ取りは必ず行う
#ifndef LOGGING
#define LOGGING
#endif

#endif // POLICY_ONLY


// 本番用のとき、プレーに無関係なものは全て削除
// ただしバグ落ちにdefineが絡む可能性があるので、強制終了時の出力は消さない
#if defined(MINIMUM)

// デバッグ
#ifdef DEBUG
#undef DEBUG
#endif

// 実況
#ifdef BROADCAST
#undef BROADCAST
#endif

// アナライザ
#ifdef USE_ANALYZER
#undef USE_ANALYZER
#endif


#endif // MINIMUM

// スレッド数として2以上が指定された場合は、マルチスレッドフラグを立てる
#if N_THREADS >= (2)
#define MULTI_THREADING
#endif

#define N_PLAY_THREADS (N_THREADS)
#define N_CHANGE_THREADS std::max(1, (N_THREADS) / 2)

#ifdef HUMAN_MODE
#undef MY_NAME
#define MY_NAME "Human"
#endif // HUMAN_MODE

#ifdef RANDOM_MODE
#undef MY_NAME
#define MY_NAME "Random"
#endif // RANDOM_MODE

#endif // UECDA_SETTINGS_H_
