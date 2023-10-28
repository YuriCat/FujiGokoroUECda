#include "../settings.h"
#include "../core/action.hpp"
#include "policy.hpp"
#include "mate.hpp"
#include "last2.hpp"
#include "simulation.hpp"

using namespace std;

namespace Settings {
    const bool L2SearchInSimulation = true;
    const bool MateSearchInSimulation = true;
    const double simulationTemperatureChange = 1.0;
    const double simulationTemperaturePlay = 1.1;
    const double simulationAmplifyCoef = 0.22;
    const double simulationAmplifyExponent = 2;
}

MoveInfo simulationMove(Field& field, const SharedData& shared,
                        ThreadTools *const ptools, double progress) {
    int turn = field.turn();
    MoveInfo *const mbuf = ptools->mbuf;
    int numMoves = genMove(mbuf, field.getCards(turn), field.board);
    if (numMoves == 1) return mbuf[0];

    if (Settings::MateSearchInSimulation) {
        Hand myHand(field.getCards(turn));
        Hand opsHand(field.getOpsCards(turn), true);
        int mateIndice[N_MAX_MOVES];
        int mates = 0;
        for (int i = 0; i < numMoves; i++) {
            bool mate = checkHandMate(0, mbuf + numMoves, mbuf[i],
                                      myHand, opsHand, field.board, field.fieldInfo);
            if (mate) mateIndice[mates++] = i;
        }
        if (mates > 0) {
            // 探索順バイアス回避のために必勝全部の中からランダムに選ぶ
            int mateIndex = mateIndice[mates > 1 ? (ptools->dice() % mates) : 0];
            MoveInfo move = mbuf[mateIndex];
            move.setMPMate();
            field.fieldInfo.setMPMate();
            return move;
        }
    }

    // 行動方策を計算
    double score[N_MAX_MOVES];
    playPolicyScore(score, mbuf, numMoves, field, shared.basePlayPolicy);
    if (turn != field.myPlayerNum && shared.playerModel.trained) {
        for (int i = 0; i < numMoves; i++) score[i] += shared.playerModel.playBiasScore(field, turn, mbuf[i]) * progress;
    }

    // ランダム選択
    BiasedSoftmaxSelector<double> selector(score, numMoves,
                                           Settings::simulationTemperaturePlay,
                                           Settings::simulationAmplifyCoef,
                                           Settings::simulationAmplifyExponent);
    return mbuf[selector.select(ptools->dice.random())];
}

int simulation(Field& field,
               SharedData *const pshared,
               ThreadTools *const ptools) {
    double progress = 1;
    while (1) {
        if (Settings::L2SearchInSimulation && field.isL2Situation()) {
            int p[2];
            p[0] = field.turn();
            p[1] = field.ps.searchOpsPlayer(p[0]);
            assert(field.isAlive(p[0]) && field.isAlive(p[1]));
            int res = judgeLast2(ptools->mbuf, field.getCards(p[0]), field.getCards(p[1]), field.board, field.fieldInfo);
            if (res == L2_WIN || res == L2_LOSE) {
                int winner = res == L2_WIN ? 0 : 1;
                field.setNewClassOf(p[winner],     field.bestClass());
                field.setNewClassOf(p[1 - winner], field.bestClass() + 1);
                break;
            }
        }
        // 手を選んで進める
        MoveInfo move = simulationMove(field, *pshared, ptools, progress);
        if (field.procFast(move) < 0) break;
        progress *= 0.95;
    }
    return 0;
}

int startPlaySimulation(Field& field, MoveInfo m,
                        SharedData *const pshared,
                        ThreadTools *const ptools) {
    if (field.proceed(m) == -1) return 0;
    return simulation(field, pshared, ptools);
}

int startChangeSimulation(Field& field, int p, Cards c,
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
            double score[N_MAX_CHANGES];
            const int numChanges = genChange(change, field.getCards(from), qty);
            changePolicyScore(score, change, numChanges, field.getCards(from), qty, pshared->baseChangePolicy);
            SoftmaxSelector<double> selector(score, numChanges, Settings::simulationTemperatureChange);
            int index = selector.select(ptools->dice.random());
            field.makeChange(from, to, qty, change[index], false);
        }
    }
    field.prepareAfterChange();
    return simulation(field, pshared, ptools);
}