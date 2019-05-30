#include "../settings.h"
#include "../core/action.hpp"
#include "policy.hpp"
#include "mate.hpp"
#include "last2.hpp"
#include "simulation.hpp"

using namespace std;

MoveInfo simulationMove(Field& field, const SharedData& shared,
                        ThreadTools *const ptools) {
    int turn = field.turn();
    int NMoves = genMove(field.mbuf, field.hand[turn].cards, field.board);
    if (NMoves == 1) return field.mbuf[0];
    if (Settings::MateSearchInSimulation) {
        int mateIndice[N_MAX_MOVES];
        int mates = 0;
        for (int i = 0; i < NMoves; i++) {
            bool mate = checkHandMate(0, field.mbuf + NMoves, field.mbuf[i],
                                      field.hand[turn], field.opsHand[turn], field.board, field.fieldInfo);
            if (mate) mateIndice[mates++] = i;
        }
        if (mates > 0) {
            // 探索順バイアス回避のために必勝全部の中からランダムに選ぶ
            int mateIndex = mateIndice[mates > 1 ? (ptools->dice() % mates) : 0];
            MoveInfo move = field.mbuf[mateIndex];
            move.setMPMate();
            field.fieldInfo.setMPMate();
            return move;
        }
    }

    // 行動方策を計算
    double score[N_MAX_MOVES];
    playPolicyScore(score, ptools->mbuf, NMoves, field, shared.basePlayPolicy, 0);

    // ランダム選択
    BiasedSoftmaxSelector<double> selector(score, NMoves,
                                           Settings::simulationTemperaturePlay,
                                           Settings::simulationAmplifyCoef,
                                           Settings::simulationAmplifyExponent);
    return field.mbuf[selector.select(ptools->dice.random())];
}

int simulation(Field& field,
               SharedData *const pshared,
               ThreadTools *const ptools) {
    while (1) {
        field.prepareForPlay();

        if (Settings::L2SearchInSimulation && field.isL2Situation()) {
            int p[2];
            p[0] = field.turn();
            p[1] = field.ps.searchOpsPlayer(p[0]);
            assert(field.isAlive(p[0]) && field.isAlive(p[1]));

            L2Judge l2(65536, field.mbuf);
            int l2Result = l2.start_judge(field.hand[p[0]], field.hand[p[1]], field.board, field.fieldInfo);
            if (l2Result == L2_WIN || l2Result == L2_LOSE) {
                int winner = l2Result == L2_WIN ? 0 : 1;
                field.setNewClassOf(p[winner],     field.getBestClass());
                field.setNewClassOf(p[1 - winner], field.getBestClass() + 1);
                break;
            }
        }
        // 手を選んで進める
        MoveInfo move = simulationMove(field, *pshared, ptools);
        if (field.procFast(move) < 0) break;
    }
    return 0;
}

int startPlaySimulation(Field& field,
                        MoveInfo m,
                        SharedData *const pshared,
                        ThreadTools *const ptools) {
    DERR << field.toString();
    DERR << "turn : " << field.turn() << endl;
    if (field.proceed(m) == -1) return 0;
    return simulation(field, pshared, ptools);
}

int startChangeSimulation(Field& field,
                          int p, Cards c,
                          SharedData *const pshared,
                          ThreadTools *const ptools) {
    
    int changePartner = field.classPlayer(getChangePartnerClass(field.classOf(p)));
    field.makeChange(p, changePartner, c.count(), c, false);
    field.prepareAfterChange();
    return simulation(field, pshared, ptools);
}

int startAllSimulation(Field& field,
                       SharedData *const pshared,
                       ThreadTools *const ptools) {
    // 試合全部行う
    if (!field.isInitGame()) {
        // 既に献上済み
        for (int cl = 0; cl < MIDDLE; cl++) {
            int from = field.classPlayer(cl);
            int to = field.classPlayer(getChangePartnerClass(cl));
            int qty = N_CHANGE_CARDS(cl);
            Cards change[N_MAX_CHANGES];
            const int changes = genChange(change, field.getCards(from), qty);
            int index = changeWithPolicy(change, changes, field.getCards(from), qty,
                                         pshared->baseChangePolicy, ptools->dice);
            field.makeChange(from, to, qty, change[index], false);
        }
    }
    field.prepareAfterChange();
    return simulation(field, pshared, ptools);
}