#pragma once

// 各プレーヤーのモデリング

#include "engineSettings.h"
#include "timeAnalysis.hpp"

namespace UECda{
	namespace Fuji{
        
        struct ProbBias{
            double size;
            double sum;
            double bias;
            
            double mean()const{
                assert(size > 0);
                return sum / size;
            }
            double value()const noexcept{
                // バイアス修正値を出す
                // 頻繁に呼ばれるのでここで長い計算はしない
                return bias;
            }
            void feed(double p){
                size += 1;
                sum += p;
                attenuate(Settings::playerBiasAttenuateRate);
                bias = calc_value(mean());
            }
            void attenuate(double r){
                size *= r;
                sum *= r;
            }
            void resize(double s){
                if(size > s){
                    sum *= s / size;
                    size = s;
                }
            }
            static double calc_value(double m){
                assert(-1 < m && m < 1);
                return 2.0 * log((1 + m) / (1 - m));
            }
            
            void init(double p, double i){
                size = i;
                sum = i * p;
                bias = calc_value(mean());
            }
        };
		
        struct PlayerModel{
            // 相手プレーヤーモデル
            // 相手プレーヤーの解析過程も一緒に載っている
            
            // 仮想プレーヤー部分空間
            bool clientFlag; // クライアント自身である
            int playerNum; // ゲーム中不変なプレーヤー番号
            
            // これまでの着手のまとめとか
            // 計算時間(ms単位)
            uint64_t allTime;
            uint32_t allPlays; // 着手決定機会
            
            // 合法着手数スケール別の計算時間
            // 1...0 2...1 3,4...2
            // 5~8...3 9~16...4 17~...5
            uint64_t NMovesTime[3][6];
            uint32_t NMovesPlays[3][6];
            
            // L2の場合の計算時間。L2計算時間に関して詳しい解析はしても仕方ないが一応
            uint64_t L2Time; uint32_t L2Plays;
            
            // 必勝の場合(必勝局面であり、必勝着手を選択した場合)
            // プレーヤーが真に必勝と認識していたかはわからないので不問。
            // L2は除外。
            uint64_t LnMateTime; uint32_t LnMatePlays;
            
            uint32_t mateSuccess, mateMiss; // 必勝判定精度
            uint32_t L2Success, L2Miss; // L2判定精度
            
            // 以下、L2と必勝は除く
            uint32_t chancePlays;
            uint32_t PassPlays;
            
            // 使用アルゴリズム指定
            BitArray32<4> type;
            
            TimeModel timeModel_;
            
            //着手決定バイアス
            ProbBias biasNF[5];
            ProbBias biasRF[3][4];
            
            TimeModel& timeModel()noexcept{ return timeModel_;}
            
            const TimeModel& timeModel()const noexcept{ return timeModel_; }
            
            bool isClient()const noexcept{
                return clientFlag;
            }
            
            void setPlayerNum(const int pnum)noexcept{
                playerNum = pnum;
            }
            void setClientFlag(const int flag)noexcept{
                clientFlag = flag;
            }
            
            int getPlayerNum()const noexcept{
                return playerNum;
            }
            
            void init(const int pNum, const bool cFlag){
                setPlayerNum(pNum);
                setClientFlag(cFlag);
                //type=0;
                timeModel_.init(time_model_easy4);
            }
            
            //結果報告
            void feedPlayTime(uint64_t argTime, uint32_t NMoves, int ph, Move pMove, bool isMate, bool isL2){
                //++allPlays;allTime+=argTime;
                
                if(isL2){
                    L2Time += argTime; ++L2Plays;
                }else if(isMate){
                    LnMateTime += argTime; ++LnMatePlays;
                }else{
                    
                    //assert(NMoves >= 1 && ph <= 2);
                    
                    //nms:NMovesのscale
                    //uint32_t nms=(NMoves==1)?0:(log2i(NMoves-1)+1);
                    //assert( nms >= 0 );
                    
                    //nms=min(nms,5U);
                    
                    //NMovesTime[ph][nms]+=argTime;
                    //++NMovesPlays[ph][nms];
                    
                    //feedTimeDist(playerNum,ph,nms,argTime);
                    
                    if(ph == 1){
                        if(NMoves == 1){ // NoChance
                            timeModel_.feed_real_time(1, argTime);
                        }else if(pMove.isPASS()){
                            timeModel_.feed_real_time(0, argTime);
                        }
                    }
                }
            }
            
            void feedChancePlay(bool isPASS)noexcept{
                ++chancePlays;
                if(isPASS){
                    ++PassPlays;
                }
            }
            
            void feedMateRel(bool success)noexcept{
                if(success){
                    ++mateSuccess;
                }else{
                    ++mateMiss;
                }
            }
            void feedL2Rel(bool success)noexcept{
                if(success){
                    ++L2Success;
                }else{
                    ++L2Miss;
                }
            }
            
            void initRecord(){
                //プレー記録初期化
                allTime = 0ULL;
                allPlays = 0U;
                PassPlays = 0U;
                chancePlays = 0U;
                for(int ph = 0; ph < 3 ;ph++){
                    for(int nms = 0; nms < 6; ++nms){
                        NMovesTime[ph][nms] = 0ULL;
                        NMovesPlays[ph][nms] = 0U;
                    }
                }
                mateSuccess = mateMiss = 0;
                L2Success = L2Miss = 0;
            }
            
            void initBias(){
                //局面推定用の時間解析初期化
                
                // 着手バイアス
                for(int i = 0; i < 5; ++i){
                    biasNF[i].init(0, Settings::playerBiasPriorSize);
                }
                for(int i = 0; i < 3; ++i){
                    for(int j = 0;j < 4; ++j){
                        biasRF[i][j].init(0, Settings::playerBiasPriorSize);
                    }
                }
            }
            
            std::string toBiasString()const{
                std::ostringstream oss;
                oss << "NF :" << endl;
                oss << " GR " << biasNF[0].mean();
                oss << " SQ " << biasNF[1].mean();
                oss << " JK " << biasNF[2].mean();
                oss << " 8C " << biasNF[3].mean();
                oss << " RV " << biasNF[4].mean();
                oss << endl;
                
                oss << "RF :"<< endl;
                oss << " SIN-";
                
                oss << " PS " << biasRF[0][0].mean();
                oss << " JO " << biasRF[0][1].mean();
                oss << " JA " << biasRF[0][2].mean();
                oss << " 8C " << biasRF[0][3].mean();
                
                oss << endl;
                
                oss << " GRO-";
                
                oss << " PS " << biasRF[1][0].mean();
                oss << " JO " << biasRF[1][1].mean();
                oss << " JA " << biasRF[1][2].mean();
                oss << " 8C " << biasRF[1][3].mean();
                
                oss << endl;
                
                oss << " SEQ-";
                
                oss << " PS " << biasRF[2][0].mean();
                oss << " JO " << biasRF[2][1].mean();
                oss << " JA " << biasRF[2][2].mean();
                oss << " 8C " << biasRF[2][3].mean();
                
                oss << endl;
                return oss.str();
            }
            
            PlayerModel(){}
            ~PlayerModel(){}
            
            
            void closeMatch(){
                cerr << "time dist of player " << playerNum << " : " << endl;
                for(int ph = 0; ph < 2; ++ph){
                    cerr<<" ";
                    for(int s = 0; s < 7; ++s){
                        cerr << timeModel_.time_dist7[ph][s] << ", ";
                    }
                    cerr << endl;
                }
            }
        };
        
        struct PlayerModelSpace{
            // 試合全体でのモデリング空間
            
            std::array<PlayerModel, N_PLAYERS> model_;
            
            PlayerModel& model(int p){ return model_[p]; }
            const PlayerModel& model(int p)const{ return model_[p]; }
            
            void init(int myPlayerNum){
                for(int p = 0; p < N_PLAYERS; ++p){
                    model_[p].init(p, p == myPlayerNum);
                    model_[p].initBias();
                    model_[p].initRecord();
                }
            }
            
            void closeMatch(){
                
            }
            
            PlayerModelSpace(){
            }
        };
        
	}
}
