#pragma once

#include "../core/action.hpp"
#include "../engine/mate.hpp"

namespace PolicyGradient {
    // 試合ログから方策勾配法で着手方策関数を棋譜の着手が選ばれやすいように近づける
    
    template <int MODELING = 0, class gameLog_t, class learningSpace_t, class threadTools_t>
    int learnChangeParamsGame(const gameLog_t& gLog,
                              const std::bitset<32> flags,
                              learningSpace_t *const plearningSpace,
                              threadTools_t *const ptools) {
        
        MoveInfo *const buf = ptools->buf;
        Field field;
        iterateGameLogBeforePlay
        (field, gLog,
            [](const Field& field)->void{}, // first callback
            [](const Field& field)->void{}, // dealt callback
            // change callback
            [buf, flags, plearningSpace](const Field& field, const int from, const int to, const Cards chosenChange)->int{
                
                int myClass = field.classOf(from);
                if (myClass > Class::MIDDLE) return 0;
                
                const Hand& myHand = field.getHand(from);
                const Hand& opsHand = field.getOpsHand(from);
                const Cards myCards = myHand.cards;
                
                if (!holdsCards(myHand.cards, chosenChange)) {
                    assert(0); return 0;
                }
                
                // generate changes
                int changeQty = N_CHANGE_CARDS(myClass);
                Cards change[N_MAX_CHANGES];
                const int NChanges = genChange(change, myCards, changeQty);
                
                if (NChanges <= 1) return 0;
                
                int idx = -1;
                for (int i = 0; i < NChanges; i++) if (change[i] == chosenChange) idx = i;
                
                // mate check なし
                double score[N_MAX_CHANGES + 1];
                auto *const plearner = &plearningSpace->changeLearner(from);
                
                const int ph = (myClass == DAIFUGO) ? 0 : 1;
                
                if (idx == -1) { // unfound
                    if (flags.test(1)) { // feed feature value
                        plearner->feedUnfoundFeatureValue(ph);
                    }
                    if (flags.test(2)) { // test
                        plearner->feedObjValue(idx, ph);
                    }
                } else {
                    
                    if (!changePolicyScore(score, change, NChanges, myCards, changeQty, field, *plearner, 1)) {
                        
                        if (flags.test(1)) { // feed feature value
                            plearner->feedFeatureValue(ph);
                        }
                        if (flags.test(0)) { // learn
                            plearner->feedSupervisedActionIndex(idx, ph);
                            plearner->updateParams();
                        }
                        if (flags.test(2)) { // test
                            plearner->feedObjValue(idx, ph);
                        }
                    }
                }
                return 0;
            },
            [](const Field&)->void{} // last callback
            );
        return 0;
    }
    
    template <int MODELING = 0, class gameLog_t, class learningSpace_t, class threadTools_t>
    int learnPlayParamsGame(const gameLog_t& gLog,
                            const std::bitset<32> flags,
                            learningSpace_t *const plearningSpace,
                            threadTools_t *const ptools) {
        
        MoveInfo *const buf = ptools->buf;
        Field field;
        iterateGameLogAfterChange
        (field, gLog,
            //after change callback
            [](const Field& field)->void{},
            //play callback
            [buf, flags, plearningSpace](const Field& field, const Move chosenMove, const uint64_t time)->int{
                
                if (field.isEndGame()) return -1;
                
                const uint32_t tp = field.turn();
                const Hand& myHand = field.getHand(tp);
                const Hand& opsHand = field.getOpsHand(tp);
                const Board bd = field.board;
                
                if (!holdsCards(myHand.cards, chosenMove.cards())) return -1;
                
                // generate moves
                const int NMoves = genMove(buf, myHand, bd);
                assert(NMoves > 0);
                
                if (NMoves == 1) return 0;
                
                int idx = searchMove(buf, NMoves, [chosenMove](const Move mv)->bool{
                    return mv == chosenMove;
                });
                
                if (searchHandMate(0, buf, NMoves, myHand, opsHand, bd, field.fieldInfo) >= 0) return 0;
                
                double score[N_MAX_MOVES + 1];
                auto *const plearner = &plearningSpace->playLearner(tp);
                
                if (idx == -1) { // unfound
                    if (flags.test(1)) { // feed feature value
                        plearner->feedUnfoundFeatureValue();
                    }
                    if (flags.test(2)) { // test
                        plearner->feedObjValue(idx);
                    }
                } else {                     
                    if (!playPolicyScore(score, buf, NMoves, field, *plearner, 1)) {  
                        if (flags.test(1)) { // feed feature value
                            plearner->feedFeatureValue();
                        }
                        if (flags.test(0)) { // learn
                            plearner->feedSupervisedActionIndex(idx);
                            plearner->updateParams();
                        }
                        if (flags.test(2)) { // test
                            plearner->feedObjValue(idx);
                        }
                    }
                }
                return 0;
            },
            // last callback
            [](const Field&)->void{}
            );
        return 0;
    }

    template <int MODELING = 0, class gameLog_t, class learningSpace_t, class threadTools_t>
    int learnParamsGame(const gameLog_t& gLog,
                        const std::bitset<32> flags,
                        learningSpace_t *const plearningSpace,
                        threadTools_t *const ptools,
                        bool change) {
        if (change) {
            return learnChangeParamsGame<MODELING>(gLog, flags, plearningSpace, ptools);
        } else {
            return learnPlayParamsGame<MODELING>(gLog, flags, plearningSpace, ptools);
        }
    }
}