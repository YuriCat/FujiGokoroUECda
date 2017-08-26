/*
 playerAnalysis.hpp
 Katsuki Ohto
 */

#ifndef UECDA_FUJI_PLAYER_ANAYLYSIS_HPP_
#define UECDA_FUJI_PLAYER_ANAYLYSIS_HPP_

#include "../../structure/primitive/prim.hpp"
#include "../learning/policyGradient.hpp"

namespace UECda{
	namespace Fuji{
		
		// 試合ログからプレーヤーモデルを解析
		// 毎試合後に行うので、1試合の中で随時着手に反映する事は現在は出来ない
		// (ただし、その試合のそれまでに得た情報が相手の行動予測に使われない訳では、もちろんない)

        template<class field_t, class sharedData_t, class threadTools_t>
        void analyzePlayerModel(const field_t& sbjField,
                                sharedData_t *const pshared,
                                threadTools_t *const ptools){
            
            // 実際にプレーされた着手についての情報
            int alrL2 = 0;
            int alrCM[N_PLAYERS] = {0};
            bool isCM, isL2;
            const int by_time = pshared->modeling_time;
            
            auto *const pmodelSpace = &pshared->playerModelSpace;
            
            // まず、観戦モードで動いていたかを推定
            if(judgeWatchingMode(pshared->gameLog)){
                // 着手間隔時間がプレー時間を反映していない可能性あり
                pshared->modeling_time = 0;
                pshared->estimating_by_time = 0;
                //cerr << "watching mode ?" << endl; getchar();
            }else{
                // 着手間隔時間がプレー時間として信頼出来そう
                pshared->modeling_time = 1;
                pshared->estimating_by_time = 1;
            }
            
            if(!pshared->gameLog.isTerminated()){ return; } // 試合が普通に終了した場合のみ解析
            
            iterateGameLogAfterChange<PlayouterField>
            (pshared->gameLog,
             // first callback
             [](const auto& field)->void{},
             // play callback
             [&isCM, &isL2, &alrL2, &alrCM, by_time, pmodelSpace, ptools](const auto& field,
                                                                          const Move chosenMove,
                                                                          const uint64_t playTime)->int{
                 
                 uint32_t tp = field.getTurnPlayer();
                 
                 MoveInfo *const pmv = ptools->buf;
                 PlayerModel *const pmodel = &pmodelSpace->model(tp);
                 
                 const Board bd = field.getBoard();
                 
                 const Hand& myHand = field.hand[tp];
                 const Hand& opsHand = field.opsHand[tp];
                 const Cards myCards = myHand.getCards();
                 
                 FieldAddInfo fieldInfo = field.fieldInfo;
                 
                 if(myCards == CARDS_NULL){ return -1; } // 一応
                 
                 if(!holdsCards(myCards, chosenMove.cards())){ return -1; } // javaサーバー対策
                 
                 const int NMoves = genMove(pmv, myHand, bd); // 合法着手生成
                 
                 if(NMoves == 0){ return -1; } // 一応
                 
                 // 場の情報をまとめる
                 isCM = isL2 = false;
                 
                 // フェーズ(空場0、通常場1、セルフフォロー2)
                 const int ph = bd.isNF() ? 0 : (fieldInfo.isPassDom() ? 2 : 1);
                 
                 if(NMoves <= 1){
                     // 合法着手1のときは詳しい解析はしない
                     if(by_time){
                         if(field.getTurnNum() != 0){ // 初手は計算時間が信頼出来ないので時間解析に含めない
                             pmodel->feedPlayTime(playTime, 1, ph, chosenMove, false, false);
                         }
                     }
                 }else{
                     
                     if(field.getNAlivePlayers() == 2){ isL2 = true; }
                     
                     //全ての着手について基本的情報をまとめる
                     
                     int chosenMoveIdx = -1;
                     //int curGMinNMelds=calcGMinNMelds(myCards);
                     for(int m = NMoves - 1; m >= 0; --m){
                         MoveInfo *const mi = &pmv[m];
                         const Move move = mi->mv();
                         
                         if(chosenMove.meldPart() == move.meldPart()){ chosenMoveIdx = m; }
                         
                         //Cards nextCards = maskCards(myCards, move.cards());
                         // 後場情報
                         if(bd.afterPrmOrder(move) != ORDER_NORMAL){ mi->setPrmOrderRev(); }
                         if(bd.afterTmpOrder(move) != ORDER_NORMAL){ mi->setTmpOrderRev(); }
                         if(bd.afterSuitsLocked(move)){ mi->setSuitLock(); }
                         
                         //支配性判定
                         if(move.isPASS()){
                             mi->setDM();
                             if(fieldInfo.isPassDom()){ mi->setDO(); }
                         }else{
                             if(field.fieldInfo.isNonPassDom()){
                                 mi->setDALL();
                             }else{
                                 if(dominatesHand(move,opsHand,bd)){
                                     mi->setDO();
                                 }
                                 if(dominatesCards(move, maskCards(myCards, move.cards<_NO>()), bd)){
                                     mi->setDM();
                                 }
                             }
                         }
                         
                         // 必勝判定
                         bool mate = checkHandMate(0, pmv + NMoves, *mi, myHand, opsHand, bd, fieldInfo);
                         if(mate){
                             mi->setMPMate(); fieldInfo.setMPMate(); isCM = true;
                         }
                         
                         // L2の場合はL2判定
                         if(isL2 && (!alrL2) && (!fieldInfo.isL2Mate())){
                             L2Judge lj(40000, pmv + NMoves);
                             isL2 = true;
                             int l2Res = lj.start_check(*mi, myHand, opsHand, bd, fieldInfo);
                             if(l2Res == L2_WIN){//L2WIN
                                 mi->setL2Mate(); fieldInfo.setL2Mate();
                             }else if(l2Res == L2_LOSE){
                                 mi->setL2GiveUp();
                             }
                         }
                         
                         // 最小分割数の減少量
                         //mi->setIncMinNMelds(max(0,calcGMinNMelds(nextCards)-curGMinNMelds+1));
                         
                         //printMove(move);info.print();CERR<<endl;
                     }// 手の性質判定
                    
#ifdef MODELING_TIME
                     if(by_time && field.getTurnNum() != 0){ // 初手は計算時間が信頼出来ないので時間解析に含めない
                         pmodel->feedPlayTime(playTime, NMoves, ph, chosenMove, isCM, isL2);
                         if(!isCM && !isL2 && !bd.isNF()){
                             pmodel->feedChancePlay(chosenMove.isPASS() ? true : false);
                         }
                     }
#endif
                     
                     if(chosenMoveIdx >= 0){
                         assert(chosenMoveIdx < NMoves);
                         // 実際の着手が合法着手生成にて生成されなかった場合、解析のしようがないので飛ばす
                         // 合法着手生成メソッドが全ての合法着手を生成しない場合があるので
                         if(isL2){
                             if(1 || !alrL2){
                                 // ラスト2人必勝状態で必勝手を出したか判定
                                 pmodel->feedL2Rel(pmv[chosenMoveIdx].isL2Mate() ? true : false);
                             }
                         }else if(isCM){
                             if(1 || !alrCM[tp]){
                                 // 多人数必勝状態で必勝手を出したか判定
                                 pmodel->feedMateRel(pmv[chosenMoveIdx].isMate() ? true : false);
                             }
                         }
                     }
                     
                 }
                 return 0;
             },
             //last callback
             [](const auto& field)->void{});
            
            // PGLearnで方策パラメータ調整
            /*BitSet32 flag(0);
            flag.set(0);
            for(int itr = 0; itr < 8; ++itr){
                PolicyGradient::learnPlayParamsGame<1>(pshared->gameLog, flag, pmodelSpace, ptools);
            }
            for(int p = 0; p < N_PLAYERS; ++p){
                CERR << p << " base distance = " << pmodelSpace->playLearner(p).calcBaseDistance() << endl;
            }*/
            if(Settings::simulationPlayModel){
                // 確率のずれによって方策パラメータ調整
                learnPlayBiasGame(sbjField.getMyPlayerNum(),
                                  pshared->gameLog, pshared->basePlayPolicy, pmodelSpace, ptools);
            }

#ifdef MODELING_TIME
            // 計算時間解析をまとめ、次以降の試合に活かす
            if(by_time){
                for(int p = 0; p < N_PLAYERS; ++p){
                    auto& tm = playerModelSpace.model(p).timeModel_;
                    tm.attenuateDist();//減衰
                    
                    if(tm.dist_sum[0] <= 0 || tm.dist_sum[1] <= 0){
                        // 分布和が0になってしまったので、計算量からの推定は不可
                        CERR << "dist_sum 0!" << endl;
                        // getchar();
                        estimating_by_time = 0;
                    }
                }
                
                // このとき自分のプレー時間も減衰させる
                // 途中でマシンの計算能力が変わった場合のために一応
                if(my_plays > 64){
                    
                    my_plays *= 63;
                    my_plays /= 64;
                    
                    my_play_time_sum *= 63;
                    my_play_time_sum /= 64;
                    
                }
            }
#endif
		}// end function
	}
}

#endif // UECDA_FUJI_PLAYER_ANAYLYSIS_HPP_
