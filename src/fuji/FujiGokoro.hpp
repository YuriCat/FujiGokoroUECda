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
#include "montecarlo/monteCarlo.hpp"

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
				
                RootInfo root;
				Cards changeCards = CARDS_NULL;
				const Cards myCards = field.getMyDealtCards();
				
#ifdef MONITOR
				cerr << "My Cards : " << OutCards(myCards) << endl;
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
                
                // ルートノード設定
                int limitSimulations = std::min(10000, (int)(pow((double)NCands, 0.8) * 700));
                root.setChange(cand, NCands, field, shared, limitSimulations);

                // 方策関数による評価
                double escore[N_MAX_CHANGES + 1], score[N_MAX_CHANGES];
                calcChangePolicyScoreSlow(escore, cand, NCands, myCards, change_qty, field,
                                          shared.baseChangePolicy);
                // 手札推定時の方策高速計算の都合により指数を取った数値が返るので、元に戻す
                for(int m = 0; m < NCands; ++m)
                    score[m] = log(max(escore[m + 1] - escore[m], 0.000001)) * Settings::temperatureChange;
                root.feedPolicyScore(score, NCands);
                
#ifndef POLICY_ONLY
                // モンテカルロ法による評価
				if(changeCards == CARDS_NULL){
					// 世界プールを整理する
					for(int th = 0; th < N_THREADS; ++th)threadTools[th].gal.clear();
                    shared.exp_wr = 0; // 最高報酬0
#ifdef USE_POLICY_TO_ROOT
                    root.addPolicyScoreToMonteCarloScore();
#endif
                    // モンテカルロ開始
                    if(Settings::NChangeThreads > 1){
                        std::vector<std::thread> thr;
                        for(int ith = 0; ith < Settings::NChangeThreads; ++ith)
                            thr.emplace_back(std::thread(&MonteCarloThread<FujiField, RootInfo, SharedData, ThreadTools>,
                                                         ith, &root, &field, &shared, &threadTools[ith]));
                        for(auto& th : thr)th.join();
                    }else{
                        MonteCarloThread<FujiField, RootInfo, SharedData, ThreadTools>(0, &root, &field, &shared, &threadTools[0]);
                    }
				}
#endif // POLICY_ONLY
                root.sort();
                field.last_exp_reward = root.getExpReward(0);
                
#ifdef POLICY_ONLY
                if(changeCards ==CARDS_NULL && Settings::temperatureChange > 0){
                    // 確率的に選ぶ場合
                    BiasedSoftmaxSelector selector(score, root.candidates,
                                                   Settings::simulationTemperatureChange,
                                                   Settings::simulationAmplifyCoef,
                                                   Settings::simulationAmplifyExponent);
                    // rootは着手をソートしているので元の着手生成バッファから選ぶ
                    changeCards = cand[selector.run_all(dice.drand())];
                }
#endif
                if(changeCards == CARDS_NULL){
                    // 最高評価の交換を選ぶ
                    changeCards = root.child[0].changeCards;
                }
                
            DECIDED_CHANGE:
                assert(countCards(changeCards) == change_qty);
                assert(holdsCards(myCards, changeCards));
#ifdef MONITOR
                cerr << root.toString();
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
                
                RootInfo root;
                
				// 先読み用バッファはスレッド0のものを使う
				MoveInfo *const searchBuffer = threadTools[0].buf;
				
				// ルート合法手生成バッファ
				MoveInfo mv[N_MAX_MOVES + 256];
				
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
					int NMoves = genMove(mv, myCards, bd);
					if(NMoves <= 0){ // 合法着手なし
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
						if(!mv[0].isPASS())field.setMyMate(); // 上がり
						return (Move)mv[0];
					}

					// 合法着手生成(特別着手)
                    int NSpecialMoves = 0;
                    if(bd.isNF())
                        NSpecialMoves += genNullPass(mv + NMoves + NSpecialMoves);
                    if(containsJOKER(myCards) && containsS3(opsCards)){
                        if(bd.isGroup())
                            NSpecialMoves += genJokerGroup(mv + NMoves + NSpecialMoves, myCards, opsCards, bd);
					}
                    NMoves += NSpecialMoves;
                    
                    // 着手多様性確保のため着手をランダムシャッフル
                    for(int i = NMoves; i > 1; --i)
                        std::swap(mv[dice.rand() % i], mv[i - 1]);
					
					// 場の情報をまとめる
                    assert(tfield.getTurnPlayer() == myPlayerNum);
                    tfield.prepareForPlay(true);
					
					// 着手追加情報を設定
					// 自分の着手の事なので詳しく調べる
					
					int curMinNMelds = calcMinNMelds(searchBuffer, myCards);
					
					setDomState(mv, NMoves, tfield); // 先の場も含めて支配状況をまとめて設定
					
                    for(int m = 0; m < NMoves; ++m){
						MoveInfo *const mi = &mv[m];
						const Move move = mi->mv();
						
						Cards nextCards = maskCards(myCards, move.cards());
						// 後場情報
						if(bd.afterPrmOrder(move) != ORDER_NORMAL){ mi->setPrmOrderRev(); }
						if(bd.afterTmpOrder(move) != ORDER_NORMAL){ mi->setTmpOrderRev(); }
						if(bd.afterSuitsLocked(move)){ mi->setSuitLock(); }
						
                        if(Settings::MateSearchOnRoot){ // 多人数必勝判定
                            if(checkHandMate(1, searchBuffer, *mi, myHand, opsHand, bd, fieldInfo)){
                                mi->setMPMate(); fieldInfo.setMPMate();
                            }
                        }
                        if(Settings::L2SearchOnRoot){
                            if(tfield.getNAlivePlayers() == 2){ // 残り2人の場合はL2判定
                                L2Judge lj(400000, searchBuffer);
                                int l2Result = (bd.isNF() && mi->isPASS()) ? L2_LOSE : lj.start_check(*mi, myHand, opsHand, bd, fieldInfo);
                                //cerr << l2Result << endl;
                                if(l2Result == L2_WIN){ // 勝ち
                                    DERR << "l2win!" << endl;
                                    mi->setL2Mate(); fieldInfo.setL2Mate();
                                    DERR << fieldInfo << endl;
                                }else if(l2Result == L2_LOSE){
                                    mi->setL2GiveUp();
                                }
                            }
                        }
						// 最小分割数の減少量
						mi->setIncMinNMelds(max(0, calcMinNMelds(searchBuffer, nextCards) - curMinNMelds + 1));
					}
					
					// 判定結果を報告
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
                    if(Settings::MateSearchOnRoot){
                        if(fieldInfo.isMPMate())field.setMyMate();
                    }
					
#ifdef MONITOR
					// 着手候補一覧表示
                    cerr << "My Cards : " << OutCards(myCards) << endl;
                    cerr << bd << " " << fieldInfo << endl;
					for(int m = 0; m < NMoves; ++m){
						Move move = mv[m].mv();
						MoveAddInfo info = MoveAddInfo(mv[m]);
						cerr << move << info << endl;
					}
#endif
					//getchar();
                    
                    // ルートノード設定
                    int limitSimulations = std::min(5000, (int)(pow((double)NMoves, 0.8) * 700));
                    root.setPlay(mv, NMoves, field, shared, limitSimulations);
                    
                    // 方策関数による評価(必勝のときも行う, 除外された着手も考慮に入れる)
                    double score[N_MAX_MOVES + 256];
#ifdef RL_POLICY
                    // 強化学習する場合には学習器を裏で動かす
                    calcPlayPolicyScoreSlow<1>(score, mv, NMoves, tfield, shared.basePlayPolicy);
#else
                    calcPlayPolicyScoreSlow<0>(score, mv, NMoves, tfield, shared.basePlayPolicy);
#endif
                    root.feedPolicyScore(score, NMoves);
                    
#ifndef POLICY_ONLY
                    // モンテカルロ法による評価(結果確定のとき以外)
					if(!fieldInfo.isMate() && !fieldInfo.isGiveUp()){
						if(rp_mc == 0) // 最初の場合は世界プールを整理する
							for(int th = 0; th < N_THREADS; ++th)threadTools[th].gal.clear();
                        shared.exp_wr = 0; // 最高報酬0
#ifdef USE_POLICY_TO_ROOT
                        root.addPolicyScoreToMonteCarloScore();
#endif
                        // モンテカルロ開始
                        if(Settings::NPlayThreads > 1){
                            std::vector<std::thread> thr;
                            for(int ith = 0; ith < Settings::NPlayThreads; ++ith)
                                thr.emplace_back(std::thread(&MonteCarloThread<FujiField, RootInfo, SharedData, ThreadTools>,
                                                             ith, &root, &field, &shared, &threadTools[ith]));
                            for(auto& th : thr)th.join();
                        }else{
                            MonteCarloThread<FujiField, RootInfo, SharedData, ThreadTools>(0, &root, &field, &shared, &threadTools[0]);
                        }
                        rp_mc++;
					}
#endif
                    // 着手決定のための情報が揃ったので着手を決定する
                    
                    // 方策とモンテカルロの評価
                    root.sort();
                    field.last_exp_reward = root.getExpReward(0);

                    // 以下必勝を判定したときのみ
                    if(fieldInfo.isMate()){
                        int next = root.candidates;
                        // 1. 必勝判定で勝ちのものに限定
                        next = root.binary_sort(next, [](const RootAction& a){ return a.move.isMate(); });
                        assert(next > 0);
                        // 2. 即上がり
                        next = root.binary_sort(next, [](const RootAction& a){ return a.move.isFinal(); });
                        // 3. 完全勝利
                        next = root.binary_sort(next, [](const RootAction& a){ return a.move.isPW(); });
                        // 4. 多人数判定による勝ち
                        next = root.binary_sort(next, [](const RootAction& a){ return a.move.isMPMate(); });
#ifdef DEFEAT_RIVAL_MATE
                        if(next > 1 && !isNoRev(myCards)){
                            // 5. ライバルに不利になるように革命を起こすかどうか
                            int prefRev = Heuristics::preferRev(field);
                            if(prefRev > 0) // 革命優先
                                next = root.sort(next, [myCards](const RootAction& a){
                                    return a.move.flipsPrmOrder() ? 2 :
                                    (isNoRev(maskCards(myCards, a.move.cards())) ? 0 : 1);
                                });
                            if(prefRev < 0) // 革命しないことを優先
                                next = root.sort(next, [myCards](const RootAction& a){
                                    return a.move.flipsPrmOrder() ? -2 :
                                    (isNoRev(maskCards(myCards, a.move.cards())) ? 0 : -1);
                                });
                        }
#endif
                        // 必勝時の出し方の見た目をよくする
                        if(fieldInfo.isUnrivaled()){
                            // 6. 独断場のとき最小分割数が減るのであればパスでないものを優先
                            //    そうでなければパスを優先
                            next = root.sort(next, [](const RootAction& a){ return -(int)a.move.getIncMinNMelds(); });
                            if(root.child[0].move.getIncMinNMelds() > 0)
                                next = root.binary_sort(next, [](const RootAction& a){ return a.move.isPASS(); });
                        }
                        // 7. 枚数が大きいものを優先
                        next = root.sort(next, [](const RootAction& a){ return a.move.qty(); });
                        // 8. 即切り役を優先
                        next = root.binary_sort(next, [](const RootAction& a){ return a.move.domInevitably(); });
                        // 9. 自分を支配していないものを優先
                        next = root.binary_sort(next, [](const RootAction& a){ return !a.move.isDM(); });
                        
                        playMove = root.child[0].move.mv(); // 必勝手から選ぶ
                    }

#ifdef POLICY_ONLY
                    if(playMove == MOVE_NONE && Settings::temperaturePlay > 0){
                        // 確率的に選ぶ場合
                        BiasedSoftmaxSelector selector(score, root.candidates,
                                                       Settings::simulationTemperaturePlay,
                                                       Settings::simulationAmplifyCoef,
                                                       Settings::simulationAmplifyExponent);
                        // rootは着手をソートしているので元の着手生成バッファから選ぶ
                        playMove = mv[selector.run_all(dice.drand())].mv();
                    }
#endif
                    if(playMove == MOVE_NONE){
                        // 最高評価の着手を選ぶ
                        playMove = root.child[0].move.mv();
                    }
#ifdef MONITOR
                    cerr << root.toString();
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
