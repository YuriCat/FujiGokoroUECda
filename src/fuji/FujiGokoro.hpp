/*
 FujiGokoro.hpp
 Katsuki Ohto
 */

// メインクライアント
// 実際のゲームでの局面更新など低次な処理はmain関数にて

#ifndef UECDA_FUJIGOKORO_HPP_
#define UECDA_FUJIGOKORO_HPP_

#define _ENGINE_FUJI_

// 思考部ヘッダ
#include "fuji.h"
#include "fujiStructure.hpp"

// クライアント部分空間

// 報酬系トップダウンパラメータ
#include "value.hpp"

// 諸々の計算とか判定

// ロジック
#include "logic/appliedLogic.hpp"

// 支配判定
#include "logic/dominance.hpp"

// 必勝判定
#include "logic/mate.hpp"

// 末端探索
#include "search/l2Judge.hpp"

// ヒューリスティクス
#include "heuristics.hpp"

// 交換、着手方策
#include "policy/changePolicy.hpp"
#include "policy/playPolicy.hpp"

//プレイアウト関連ヘッダ
#include "montecarlo/playout.h"

#ifndef POLICY_ONLY

// 相手の行動モデル
#include "model/timeAnalysis.hpp"
#include "model/playerModel.hpp"

// プレイアウト関連

// 仮想世界プール
#include "estimation/galaxy.hpp"

#endif // !POLICY_ONLY

// 学習
#include "learning/policyGradient.hpp"

#ifndef POLICY_ONLY

// モンテカルロ
#include "montecarlo/monteCarloPlayer.hpp"

// 相手の行動解析
#include "model/playerAnalysis.hpp"

#endif // ！POLICY_ONLY

namespace UECda{
	namespace Fuji{
		class Client{
		private:
            using dice64_t = XorShift64;
            
            ThreadTools threadTools[N_THREADS];
            SharedData shared;
            
            // 0番スレッドのサイコロをメインサイコロとして使う
            dice64_t& dice = threadTools[0].dice;
            
        public:
            using field_t = UECda::Fuji::FujiField;
            
            field_t field;
            
            decltype(shared.gameLog)& gameLog = shared.gameLog;
            decltype(shared.matchLog)& matchLog = shared.matchLog;
            
            void replaceField(const field_t& afield)noexcept{
                field = afield;
            }
            
            void setRandomSeed(uint64_t s)noexcept{
                // 乱数系列を初期化
                XorShift64 tdice;
                tdice.srand(s);
                for(int th = 0; th < N_THREADS; ++th){
                    threadTools[th].dice.srand(tdice.rand() * (th + 111));
                }
            }
			
			void initMatch(){
                // field にプレーヤー番号が入っている状態で呼ばれる
                shared.setMyPlayerNum(field.getMyPlayerNum());
                
                // サイコロ初期化
                // シード指定の場合はこの後に再設定される
                setRandomSeed((uint32_t)time(NULL));
                
                // スレッドごとのデータ初期化
                for(int th = 0; th < N_THREADS; ++th){
                    threadTools[th].init(th);
                }
                
#ifndef POLICY_ONLY
                // 世界プール監視員を設定
                for(int th = 0; th < N_THREADS; ++th){
                    shared.ga.set(th, &threadTools[th].gal);
                }
#endif
                
                auto& playPolicy = shared.basePlayPolicy;
                auto& changePolicy = shared.baseChangePolicy;
                
                shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");
                shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
            
#ifndef POLICY_ONLY
                // 推定用ポリシー
                std::ifstream ifs;
                ifs.open(DIRECTORY_PARAMS_IN + "play_policy_param_estimation.dat");
                if(ifs){
                    shared.estimationPlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param_estimation.dat");
                }else{
                    shared.estimationPlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");
                }
                ifs.close();
                ifs.open(DIRECTORY_PARAMS_IN + "change_policy_param_estimation.dat");
                if(ifs){
                    shared.estimationChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param_estimation.dat");
                }else{
                    shared.estimationChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
                }
                ifs.close();
#endif
                
                // 方策の温度
                shared.basePlayPolicy.setTemperature(Settings::simulationTemperaturePlay);
                shared.baseChangePolicy.setTemperature(Settings::simulationTemperatureChange);
#ifndef POLICY_ONLY
                shared.estimationPlayPolicy.setTemperature(Settings::simulationTemperaturePlay);
                shared.estimationChangePolicy.setTemperature(Settings::simulationTemperatureChange);
#endif
			}
			void initGame(){
				
				// 汎用変数の設定
#ifndef POLICY_ONLY
				// 報酬設定
				int bIG = getNGamesForIG(field.getGameNum());
				// ランク初期化まで残り何試合か
				// 公式には未定だが、多くの場合100試合だろうから100試合としておく
				
                // N_REWARD_CALCULATED_GAMES 試合前より以前は同じ報酬とする
				bIG = min(bIG, N_REWARD_CALCULATED_GAMES - 1);
				for(int r = 0; r < N_PLAYERS; ++r){
					shared.gameReward[r] = iReward[bIG][r];
				}
				
				// 前の試合の順位から予測される期待報酬
				double dist_sum = 0;
				double last_er = 0;
				for(int r = 0; r < N_PLAYERS; ++r){
					dist_sum += seni_dist[field.getMyClass()][r];
					last_er += seni_dist[field.getMyClass()][r] * (double)shared.gameReward[r];
				}
				if(dist_sum > 0){
					last_er /= dist_sum;
				}else{
					last_er = 420; // なぜか分布がおかしいときは適当に設定
				}
				field.last_exp_reward = last_er;
                
                // 置換表初期化
#ifdef USE_L2BOOK
                L2::book.init();
#endif
#ifdef USE_LNCIBOOK
                LnCI::book.init();
#endif
#endif // !POLICY_ONLY
				
			}
			
			Cards change(uint32_t change_qty){ // 交換関数
				assert(change_qty == 1U || change_qty == 2U);
				
				Cards changeCards = CARDS_NULL;
				const Cards myCards = field.getMyDealtCards();
				
#ifdef MONITOR
				CERR << "My Cards : " << OutCards(myCards) << endl;
#endif
				
#ifdef RARE_PLAY
				// レアプレーを行っていない場合は行う
				if(!field.rare_play_flag.test(8)
				   && containsJOKER(myCards) 
				   && change_qty == 1
				   ){
					// ジョーカー捨て
					// 2枚交換のとき1枚のみ確定するのは面倒なので、1枚交換の時のみにする
					field.rare_play_flag.set(8);
					return CARDS_JOKER;
				}
#endif
				// 先読み用バッファはスレッド0のもの
				MoveInfo *const searchBuffer = threadTools[0].buf;
                
                // 手札レベルで枝刈りする場合
#ifdef PRUNE_ROOT_CHANGE_BY_HEURISTICS
                Cards tmp = Heuristics::pruneCards(myCards, change_qty);
#else
                Cards tmp = myCards;
#endif
                
                if(countCards(tmp) == change_qty){ return tmp; }
				
				// 合法交換候補生成
				Cards cand[N_MAX_CHANGES]; // 最大候補数

                int NCands = genChange(cand, tmp, change_qty);
                
				if(NCands == 1){
					assert(holdsCards(myCards, cand[0]) && countCards(cand[0]) == change_qty);
					return cand[0];
				}
				
				// 必勝チェック&枝刈り
				for(int c = 0; c < NCands;){
					
					assert(holdsCards(myCards, cand[c]));
					
					// 交換後の自分のカード
					Cards restCards = subtrCards(myCards, cand[c]);
					
					Hand mine;
					mine.set(restCards);
					
					// D3を持っている場合、自分からなので必勝チェック
					if(containsD3(myCards)){
						const Board bd = OrderToNullBoard(ORDER_NORMAL); // 通常オーダーの空場
                        FieldAddInfo fieldInfo;
                        fieldInfo.init();
                        fieldInfo.setFlushLead();
                        fieldInfo.setMinNCardsAwake(10);
                        fieldInfo.setMinNCards(10);
                        fieldInfo.setMaxNCardsAwake(11);
                        fieldInfo.setMaxNCards(11);
						Hand ops;
						ops.set(subtrCards(CARDS_ALL, restCards));
						if(judgeHandMate(1, searchBuffer, mine, ops, bd, fieldInfo)){
							// 必勝
							CERR << "CHANGE MATE!" << endl;
							assert(holdsCards(myCards, cand[c]) && countCards(cand[c]) == change_qty);
							return cand[c];
						}
					}

#ifdef PRUNE_ROOT_CHANGE_BY_HEURISTICS
					// 残り札に革命が無い場合,Aもダメ
					if(isNoRev(restCards)){
						if(andCards(cand[c], CARDS_A)){
							std::swap(cand[c], cand[--NCands]);
							goto TO_NEXT;
						}
					}
                    
				TO_NEXT:
					if(NCands == 1){
						assert(holdsCards(myCards, cand[0]) && countCards(cand[0]) == change_qty);
						return cand[0];
					}
#endif
					++c;	
				}
				CERR << "for Change MC" << endl;
				
#ifndef POLICY_ONLY
				if(changeCards == CARDS_NULL){
					// 世界プールを整理する
					for(int th = 0; th < N_THREADS; ++th){
						threadTools[th].gal.clear();
					}
					MonteCarloPlayer mcp;
					int MCId = mcp.change(cand, NCands, field, &shared, threadTools);
					assert(0 <= MCId && MCId < NCands);
					changeCards = cand[MCId];
				}
                
#endif // POLICY_ONLY
				
				// 方策関数による交換決定
				if(changeCards == CARDS_NULL){
                    int idx = changeWithBestPolicy(cand, NCands, myCards, change_qty, field,
                                                   shared.baseChangePolicy, &dice);
                    assert(0 <= idx && idx < NCands);
                    changeCards = cand[idx];
				}
                
            DECIDED_CHANGE:
                assert(countCards(changeCards) == change_qty);
                assert(holdsCards(myCards, changeCards));
#ifdef MONITOR
                cerr << "\033[1m";
                cerr << "\033[" << 34 << "m";
                cerr << "Best Change : " << OutCards(changeCards) << endl;
                cerr << "\033[" << 39 << "m";
                cerr << "\033[0m";
#endif
				return changeCards;
			}
			void afterChange(){
				
			}
			void waitChange(){
				
			}
			void prepareForGame(){
#ifndef POLICY_ONLY
				field.initWorldPatterns();
#endif
			}
            Move play(){
                // 自分のプレーについての変数を更新
                ClockMicS clms;
                clms.start();
                Move ret = playSub();
#ifndef POLICY_ONLY
                shared.timeAnalyzer.my_play_time_sum += clms.stop();
                shared.timeAnalyzer.my_plays++;
#endif
                return ret;
            }
			Move playSub(){ // ここがプレー関数
                
                // 思考用の局面表現に変換
                PlayouterField tfield;
                
                setSubjectiveField(field, &tfield);
				
				Move playMove = MOVE_NONE;
				
				// 先読み用バッファはスレッド0のものを使う
				MoveInfo *const searchBuffer = threadTools[0].buf;
				
				// 生成バッファは独立なものを使う
				MoveInfo buf[N_MAX_MOVES + 256];
				PlaySpace<MoveInfo> ps(buf);
				const int& NMoves = ps.qty;
				MoveInfo *const mv = ps.getMovePtr();
				
				FieldAddInfo& fieldInfo = tfield.fieldInfo;
				
                const int myPlayerNum = field.getMyPlayerNum();
				const Hand& myHand = tfield.getHand(myPlayerNum);
				const Hand& opsHand = tfield.getOpsHand(myPlayerNum);
				const Cards myCards = myHand.getCards();
				const Cards opsCards = opsHand.getCards();

				const Board bd = tfield.getBoard();
                CERR << bd << endl;
				
				// サーバーの試合進行バグにより無条件支配役が流れずに残っている場合はリジェクトにならないようにパスしておく
				if(bd.domInevitably()){ return MOVE_PASS; }
				
				// 既に手が決まっている場合
				/*if(field.nextMoves.any()){
				 playMove = field.nextMoves.top();
				 return playMove;
				 }*/
				
#ifdef RARE_PLAY
				// レアプレーを行っていない場合は行う
				if(bd.isNF() && !rare_play_flag.test(0)){ // 空場パス
					rare_play_flag.set(0);
					return MOVE_PASS;
				}
#endif
				
				// ここから通常の着手決定...
				Move decided_move[4]; // 先の手まで決める場合
				
				// 着手決定行動前の初期化
				int rp = 0, rp_mc = 0; // 反復プレー数
				
				// 現在は反復決定は行っていない
				for(rp = 0; rp < 1 ; ++rp){
					
					// 合法着手生成
					ps.qty = ps.activeQty = genMove(ps.mv, myCards, bd);
					
					if(NMoves <= 0){
						// 合法着手なし
						cerr << "No valid move." << endl;
						return MOVE_PASS;
					}
					if(NMoves == 1){
						// 合法着手1つ。パスのみか、自分スタートで手札1枚
						// 本当はそういう場合にすぐ帰ると手札がばれるのでよくない
#ifdef MONITOR
                        if(!mv[0].isPASS()){
                            cerr << "final move. " << mv[0] << endl;
                        }else{
                            cerr << "no chance. PASS" << endl;
                        }
#endif
                        
						if(!mv[0].isPASS()){ field.setMyMate(); }
						return (Move)mv[0];
					}
					
					
					// 合法着手生成(特別着手)
                    int ex_cnt = 0;
                    if(bd.isNF()){
                        ex_cnt += genNullPass(ps.mv + ps.qty + ex_cnt);
                    }
					if(containsJOKER(myCards)
					   && containsS3(opsCards)){
                        if(bd.isGroup()){
							ex_cnt += genJokerGroup(ps.mv + ps.qty + ex_cnt, myCards, opsCards, bd);
						}
					}
					
					if(ex_cnt > 0){
						ps.qty += ex_cnt;
						ps.activeQty += ex_cnt;
					}
                    
                    // 着手多様性確保のため着手をランダムシャッフル
                    for(int i = NMoves; i > 1; --i){
                        int idx = dice.rand() % i;
                        std::swap(ps.mv[idx], ps.mv[i - 1]);
                    }
					
					// 場の情報をまとめる
                    assert(tfield.getTurnPlayer() == myPlayerNum);
                    tfield.prepareForPlay(true);
					
					// 着手追加情報を設定
					// 自分の着手の事なので詳しく調べる
					
					int curMinNMelds = calcMinNMelds(searchBuffer, myCards);
					
					setDomState(&ps, tfield); // 先の場も含めて支配状況をまとめて設定
					
					for(int m = NMoves - 1; m >= 0; --m){
						MoveInfo *const mi = &mv[m];
						const Move move = mi->mv();
						
						Cards nextCards = maskCards(myCards, move.cards());
						// 後場情報
						if(bd.afterPrmOrder(move) != ORDER_NORMAL){ mi->setPrmOrderRev(); }
						if(bd.afterTmpOrder(move) != ORDER_NORMAL){ mi->setTmpOrderRev(); }
						if(bd.afterSuitsLocked(move)){ mi->setSuitLock(); }
						
#ifdef SEARCH_ROOT_MATE
                        if(Settings::MateSearchOnRoot){
                            // 必勝判定
                            DERR << *mi << endl;
                            if(checkHandMate(1, searchBuffer, *mi, myHand, opsHand, bd, fieldInfo)){
                                mi->setMate(); fieldInfo.setMate();
#ifndef CHECK_ALL_MOVES
                                break;
#endif
                            }
                        }
#endif
                        
#ifdef SEARCH_ROOT_L2
                        if(Settings::L2SearchOnRoot){
                            if(tfield.getNAlivePlayers() == 2){
                                // 残り2人の場合はL2判定
                                L2Judge lj(400000, searchBuffer);
                                int l2Result = (bd.isNF() && mi->isPASS()) ? L2_LOSE : lj.start_check(*mi, myHand, opsHand, bd, fieldInfo);
                                //cerr << l2Result << endl;
                                if(l2Result == L2_WIN){ // 勝ち
                                    DERR << "l2win!" << endl;
                                    mi->setL2Mate(); fieldInfo.setL2Mate();
                                    DERR << fieldInfo << endl;
#ifndef CHECK_ALL_MOVES
                                    break;
#endif
                                }else if(l2Result == L2_LOSE){
                                    mi->setL2GiveUp();
                                }
                            }
                        }
#endif
						
						// 最小分割数の減少量
						mi->setIncMinNMelds(max(0, calcMinNMelds(searchBuffer, nextCards) - curMinNMelds + 1));
					}
					
					
					// 結果を報告
#ifdef SEARCH_ROOT_L2
                    if(Settings::L2SearchOnRoot){
                        if(tfield.getNAlivePlayers() == 2){
                            // L2の際、結果不明を考えなければ、MATEが立っていれば勝ち、立っていなければ負けのはず
                            // ただしL2探索を行う場合のみ
                            CERR << fieldInfo << endl;
                            if(fieldInfo.isL2Mate()){
                                field.setMyL2Mate();
                            }else{
                                fieldInfo.setL2GiveUp();
                                field.setMyL2GiveUp();
                            }
                        }
                    }
#endif
#ifdef SEARCH_ROOT_MATE
                    if(Settings::MateSearchOnRoot){
                        if(fieldInfo.isMate()){
                            field.setMyMate();
                        }
                    }
#endif
					
					// 着手候補一覧表示
					CERR << "My Cards : " << OutCards(myCards) << endl << fieldInfo << endl;
					for(int m = 0; m < NMoves; ++m){
						Move move = mv[m].mv();
						MoveAddInfo info = MoveAddInfo(mv[m]);
						CERR << move << info << endl;
					}
					//getchar();
					
					// 必勝がある場合は必勝着手から1つ選ぶ
					if(fieldInfo.isMate()){
                        Move mateMove = Heuristics::chooseMateMove(&ps, NMoves, field, fieldInfo, &dice);
						playMove = mateMove;
					}
					
#ifndef POLICY_ONLY
                    // モンテカルロ着手決定
					if(playMove == MOVE_NONE && !fieldInfo.isGiveUp() && tfield.getNAlivePlayers() > 2){
						
						if(rp_mc == 0){
							// 最初の場合は世界プールを整理する
							for(int th = 0; th < N_THREADS; ++th){
								threadTools[th].gal.clear();
							}
						}
						
						MonteCarloPlayer mcp;
						Move MCMove = mcp.play(ps, NMoves, field, fieldInfo, &shared, threadTools);
						
						playMove = MCMove;
						
						++rp_mc;
					}
#endif
                    // 方策関数による着手決定
                    if(playMove == MOVE_NONE){
                        int idx = 0;
#if defined(POLICY_ONLY) && defined(RL_POLICY)
                        idx = playWithPolicy<1>(buf, NMoves, tfield, shared.basePlayPolicy, &dice); // stock mode
                        shared.playLearner.feedChosenActionIndexToLatestStock(idx, 0); // feed chosen action
#else
                        if(Settings::temperaturePlay <= 0){
                            idx = playWithBestPolicy<0>(buf, NMoves, tfield, shared.basePlayPolicy, &dice);
                        }else{
                            //double entropy = 0;
                            
                            //idx = playWithPolicy<0>(buf, NMoves, field, shared.basePlayPolicy, &dice, &entropy);
                            //field.playEntropySum += entropy;
                            field.fuzzyPlayTimes += 1;
                            
                            double score[N_MAX_MOVES];
                            calcPlayPolicyScoreSlow<0>(score, buf, NMoves, tfield, shared.basePlayPolicy);
                            BiasedSoftmaxSelector selector(score, NMoves,
                                                           Settings::simulationTemperaturePlay,
                                                           Settings::simulationAmplifyCoef,
                                                           Settings::simulationAmplifyExponent);
                            idx = selector.run_all(dice.drand());
                        }
#endif
                        //cerr << idx << endl;
                        playMove = buf[idx].mv();
                    }
                    
					// ランダム着手決定(ここまで来ないはずだが)
					if(playMove == MOVE_NONE){
						playMove = mv[dice.rand() % NMoves].mv();
					}
					
#ifdef MONITOR
                    cerr << "\033[1m";
                    cerr << "\033[" << 31 << "m";
					cerr << "Best Move : " << playMove << endl;
                    cerr << "\033[" << 39 << "m";
                    cerr << "\033[0m";
#endif
					
					decided_move[rp] = playMove;
					
					// この着手が確実に場を支配するかを確かめ、
					// 他支配の場合は勝手に局面を進めて次の手まで決める予定...
					
				} // 反復着手決定ループここまで
				
				// 決定した後の着手をスタックに積む
				/*for(int i = rp; i > 0; --i){
				 field.nextMoves.push(decided_move[i]);
				 }*/
				
                /*if(bd.isNF() && decided_move[0].isPASS()){
                    field.broadcastBP();
                    cerr << shared.gameLog.toString(field.getGameNum()) << endl;
                    getchar();
                }*/
				return decided_move[0];
			}
			void afterMyPlay(){
#ifndef POLICY_ONLY
				field.procWorldPatterns(field.lastMove.qty());
#endif
			}
			void afterOthersPlay(){
#ifndef POLICY_ONLY
				// 世界推定力をたしかめる
                field.procWorldPatterns(field.lastMove.qty());
				if(!field.lastMove.isPASS() && field.lastWorldPatterns > 1.0){
					shared.ga.proceed(field.lastTurnPlayer, field.lastMove, field.getWPCmp());
				}
#endif
			}
			void waitBeforeWon(){
				
			}
			void waitAfterWon(){
				
			}
			void tellOpponentsCards(){
				// 相手のカードをサーバーからこっそり教えてもらう
				// 旧版にはあったのだが、新版では未実装
				// 先行研究の通り、全部分かった方が強くなるようだ
			}
			void closeGame(){
#ifndef POLICY_ONLY
                // 自分の主観的プレー時間から、
                // マシン間の計算速度の比を概算
                shared.timeAnalyzer.modifyTimeRate();
                // ログから行動モデル解析
				analyzePlayerModel(field, &shared, &threadTools[0]);
#endif
                
                if(field.getGameNum() % 100 == 0){
#ifdef MONITER
#ifndef POLICY_ONLY
#ifdef MODELING_PLAY
                    for(int p = 0; p < N_PLAYERS; ++p){
                        // バイアス項を表示
                        cerr << shared.playerModelSpace.model(p).toBiasString() << endl;
                    }
#endif
#endif
#endif
                    
#ifdef RL_POLICY
                    // 強化学習で得たパラメータをファイルに書き出し
                    std::ostringstream oss;
                    oss << DIRECTORY_PARAMS_OUT << "play_policy_comment_rl.txt";
                    foutComment(shared.basePlayPolicy, oss.str());
                    shared.basePlayPolicy.fout(DIRECTORY_PARAMS_OUT + "play_policy_rl.dat");
#endif
                }
                shared.closeGame(field);
			}
			void closeMatch(){
                shared.closeMatch();
                field.closeMatch();
                for(int th = 0; th < N_THREADS; ++th){
                    threadTools[th].close();
                }
			}
			~Client(){
				closeMatch();
			}
		};
	}
}

#endif // UECDA_FUJIGOKORO_HPP_
