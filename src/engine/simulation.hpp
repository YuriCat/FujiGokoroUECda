#pragma once

// ランダムシミュレーション
#include "engineSettings.h"
#include "../core/daifugo.hpp"
#include "../core/prim2.hpp"
#include "../core/field.hpp"
#include "../core/action.hpp"
#include "engineStructure.hpp"
#include "linearPolicy.hpp"
#include "mate.hpp"
#include "lastTwo.hpp"

namespace UECda {
    int startRootSimulation(Field &f,
                            EngineSharedData *const pshared,
                            EngineThreadTools *const ptools) {
        double progress = 1;
        f.initForPlayout();
        while (1) {
            DERR << f.toString();
            DERR << "turn : " << f.turn() << endl;
            uint32_t tp = f.turn();

            f.prepareForPlay();

            if (Settings::L2SearchInSimulation && f.isL2Situation()) { // L2
#ifdef SEARCH_LEAF_L2
                int black = tp;
                int white = f.seatPlayer(f.board.nextAlive(black));
                assert(f.isAlive(black) && f.isAlive(white));

                L2Judge l2(65536, f.mv);
                int l2Result = l2.start_judge(f.hand[black], f.hand[white],
                                              f.board, f.fieldInfo);

                if (l2Result == L2_WIN) {
                    f.setNewClassOf(black, f.worstClass() - 1);
                    f.setNewClassOf(white, f.worstClass());
                    goto GAME_END;
                } else if (l2Result == L2_LOSE) {
                    f.setNewClassOf(white, f.worstClass() - 1);
                    f.setNewClassOf(black, f.worstClass());
                    goto GAME_END;
                }
#endif // SEARCH_LEAF_L2
            }
            // 合法着手生成
            f.NMoves = f.NActiveMoves = genMove(f.mv, f.hand[tp].cards, f.board);
            if (f.NMoves == 1) {
                f.setPlayMove(f.mv[0]);
            } else {
                // search mate-move
                int idxMate = -1;
#ifdef SEARCH_LEAF_MATE
                if (Settings::MateSearchInSimulation) {
                    int mateIndex[N_MAX_MOVES];
                    int mates = 0;
                    for (int m = 0; m < f.NActiveMoves; m++) {
                        bool mate = checkHandMate(0, f.mv + f.NActiveMoves, f.mv[m],
                                                    f.hand[tp], f.opsHand[tp], f.board, f.fieldInfo);
                        if (mate) mateIndex[mates++] = m;
                    }
                    // 探索順バイアス回避のために必勝全部の中からランダムに選ぶ
                    if (mates == 1) idxMate = mateIndex[0];
                    else if (mates > 1) idxMate = mateIndex[ptools->dice.rand() % mates];
                }
#endif // SEARCH_LEAF_MATE
                if (idxMate != -1) { // mate
                    f.setPlayMove(f.mv[idxMate]);
                    f.playMove.setMPMate();
                    f.fieldInfo.setMPMate();
                } else {
                    if (f.NActiveMoves <= 1) {
                        f.setPlayMove(f.mv[0]);
                    } else {
                        double score[N_MAX_MOVES + 1];

                        // 行動評価関数を計算
                        calcPlayPolicyScoreSlow(score, f, pshared->basePlayPolicy, 0);

                        // 行動評価関数からの着手の選び方は複数パターン用意して実験できるようにする
                        BiasedSoftmaxSelector<double> selector(score, f.NActiveMoves,
                                                               Settings::simulationTemperaturePlay,
                                                               Settings::simulationAmplifyCoef,
                                                               Settings::simulationAmplifyExponent);
                        int idx = selector.select(ptools->dice.drand());
                        f.setPlayMove(f.mv[idx]);
                    }
                }
            }
            DERR << tp << " : " << f.playMove << " (" << f.hand[tp].qty << ")" << endl;
            DERR << f.playMove << " " << endl;
            
            // 盤面更新
            int nextTurnPlayer = f.play(f.playMove);
            
            if (nextTurnPlayer == -1) goto GAME_END;
            progress *= 0.95;
        }
    GAME_END:
        for (int p = 0; p < N_PLAYERS; p++) {
            f.infoReward.assign(p, pshared->gameReward[f.newClassOf(p)]);
        }
        return 0;
    }

    int startRootSimulation(Field& f,
                            MoveInfo mv,
                            EngineSharedData *const pshared,
                            EngineThreadTools *const ptools) {
        DERR << f.toString();
        DERR << "turn : " << f.turn() << endl;
        if (f.play(mv) < 0) return 0;
        return startRootSimulation(f, pshared, ptools);
    }

    int startChangeSimulation(Field& f, int p, Cards c,
                              EngineSharedData *const pshared,
                              EngineThreadTools *const ptools) {
        
        int changePartner = f.classPlayer(getChangePartnerClass(f.classOf(p)));
        
        f.makeChange(p, changePartner, c);
        f.prepareAfterChange();
        
        DERR << f.toString();
        DERR << "turn : " << f.turn() << endl;

        return startRootSimulation(f, pshared, ptools);
    }

    int startAllSimulation(Field& f,
                           EngineSharedData *const pshared,
                           EngineThreadTools *const ptools) {
        //cerr << f.toString();
        // 試合全部行う
        if (!f.isInitGame()) {
            // 献上
            f.removePresentedCards(); // 代わりにこちらの操作を行う
            // 交換
            Cards change[N_MAX_CHANGES];
            for (int cl = 0; cl < MIDDLE; ++cl) {
                const int from = f.classPlayer(cl);
                const int to = f.classPlayer(getChangePartnerClass(cl));
                const int changes = genChange(change, f.getCards(from), N_CHANGE_CARDS(cl));
                
                unsigned int index = changeWithPolicy(change, changes,
                                                f.getCards(from), N_CHANGE_CARDS(cl),
                                                f, pshared->baseChangePolicy, &ptools->dice);
                ASSERT(index < changes, cerr << index << " in " << changes << endl;)
                f.makeChange(from, to, change[index], N_CHANGE_CARDS(cl));
            }
        }
        f.prepareAfterChange();
        DERR << f.toString();
        //cerr << f.toString();
        return startRootSimulation(f, pshared, ptools);
    }
}
