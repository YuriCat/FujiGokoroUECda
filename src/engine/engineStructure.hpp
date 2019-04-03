#pragma once

// 思考用の構造体
#include "../core/data.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"

#include "galaxy.hpp"
#include "linearPolicy.hpp"

namespace UECda {
    // Field以外のデータ構造
    // Fieldは基本盤面情報+盤面を進めたり戻したりするときに値が変化するもの
    // それ以外の重目のデータ構造は SharedData
    // スレッドごとのデータは ThreadTools
    
    struct EngineThreadTools : public ThreadTools {
        // 各スレッドの持ち物
        
#ifndef POLICY_ONLY
        // MCしないなら世界生成なし
        using galaxy_t = Galaxy<ImaginaryWorld>;
        
        // 世界生成プール
        galaxy_t gal;
#endif
        void init(int index) {
            ThreadTools::init(index);
#ifndef POLICY_ONLY
            gal.clear();
#endif
        }
        void close() {
            ThreadTools::close();
        }
    };
    
    struct EngineSharedData : public SharedData {
        // 全体で共通のデータ
        
        // 計算量解析
        int modeling_time;
        int estimating_by_time;
        
        // 毎回報酬テーブルから持ってこなくてもいいようにこの試合の報酬を置いておくテーブル
        uint16_t gameReward[N_CLASSES];
        uint16_t daifugoSeatGameReward[N_PLAYERS][N_CLASSES];
        
        // 基本方策
        ChangePolicy<policy_value_t> baseChangePolicy;
        PlayPolicy<policy_value_t> basePlayPolicy;

#ifndef POLICY_ONLY
        using galaxy_t = EngineThreadTools::galaxy_t;
        GalaxyAnalyzer<galaxy_t, N_THREADS> ga;
#endif
        // 1ゲーム中に保存する一次データのうち棋譜に含まれないもの
        int mateClass; // 初めてMATEと判定した階級の宣言
        int L2Class; // L2において判定した階級の宣言
        
        // クライアントの個人的スタッツ
        std::array<std::array<uint32_t, 3>, 2> myL2Result; // (勝利宣言, 無宣言, 敗戦宣言) × (勝利, 敗戦)
        std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS> myMateResult; // MATEの宣言結果
        
        void setMyMate(int bestClass) { // 詰み宣言
            if (mateClass == -1) mateClass = bestClass;
        }
        void setMyL2Mate() { // L2詰み宣言
            if (L2Class == -1) L2Class = N_PLAYERS - 2;
        }
        void setMyL2GiveUp() { // L2敗北宣言
            if (L2Class == -1) L2Class = N_PLAYERS - 1;
        }
        
        void feedL2Result(int realClass) {
            int index = (L2Class == -1) ? 1 : (L2Class == N_PLAYERS - 1 ? 2 : 0);
            if (L2Class != -1) {
                if (L2Class > realClass) CERR << "L2 Lucky!" << endl;
                if (L2Class < realClass) CERR << "L2 Miss!" << endl;
            }
            myL2Result[realClass - (N_PLAYERS - 2)][index] += 1;
        }
        void feedMyMateResult(int realClass) {
            if (mateClass != realClass) { // MATE宣言失敗
                CERR << "Mate Miss! DCL:" << mateClass;
                CERR << " REAL:" << realClass << endl;
            }
            myMateResult[realClass][mateClass] += 1;
        }
        
        void feedResult(int myClass) {
            // ラスト2人
            if (myClass >= N_PLAYERS - 2) feedL2Result(myClass);
            // MATE宣言あり
            if (mateClass != -1) feedMyMateResult(myClass);
        }
        
        void initMatch() {
            // スタッツ初期化
            for (auto& a : myMateResult) a.fill(0);
            for (auto& a : myL2Result) a.fill(0);
            // 計算量解析初期化
            modeling_time = 0;
            estimating_by_time = 0;
        }
        void setMyPlayerNum(int p) {}
        void initGame() {
            SharedData::initGame();
            mateClass = L2Class = -1;
        }
        
        void closeGame() {
            const auto& gameLog = matchLog.latestGame();
            SharedData::closeGame(gameLog);
            int myNewClass = gameLog.getPlayerNewClass(matchLog.getMyPlayerNum());
            
#if defined(POLICY_ONLY) && defined(RL_POLICY)
            // reinforcement learning
            playLearner.feedReward(((N_PLAYERS - 1) / 2.0 - myNewClass) / (N_PLAYERS - 1));
            playLearner.updateParams();
#endif
            // スタッツ更新
            feedResult(myNewClass);
        }
        void closeMatch() {
            //#ifdef MONITOR
            // 全体スタッツ表示
            
            //#endif
            
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
            }cerr << endl;

            SharedData::closeMatch();
        }
    };

    /**************************ルートの手の情報**************************/
    
    struct RootAction {
        MoveInfo move;
        Cards changeCards;
        bool pruned;
        
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
            move = MOVEINFO_NONE;
            changeCards = CARDS_NULL;
            simulations = 0;
            turnSum = 0;
            for (int p = 0; p < N_PLAYERS; ++p)
                for (int cl = 0; cl < N_CLASSES; ++cl)
                    classDistribution[p][cl] = 0;
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
            for (int m = 0; m < actions; m++)
                monteCarloAllScore += child[m].monteCarloScore;
            myPlayerNum = shared.matchLog.getMyPlayerNum();
            rivalPlayerNum = -1;
            bestClass = field.getBestClass();
            worstClass = field.getWorstClass();
            bestReward = shared.gameReward[bestClass];
            worstReward = shared.gameReward[worstClass];
            rewardGap = bestReward - worstReward;
            uint32_t rivals = field.getRivalPlayersFlag(myPlayerNum);
            if (countBits(rivals) == 1) {
                int rnum = bsf(rivals);
                if (field.isAlive(rnum)) {
                    rivalPlayerNum = rnum;
                }
            }
            limitSimulations = (limSim < 0) ? 100000 : limSim;
        }
        void setChange(const Cards *const a, int num,
                       const Field& field, const EngineSharedData& shared, int limSim = -1) {
            isChange = true;
            for (int m = 0; m < num; m++) child[m].setChange(a[m]);
            setCommonInfo(num, field, shared, limSim);
        }
        void setPlay(const MoveInfo *const a, int num,
                     const Field& field, const EngineSharedData& shared, int limSim = -1) {
            isChange = false;
            for (int m = 0; m < num; m++) child[m].setPlay(a[m]);
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
            for (int m = 0; m < candidates; m++) {
                maxScore = max(maxScore, child[m].policyScore + 0.000001);
                minScore = min(minScore, child[m].policyScore);
            }
            // 初期値として加算
            double n = 0;
            if (isChange)
                n = Settings::rootChangePriorCoef * pow(double(candidates - 1), Settings::rootChangePriorExponent);
            else
                n = Settings::rootPlayPriorCoef * pow(double(candidates - 1), Settings::rootPlayPriorExponent);
            for (int m = 0; m < candidates; m++) {
                double r = (child[m].policyScore - minScore) / (maxScore - minScore);
                child[m].monteCarloScore += BetaDistribution(r, 1 - r) * n;
            }
            for (int m = 0; m < candidates; m++)
                monteCarloAllScore += child[m].monteCarloScore;
        }
        
        void feedPolicyScore(const double *const score, int num) {
            // 方策関数の出力得点と選択確率を記録
            assert(num <= actions && actions > 0 && num > 0);
            double tscore[N_MAX_MOVES + 64];
            for (int m = 0; m < num; m++)
                child[m].policyScore = tscore[m] = score[m];
            SoftmaxSelector selector(tscore, num, 1);
            selector.to_prob();
            for (int m = 0; m < num; m++)
                child[m].policyProb = selector.prob(m);
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
            child[triedIndex].turnSum += field.getTurnNum();
            
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
            for (int m = 0; m < num; m++)
                if (callback(child[m]) < callback(child[0])) return m;
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
            for (int m = 0; m < num; m++)
                if ((callback(child[m]) ? 1 : 0) < (callback(child[0]) ? 1 : 0)) return m;
            return num;
        }
        
        std::string toString(int num = -1) const {
            if (num == -1)num = actions; // numで表示最大数を設定
            std::ostringstream oss;
            // 先にソートしておく必要あり
            oss << "Reward Zone [ " << worstReward << " ~ " << bestReward << " ] ";
            oss << allSimulations << " trials." << endl;
            for (int m = 0; m < min(actions, num); m++) {
                const int rg = (int)(child[m].mean() * rewardGap);
                const int rew = rg + worstReward;
                const int nrg = (int)(child[m].naive_mean() * rewardGap);
                const int nrew = nrg + worstReward;
                double sem = sqrt(child[m].mean_var());
                const int rewZone[2] = {rew - (int)(sem * rewardGap), rew + (int)(sem * rewardGap)};
                
                if (m == 0) oss << "\033[1m";
                oss << m << " ";
                
                if (isChange) oss << child[m].changeCards;
                else oss << child[m].move.mv();
                
                oss << " : ";
                
                if (child[m].simulations > 0) {
                    // まず総合評価点を表示
                    oss << rew << " ( " << rewZone[0] << " ~ " << rewZone[1] << " ) ";
                    oss << "{mc: " << nrg << "} ";
#ifdef DEFEAT_RIVAL_MC
                    if (rivalPlayerNum >= 0) {
                        // 自分とライバルの評価点を表示
                        oss << child[m].myScore;
                        oss << " [mine = " << (worstReward + (int)(child[m].myScore.mean() * (double)rewardGap)) << "] ";
                        oss << child[m].rivalScore;
                        oss << " [rival's = ~" << (bestReward - (int)(child[m].rivalScore.mean() * (double)rewardGap)) << "] ";
                    }
#endif
                }
                oss << "prob = " << child[m].policyProb; // 方策関数の確率出力
                oss << " (pol = " << child[m].policyScore << ") "; // 方策関数のスコア
                if (child[m].simulations > 0) {
                    oss << "t = " << child[m].turnSum / (double)child[m].simulations << " "; // 統計量
                }
                oss << child[m].simulations << " trials." << endl;
                
                if (m == 0)oss << "\033[0m";
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
}
