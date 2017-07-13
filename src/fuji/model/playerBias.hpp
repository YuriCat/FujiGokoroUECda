/*
 playerBias.hpp
 Katsuki Ohto
 */

// プレーヤーごとの行動バイアスの計算

#ifndef UECDA_FUJI_PLAYERBIAS_HPP_
#define UECDA_FUJI_PLAYERBIAS_HPP_

#include "../logic/mate.hpp"
#include "../montecarlo/playout.h"

namespace UECda{
	namespace Fuji{
        
        template<class field_t, class model_t>
        double calcPlayerPlayBiasScore(const field_t& field, const MoveInfo& mv, const int NMoves, const model_t& model){
            // 昔は積形式で返していたが今は和形式で返す
            double s = 0;
            if(field.isNF()){
                if(mv.containsJOKER()){
                    s += model.biasNF[2].value();
                }else if(mv.domInevitably()){
                    s += model.biasNF[3].value();
                }
                if(mv.isGroup()){
                    s += model.biasNF[0].value();
                }else if(mv.isSeq()){
                    s += model.biasNF[1].value();
                }
                if(mv.flipsPrmOrder()){
                    s += model.biasNF[4].value();
                }
            }else{
                const int bdid = field.getBoard().isSeq() ? 2 : (field.getBoard().isGroup() ? 1 : 0);
                if(!mv.isPASS()){
                    if(mv.containsJOKER()){
                        if(mv.isSingleJOKER() && NMoves == 2){
                            s += model.biasRF[bdid][1].value();
                        }else{
                            s += model.biasRF[bdid][2].value();
                        }
                    }else if(mv.domInevitably()){
                        s += model.biasRF[bdid][3].value();
                    }
                }else{
                    s += model.biasRF[bdid][0].value();
                }
            }
            //cerr << s << endl;
            return s;
        }
        
        template<class field_t, class model_t>
        void addPlayerPlayBias(double *const score, MoveInfo *const buf, const int moves,
                               const field_t& field, const model_t& model, double coef){
            for(int i = 0; i < moves; ++i){
                score[i] += coef * calcPlayerPlayBiasScore(field, buf[i], moves, model);
            }
        }
		
        template<class gameLog_t, class playPolicy_t, class modelSpace_t, class threadTools_t>
        int learnPlayBiasGame(const int myPlayerNum,
                              const gameLog_t& gLog,
                              const playPolicy_t& policy,
                              modelSpace_t *const pmodelSpace,
                              threadTools_t *const ptools){
            // 1試合の結果からバイアスをアップデート
            
            MoveInfo *const buf = ptools->buf;
            
            iterateGameLogAfterChange<PlayouterField>
            (gLog,
             // after change callback
             [](const auto& field)->void{},
             // play callback
             [myPlayerNum, buf, pmodelSpace, &policy](const auto& field, const Move chosenMove, const uint64_t time)->int{
                 
                 if(field.getTurnPlayer() == myPlayerNum){ return 0; }
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
                     return mv.meldPart(), chosenMove.meldPart();
                 });
                 
                 for(int m = 0; m < NMoves; ++m){
                     bool mate = checkHandMate(0, buf + NMoves, buf[m], myHand, opsHand, bd, field.fInfo, 1, 1);
                     if(mate){ return 0; }
                 }
                 
                 double score[N_MAX_MOVES + 1];
                 auto *const pmodel = &pmodelSpace->model(tp);
                 
                 if(idx == -1){ // unfound
                 }else{
                     if(!calcPlayPolicyScoreSlow<0>(score, buf, NMoves, field, policy)){
                         // naive softmax selectorによりベース方策の確率を計算
                         //SoftmaxSelector selector(score, NMoves, Settings::simulationTemperaturePlay);
                         
                         // バイアス計算のための確率値
                         double score_sum = 0.0;
                         double score_jk_sum = 0.0;
                         double score_8_sum = 0.0;
                         double score_gr_sum = 0.0;
                         double score_seq_sum = 0.0;
                         double score_rev_sum = 0.0;
                         double score_pass_sum = 0.0;
                         
                         for(int i = 0; i < NMoves; ++i){
                             const MoveInfo mi = buf[i];
                             
                             const double s = exp(score[i] / Settings::simulationTemperaturePlay);
                             
                             if(mi.containsJOKER()){
                                 score_jk_sum += s;
                             }else if(mi.domInevitably()){
                                 score_8_sum += s;
                             }
                             if(mi.isSeq()){
                                 score_seq_sum += s;
                             }else if(mi.isGroup()){
                                 score_gr_sum += s;
                             }else if(mi.isPASS()){
                                 score_pass_sum += s;
                             }
                             if(mi.flipsPrmOrder()){
                                 score_rev_sum += s;
                             }
                             score_sum += s;
                         }
                         
                         double pos;
                         
                         // バイアス検討
                         if(score_sum > 0){
                             if(field.isNF()){
                                 // タイプバイアス
                                 if(score_gr_sum > 0){
                                     pos= score_gr_sum/score_sum;
                                     assert(0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.isGroup() ){
                                             pmodel->biasNF[0].feed(1-pos);
                                         }else{
                                             pmodel->biasNF[0].feed(-pos);
                                         }
                                     }
                                 }
                                 if(score_seq_sum > 0){
                                     pos= score_seq_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.isSeq() ){
                                             pmodel->biasNF[1].feed(1-pos);
                                         }else{
                                             pmodel->biasNF[1].feed(-pos);
                                         }
                                     }
                                 }
                                 // 特殊効果バイアス
                                 if( score_jk_sum > 0 ){
                                     pos= score_jk_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.containsJOKER() ){
                                             pmodel->biasNF[2].feed(1-pos);
                                         }else{
                                             pmodel->biasNF[2].feed(-pos);
                                         }
                                     }
                                 }
                                 if( score_8_sum > 0 ){
                                     pos= score_8_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.domInevitably() ){
                                             pmodel->biasNF[3].feed(1-pos);
                                         }else{
                                             pmodel->biasNF[3].feed(-pos);
                                         }
                                     }
                                 }
                                 if( score_rev_sum > 0 ){
                                     pos= score_rev_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.flipsPrmOrder() ){
                                             pmodel->biasNF[4].feed(1-pos);
                                         }else{
                                             pmodel->biasNF[4].feed(-pos);
                                         }
                                     }
                                 }
                                 
                             }else{
                                 double pos;
                                 
                                 int bdid = bd.isSeq()?2:(bd.isGroup()?1:0);
                                 
                                 // パスバイアス
                                 if( score_pass_sum > 0 ){
                                     pos= score_pass_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.isPASS() ){
                                             pmodel->biasRF[bdid][0].feed(1-pos);
                                         }else{
                                             pmodel->biasRF[bdid][0].feed(-pos);
                                         }
                                     }
                                 }
                                 // ジョーカーバイアス
                                 if( score_jk_sum > 0 ){
                                     pos= score_jk_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( fabs(score_jk_sum + score_pass_sum - score_sum) < 0.01 ){
                                             // ジョーカーを使わなければ出せない
                                             if( chosenMove.containsJOKER() ){
                                                 pmodel->biasRF[bdid][1].feed(1-pos);
                                             }else{
                                                 pmodel->biasRF[bdid][1].feed(-pos);
                                             }
                                         }else{
                                             // ジョーカーを使わなくても出せる
                                             if( chosenMove.containsJOKER() ){
                                                 pmodel->biasRF[bdid][2].feed(1-pos);
                                             }else{
                                                 pmodel->biasRF[bdid][2].feed(-pos);
                                             }
                                         }
                                     }
                                 }
                                 // 特殊効果バイアス
                                 if( score_8_sum > 0 ){
                                     pos= score_8_sum/score_sum;
                                     assert( 0<=pos && pos<=1 );
                                     if( pos > .01 ){
                                         if( chosenMove.domInevitably() ){
                                             pmodel->biasRF[bdid][3].feed(1-pos);
                                         }else{
                                             pmodel->biasRF[bdid][3].feed(-pos);
                                         }
                                     }
                                 }
                                 
                             }
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
	}
}

#endif // UECDA_FUJI_PLAYERBIAS_HPP_
