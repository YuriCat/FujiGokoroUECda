
#ifndef _MONTECARLOLOG_HPP
#define _MONTECARLOLOG_HPP

// モンテカルロ着手決定のログ
// 自分のプレー時における認識と、現実との差を調べる
// クライアントが随時着手に反映出来れば良いのだが、
// 現在は開発者が試合終了後に確認するだけ

namespace UECda{
    namespace Fuji{
        
        struct MonteCarloPlayLog{
            
            int turnNum;
            uint64_t time;
            
            MonteCarloChild child;
            
            int expReward;//予測報酬
            int realReward;//実報酬
            
            std::bitset<3> realDomFlag;//実際の支配orNot
            
            const ClientPlayLog<N_PLAYERS> *pLog;
            
            void setID(){realDomFlag.set(0);}
            void setCD(){realDomFlag.set(1);}
            void setBD(){realDomFlag.set(2);}
            
            void feedChild(int t,const MonteCarloChild& ch,int rew){
                child=ch;
                expReward=rew;
                turnNum=t;
            }
        };
        
        struct MonteCarloGameLog{
            
            int gameNum;
            MonteCarloPlayLog play[PLAY_MAX + 1];
            
            int plays;//これまでのプレー数
            
            int realReward;
            
            void feedPlay(int t,const MonteCarloChild& ch,int rew){
                if( plays <= PLAY_MAX ){
                    //枠があるときのみ記録する
                    play[plays].feedChild(t,ch,rew);
                    ++plays;
                }
            }
            
            template<class gameLog_t>
            void feedGameLog(uint32_t pn,const gameLog_t *const gLog){
                
                assert( gLog != nullptr );
                
                //ゲームのログから支配情報等をまとめる
                int bIG=min( 14, gLog->gameNum );//ランク初期化まで残り何試合か
                
                //この試合で実際に得た報酬
                realReward=iReward[bIG][ gLog->getPlayerNewClass(pn) ];
                
                int mcpl=0;//見たいMCログのインデックス
                //int mclpl=0;//自分が前にプレーしたターンのインデックス(初期に-1にしないための処理)
                
                int afterMyTurn=0;
                int isNoChance=0;
                
                MonteCarloPlayLog *pMCLog_last;
                //MonteCarloPlayLog *pMCLog;
                
                pMCLog_last=nullptr;
                
                for(int t=0;t<gLog->turns;++t){
                    
                    //プレイログのポインタをセット
                    const auto *const pLog=&gLog->play[t];
                    
                    if( pLog->getTurnPlayer() == pn){
                        //自分のターン
                        if( pMCLog_last!=nullptr && afterMyTurn ){
                            //前の自分の着手がモンテカルロであり、
                            //前に自分が出してから誰も出していない
                            pMCLog_last->setCD();
                            if(isNoChance){
                                //前に自分が出してから誰も出せなかった
                                //COUT<<"NC";getchar();
                                pMCLog_last->setID();
                            }
                            
                            if( pLog->preBd.isNF() ){
                                //このとき、ブロックを取ったので前にさかのぼってBDを設定(未実装)
                                int bl=pLog->blockNum-1;
                                
                                //cout<<bl<<",";
                                
                                if( bl>=0 ){
                                    //同じブロックの着手全てにBDを設定
                                    for(int mct=mcpl;mct>=0;--mct){
                                        int tmp_bl=gLog->play[ play[mct].turnNum ].blockNum;
                                        if( tmp_bl == bl ){
                                            //同じブロックだったのでBD
                                            play[mct].setBD();
                                        }else if( tmp_bl<bl ){
                                            //もう前に戻っても意味無し
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        
                        if( pLog->cards[pn] == pLog->move.c() ){//あがり
                            break;
                        }
                        
                        if( t == play[mcpl].turnNum ){
                            //自分がMCで着手決定したターン
                            
                            //pMCLog=&play[mcpl];
                            
                            afterMyTurn=1;
                            isNoChance=1;
                            
                            pMCLog_last=&play[mcpl];
                            
                            if(mcpl < PLAY_MAX){//容量オーバーでない
                                ++mcpl;
                            }
                        }else{
                            afterMyTurn=0;
                            pMCLog_last=nullptr;
                        }
                        
                    }else{
                        //自分以外のプレーヤーのターン
                        if(!pLog->move.isPASS()){
                            //他の誰かが出した
                            afterMyTurn=0;
                            isNoChance=0;
                        }else{
                            if(!dominatesCards(pLog->preBd,pLog->cards[ pLog->getTurnPlayer() ])){
                                //必然支配ではなかった
                                isNoChance=0;
                            }
                        }
                    }
                }
            }
            
            void init(){
                plays=0;
            }
        };
        
        struct MonteCarloLog{
            
            int playerNum;
            
            MonteCarloGameLog* thisGameLog;
            std::vector<MonteCarloGameLog*> log_cycle;
            
            template<class gameLog_t>
            void feedGameLog(int pn,const gameLog_t *const gLog){
                assert(thisGameLog != nullptr);
                thisGameLog->feedGameLog(pn,gLog);
            }
            
            template<class log_t>
            void analyze(const log_t& log){
                
                //支配率判定
                int64_t expCDR[8]={0ULL};
                int64_t expIDR[8]={0ULL};
                int64_t expBDR[8]={0ULL};
                
                //実支配率
                int64_t realCDR[8]={0ULL};
                int64_t realIDR[8]={0ULL};
                int64_t realBDR[8]={0ULL};
                
                //報酬
                int64_t expReward[24]={0ULL};
                int64_t realReward[24]={0ULL};
                
                int cntT[24]={0};
                int cntM[8]={0};
                
                int cntAll=0;
                //int trialAll=0;
                
                DOUT<<"games : "<<log.games<<endl;
                
                for(int g=0; g<log.games; ++g){
                    
                    const auto *const gLog=log.read(g);//その試合のログ
                    const MonteCarloGameLog *const gMCLog=read(g);//その試合のログ
                    
                    DOUT<<"game "<<g<<" gl ptr "<<(uint64_t)gLog<<" , "<<(uint64_t)gMCLog<<endl;
                    
                    assert( gLog != nullptr );
                    assert( gMCLog != nullptr );
                    
                    DOUT<<"my plays : "<<gMCLog->plays<<endl;
                    
                    for(int mcpl=0;mcpl<gMCLog->plays;mcpl++){
                        
                        
                        const MonteCarloPlayLog *const pMCLog=&gMCLog->play[mcpl];
                        
                        
                        int turnNum=pMCLog->turnNum;
                        
                        if(pMCLog->turnNum < 0 || pMCLog->turnNum > TURN_MAX){
                            DOUT<<"over turn "<<pMCLog->turnNum<<endl;
                            continue;
                        }//pLogの枠オーバー
                        
                        DOUT<<"turn "<<turnNum;
                        
                        const auto *const pLog=&gLog->play[turnNum];
                        
                        DOUT<<" pl ptr "<<(uint64_t)pLog<<" , "<<(uint64_t)pMCLog<<endl;
                        
                        int bType=pLog->preBd.getTypeNum();
                        int mType=pLog->move.getTypeNum();
                        
                        if(( bType >= 0 && bType <= 7) && ( mType >= 0 && mType <= 7)){//例外場、着手でない
                            //const int tmpTrials=pMCLog->child.trials;
                            
                            if( pMCLog->child.trials > 0 ){
                                
                                //支配
                                expIDR[mType]+=pMCLog->child.iDom * 999 /pMCLog->child.trials;
                                expCDR[mType]+=pMCLog->child.cDom * 999 /pMCLog->child.trials;
                                expBDR[mType]+=pMCLog->child.bDom * 999 /pMCLog->child.trials;
                                
                                realIDR[mType]+=pMCLog->realDomFlag.test(0)? 999 :0;
                                realCDR[mType]+=pMCLog->realDomFlag.test(1)? 999 :0;
                                realBDR[mType]+=pMCLog->realDomFlag.test(2)? 999 :0;
                                
                            }
                            ++cntM[mType];
                            
                            int ph=pLog->preBd.isNF()?mType:(8+ bType*2 +(mType==0?0:1));
                            
                            if( 0<=ph && ph<24 ){
                                
                                //報酬系
                                expReward[ph]+=pMCLog->expReward;
                                realReward[ph]+=gMCLog->realReward;
                                
                                ++cntT[ph];
                            }
                            
                            ++cntAll;
                            //trialsAll+=tmpTrials;
                        }
                    }
                    
                }
                
                for(int type=0;type<24;type++){
                    if(cntT[type]){
                        int cnt=cntT[type];
                        expReward[type]/=cnt;
                        realReward[type]/=cnt;
                    }
                }
                
                for(int type=0;type<8;type++){
                    if(cntM[type]){
                        int cnt=cntM[type];
                        
                        expIDR[type]/=cnt;
                        expCDR[type]/=cnt;
                        expBDR[type]/=cnt;
                        /*
                         if( expCDR[type] < 999 ){
                         expBDR[type]= 999 * (expBDR[type]-expCDR[type]) / ( 999 - expCDR[type] );
                         }else{
                         expBDR[type]=0;
                         }
                         
                         if( expIDR[type] < 999 ){
                         expCDR[type]= 999 * (expCDR[type]-expIDR[type]) / ( 999 - expIDR[type] );
                         }else{
                         expCDR[type]=0;
                         }*/
                        
                        //実支配率
                        
                        realIDR[type]/=cnt;
                        realCDR[type]/=cnt;
                        realBDR[type]/=cnt;
                        /*
                         if( realCDR[type] < 999 ){
                         realBDR[type]= 999 * (realBDR[type]-realCDR[type]) / ( 999 - realCDR[type] );
                         }else{
                         realBDR[type]=0;
                         }
                         */
                        /*
                         if( realIDR[type] < 999 ){
                         realCDR[type]= 999 * (realCDR[type]-realIDR[type]) / ( 999 - realIDR[type] );
                         }else{
                         realCDR[type]=0;
                         }*/
                    }
                }
                cout<<"---Monte Carlo Analysis---"<<endl;
                
                cout<<"Reward :"<<endl;
                cout<<"Nul PAS: exp "<<setw(4)<<expReward[0]<<"  real "<<setw(4)<<realReward[0]<<" ("<<setw(5)<<cntT[0]<<")"<<endl;
                cout<<"Nul Sin: exp "<<setw(4)<<expReward[1]<<"  real "<<setw(4)<<realReward[1]<<" ("<<setw(5)<<cntT[1]<<")"<<endl;
                cout<<"Nul Dou: exp "<<setw(4)<<expReward[2]<<"  real "<<setw(4)<<realReward[2]<<" ("<<setw(5)<<cntT[2]<<")"<<endl;
                cout<<"Nul Tri: exp "<<setw(4)<<expReward[3]<<"  real "<<setw(4)<<realReward[3]<<" ("<<setw(5)<<cntT[3]<<")"<<endl;
                cout<<"Nul Qua: exp "<<setw(4)<<expReward[4]<<"  real "<<setw(4)<<realReward[4]<<" ("<<setw(5)<<cntT[4]<<")"<<endl;
                cout<<"Nul 3Sq: exp "<<setw(4)<<expReward[5]<<"  real "<<setw(4)<<realReward[5]<<" ("<<setw(5)<<cntT[5]<<")"<<endl;
                cout<<"Nul 4Sq: exp "<<setw(4)<<expReward[6]<<"  real "<<setw(4)<<realReward[6]<<" ("<<setw(5)<<cntT[6]<<")"<<endl;
                cout<<"Nul 5Sq: exp "<<setw(4)<<expReward[7]<<"  real "<<setw(4)<<realReward[7]<<" ("<<setw(5)<<cntT[7]<<")"<<endl;
                
                cout<<"Sin PAS: exp "<<setw(4)<<expReward[10]<<"  real "<<setw(4)<<realReward[10]<<" ("<<setw(5)<<cntT[10]<<")"<<endl;
                cout<<"Sin Sin: exp "<<setw(4)<<expReward[11]<<"  real "<<setw(4)<<realReward[11]<<" ("<<setw(5)<<cntT[11]<<")"<<endl;
                cout<<"Dou PAS: exp "<<setw(4)<<expReward[12]<<"  real "<<setw(4)<<realReward[12]<<" ("<<setw(5)<<cntT[12]<<")"<<endl;
                cout<<"Dou Dou: exp "<<setw(4)<<expReward[13]<<"  real "<<setw(4)<<realReward[13]<<" ("<<setw(5)<<cntT[13]<<")"<<endl;
                cout<<"Tri PAS: exp "<<setw(4)<<expReward[14]<<"  real "<<setw(4)<<realReward[14]<<" ("<<setw(5)<<cntT[14]<<")"<<endl;
                cout<<"Tri Tri: exp "<<setw(4)<<expReward[15]<<"  real "<<setw(4)<<realReward[15]<<" ("<<setw(5)<<cntT[15]<<")"<<endl;
                cout<<"Qua PAS: exp "<<setw(4)<<expReward[16]<<"  real "<<setw(4)<<realReward[16]<<" ("<<setw(5)<<cntT[16]<<")"<<endl;
                cout<<"Qua Qua: exp "<<setw(4)<<expReward[17]<<"  real "<<setw(4)<<realReward[17]<<" ("<<setw(5)<<cntT[17]<<")"<<endl;
                
                cout<<"3sq PAS: exp "<<setw(4)<<expReward[18]<<"  real "<<setw(4)<<realReward[18]<<" ("<<setw(5)<<cntT[18]<<")"<<endl;
                cout<<"3sq 3sq: exp "<<setw(4)<<expReward[19]<<"  real "<<setw(4)<<realReward[19]<<" ("<<setw(5)<<cntT[19]<<")"<<endl;
                cout<<"4sq PAS: exp "<<setw(4)<<expReward[20]<<"  real "<<setw(4)<<realReward[20]<<" ("<<setw(5)<<cntT[20]<<")"<<endl;
                cout<<"4sq 4sq: exp "<<setw(4)<<expReward[21]<<"  real "<<setw(4)<<realReward[21]<<" ("<<setw(5)<<cntT[21]<<")"<<endl;
                cout<<"5sq PAS: exp "<<setw(4)<<expReward[22]<<"  real "<<setw(4)<<realReward[22]<<" ("<<setw(5)<<cntT[22]<<")"<<endl;
                cout<<"5sq 5sq: exp "<<setw(4)<<expReward[23]<<"  real "<<setw(4)<<realReward[23]<<" ("<<setw(5)<<cntT[23]<<")"<<endl;
                
                cout<<endl;
                
                cout<<"Dominance :"<<endl;
                
                cout<<"PAS: exp I "<<setw(3)<<expIDR[0]<<" C "<<setw(3)<<expCDR[0]<<" B "<<setw(3)<<expBDR[0]<<"  real I "<<setw(3)<<realIDR[0]<<" C "<<setw(3)<<realCDR[0]<<" B "<<setw(3)<<realBDR[0]<<" ( "<<setw(5)<<cntM[0]<<")"<<endl;
                cout<<"Sin: exp I "<<setw(3)<<expIDR[1]<<" C "<<setw(3)<<expCDR[1]<<" B "<<setw(3)<<expBDR[1]<<"  real I "<<setw(3)<<realIDR[1]<<" C "<<setw(3)<<realCDR[1]<<" B "<<setw(3)<<realBDR[1]<<" ( "<<setw(5)<<cntM[1]<<")"<<endl;
                cout<<"Dou: exp I "<<setw(3)<<expIDR[2]<<" C "<<setw(3)<<expCDR[2]<<" B "<<setw(3)<<expBDR[2]<<"  real I "<<setw(3)<<realIDR[2]<<" C "<<setw(3)<<realCDR[2]<<" B "<<setw(3)<<realBDR[2]<<" ( "<<setw(5)<<cntM[2]<<")"<<endl;
                cout<<"Tri: exp I "<<setw(3)<<expIDR[3]<<" C "<<setw(3)<<expCDR[3]<<" B "<<setw(3)<<expBDR[3]<<"  real I "<<setw(3)<<realIDR[3]<<" C "<<setw(3)<<realCDR[3]<<" B "<<setw(3)<<realBDR[3]<<" ( "<<setw(5)<<cntM[3]<<")"<<endl;
                cout<<"Qua: exp I "<<setw(3)<<expIDR[4]<<" C "<<setw(3)<<expCDR[4]<<" B "<<setw(3)<<expBDR[4]<<"  real I "<<setw(3)<<realIDR[4]<<" C "<<setw(3)<<realCDR[4]<<" B "<<setw(3)<<realBDR[4]<<" ( "<<setw(5)<<cntM[4]<<")"<<endl;
                cout<<"3sq: exp I "<<setw(3)<<expIDR[5]<<" C "<<setw(3)<<expCDR[5]<<" B "<<setw(3)<<expBDR[5]<<"  real I "<<setw(3)<<realIDR[5]<<" C "<<setw(3)<<realCDR[5]<<" B "<<setw(3)<<realBDR[5]<<" ( "<<setw(5)<<cntM[5]<<")"<<endl;
                cout<<"4sq: exp I "<<setw(3)<<expIDR[6]<<" C "<<setw(3)<<expCDR[6]<<" B "<<setw(3)<<expBDR[6]<<"  real I "<<setw(3)<<realIDR[6]<<" C "<<setw(3)<<realCDR[6]<<" B "<<setw(3)<<realBDR[6]<<" ( "<<setw(5)<<cntM[6]<<")"<<endl;
                cout<<"5sq: exp I "<<setw(3)<<expIDR[7]<<" C "<<setw(3)<<expCDR[7]<<" B "<<setw(3)<<expBDR[7]<<"  real I "<<setw(3)<<realIDR[7]<<" C "<<setw(3)<<realCDR[7]<<" B "<<setw(3)<<realBDR[7]<<" ( "<<setw(5)<<cntM[7]<<")"<<endl;
                
                cout<<endl;
                
            }
            
            void initAllG(){
                thisGameLog=nullptr;
                log_cycle.clear();
            }
            
            void init1G(int gn){
                //サイクルにあわせてメモリ確保
                if(gn % GAME_CYCLE == 0){
                    thisGameLog=(MonteCarloGameLog*)calloc(GAME_CYCLE,sizeof(MonteCarloGameLog));
                    if( thisGameLog == NULL ){
                        cout<<"failed to get log memory."<<endl;
                        exit(1);
                    }
                    log_cycle.push_back(thisGameLog);
                }else{
                    ++thisGameLog;
                }
                thisGameLog->init();
            }
            
            void close1G(){
                //なし
            }
            
            void closeAllG(){
                //メモリ解放
                for(int i=log_cycle.size()-1;i>=0;--i){
                    free(log_cycle[i]);
                }
                COUT<<"mc_log closed."<<endl;
            }
            
            MonteCarloGameLog* access(int gn){
                MonteCarloGameLog *res=log_cycle.at(gn / GAME_CYCLE) + (gn % GAME_CYCLE);
                assert(res != nullptr);
                return res;
            }
            
            const MonteCarloGameLog* read(int gn)const{
                //こちらは書き換え不可
                MonteCarloGameLog *res=log_cycle.at(gn / GAME_CYCLE) + (gn % GAME_CYCLE);
                assert(res != nullptr);
                return res;
            }
            
            MonteCarloLog(){
            }
            
            ~MonteCarloLog(){
                closeAllG();
            }
        };
        
    }
}


#endif /* _MONTECARLOLOG_HPP */

