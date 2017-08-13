/*
 monteCarlo.h
 Katsuki Ohto
 */

#ifndef UECDA_FUJI_MONTECARLO_H_
#define UECDA_FUJI_MONTECARLO_H_

#include "../../structure/primitive/prim.hpp"
#include "../../structure/primitive/prim2.hpp"

#include "../policy/changePolicy.hpp"
#include "../policy/playPolicy.hpp"

#include "playout.h"
#include "playouter.hpp"

// モンテカルロ法ヘッダ

namespace UECda{
    namespace Fuji{
        
        namespace MonteCarlo{
            // モンテカルロ法
            constexpr uint32_t MINNEC_N_TRIALS = 4; // 全体での最小限のトライ数。UCB-Rootにしたので実質不要になった
        }
        
        struct MonteCarloChild{
            // モンテカルロ探索候補着手
            
            //std::atomic<uint64_t> evalSum;
            uint32_t trials;
            
#ifdef DEFEAT_RIVAL_MC
            // 自分とライバルの結果を別々に記録する場合
            BetaDistribution myScore;
            BetaDistribution rivalScore;
#endif
            
            // シミュレーション結果
            BetaDistribution score;
            
            // 方策関数の出したスコア
            double policyScore;
            
            MoveInfo mi; // プレー用
            Cards c; // 交換用
            
            uint64_t hash; // 手を表すハッシュ値があれば
            
            // 統計量
            uint64_t turnSum; // 終端（あがり、負け、末端探索）までの手数合計
            
            void init(){
                // 初期分布
                constexpr double BETA_A = 1.0;
                constexpr double BETA_B = 1.0;
                
                score.set(BETA_A, BETA_B);
                
#ifdef DEFEAT_RIVAL_MC
                myScore.set(BETA_A, BETA_B);
                rivalScore.set(BETA_A, BETA_B);
#endif
                
                trials = 0;
                policyScore = 0;
                
                // 統計量初期化
                turnSum = 0;
                
                mi = MOVEINFO_NULL;
                c = CARDS_NULL;
            }
            void setPlay(const MoveInfo ami){
                mi = ami;
            }
            void setChange(const Cards ac){
                c = ac;
            }
            
            // 統計量を返す
            double meanTurn()const{ return turnSum / (double)trials; }
            
            double mean()const{ return score.mean(); }
            double size()const{ return score.size(); }
            double mean_var()const{ return score.var(); }
            double var()const{ return score.var() * size(); }
            
            std::string toString()const{
                std::ostringstream oss;
                oss << "size = " << size() << " mean = " << mean() << " var = " << var();
                return oss.str();
            }
        };
        
        struct MonteCarloRootNode{
            // 現在着手決定する根ノード
            // 候補の与えられ方がMoveInfo型でない場合にも対応必須
            
            bool isChange;
            
            std::atomic<uint32_t> trials;
            uint32_t maxTrials; // シミュレーション回数の限度
            
            int NChilds;
            //std::atomic<int> ex; // 終了指示
            int ex;
            
#ifdef MULTI_THREADING
            SpinLock<int> lock_;
#endif
            
            BetaDistribution allScore;
            
            // 報酬の上界、下界
            int bestReward, worstReward;
            int rewardGap;
            
            int myPlayerNum;
            int rivalPlayerNum;
            
            int NPlayers;
            int bestClass, worstClass;
            
            FieldAddInfo fieldInfo;
            
            MonteCarloChild child[256];
            
            
#ifdef MULTI_THREADING
            void lock()noexcept{ lock_.lock(); }
            void unlock()noexcept{ lock_.unlock(); }
#else
            void lock()const noexcept{}
            void unlock()const noexcept{}
#endif
            
            template<class field_t, class sharedData_t>
            void setCommon(const field_t& field, const sharedData_t& shared){
                myPlayerNum = field.getMyPlayerNum();
                NPlayers = field.getNAlivePlayers();
                bestClass = field.getBestClass();
                worstClass = field.getWorstClass();
                
                // 報酬設定
                {
                    bestReward = shared.game_reward[bestClass];
                    worstReward = shared.game_reward[worstClass];
                    rewardGap = bestReward - worstReward;
                }
                
#ifdef DEFEAT_RIVAL_MC
                uint32_t rivals = field.getRivalPlayersFlag();
                if(countBits(rivals) == 1){
                    int rnum = bsf(rivals);
                    if(field.isAlive(rnum)){
                        rivalPlayerNum = rnum;
                    }
                }
#endif
            }
            
            template<class playSpace_t, class field_t, class sharedData_t>
            void setPlay(playSpace_t& moves, const int NCands,
                         const field_t& field, const FieldAddInfo& argFieldInfo,
                         const unsigned int maxT,
                         const sharedData_t& shared){
                
                isChange = false;
                setCommon(field, shared);
                
                maxTrials = maxT;
                fieldInfo = argFieldInfo;
                
                NChilds = min(256, NCands); // 多すぎる時はカット
                
                for(int c = 0; c < NChilds; ++c){
                    child[c].init();
                    child[c].setPlay(moves.getMoveById(c));
                }
                
                double score[256];
                PlayouterField tfield;
                setSubjectiveField(field, &tfield);
                calcPlayPolicyScoreSlow<0>(score, moves.getMovePtr(), NChilds, tfield, shared.basePlayPolicy);
                // 線形加算の次元でスコアを計算
                for(int m = 0; m < NChilds; ++m){
                    child[m].policyScore = score[m]; // スコアを記録
                }
                
#ifdef USE_POLICY_TO_ROOT
                // 0 ~ 1 の値にする
                double maxScore = -DBL_MAX, minScore = +DBL_MAX;
                for(int m = 0; m < NChilds; ++m){
                    maxScore = max(maxScore, score[m] + 0.000001);
                    minScore = min(minScore, score[m]);
                }
                // 初期値として加算
                double n = Settings::rootPlayPriorCoef * pow(double(NChilds - 1), Settings::rootPlayPriorExponent);
                for(int m = 0; m < NChilds; ++m){
                    double r = (score[m] - minScore) / (maxScore - minScore);
                    child[m].score += BetaDistribution(r, 1 - r) * n;
                }
#endif
                for(int c = 0; c < NChilds; ++c){
                    allScore += child[c].score;
                }
            }
            
            template<class field_t, class sharedData_t>
            void setChange(const Cards *const cand, const int NCands, const field_t& field,
                           const unsigned int maxT,
                           const sharedData_t& shared){
                
                isChange = true;
                
                setCommon(field, shared);
                
                maxTrials = maxT;
                
                NChilds = NCands;
                
                for(int c = 0; c < NChilds; ++c){
                    child[c].init();
                    child[c].setChange(cand[c]);
                }
                
                double score[256 + 1];
                PlayouterField tfield;
                setSubjectiveField(field, &tfield);
                calcChangePolicyScoreSlow<0>(score, cand, NChilds,
                                             field.getMyCards(), N_CHANGE_CARDS(field.getMyClass()),
                                             tfield, shared.baseChangePolicy);
                double score2[256];
                // scoreを差分をとって通常の形状にし、線形加算の次元に直す
                for(int m = 0; m < NChilds; ++m){
                    score2[m] = log(max(score[m + 1] - score[m], 0.000001)) * Settings::temperatureChange;
                }
                for(int m = 0; m < NChilds; ++m){
                    child[m].policyScore = score2[m]; // スコアを記録
                }
                
#ifdef USE_POLICY_TO_ROOT
                // 0 ~ 1 の値にする
                double maxScore = -DBL_MAX, minScore = +DBL_MAX;
                for(int m = 0; m < NChilds; ++m){
                    maxScore = max(maxScore, score2[m] + 0.000001);
                    minScore = min(minScore, score2[m]);
                }
                // 初期値として加算
                double n = Settings::rootChangePriorCoef * pow(double(NChilds - 1), Settings::rootChangePriorExponent);
                for(int m = 0; m < NChilds; ++m){
                    double r = (score2[m] - minScore) / (maxScore - minScore);
                    child[m].score += BetaDistribution(r, 1 - r) * n;
                }
#endif
                for(int c = 0; c < NChilds; ++c){
                    allScore += child[c].score;
                }
            }
            
            template<class sharedData_t>
            void feedResult(const int tryId, const PlayouterField& field, sharedData_t *const pshared){
                // シミュレーション結果を記録
                // ロックが必要な演算とローカルでの演算が混ざっているのでこの関数内で排他制御する
                
                // 新たに得た証拠分布
                int myRew = field.infoReward[myPlayerNum];
                ASSERT(0 <= myRew && myRew <= bestReward, cerr << myRew << endl;);
                
                // 自分のシミュレーション結果を分布に変換
                BetaDistribution mySc = BetaDistribution((myRew - worstReward) / (double)rewardGap, (bestReward - myRew) / (double)rewardGap);
                
#ifdef DEFEAT_RIVAL_MC
                if(rivalPlayerNum < 0){
                    // 自分の結果だけ考えるとき
                    lock();
                    child[tryId].score += mySc;
                    allScore += mySc;
                }else{
                    // ライバルの結果も考えるとき
                    int rivalRew = field.infoReward[rivalPlayerNum];
                    ASSERT(0 <= rivalRew && rivalRew <= bestReward, cerr << rivalRew << endl;);
                    
                    BetaDistribution rivalSc = BetaDistribution((rivalRew - worstReward) / (double)rewardGap, (bestReward - rivalRew) / (double)rewardGap);

                    constexpr double RIVAL_RATE = 1 / 16.0;
                    
                    lock();
                    child[tryId].myScore += mySc);
                    child[tryId].rivalScore += rivalSc;
                    
                    mySc *= (1 - RIVAL_RATE);
                    rivalSc.mul(RIVAL_RATE).rev();
                    
                    child[tryId].score += mySc + rivalSc;
                    allScore += mySc + rivalSc;
                    
                    if(tryId == 0){
                        //cerr<<child[tryId].score<<endl;getchar();
                    }
                }
#else
                lock();
                child[tryId].score += mySc;
                allScore += mySc;
#endif
                
                ++child[tryId].trials;
                ++trials;
                
                // 以下参考にする統計量
                child[tryId].turnSum += field.getTurnNum();
                
#ifdef FIXED_N_PLAYOUTS
                if(trials >= (FIXED_N_PLAYOUTS)){
                    ex = 1;
                }
#else
                if(trials >= maxTrials){
                    ex = 1;
                }
#endif
                unlock();
                
                if(child[tryId].mean() > pshared->exp_wr){
                    pshared->exp_wr = child[tryId].mean();
                }
            }
            
            int getExpReward(int idx)const{
                const int rg = (int)(child[idx].mean() * (double)rewardGap);
                const int rew = rg + worstReward;
                return rew;
            }
            
            int searchBestIndex()const{
                // 最高評価のidを1つ返す
                int best = 0;
                double bestScore = -99999;
                for(int c = 0; c < NChilds; ++c){
                    double tmpScore = child[c].mean();
                    if(tmpScore > bestScore){
                        bestScore = tmpScore;
                        best = c;
                    }
                }
                return best;
            }
            
            int searchMove(Move mv)const{
                // 候補からmvを探してインデックスを返す
                for(int c = 0; c < NChilds; ++c){
                    if((uint32_t)(child[c].mi.mv()) == (uint32_t)(mv)){
                        return c;
                    }
                }
                return -1;
            }
            
            template<class dice_t>
            int chooseBestUCB(dice_t *const dice)const{
                
                int bestId = -1;
                //int bestId = 0; // UCB値がnanやinfになったときのために仕方なく
                
                switch(NChilds){
                    case 0: UNREACHABLE; break;
                    case 1: UNREACHABLE; break;
                    case 2:{
                        // 2つの時は同数(分布サイズ単位)に割り振る
                        if(child[0].size() < child[1].size()){
                            bestId = 0;
                        }else if(child[0].size() > child[1].size()){
                            bestId = 1;
                        }else{
                            bestId = dice->rand() % 2;
                        }
                    }break;
                    default:{
                        // UCB-root アルゴリズムに変更
                        double bestScore = -DBL_MAX;
                        const double allSize = allScore.size();
                        const double sqrtAS = sqrt(allSize);
                        for(int c = 0; c < NChilds; ++c){
                            double tmpScore;
                            double size = child[c].size();
                            if(child[c].trials < MonteCarlo::MINNEC_N_TRIALS){
                                // 最低プレイアウト数をこなしていないものは、大きな値にする
                                // ただし最低回数のもののうちどれかがランダムに選ばれるようにする
                                tmpScore = (double)((1U << 16) - (child[c].trials << 8) + (dice->rand() % (1U << 6)));
                            }else{
                                ASSERT(size, cerr << child[c].toString() << endl;);
                                double ucbr = child[c].mean() + 0.7 * sqrt(sqrtAS / size); // ucbr値
                                tmpScore = ucbr;
                            }
                            if(tmpScore > bestScore){
                                bestScore = tmpScore;
                                bestId = c;
                            }
                        }
                    }break;
                }
                return bestId;
            }
            
            void printResult(int num = 10)const{
                // モンテカルロ解析結果表示
                cerr << "Reward Zone [ " << worstReward << " ~ " << bestReward << " ]" << endl;
                
                // データをソート
                std::array<MonteCarloChild, 256> tchild;
                for(int c = 0; c < NChilds; ++c){
                    tchild[c] = child[c];
                }
                std::sort(tchild.begin(), tchild.begin() + NChilds, [](const auto& ch0, const auto& ch1)->bool{
                    return ch0.mean() > ch1.mean();
                });
                
                for(int c = 0; c < min(NChilds, num); ++c){
                    
                    const int rg = (int)(tchild[c].mean() * (double)rewardGap);
                    const int rew = rg + worstReward;
                    double sem = sqrt(tchild[c].mean_var());
                    const int rewZone[2] = {rew - (int)(sem * rewardGap), rew + (int)(sem * rewardGap)};
                    
                    if(c == 0){
                        cerr << "\033[1m";
                    }
                    cerr << c << " ";
                    
                    if(isChange){
                        cerr << OutCards(tchild[c].c);
                    }else{
                        cerr << tchild[c].mi.mv();
                    }
                    
                    // まず総合評価点を表示
                    cerr << " : " << rew << " ( " << rewZone[0] << " ~ " << rewZone[1] << " ) ";
                    
#ifdef DEFEAT_RIVAL_MC
                    if(rivalPlayerNum >= 0){
                        // 自分とライバルの評価点を表示
                        cerr << tchild[c].myScore;
                        cerr << " [mine = " << (worstReward + (int)(tchild[c].myScore.mean() * (double)rewardGap)) << "] ";
                        cerr << tchild[c].rivalScore;
                        cerr << " [rival's = ~" << (bestReward - (int)(tchild[c].rivalScore.mean() * (double)rewardGap)) << "] ";
                    }
#endif
                    // 方策関数のスコアを表示
                    cerr << "pol = " << tchild[c].policyScore << " ";
                    
                    // 統計量を表示
                    cerr << "t = " << tchild[c].meanTurn() << " ";
                    
                    cerr << tchild[c].trials << " trials." << endl;
                    
                    if(c == 0){
                        cerr << "\033[0m";
                    }
                }
            }
            
            void init(){
                trials = 0;
                ex = 0;
                rivalPlayerNum = -1;
                allScore.set(0, 0);
                unlock();
            }
            MonteCarloRootNode(){
                init();
            }
        };
    }
}
#endif // UECDA_FUJI_MONTECARLO_H_

