#pragma once

#include "../core/action.hpp"
#include "../engine/mate.hpp"
#include "../engine/data.hpp"

namespace PolicyGradient {
    // 棋譜から方策勾配法で着手方策関数を棋譜の着手が選ばれやすいように近づける

    template <class learningSpace_t>
    int learnChangeParamsGame(const GameRecord& game,
                              const std::bitset<32> flags,
                              learningSpace_t *const plearningSpace,
                              ThreadTools *const ptools) {

        Move *const mbuf = ptools->mbuf;
        Field field;
        for (ChangeRecord change : ChangeRoller(field, game)) {
            int myClass = field.classOf(change.from);
            if (myClass > Class::MIDDLE) continue;

            const Hand& myHand = field.getHand(change.from);
            const Hand& opsHand = field.getOpsHand(change.from);
            const Cards myCards = myHand.cards;

            if (!holdsCards(myHand.cards, change.cards)) break;

            // generate changes
            int changeQty = N_CHANGE_CARDS(myClass);
            Cards cbuf[N_MAX_CHANGES];
            const int numChanges = genChange(cbuf, myCards, changeQty);
            if (numChanges <= 1) continue;

            int idx = -1;
            for (int i = 0; i < numChanges; i++) if (cbuf[i] == change.cards) idx = i;

            // mate check なし
            double score[N_MAX_CHANGES];
            auto *const plearner = &plearningSpace->changeLearner(change.from);

            const int ph = myClass == DAIFUGO ? 0 : 1;

            if (idx == -1) { // unfound
                if (flags.test(1)) plearner->feedUnfoundFeatureValue(ph);
                if (flags.test(2)) plearner->feedObjValue(idx, ph); // test
            } else {
                if (!changePolicyScore(score, cbuf, numChanges, myCards, changeQty, *plearner, 1)) {
                    if (flags.test(1)) plearner->feedFeatureValue(ph);
                    if (flags.test(0)) { // learn
                        plearner->feedSupervisedActionIndex(idx, ph);
                        plearner->updateParams();
                    }
                    if (flags.test(2)) plearner->feedObjValue(idx, ph); // test
                }
            }
        }
        return 0;
    }

    template <int MODELING = 0, class learningSpace_t>
    int learnPlayParamsGame(const GameRecord& game,
                            const std::bitset<32> flags,
                            learningSpace_t *const plearningSpace,
                            ThreadTools *const ptools) {

        Move *const mbuf = ptools->mbuf;
        Field field;
        for (Move move : PlayRoller(field, game)) {
            if (field.isEndGame()) break;

            const int turn = field.turn();
            const Hand& myHand = field.getHand(turn);
            const Hand& opsHand = field.getOpsHand(turn);
            const Board b = field.board;

            if (!holdsCards(myHand.cards, move.cards())) break;

            // generate moves
            const int numMoves = genMove(mbuf, myHand, b);
            if (numMoves <= 1) continue;
            int idx = searchMove(mbuf, numMoves, move);

            if (searchHandMate(0, mbuf, numMoves, myHand, opsHand, b) >= 0) continue;

            double score[N_MAX_MOVES];
            auto *const plearner = &plearningSpace->playLearner(turn);

            if (idx == -1) { // unfound
                if (flags.test(1)) plearner->feedUnfoundFeatureValue();
                if (flags.test(2)) plearner->feedObjValue(idx);
            } else {                     
                if (!playPolicyScore(score, mbuf, numMoves, field, *plearner, 1)) {  
                    if (flags.test(1)) plearner->feedFeatureValue();
                    if (flags.test(0)) { // learn
                        plearner->feedSupervisedActionIndex(idx);
                        plearner->updateParams();
                    }
                    if (flags.test(2)) plearner->feedObjValue(idx); // test
                }
            }
        }
        return 0;
    }

    template <class learningSpace_t>
    int learnParamsGame(const GameRecord& gLog,
                        const std::bitset<32> flags,
                        learningSpace_t *const plearningSpace,
                        ThreadTools *const ptools,
                        bool change) {
        if (change) {
            return learnChangeParamsGame(gLog, flags, plearningSpace, ptools);
        } else {
            return learnPlayParamsGame(gLog, flags, plearningSpace, ptools);
        }
    }
}