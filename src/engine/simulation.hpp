#pragma once

// ランダムシミュレーション
#include "engineSettings.h"
#include "../core/daifugo.hpp"
#include "../core/prim2.hpp"
#include "../core/field.hpp"
#include "../core/action.hpp"
#include "data.hpp"
#include "linearPolicy.hpp"
#include "mate.hpp"
#include "lastTwo.hpp"

int simulation(Field& field,
               SharedData *const pshared,
               ThreadTools *const ptools) {
    double progress = 1;
    field.initForPlayout();
    while (1) {
        DERR << field.toString();
        DERR << "turn : " << field.turn() << endl;
        uint32_t tp = field.turn();
        field.prepareForPlay();

        if (Settings::L2SearchInSimulation && field.isL2Situation()) { // L2
#ifdef SEARCH_LEAF_L2
            const uint32_t blackPlayer = tp;
            const uint32_t whitePlayer = field.ps.searchOpsPlayer(blackPlayer);

            ASSERT(field.isAlive(blackPlayer) && field.isAlive(whitePlayer),);

            L2Judge l2(65536, field.mv);
            int l2Result = l2.start_judge(field.hand[blackPlayer], field.hand[whitePlayer], field.board, field.fieldInfo);

            if (l2Result == L2_WIN) {
                field.setNewClassOf(blackPlayer, field.getWorstClass() - 1);
                field.setNewClassOf(whitePlayer, field.getWorstClass());
                goto GAME_END;
            } else if (l2Result == L2_LOSE) {
                field.setNewClassOf(whitePlayer, field.getWorstClass() - 1);
                field.setNewClassOf(blackPlayer, field.getWorstClass());
                goto GAME_END;
            }
#endif // SEARCH_LEAF_L2
        }
        // 合法着手生成
        field.NMoves = field.NActiveMoves = genMove(field.mv, field.hand[tp].cards, field.board);
        if (field.NMoves == 1) {
            field.setPlayMove(field.mv[0]);
        } else {
            // search mate-move
            int idxMate = -1;
#ifdef SEARCH_LEAF_MATE
            if (Settings::MateSearchInSimulation) {
                int mateIndex[N_MAX_MOVES];
                int mates = 0;
                for (int m = 0; m < field.NActiveMoves; m++) {
                    bool mate = checkHandMate(0, field.mv + field.NActiveMoves, field.mv[m],
                                              field.hand[tp], field.opsHand[tp], field.board, field.fieldInfo);
                    if (mate) mateIndex[mates++] = m;
                }
                // 探索順バイアス回避のために必勝全部の中からランダムに選ぶ
                if (mates == 1) idxMate = mateIndex[0];
                else if (mates > 1) idxMate = mateIndex[ptools->dice() % mates];
            }
#endif // SEARCH_LEAF_MATE
            if (idxMate != -1) { // mate
                field.setPlayMove(field.mv[idxMate]);
                field.playMove.setMPMate();
                field.fieldInfo.setMPMate();
            } else {
                if (field.NActiveMoves <= 1) {
                    field.setPlayMove(field.mv[0]);
                } else {
                    double score[N_MAX_MOVES + 1];

                    // 行動評価関数を計算
                    calcPlayPolicyScoreSlow(score, field, pshared->basePlayPolicy, 0);

                    // 行動評価関数からの着手の選び方は複数パターン用意して実験できるようにする
                    BiasedSoftmaxSelector<double> selector(score, field.NActiveMoves,
                                                           Settings::simulationTemperaturePlay,
                                                           Settings::simulationAmplifyCoef,
                                                           Settings::simulationAmplifyExponent);
                    int idx = selector.select(ptools->dice.random());
                    field.setPlayMove(field.mv[idx]);
                }
            }
        }
        DERR << tp << " : " << field.playMove << " (" << field.hand[tp].qty << ")" << endl;
        DERR << field.playMove << " " << field.ps << endl;
        
        // 盤面更新
        int nextTurnPlayer = field.proc(tp, field.playMove);
        
        if (nextTurnPlayer == -1) goto GAME_END;
        progress *= 0.95;
    }
GAME_END:
    for (int p = 0; p < N_PLAYERS; p++) {
        field.infoReward.assign(p, pshared->gameReward[field.newClassOf(p)]);
    }
    return 0;
}

int startPlaySimulation(Field& field,
                        MoveInfo mv,
                        SharedData *const pshared,
                        ThreadTools *const ptools) {
    DERR << field.toString();
    DERR << "turn : " << field.turn() << endl;
    if (field.procSlowest(mv) == -1) return 0;
    return simulation(field, pshared, ptools);
}

int startChangeSimulation(Field& field,
                          int p, Cards c,
                          SharedData *const pshared,
                          ThreadTools *const ptools) {
    
    int changePartner = field.classPlayer(getChangePartnerClass(field.classOf(p)));
    
    field.makeChange(p, changePartner, c);
    field.prepareAfterChange();
    
    DERR << field.toString();
    DERR << "turn : " << field.turn() << endl;

    return simulation(field, pshared, ptools);
}

int startAllSimulation(Field& field,
                       SharedData *const pshared,
                       ThreadTools *const ptools) {
    //cerr << field.toString();
    // 試合全部行う
    if (!field.isInitGame()) {
        // 献上
        field.removePresentedCards(); // 代わりにこちらの操作を行う
        // 交換
        Cards change[N_MAX_CHANGES];
        for (int cl = 0; cl < MIDDLE; cl++) {
            const int from = field.classPlayer(cl);
            const int to = field.classPlayer(getChangePartnerClass(cl));
            const int changes = genChange(change, field.getCards(from), N_CHANGE_CARDS(cl));
            
            int index = changeWithPolicy(change, changes,
                                         field.getCards(from), N_CHANGE_CARDS(cl),
                                         field, pshared->baseChangePolicy, ptools->dice);
            ASSERT(index < changes, cerr << index << " in " << changes << endl;)
            field.makeChange(from, to, change[index]);
        }
    }
    field.prepareAfterChange();
    DERR << field.toString();
    return simulation(field, pshared, ptools);
}