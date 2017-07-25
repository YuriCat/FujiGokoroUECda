/*
 policyGradient.hpp
 Katsuki Ohto
 */

#ifndef UECDA_FUJI_POLICYGRADIENT_HPP_
#define UECDA_FUJI_POLICYGRADIENT_HPP_

#include "../../include.h"

#include "../../structure/primitive/prim.hpp"
#include "../../structure/hand.hpp"
#include "../../structure/log/logIteration.hpp"

#include "../logic/mate.hpp"

#include "../montecarlo/playout.h"

namespace UECda{
	namespace Fuji{
        namespace PolicyGradient{
            // 試合ログから方策勾配法で着手方策関数を棋譜の着手が選ばれやすいように近づける
            
            //std::map<std::string, int> teacherMap; // how many plays by each player?
            
            template<int MODELING = 0, class gameLog_t, class learningSpace_t, class threadTools_t>
            int learnChangeParamsGame(const gameLog_t& gLog,
                                    const BitSet32 flags,
                                    learningSpace_t *const plearningSpace,
                                    threadTools_t *const ptools){
                
                MoveInfo *const buf = ptools->buf;
                
                iterateGameLogBeforePlay<PlayouterField>
                (gLog,
                 [](const auto& field)->void{}, // first callback
                 [](const auto& field)->void{}, // dealt callback
                 // change callback
                 [buf, flags, plearningSpace](const auto& field, const int from, const int to, const Cards chosenChange)->int{
                     
                     int myClass = field.getPlayerClass(from);
                     if(myClass > Class::MIDDLE){ return 0; }
                     
                     const Hand& myHand = field.getHand(from);
                     const Hand& opsHand = field.getOpsHand(from);
                     const Cards myCards = myHand.cards;
                     
                     if(!holdsCards(myHand.cards, chosenChange)){
                         assert(0);
                         return 0;
                     }
                     
                     // generate changes
                     int changeQty = N_CHANGE_CARDS(myClass);
                     Cards change[N_MAX_CHANGES];
                     const int NChanges = genChange(change, myCards, changeQty);
                                                    
                     if(NChanges <= 1){ return 0; }
                     
                     int idx = searchCards(change, NChanges, [chosenChange](const auto& ch)->bool{
                         return (ch == chosenChange);
                     });

                     // mate check なし
                     
                     double score[N_MAX_CHANGES + 1];
                     auto *const plearner = &plearningSpace->changeLearner(from);
                     
                     const int ph = (myClass == DAIFUGO) ? 0 : 1;
                     
                     if(idx == -1){ // unfound
#if 0
                         cerr << "unfound " << chosenMove << chosenMove.data() << endl;
                         Move tmp = chosenMove;tmp.resetEffects();
                         cerr << tmp.data() << endl;
                         cerr << field.toDebugString();
                         for(int m = 0;m < NMoves; ++m){
                             cerr << buf[m] << buf[m].mv().bits() << " ";
                         }
                         cerr << endl;
                         getchar();
#endif
                         if(flags.test(1)){ // feed feature value
                             plearner->feedUnfoundFeatureValue(ph);
                         }
                         if(flags.test(2)){ // test
                             plearner->feedObjValue(idx, ph);
                         }
                     }else{
                         
                         if(!calcChangePolicyScoreSlow<1>(score, change, NChanges, myCards, changeQty, field, *plearner)){
                             
                             if(flags.test(1)){ // feed feature value
                                 plearner->feedFeatureValue(ph);
                             }
                             if(flags.test(0)){ // learn
                                 plearner->feedSupervisedActionIndex(idx, ph);
#if 0
                                 cerr << policy.plearner_ << endl;
                                 cerr << " score_sum = " << plearner->score_sum_ << endl;
                                 for(int m = 0; m < NMoves; ++m){
                                     cerr << buf[m] << " " << plearner->score_.at(m) / plearner->score_sum_;
                                     if(m == idx){ cerr << " <-"; }
                                     cerr << endl;
                                 }
                                 cerr << plearner->toFeatureString() << endl;
                                 
                                 getchar();
#endif
                                 plearner->updateParams();
                             }
                             if(flags.test(2)){ // test
                                 plearner->feedObjValue(idx, ph);
                             }
                         }
                     }
                     return 0;
                 },
                 [](const auto&)->void{} // last callback
                 );
                return 0;
            }
            
            template<int MODELING = 0, class gameLog_t, class learningSpace_t, class threadTools_t>
            int learnPlayParamsGame(const gameLog_t& gLog,
                                    const BitSet32 flags,
                                    learningSpace_t *const plearningSpace,
                                    threadTools_t *const ptools){
                
                MoveInfo *const buf = ptools->buf;
                
                iterateGameLogAfterChange<PlayouterField>
                (gLog,
                 //after change callback
                 [](const auto& field)->void{},
                 //play callback
                 [buf, flags, plearningSpace](const auto& field, const Move chosenMove, const uint64_t time)->int{
                     
                     if(field.isEndGame()){ return -1; }
                     
                     const uint32_t tp = field.getTurnPlayer();
                     const Hand& myHand = field.getHand(tp);
                     const Hand& opsHand = field.getOpsHand(tp);
                     const Board bd = field.getBoard();
                     
                     if(!holdsCards(myHand.cards, chosenMove.cards())){
                         return -1;
                     }
                     
                     // generate moves
                     const int NMoves = genMove(buf, myHand, bd);
                     assert(NMoves > 0);
                     
                     if(NMoves == 1){ return 0; }
                     
                     int idx = searchMove(buf, NMoves, [chosenMove](const auto& mv)->bool{
                         return mv.mv().meldPart() == chosenMove.meldPart();
                     });
                     
                     if(searchHandMate(0, buf, NMoves, myHand, opsHand, bd, field.fInfo) >= 0){                         return 0;
                     }
                     
                     double score[N_MAX_MOVES + 1];
                     auto *const plearner = &plearningSpace->playLearner(tp);
                     
                     if(idx == -1){ // unfound
#if 0
                         cerr << "unfound " << chosenMove << chosenMove.data() << endl;
                         Move tmp = chosenMove;tmp.resetEffects();
                         cerr << tmp.data() << endl;
                         cerr << field.toDebugString();
                         for(int m = 0; m < NMoves; ++m){
                             cerr << buf[m] << buf[m].mv().bits() << " ";
                         }
                         cerr << endl;
                         getchar();
#endif
                         if(flags.test(1)){ // feed feature value
                             plearner->feedUnfoundFeatureValue();
                         }
                         if(flags.test(2)){ // test
                             plearner->feedObjValue(idx);
                         }
                     }else{
                         
                         if(!calcPlayPolicyScoreSlow<1, MODELING>(score, buf, NMoves, field, *plearner)){
                             
                             if(flags.test(1)){ // feed feature value
                                 plearner->feedFeatureValue();
                             }
                             if(flags.test(0)){ // learn
                                 plearner->feedSupervisedActionIndex(idx);
#if 0
                                 cerr << policy.plearner_ << endl;
                                 cerr << " score_sum = " << plearner->score_sum_ << endl;
                                 for(int m = 0; m < NMoves; ++m){
                                     cerr << buf[m] << " " << plearner->score_.at(m) / plearner->score_sum_;
                                     if(m == idx){ cerr << " <-"; }
                                     cerr << endl;
                                 }
                                 cerr << plearner->toFeatureString() << endl;
                                 
                                 getchar();
#endif
                                 plearner->updateParams();
                             }
                             if(flags.test(2)){ // test
                                 plearner->feedObjValue(idx);
                             }
                         }
                     }
                     return 0;
                 },
                 // last callback
                 [](const auto&)->void{}
                 );
                return 0;
            }
            
            template<class matchLog_t, class learningSpace_t, class threadTools_t>
            int learnPlayParamsMatch(const matchLog_t& mLog,
                                     const BitSet32 flags,
                                     learningSpace_t *const plearningSpace,
                                     threadTools_t *const ptools,
                                     int *const list = nullptr,
                                     int nlist = 0){
                if(list == nullptr){
                    // 全試合で学習 or テスト
                    for(int g = 0; g < mLog.games(); ++g){
                        learnPlayParamsGame(mLog.game(g), flags, plearningSpace, ptools);
                    }
                }else{
                    // リストの試合で学習 or テスト
                    for(int n = 0; n < nlist; ++n){
                        learnPlayParamsGame(mLog.game(list[n]), flags, plearningSpace, ptools);
                    }
                }
                return 0;
            }
            
            template<int MODELING = 0, class gameLog_t, class learningSpace_t, class threadTools_t>
            int learnParamsGame(const gameLog_t& gLog,
                                const BitSet32 flags,
                                learningSpace_t *const plearningSpace,
                                threadTools_t *const ptools,
                                bool change){
                if(change){
                    return learnChangeParamsGame<MODELING>(gLog, flags, plearningSpace, ptools);
                }else{
                    return learnPlayParamsGame<MODELING>(gLog, flags, plearningSpace, ptools);
                }
            }
        }
    }
}

#endif // UECDA_FUJI_POLICYGRADIENT_HPP_
