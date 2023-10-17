#pragma once

// 思考用の構造体
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "policy.hpp"
#include "modeling.hpp"

// Field以外のデータ構造
// Fieldは基本盤面情報+盤面を進めたり戻したりするときに値が変化するもの
// それ以外の重目のデータ構造は SharedData
// スレッドごとのデータは ThreadTools

struct ThreadTools {
    // 各スレッドの持ち物
    Dice dice; // サイコロ
    MoveInfo mbuf[8192]; // 着手生成バッファ
    ThreadTools() {
        dice.srand(0);
        memset(mbuf, 0, sizeof(mbuf));
    }
};

struct BaseSharedData {
    std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS> classDestination; // 階級到達回数
    std::array<std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS>, N_PLAYERS> classTransition; // 階級遷移回数

    MatchRecord record; // 主観的な対戦棋譜

    // クライアントの個人的スタッツ
    uint32_t playRejection, changeRejection; // リジェクト

    void feedPlayRejection() { playRejection += 1; }
    void feedChangeRejection() { changeRejection += 1; }
    void feedResult(int p, int cl, int ncl) {
        classDestination[p][ncl] += 1;
        classTransition[p][cl][ncl] += 1;
    }
    void initMatch(int playerNum) {
        // スタッツ初期化
        record.init(playerNum);
        for (int p = 0; p < N_PLAYERS; p++) {
            classDestination[p].fill(0);
            for (int cl = 0; cl < N_PLAYERS; cl++) {
                classTransition[p][cl].fill(0);
            }
        }
        playRejection = changeRejection = 0;
    }
    void initGame() {
        record.initGame();
    }
    void closeGame(const GameRecord& g) {
        // 試合順位の記録
        for (int p = 0; p < N_PLAYERS; p++) {
            feedResult(p, g.classOf(p), g.newClassOf(p));
        }
    }
    void closeMatch();
};

struct SharedData : public BaseSharedData {
    using base_t = BaseSharedData;
    // 全体で共通のデータ

    // 毎回報酬テーブルから持ってこなくてもいいようにこの試合の報酬を置いておくテーブル
    std::array<double, N_PLAYERS> gameReward;

    // 基本方策
    ChangePolicy<policy_value_t> baseChangePolicy;
    PlayPolicy<policy_value_t> basePlayPolicy;

    // 相手モデリング
    PlayerModel playerModel;

    // 1ゲーム中に保存する一次データのうち棋譜に含まれないもの
    int mateClass; // 初めてMATEと判定した階級の宣言
    int L2Result; // L2における判定結果

    // クライアントの個人的スタッツ
    // (勝利, 敗戦) x (勝利宣言, 判定失敗(引き分け), 敗戦宣言, 無宣言)
    std::array<std::array<long long, 4>, 2> myL2Result;
    // MATEの宣言結果
    std::array<std::array<long long, N_PLAYERS>, N_PLAYERS> myMateResult;

    void setMyMate(int bestClass) { // 詰み宣言
        if (mateClass == -1) mateClass = bestClass;
    }
    void setMyL2Result(int result) { // L2詰み宣言
        // ゲーム中初めてのL2宣言だけを記録
        if (L2Result == -2) L2Result = result;
    }
    void feedMyResult(int realClass) {
        // ラスト2人
        if (realClass >= N_PLAYERS - 2) {
            bool realWin = realClass == N_PLAYERS - 2;
            if (realWin && L2Result == -1) CERR << "L2 Lucky!" << std::endl;
            if (!realWin && L2Result == 1) CERR << "L2 Miss!" << std::endl;
            myL2Result[realClass - (N_PLAYERS - 2)][1 - L2Result] += 1;
        }
        // MATE宣言あり
        if (mateClass != -1) {
            if (mateClass != realClass) { // MATE宣言失敗
                CERR << "Mate Miss! DCL:" << mateClass;
                CERR << " REAL:" << realClass << std::endl;
            }
            myMateResult[realClass][mateClass] += 1;
        }
    }

    void initMatch(int playerNum) {
        base_t::initMatch(playerNum);
        // スタッツ初期化
        for (auto& a : myMateResult) a.fill(0);
        for (auto& a : myL2Result) a.fill(0);
    }
    void initGame() {
        base_t::initGame();
        mateClass = -1;
        L2Result = -2;
    }

    void closeGame() {
        const auto& game = record.latestGame();
        base_t::closeGame(game);
        int myNewClass = game.newClassOf(record.myPlayerNum);

        // 自己スタッツ更新
        feedMyResult(myNewClass);
    }
    void closeMatch();
};

/**************************ルートの手の情報**************************/

struct RootAction {
    MoveInfo move;
    Cards changeCards;
    int nextDivision;

    // 方策関数出力
    double policyScore, policyProb;

    // モンテカルロ結果
    BetaDistribution monteCarloScore;
    BetaDistribution naiveScore;

    // モンテカルロの詳細な結果
    BetaDistribution myScore, rivalScore;
    uint64_t simulations;
    std::array<std::array<int64_t, N_CLASSES>, N_PLAYERS> classDistribution;
    uint64_t turnSum;

    double mean() const { return monteCarloScore.mean(); }
    double size() const { return monteCarloScore.size(); }
    double mean_var() const { return monteCarloScore.var(); }
    double var() const { return monteCarloScore.var() * size(); }
    double naive_mean() const { return naiveScore.mean(); }

    void clear();
    void setChange(Cards cc) { clear(); changeCards = cc; }
    void setPlay(MoveInfo m) { clear(); move = m; }
};

/**************************ルートの全体の情報**************************/

struct RootInfo {
    std::array<RootAction, N_MAX_MOVES> child;
    int candidates; // 選択の候補とする数
    bool isChange;
    bool policyAdded = false;

    // 雑多な情報
    int myPlayerNum = -1, rivalPlayerNum = -1;
    double bestReward, worstReward;
    double rewardGap;

    // モンテカルロ用の情報
    std::atomic<bool> exitFlag;
    uint64_t limitSimulations;
    BetaDistribution monteCarloAllScore;
    uint64_t allSimulations;

    // 排他処理
    SpinLock<int> lock_;

    void lock() { lock_.lock(); }
    void unlock() { lock_.unlock(); }

    void setCommonInfo(int num, const Field& field, const SharedData& shared, int limSim);
    void setChange(const Cards *const a, int num,
                   const Field& field, const SharedData& shared, int limSim = -1);
    void setPlay(const MoveInfo *const a, int num,
                 const Field& field, const SharedData& shared, int limSim = -1);

    void addPolicyScoreToMonteCarloScore();
    void feedPolicyScore(const double *const score, int num);

    void feedSimulationResult(int triedIndex, const Field& field, SharedData *const pshared);

    void sort();

    template <class callback_t>
    int sort(int ed, const callback_t& callback) { // 数値基準を定義して候補行動をソート
        const int num = std::min(ed, candidates);
        std::stable_sort(child.begin(), child.begin() + num,
                            [&](const RootAction& a, const RootAction& b)->bool{
                                return callback(a) > callback(b);
                            });
        // 最高評価なものの個数を返す
        for (int i = 0; i < num; i++) {
            if (callback(child[i]) < callback(child[0])) return i;
        }
        return num;
    }
    template <class callback_t>
    int binary_sort(int ed, const callback_t& callback) { // ブール値を定義して候補行動をソート
        const int num = std::min(ed, candidates);
        std::stable_sort(child.begin(), child.begin() + num,
                            [&](const RootAction& a, const RootAction& b)->bool{
                                return (callback(a) ? 1 : 0) > (callback(b) ? 1 : 0);
                            });
        // 最高評価なものの個数を返す
        for (int i = 0; i < num; i++) {
            if ((callback(child[i]) ? 1 : 0) < (callback(child[0]) ? 1 : 0)) return i;
        }
        return num;
    }

    std::string toString(int num = -1) const;

    RootInfo() {
        candidates = -1;
        monteCarloAllScore.set(0, 0);
        allSimulations = 0;
        rivalPlayerNum = -1;
        exitFlag = false;
        unlock();
    }
};