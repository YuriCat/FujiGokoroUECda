/*
 rating.hpp
 Katsuki Ohto
 */

#pragma once

// シミュレーション結果を用いた相対レーティングの計算
#include "../core/minLog.hpp"
#include "simulation.hpp"

namespace UECda{
    namespace Fuji{
        
        struct RateCalculationData{
            SpinLock<uint32_t> lock;
            std::atomic<int> trials;
            
            //std::array<double, N_PLAYERS> rewardSum = {0};
            // プレーヤー間の相対的な勝ち数
            std::array<std::array<BetaDistribution, N_PLAYERS>, N_PLAYERS> relativeWins;
            std::array<BetaDistribution, N_PLAYERS> absoluteWins;
            
            RateCalculationData(){
                lock.unlock();
                trials = 0;
                for(int p = 0; p < N_PLAYERS; ++p){
                    for(int pp = 0; pp < N_PLAYERS; ++pp){
                        relativeWins[p][pp].set(0.5, 0.5);
                    }
                }
                for(int p = 0; p < N_PLAYERS; ++p){
                    absoluteWins[p].set(0.5, 0.5);
                }
            }
        };
        
        template<class field_t, class sharedData_t, class threadTools_t>
        void simulationThreadForRating(RateCalculationData *const pdst,
                                       const field_t *const pfield,
                                       const int simulations,
                                       sharedData_t *const pshared,
                                       threadTools_t *const ptools){
            while (pdst->trials++ < simulations)
            {
                Field tfield = *pfield;
                tfield.setMoveBuffer(ptools->buf);
                tfield.setDice(&ptools->dice);
                
                // シミュレーション終了の条件は試合終了となっているので再設定しなくてよい
                startAllSimulation(&tfield, pshared, ptools);
                
                pdst->lock.lock();
                // 相対勝ち数を加算
                for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                    for(int p1 = 0; p1 < N_PLAYERS; ++p1){
                        if(tfield.getPlayerNewClass(p0) < tfield.getPlayerNewClass(p1)){
                            pdst->relativeWins[p0][p1] += BetaDistribution(1, 0);
                        }else{
                            pdst->relativeWins[p0][p1] += BetaDistribution(0, 1);
                        }
                    }
                }
                // 絶対勝ち数を加算
                for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                    int wins = DAIHINMIN - tfield.getPlayerNewClass(p0);
                    double score = wins / double(N_PLAYERS - 1);
                    pdst->absoluteWins[p0] += BetaDistribution(score, 1 - score);
                }
                pdst->lock.unlock();
            }
        }
        
        template<class gameLog_t, class sharedData_t, class threadTools_t>
        void doSimulationsToEvaluate(const gameLog_t& gLog,
                                     const int simulations,
                                     RateCalculationData *const presult,
                                     sharedData_t *const pshared,
                                     threadTools_t tools[]){
            Field field;
            iterateGameLogBeforePlay
            (field, gLog,
             [](const auto& field)->void{}, // first callback
             [&](const auto& field)->void{ // dealt callback
                 // シミュレーションにより結果を予測
                 std::vector<std::thread> thr;
                 for(int ith = 0; ith < N_THREADS; ++ith){
                     thr.emplace_back(std::thread(&simulationThreadForRating<Field, sharedData_t, threadTools_t>,
                                                  presult, &field, simulations, pshared, &tools[ith]));
                 }
                 for(auto& th : thr){
                     th.join();
                 }
             },
             [](const auto& field, const int from, const int to, const Cards c)->int{ return 0; }, // change callback
             [](const auto& field)->void{}); // last callback
        }
        
        template<class gameLog_t, class sharedData_t, class threadTools_t>
        std::array<std::array<BetaDistribution, N_PLAYERS>, N_PLAYERS> doSimulationsToGetRalativeWp(const gameLog_t& gLog,
                                                                                                    const int simulations,
                                                                                                    sharedData_t *const pshared,
                                                                                                    threadTools_t tools[]){
            RateCalculationData result;
            doSimulationsToEvaluate(gLog, simulations, &result, pshared, tools);
            return result.relativeWins;
        }
        
        template<class gameLog_t, class sharedData_t, class threadTools_t>
        std::array<BetaDistribution, N_PLAYERS> doSimulationsToGetAbsoluteWp(const gameLog_t& gLog,
                                                                             const int simulations,
                                                                             sharedData_t *const pshared,
                                                                             threadTools_t tools[]){
            RateCalculationData result;
            doSimulationsToEvaluate(gLog, simulations, &result, pshared, tools);
            return result.absoluteWins;
        }
        
        double calcBiasedExpectedWp(const double r0, const double r1, const double fieldWp){
            return 1 / (1 + pow(10, -(r0 - r1) / 400 - log(fieldWp / (1 - fieldWp))));
        }
        
        template<class array_t, class distributionMatrix_t>
        std::array<std::array<double, N_PLAYERS>, N_PLAYERS> calcExpectedRelativeWp(const array_t& orgRate,
                                                                                    const distributionMatrix_t& relativeWins){
            // 現在のレートと初期盤面を考慮した相対勝率を計算
            std::array<std::array<double, N_PLAYERS>, N_PLAYERS> expectedWp;
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                for(int p1 = 0; p1 < N_PLAYERS; ++p1){
                    double fieldWp = relativeWins[p0][p1].mean();
                    expectedWp[p0][p1] = calcBiasedExpectedWp(orgRate[p0], orgRate[p1], fieldWp);
                }
            }
            return expectedWp;
        }
        template<class array_t, class distributionArray_t>
        std::array<double, N_PLAYERS> calcExpectedAbsoluteWp(const array_t& orgRate,
                                                             const distributionArray_t& absoluteWins){
            // 現在のレートと初期盤面を考慮した絶対勝率を計算
            std::array<double, N_PLAYERS> expectedWp;
            double sumRate = 0;
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                sumRate += orgRate[p0];
            }
            double sum = 0;
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                double fieldWp = absoluteWins[p0].mean();
                expectedWp[p0] = calcBiasedExpectedWp(orgRate[p0], (sumRate - orgRate[p0]) / double(N_PLAYERS - 1), fieldWp);
                sum += expectedWp[p0];
            }
            sum /= N_PLAYERS;
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                expectedWp[p0] -= sum - 0.5;
            }
            return expectedWp;
        }
        
        template<class gameLog_t, class matrix_t>
        std::array<double, N_PLAYERS> calcDiffRateByRelativeWp(const gameLog_t& gLog,
                                                               const matrix_t& expectedWp,
                                                               const double coef){
            // 相対勝率からレート差分を計算
            std::array<double, N_PLAYERS> diffRate;
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                double sum = 0;
                for(int p1 = 0; p1 < N_PLAYERS; ++p1){
                    cerr << expectedWp[p0][p1] << " ";
                    if(p0 != p1){
                        double realWin = (gLog.getPlayerNewClass(p0) < gLog.getPlayerNewClass(p1)) ? 1 : 0;
                        sum += realWin - expectedWp[p0][p1];
                    }
                }cerr << endl;
                diffRate[p0] = coef * sum / (N_PLAYERS - 1);
            }
            return diffRate;
        }
        
        template<class gameLog_t, class array_t>
        std::array<double, N_PLAYERS> calcDiffRateByAbsoluteWp(const gameLog_t& gLog,
                                                               const array_t& expectedWp,
                                                               const double coef){
            // 絶対勝率からレート差分を計算
            std::array<double, N_PLAYERS> diffRate;
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                //cerr << expectedWp[p0] << " ";
                double realScore = (DAIHINMIN - gLog.getPlayerNewClass(p0)) / double(N_PLAYERS - 1);
                //cerr << expectedWp[p0] << " -> " << realScore << endl;
                diffRate[p0] = coef * (realScore - expectedWp[p0]);
            }cerr << endl;
            cerr << mean(expectedWp) << endl;
            return diffRate;
        }
        
        template<class array_t, class gameLog_t, class sharedData_t, class threadTools_t>
        std::array<double, N_PLAYERS> calcDiffRateByRelativeWpWithSimulation(const array_t& orgRate,
                                                                             const gameLog_t& gLog,
                                                                             const int simulations,
                                                                             const double coef,
                                                                             sharedData_t *const pshared,
                                                                             threadTools_t tools[]){
            // シミュレーションを行い、相対勝率もとにレートの更新幅を計算する
            auto result = doSimulationsToGetRalativeWp(gLog, simulations, pshared, tools);
            auto expectedWp = calcExpectedRelativeWp(orgRate, result);
            return calcDiffRateByRelativeWp(gLog, expectedWp, coef);
        }
        
        template<class array_t, class gameLog_t, class sharedData_t, class threadTools_t>
        std::array<double, N_PLAYERS> calcDiffRateByAbsoluteWpWithSimulation(const array_t& orgRate,
                                                                             const gameLog_t& gLog,
                                                                             const int simulations,
                                                                             const double coef,
                                                                             sharedData_t *const pshared,
                                                                             threadTools_t tools[]){
            // シミュレーションを行い、相対勝率もとにレートの更新幅を計算する
            auto result = doSimulationsToGetAbsoluteWp(gLog, simulations, pshared, tools);
            auto expectedWp = calcExpectedAbsoluteWp(orgRate, result);
            return calcDiffRateByAbsoluteWp(gLog, expectedWp, coef);
        }
        
        template<class distribution_t>
        std::vector<double> calcExpectedTotalScore(const std::vector<double>& rates,
                                                   const distribution_t& distribution){
            // 与えられたレートのプレーヤー集合にて総当たりリーグ戦を行ったときの推定得点を求める
            const int players = rates.size();
            /*std::vector<std::vector<double>> matrix;
             for(int p = 0; p < players; ++p){
             matrix.emplace_back(std::vector<double>(players, 0));
             }*/
            std::vector<double> ans;
            for(int p0 = 0; p0 < players; ++p0){
                double expectedWpSum = 0;
                for(int p1 = 0; p1 < players; ++p1){
                    if(p0 != p1){
                        // 数値積分
                        const int precision = 50000;
                        double sum = 0;
                        for(int i = 1; i <= precision; ++i){
                            double x = i / double(precision + 1);
                            double density = distribution.dens(x);
                            double wp = calcBiasedExpectedWp(rates[p0], rates[p1], x);
                            //cerr << x << " " << density << " " << wp << endl;
                            sum += density * wp;
                        }
                        sum /= precision;
                        expectedWpSum += sum;
                    }
                }
                ans.push_back(expectedWpSum * (N_PLAYERS - 1) / (players - 1) + 1);
            }
            return ans;
        }
    }
}
