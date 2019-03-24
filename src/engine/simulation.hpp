#pragma once

// ランダムシミュレーション
#include "engineSettings.h"
#include "../core/daifugo.hpp"
#include "../core/prim2.hpp"
#include "../core/field.hpp"
#include "../core/action.hpp"

#ifdef MODELING_PLAY
#include "playerBias.hpp"
#endif

#ifdef SEARCH_LEAF_MATE
#include "../core/mate.hpp"
#endif

#ifdef SEARCH_LEAF_L2
#include "lastTwo.hpp"
#endif

namespace UECda {
    namespace Fuji {
        template <class sharedData_t, class threadTools_t>
        int startRootSimulation(Field *const pfield,
                                sharedData_t *const pshared,
                                threadTools_t *const ptools) {
            double progress = 1;
            pfield->initForPlayout();
            while (1) {
                DERR << pfield->toString();
                DERR << "turn : " << pfield->getTurnPlayer() << endl;
                uint32_t tp = pfield->getTurnPlayer();

                pfield->prepareForPlay();

                if (Settings::L2SearchInSimulation && pfield->isL2Situation()) { // L2
#ifdef SEARCH_LEAF_L2
                    const uint32_t blackPlayer = tp;
                    const uint32_t whitePlayer = pfield->ps.searchOpsPlayer(blackPlayer);

                    ASSERT(pfield->isAlive(blackPlayer) && pfield->isAlive(whitePlayer),);

                    L2Judge l2(65536, pfield->mv);
                    int l2Result = l2.start_judge(pfield->hand[blackPlayer], pfield->hand[whitePlayer], pfield->bd, pfield->fieldInfo);

                    if (l2Result == L2_WIN) {
                        pfield->setPlayerNewClass(blackPlayer, pfield->getWorstClass() - 1);
                        pfield->setPlayerNewClass(whitePlayer, pfield->getWorstClass());
                        goto GAME_END;
                    } else if (l2Result == L2_LOSE) {
                        pfield->setPlayerNewClass(whitePlayer, pfield->getWorstClass() - 1);
                        pfield->setPlayerNewClass(blackPlayer, pfield->getWorstClass());
                        goto GAME_END;
                    }
#endif // SEARCH_LEAF_L2
                }
                // 合法着手生成
                pfield->NMoves = pfield->NActiveMoves = genMove(pfield->mv, pfield->hand[tp].cards, pfield->bd);
                if (pfield->NMoves == 1) {
                    pfield->setPlayMove(pfield->mv[0]);
                } else {
                    // search mate-move
                    //int idxMate = searchHandMate(0, pfield->mv, pfield->NActiveMoves, pfield->hand[tp], pfield->opsHand[tp], pfield->bd, 1, 1);
                    int idxMate = -1;
#ifdef SEARCH_LEAF_MATE
                    if (Settings::MateSearchInSimulation) {
                        int mateIndex[N_MAX_MOVES];
                        int mates = 0;
                        for (int m = 0; m < pfield->NActiveMoves; m++) {
                            bool mate = checkHandMate(0, pfield->mv + pfield->NActiveMoves, pfield->mv[m],
                                                      pfield->hand[tp], pfield->opsHand[tp], pfield->bd, pfield->fieldInfo);
                            if (mate) mateIndex[mates++] = m;
                        }
                        // 探索順バイアス回避のために必勝全部の中からランダムに選ぶ
                        if (mates == 1) idxMate = mateIndex[0];
                        else if (mates > 1) idxMate = mateIndex[ptools->dice.rand() % mates];
                    }
#endif // SEARCH_LEAF_MATE
                    if (idxMate != -1) { // mate
                        pfield->setPlayMove(pfield->mv[idxMate]);
                        pfield->playMove.setMPMate();
                        pfield->fieldInfo.setMPMate();
                    } else {
                        if (pfield->NActiveMoves <= 1){
                            pfield->setPlayMove(pfield->mv[0]);
                        }else{
                            double score[N_MAX_MOVES + 1];

                            // 行動評価関数を計算
                            calcPlayPolicyScoreSlow(score, *pfield, pshared->basePlayPolicy, 0);

                            // 行動評価関数からの着手の選び方は複数パターン用意して実験できるようにする
                            int idx;
                            if (Settings::simulationSelector == Selector::EXP_BIASED) {
                                // 点差の指数増幅
                                ExpBiasedSoftmaxSelector selector(score, pfield->NActiveMoves,
                                                                  Settings::simulationTemperaturePlay,
                                                                  Settings::simulationAmplifyCoef,
                                                                  1 / log(Settings::simulationAmplifyExponent));
                                if (Settings::simulationPlayModel) {
#ifdef MODELING_PLAY
                                    addPlayerPlayBias(score, pfield->mv, pfield->NActiveMoves, *pfield, pshared->playerModelSpace.model(tp), Settings::playerBiasCoef * progress);
#endif
                                }
                                selector.amplify();
                                selector.to_prob();
                                idx = selector.select(ptools->dice.drand());
                            } else if (Settings::simulationSelector == Selector::POLY_BIASED) {
                                // 点差の多項式増幅
                                BiasedSoftmaxSelector selector(score, pfield->NActiveMoves,
                                                               Settings::simulationTemperaturePlay,
                                                               Settings::simulationAmplifyCoef,
                                                               Settings::simulationAmplifyExponent);
                                if (Settings::simulationPlayModel){
#ifdef MODELING_PLAY
                                    addPlayerPlayBias(score, pfield->mv, pfield->NActiveMoves, *pfield, pshared->playerModelSpace.model(tp), Settings::playerBiasCoef * progress);
#endif
                                }
                                selector.amplify();
                                selector.to_prob();
                                idx = selector.select(ptools->dice.drand());
                            } else if (Settings::simulationSelector == Selector::THRESHOLD) {
                                // 閾値ソフトマックス
                                idx = selectByThresholdSoftmax(score, pfield->NActiveMoves, Settings::simulationTemperaturePlay, 0.02, &ptools->dice);
                            } else {
                                // 単純ソフトマックス
                                idx = selectBySoftmax(score, pfield->NActiveMoves, Settings::simulationTemperaturePlay, &ptools->dice);
                            }
                            pfield->setPlayMove(pfield->mv[idx]);
                        }
                    }
                }
                DERR << tp << " : " << pfield->playMove << " (" << pfield->hand[tp].qty << ")" << endl;
                DERR << pfield->playMove << " " << pfield->ps << endl;
                
                // 盤面更新
                int nextTurnPlayer = pfield->proc(tp, pfield->playMove);
                
                if (nextTurnPlayer == -1) goto GAME_END;
                progress *= 0.95;
            }
        GAME_END:
            for (int p = 0; p < N_PLAYERS; p++) {
                pfield->infoReward.replace(p, pshared->gameReward[pfield->getPlayerNewClass(p)]);
            }
            return 0;
        }

        template <class sharedData_t, class threadTools_t>
        int startRootSimulation(Field *const pfield,
                                MoveInfo mv,
                                sharedData_t *const pshared,
                                threadTools_t *const ptools) {
            DERR << pfield->toString();
            DERR << "turn : " << pfield->getTurnPlayer() << endl;
            if (pfield->procSlowest(mv) == -1) return 0;
            return startRootSimulation(pfield, pshared, ptools);
        }

        template <class sharedData_t, class threadTools_t>
        int startChangeSimulation(Field *const pfield,
                                  int p, Cards c,
                                  sharedData_t *const pshared,
                                  threadTools_t *const ptools) {
            
            int changePartner = pfield->getClassPlayer(getChangePartnerClass(pfield->getPlayerClass(p)));
            
            pfield->makeChange(p, changePartner, c);
            pfield->prepareAfterChange();
            
            DERR << pfield->toString();
            DERR << "turn : " << pfield->getTurnPlayer() << endl;

            return startRootSimulation(pfield, pshared, ptools);
        }
        
        template <class sharedData_t, class threadTools_t>
        int startAllSimulation(Field *const pfield,
                               sharedData_t *const pshared,
                               threadTools_t *const ptools) {
            //cerr << pfield->toString();
            // 試合全部行う
            if(!pfield->isInitGame()){
                // 献上
                //pfield->makePresents(); dealt callbackの時点で行われている
                pfield->removePresentedCards(); // 代わりにこちらの操作を行う
                // 交換
                //double score[N_MAX_CHANGES + 1];
                Cards change[N_MAX_CHANGES];
                for(int cl = 0; cl < MIDDLE; ++cl){
                    const int from = pfield->getClassPlayer(cl);
                    const int to = pfield->getClassPlayer(getChangePartnerClass(cl));
                    const int changes = genChange(change, pfield->getCards(from), N_CHANGE_CARDS(cl));
                    
                    unsigned int index = changeWithPolicy(change, changes,
                                                 pfield->getCards(from), N_CHANGE_CARDS(cl),
                                                 *pfield, pshared->baseChangePolicy, &ptools->dice);
                    ASSERT(index < changes, cerr << index << " in " << changes << endl;)
                    pfield->makeChange(from, to, change[index], N_CHANGE_CARDS(cl));
                }
            }
            pfield->prepareAfterChange();
            DERR << pfield->toString();
            //cerr << pfield->toString();
            return startRootSimulation(pfield, pshared, ptools);
        }
    }
}
