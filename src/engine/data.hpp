#pragma once

// 思考用の構造体
#include "../core/record.hpp"
#include "../core/field.hpp"

#include "galaxy.hpp"
#include "linearPolicy.hpp"

// Field以外のデータ構造
// Fieldは基本盤面情報+盤面を進めたり戻したりするときに値が変化するもの
// それ以外の重目のデータ構造は SharedData
// スレッドごとのデータは ThreadTools

struct ThreadTools {
    // 各スレッドの持ち物
    int threadIndex; // スレッド番号
    XorShift64 dice; // サイコロ
    MoveInfo buf[8192]; // 着手生成バッファ
    Galaxy<ImaginaryWorld> gal;
    void init(int index) {
        threadIndex = index;
        memset(buf, 0, sizeof(buf));
        gal.clear();
    }
    void close() {}
};

struct BaseSharedData {
    std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS> classDestination; // 階級到達回数
    std::array<std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS>, N_PLAYERS> classTransition; // 階級遷移回数
    
    EngineMatchRecord record; // 主観的な対戦棋譜

    // クライアントの個人的スタッツ
    uint32_t playRejection, changeRejection; // リジェクト

    void feedPlayRejection() { playRejection += 1; }
    void feedChangeRejection() { changeRejection += 1; }
    void feedResult(int p, int cl, int ncl) {
        classDestination[p][ncl] += 1;
        classTransition[p][cl][ncl] += 1;
    }
    void initMatch() {
        // スタッツ初期化
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
    template <class gameRecord_t>
    void closeGame(const gameRecord_t& g) {
        // 試合順位の記録
        for (int p = 0; p < N_PLAYERS; p++) {
            feedResult(p, g.classOf(p), g.newClassOf(p));
        }
    }
    void closeMatch() {
        // 共通スタッツ表示
#ifdef MY_MANE
        cerr << endl << "---" << MY_NAME << "'s seiseki happyou!!!---" << endl;
        cerr << "R*NR";
        for (int cl = 0; cl < N_PLAYERS; cl++) {
            cerr << " " << cl << " ";
        }
        cerr << " total" << endl;
        for (int cl = 0; cl < N_PLAYERS; cl++) {
            cerr << "  " << cl << "  ";
            for (int ncl = 0; ncl < N_PLAYERS; ncl++) {
                cerr << classTransition[getMyPlayerNum()][cl][ncl] << "  ";
            }
            cerr << classDestination[getMyPlayerNum()][cl] << endl;
        }
        cerr << "Other Players" << endl;
        for (int p = 0; p < N_PLAYERS; p++) {
            if (p != (int)getMyPlayerNum()) {
                cerr << "Player " << p << " :" << endl;
                cerr << "R*NR";
                for (int cl = 0; cl < N_PLAYERS; cl++) {
                    cerr << " " << cl << " ";
                }
                cerr << " total" << endl;
                for (int cl = 0; cl < N_PLAYERS; cl++) {
                    cerr << "  " << cl << "  ";
                    for (int ncl = 0; ncl < N_PLAYERS; ncl++) {
                        cerr << classTransition[p][cl][ncl] << "  ";
                    }
                    cerr << classDestination[p][cl] << endl;
                }
            }
        }
#else
        cerr << "Players Stats" << endl;
        for (int p = 0; p < N_PLAYERS; p++) {
            cerr << "Player " << p << " :" << endl;
            cerr << "R*NR 0  1  2  3  4  total" << endl;
            for (int cl = 0; cl < N_PLAYERS; cl++) {
                cerr << "  " << cl << "  ";
                for (int ncl = 0; ncl < N_PLAYERS; ncl++) {
                    cerr << classTransition[p][cl][ncl] << "  ";
                }
                cerr << classDestination[p][cl] << endl;
            }
        }
#endif
    }
};

struct SharedData : public BaseSharedData {
    using base_t = BaseSharedData;
    // 全体で共通のデータ

    // 毎回報酬テーブルから持ってこなくてもいいようにこの試合の報酬を置いておくテーブル
    uint16_t gameReward[N_CLASSES];
    
    // 基本方策
    ChangePolicy<policy_value_t> baseChangePolicy;
    PlayPolicy<policy_value_t> basePlayPolicy;

    // 1ゲーム中に保存する一次データのうち棋譜に含まれないもの
    int mateClass; // 初めてMATEと判定した階級の宣言
    int L2Result; // L2における判定結果
    
    // クライアントの個人的スタッツ
    // (勝利, 敗戦) x (勝利宣言, 判定失敗, 敗戦宣言, 無宣言)
    std::array<std::array<long long, 5>, 2> myL2Result;
    // MATEの宣言結果
    std::array<std::array<long long, N_PLAYERS>, N_PLAYERS> myMateResult;
    
    void setMyMate(int bestClass) { // 詰み宣言
        if (mateClass == -1) mateClass = bestClass;
    }
    void setMyL2Result(int result) { // L2詰み宣言
        if (L2Result == -2) L2Result = result;
    }
    void feedResult(int realClass) {
        // ラスト2人
        if (realClass >= N_PLAYERS - 2) {
            bool realWin = realClass == N_PLAYERS - 2;
            if (realWin && L2Result == -1) CERR << "L2 Lucky!" << endl;
            if (!realWin && L2Result == 1) CERR << "L2 Miss!" << endl;
            myL2Result[realClass - (N_PLAYERS - 2)][1 - L2Result] += 1;
        }
        // MATE宣言あり
        if (mateClass != -1) {
            if (mateClass != realClass) { // MATE宣言失敗
                CERR << "Mate Miss! DCL:" << mateClass;
                CERR << " REAL:" << realClass << endl;
            }
            myMateResult[realClass][mateClass] += 1;
        }
    }
    
    void initMatch() {
        base_t::initMatch();
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
        const auto& gameLog = record.latestGame();
        base_t::closeGame(gameLog);
        int myNewClass = gameLog.newClassOf(record.myPlayerNum);

        // 自己スタッツ更新
        feedResult(myNewClass);
    }
    void closeMatch() {
        // 自己スタッツ表示
        cerr << "My Stats" << endl;
        
        cerr << "change rejection by server : " << SharedData::changeRejection << endl;
        cerr << "play rejection by server : " << SharedData::playRejection << endl;
        cerr << "L2 result : " << endl;
        for (auto& a : myL2Result) {
            for (auto i : a) cerr << i << " ";
            cerr << endl;
        }
        cerr << "mate result : " << endl;
        for (auto& a : myMateResult) {
            for (auto i : a) cerr << i << " ";
            cerr << endl;
        }
        cerr << endl;

        base_t::closeMatch();
    }
};

using EngineSharedData = SharedData;
using EngineThreadTools = ThreadTools;

/**************************ルートの手の情報**************************/

struct RootAction {
    MoveInfo move;
    Cards changeCards;
    bool pruned;
    int nextDivision;
    
    // 方策関数出力
    double policyScore, policyProb;
    
    // モンテカルロ結果
    BetaDistribution monteCarloScore;
    BetaDistribution naiveScore;
    
    // モンテカルロの詳細な結果
    BetaDistribution myScore;
    BetaDistribution rivalScore;
    uint64_t simulations;
    std::array<std::array<int64_t, N_CLASSES>, N_PLAYERS> classDistribution;
    uint64_t turnSum;
    
    double mean() const { return monteCarloScore.mean(); }
    double size() const { return monteCarloScore.size(); }
    double mean_var() const { return monteCarloScore.var(); }
    double var() const { return monteCarloScore.var() * size(); }
    double naive_mean() const { return naiveScore.mean(); }
    
    void setChange(Cards cc) {
        clear();
        changeCards = cc;
    }
    void setPlay(MoveInfo mi) {
        clear();
        move = mi;
    }
    
    void clear() {
        move = MOVE_NONE;
        changeCards = CARDS_NULL;
        simulations = 0;
        turnSum = 0;
        for (int p = 0; p < N_PLAYERS; p++) {
            for (int cl = 0; cl < N_CLASSES; cl++) {
                classDistribution[p][cl] = 0;
            }
        }
        monteCarloScore.set(1, 1);
        naiveScore.set(0, 0);
        myScore.set(1, 1);
        rivalScore.set(1, 1);
        policyScore = 0;
        policyProb = -1; // 方策計算に含めないものがあれば自動的に-1になるようにしておく
        pruned = false;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "size = " << size();
        oss << " mean = " << mean();
        oss << " var = " << var();
        return oss.str();
    }
};

/**************************ルートの全体の情報**************************/

struct RootInfo {
    std::array<RootAction, N_MAX_MOVES + 64> child;
    int actions; // 全候補行動数
    int candidates; // 選択の候補とする数
    bool isChange;
    
    // 雑多な情報
    int myPlayerNum, rivalPlayerNum;
    int bestReward, worstReward;
    int rewardGap;
    int bestClass, worstClass;
    
    // モンテカルロ用の情報
    bool exitFlag;
    uint64_t limitSimulations;
    //uint64_t limitTime;
    BetaDistribution monteCarloAllScore;
    uint64_t allSimulations;
#ifdef MULTI_THREADING
    SpinLock<int> lock_;
    void lock() { lock_.lock(); }
    void unlock() { lock_.unlock(); }
#else
    void lock() const {}
    void unlock() const {}
#endif
    void setCommonInfo(int num, const Field& field, const EngineSharedData& shared, int limSim) {
        actions = candidates = num;
        for (int i = 0; i < actions; i++) {
            monteCarloAllScore += child[i].monteCarloScore;
        }
        myPlayerNum = shared.record.myPlayerNum;
        rivalPlayerNum = -1;
        bestClass = field.getBestClass();
        worstClass = field.getWorstClass();
        bestReward = shared.gameReward[bestClass];
        worstReward = shared.gameReward[worstClass];
        rewardGap = bestReward - worstReward;
        uint32_t rivals = field.getRivalPlayersFlag(myPlayerNum);
        if (popcnt(rivals) == 1) {
            int rnum = bsf(rivals);
            if (field.isAlive(rnum)) {
                rivalPlayerNum = rnum;
            }
        }
        limitSimulations = limSim < 0 ? 100000 : limSim;
    }
    void setChange(const Cards *const a, int num,
                   const Field& field, const EngineSharedData& shared, int limSim = -1) {
        isChange = true;
        for (int i = 0; i < num; i++) child[i].setChange(a[i]);
        setCommonInfo(num, field, shared, limSim);
    }
    void setPlay(const MoveInfo *const a, int num,
                 const Field& field, const EngineSharedData& shared, int limSim = -1) {
        isChange = false;
        for (int i = 0; i < num; i++) child[i].setPlay(a[i]);
        setCommonInfo(num, field, shared, limSim);
    }
    
    void prune(int m) {
        // m番目の候補を除外し、空いた位置に末尾の候補を代入
        std::swap(child[m], child[--candidates]);
        child[candidates].pruned = true;
    }
    void addPolicyScoreToMonteCarloScore() {
        // 方策関数の出力をモンテカルロ結果の事前分布として加算
        // 0 ~ 1 の値にする
        double maxScore = -DBL_MAX, minScore = DBL_MAX;
        for (int i = 0; i < candidates; i++) {
            maxScore = max(maxScore, child[i].policyScore + 0.000001);
            minScore = min(minScore, child[i].policyScore);
        }
        // 初期値として加算
        double n = 0;
        if (isChange) {
            n = Settings::rootChangePriorCoef * pow(double(candidates - 1), Settings::rootChangePriorExponent);
        } else {
            n = Settings::rootPlayPriorCoef * pow(double(candidates - 1), Settings::rootPlayPriorExponent);
        }
        for (int i = 0; i < candidates; i++) {
            double r = (child[i].policyScore - minScore) / (maxScore - minScore);
            child[i].monteCarloScore += BetaDistribution(r, 1 - r) * n;
        }
        for (int i = 0; i < candidates; i++) {
            monteCarloAllScore += child[i].monteCarloScore;
        }
    }
    
    void feedPolicyScore(const double *const score, int num) {
        // 方策関数の出力得点と選択確率を記録
        assert(num <= actions && actions > 0 && num > 0);
        double tscore[N_MAX_MOVES + 64];
        for (int i = 0; i < num; i++) {
            child[i].policyScore = tscore[i] = score[i];
        }
        SoftmaxSelector<double> selector(tscore, num, 1);
        for (int i = 0; i < num; i++) {
            child[i].policyProb = selector.prob(i);
        }
    }

    void feedSimulationResult(int triedIndex, const Field& field, EngineSharedData *const pshared) {
        // シミュレーション結果を記録
        // ロックが必要な演算とローカルでの演算が混ざっているのでこの関数内で排他制御する
        
        // 新たに得た証拠分布
        int myRew = field.infoReward[myPlayerNum];
        ASSERT(0 <= myRew && myRew <= bestReward, cerr << myRew << endl;);
        
        // 自分のシミュレーション結果を分布に変換
        BetaDistribution mySc = BetaDistribution((myRew - worstReward) / (double)rewardGap,
                                                 (bestReward - myRew) / (double)rewardGap);
        
#ifdef DEFEAT_RIVAL_MC
        if (rivalPlayerNum < 0) { // 自分の結果だけ考えるとき
            lock();
            child[triedIndex].monteCarloScore += mySc;
            monteCarloAllScore += mySc;
        } else {
            // ライバルの結果も考えるとき
            int rivalRew = field.infoReward[rivalPlayerNum];
            ASSERT(0 <= rivalRew && rivalRew <= bestReward, cerr << rivalRew << endl;);
            
            BetaDistribution rivalSc = BetaDistribution((rivalRew - worstReward) / (double)rewardGap,
                                                        (bestReward - rivalRew) / (double)rewardGap);
            
            constexpr double RIVAL_RATE = 1 / 16.0; // ライバルの結果を重視する割合 0.5 で半々
            
            lock();
            child[triedIndex].myScore += mySc);
            child[triedIndex].rivalScore += rivalSc;
            
            mySc *= 1 - RIVAL_RATE;
            rivalSc.mul(RIVAL_RATE).rev();
            
            child[triedIndex].monteCarloScore += mySc + rivalSc;
            monteCarloAllScore += mySc + rivalSc;
        }
#else
        lock();
        child[triedIndex].monteCarloScore += mySc;
        child[triedIndex].naiveScore += mySc;
        monteCarloAllScore += mySc;
#endif
        
        child[triedIndex].simulations += 1;
        allSimulations += 1;
        
        // 以下参考にする統計量
        child[triedIndex].turnSum += field.turnCount();
        
#ifdef FIXED_N_PLAYOUTS
        if (allSimulations >= FIXED_N_PLAYOUTS) exitFlag = true;
#else
        if (allSimulations >= limitSimulations) exitFlag = true;
#endif
        unlock();
    }
    
    void sort() { // 評価が高い順に候補行動をソート
        std::stable_sort(child.begin(), child.begin() + actions,
                            [&](const RootAction& a, const RootAction& b)->bool{
                                // モンテカルロが同点(またはモンテカルロをやっていない)なら方策の点で選ぶ
                                // ただしルートで方策の点を使わないときにはそうではない
                                // いちおう除外されたものに方策の点がついた場合にはその中で並べ替える
                                if (a.pruned && !b.pruned) return false;
                                if (!a.pruned && b.pruned) return true;
                                if (a.mean() > b.mean()) return true;
                                if (a.mean() < b.mean()) return false;
#ifndef USE_POLICY_TO_ROOT
                                if (allSimulations > 0) return true;
#endif
                                if (a.policyProb > b.policyProb) return true;
                                if (a.policyProb < b.policyProb) return false;
                                return a.policyScore > b.policyScore;
                            });
    }
    template <class callback_t>
    int sort(int ed, const callback_t& callback) { // 数値基準を定義して候補行動をソート
        const int num = min(ed, candidates);
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
        const int num = min(ed, candidates);
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
    
    std::string toString(int num = -1) const {
        if (num == -1)num = actions; // numで表示最大数を設定
        std::ostringstream oss;
        // 先にソートしておく必要あり
        oss << "Reward Zone [ " << worstReward << " ~ " << bestReward << " ] ";
        oss << allSimulations << " trials." << endl;
        for (int i = 0; i < min(actions, num); i++) {
            const int rg = (int)(child[i].mean() * rewardGap);
            const int rew = rg + worstReward;
            const int nrg = (int)(child[i].naive_mean() * rewardGap);
            const int nrew = nrg + worstReward;
            double sem = sqrt(child[i].mean_var());
            const int rewZone[2] = {rew - (int)(sem * rewardGap), rew + (int)(sem * rewardGap)};
            
            if (i == 0) oss << "\033[1m";
            oss << i << " ";
            
            if (isChange) oss << child[i].changeCards;
            else oss << child[i].move;
            
            oss << " : ";
            
            if (child[i].simulations > 0) {
                // まず総合評価点を表示
                oss << rew << " ( " << rewZone[0] << " ~ " << rewZone[1] << " ) ";
                oss << "{mc: " << nrg << "} ";
#ifdef DEFEAT_RIVAL_MC
                if (rivalPlayerNum >= 0) {
                    // 自分とライバルの評価点を表示
                    oss << child[i].myScore;
                    oss << " [mine = " << (worstReward + (int)(child[i].myScore.mean() * (double)rewardGap)) << "] ";
                    oss << child[i].rivalScore;
                    oss << " [rival's = ~" << (bestReward - (int)(child[i].rivalScore.mean() * (double)rewardGap)) << "] ";
                }
#endif
            }
            oss << "prob = " << child[i].policyProb; // 方策関数の確率出力
            oss << " (pol = " << child[i].policyScore << ") "; // 方策関数のスコア
            if (child[i].simulations > 0) {
                oss << "t = " << child[i].turnSum / (double)child[i].simulations << " "; // 統計量
            }
            oss << child[i].simulations << " trials." << endl;
            
            if (i == 0) oss << "\033[0m";
        }
        return oss.str();
    }
    int getExpReward(int idx) const {
        const int rg = (int)(child[idx].mean() * (double)rewardGap);
        const int rew = rg + worstReward;
        return rew;
    }
    
    RootInfo() {
        actions = candidates = -1;
        monteCarloAllScore.set(0, 0);
        allSimulations = 0;
        rivalPlayerNum = -1;
        exitFlag = false;
        unlock();
    }
};
