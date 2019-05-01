#pragma once

// 思考部メイン
// 実際のゲームでの局面更新など低次な処理はmain関数にて

#define _ENGINE_FUJI_

#include <thread>

// 思考部ヘッダ
#include "engineSettings.h"
#include "engineStructure.hpp"

// クライアント部分空間

// 報酬系トップダウンパラメータ
#include "value.hpp"

// 諸々の計算とか判定
#include "../core/dominance.hpp" // 支配判定
#include "mate.hpp" // 必勝判定
#include "lastTwo.hpp"
#include "heuristics.hpp"
#include "linearPolicy.hpp"

#ifndef POLICY_ONLY
#include "galaxy.hpp"
#include "monteCarlo.hpp"
#endif // ！POLICY_ONLY

namespace UECda {
    class WisteriaEngine {
    private:
        EngineThreadTools threadTools[N_THREADS];
        
        // 0番スレッドのサイコロをメインサイコロとして使う
        decltype(threadTools[0].dice)& dice = threadTools[0].dice;
        // 先読み用バッファはスレッド0のもの
        MoveInfo *const searchBuffer = threadTools[0].buf;
        
    public:
        EngineSharedData shared;
        decltype(shared.record)& record = shared.record;
        bool monitor = false;
        
        void setRandomSeed(uint64_t s) {
            // 乱数系列を初期化
            for (int th = 0; th < N_THREADS; th++) {
                threadTools[th].dice.srand(s + th);
            }
        }
        
        void initMatch() {
            // record にプレーヤー番号が入っている状態で呼ばれる
            shared.setMyPlayerNum(record.myPlayerNum);
            // サイコロ初期化
            // シード指定の場合はこの後に再設定される
            setRandomSeed((uint32_t)time(NULL));
            // スレッドごとのデータ初期化
            for (int th = 0; th < N_THREADS; th++) {
                threadTools[th].init(th);
            }
                
#ifndef POLICY_ONLY
            // 世界プール監視員を設定
            for (int th = 0; th < N_THREADS; th++) {
                shared.ga.set(th, &threadTools[th].gal);
            }
#endif
            
            auto& playPolicy = shared.basePlayPolicy;
            auto& changePolicy = shared.baseChangePolicy;
            
            shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");
            shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");

            // 方策の温度
            shared.basePlayPolicy.setTemperature(Settings::simulationTemperaturePlay);
            shared.baseChangePolicy.setTemperature(Settings::simulationTemperatureChange);
        }
        void initGame() {
            // 汎用変数の設定
            shared.initGame();
#ifndef POLICY_ONLY
            // 報酬設定
            // ランク初期化まで残り何試合か
            // 公式には未定だが、多くの場合100試合だろうから100試合としておく
            const int gamesForCIG = getNGamesForClassInitGame(record.getLatestGameNum());
            for (int cl = 0; cl < N_PLAYERS; cl++) {
                shared.gameReward[cl] = int(standardReward(gamesForCIG, cl) * 100);
            }
            
            // 置換表初期化
#ifdef USE_L2BOOK
            L2::book.init();
#endif
#ifdef USE_LNCIBOOK
            LnCI::book.init();
#endif
#endif // !POLICY_ONLY
            
        }
        
        Cards change(unsigned changeQty) { // 交換関数
            const auto& gameLog = record.latestGame();
            const int myPlayerNum = record.myPlayerNum;
            
            assert(changeQty <= 2);
            
            RootInfo root;
            Cards changeCards = CARDS_NULL;
            const Cards myCards = gameLog.getDealtCards(myPlayerNum);
            if (monitor) cerr << "My Cards : " << myCards << endl;
            
#ifdef RARE_PLAY
            // レアプレーを行っていない場合は行う
            if (!shared.rare_play_flag.test(8)
                && containsJOKER(myCards)
                && change_qty == 1) {
                // ジョーカー捨て
                // 2枚交換のとき1枚のみ確定するのは面倒なので、1枚交換の時のみにする
                shared.rare_play_flag.set(8);
                return CARDS_JOKER;
            }
#endif
            // 手札レベルで枝刈りする場合
            Cards tmp = myCards;
#ifdef PRUNE_ROOT_CHANGE_BY_HEURISTICS
            tmp = Heuristics::pruneCards(myCards, changeQty);    
#endif
            
            if (countCards(tmp) == changeQty) return tmp;

            // 合法交換候補生成
            Cards cand[N_MAX_CHANGES];
            int NCands = genChange(cand, tmp, changeQty);
            for (int i = 0; i < NCands; i++) {
                assert(cand[i].count() == changeQty && myCards.holds(cand[i]));
            }
            if (NCands == 1) return cand[0];
            
            // 必勝チェック&枝刈り
            for (int i = 0; i < NCands; i++) {
                // 交換後の自分のカード
                Cards restCards = subtrCards(myCards, cand[i]);

                // D3を持っている場合、自分からなので必勝チェック
                if (containsD3(restCards)) {
                    const Board b = OrderToNullBoard(0); // 通常オーダーの空場
                    FieldAddInfo fieldInfo;
                    fieldInfo.init();
                    fieldInfo.setFlushLead();
                    fieldInfo.setMinNCardsAwake(10);
                    fieldInfo.setMinNCards(10);
                    fieldInfo.setMaxNCardsAwake(11);
                    fieldInfo.setMaxNCards(11);
                    Hand mine, ops;
                    mine.set(restCards);
                    ops.set(subtrCards(CARDS_ALL, restCards));
                    if (judgeHandMate(1, searchBuffer, mine, ops, b, fieldInfo)) {
                        CERR << "CHANGE MATE!" << endl;
                        return cand[i]; // 必勝
                    }
                }
            }
#ifdef PRUNE_ROOT_CHANGE_BY_HEURISTICS
            for (int i = 0; i < NCands; i++) {
                Cards restCards = subtrCards(myCards, cand[i]);
                // 残り札に革命が無い場合,Aもダメ
                if (isNoRev(restCards)) {
                    if (cand[i] & CARDS_A) std::swap(cand[i], cand[--NCands]);
                }
                if (NCands == 1) return cand[0];
            }
#endif
            // ルートノード設定
            Field field;
            setFieldFromClientLog(gameLog, myPlayerNum, &field);
            int limitSimulations = std::min(10000, (int)(pow((double)NCands, 0.8) * 700));
            root.setChange(cand, NCands, field, shared, limitSimulations);
            
            // 方策関数による評価
            double escore[N_MAX_CHANGES + 1], score[N_MAX_CHANGES];
            calcChangePolicyScoreSlow(escore, cand, NCands, myCards, changeQty, field,
                                      shared.baseChangePolicy);
            // 手札推定時の方策高速計算の都合により指数を取った数値が返るので、元に戻す
            for (int i = 0; i < NCands; i++) {
                score[i] = log(max(escore[i + 1] - escore[i], 0.000001)) * Settings::temperatureChange;
            }
            root.feedPolicyScore(score, NCands);
            
#ifndef POLICY_ONLY
            // モンテカルロ法による評価
            if (changeCards == CARDS_NULL) {
                // 世界プールを整理する
                for (int th = 0; th < N_THREADS; th++) {
                    threadTools[th].gal.clear();
                }
#ifdef USE_POLICY_TO_ROOT
                root.addPolicyScoreToMonteCarloScore();
#endif
                // モンテカルロ開始
                if (Settings::NChangeThreads > 1) {
                    std::vector<std::thread> thr;
                    for (int ith = 0; ith < Settings::NChangeThreads; ith++) {
                        thr.emplace_back(std::thread(&MonteCarloThread, ith, &root, &field, &shared, &threadTools[ith]));
                    }
                    for (auto& th : thr) th.join();
                } else {
                    MonteCarloThread(0, &root, &field, &shared, &threadTools[0]);
                }
            }
#endif // POLICY_ONLY
            root.sort();
            
#ifdef POLICY_ONLY
            if (changeCards == CARDS_NULL && Settings::temperatureChange > 0) {
                // 確率的に選ぶ場合
                BiasedSoftmaxSelector<double> selector(score, root.candidates,
                                                       Settings::simulationTemperatureChange,
                                                       Settings::simulationAmplifyCoef,
                                                       Settings::simulationAmplifyExponent);
                // rootは着手をソートしているので元の着手生成バッファから選ぶ
                changeCards = cand[selector.select(dice.drand())];
            }
#endif
            if (changeCards == CARDS_NULL) {
                // 最高評価の交換を選ぶ
                changeCards = root.child[0].changeCards;
            }
            
        DECIDED_CHANGE:
            assert(countCards(changeCards) == changeQty);
            assert(holdsCards(myCards, changeCards));
            if (monitor) {
                cerr << root.toString();
                cerr << "\033[1m";
                cerr << "\033[" << 34 << "m";
                cerr << "Best Change : " << changeCards << endl;
                cerr << "\033[" << 39 << "m";
                cerr << "\033[0m";
            }
            return changeCards;
        }
        
        void afterChange() {}
        void waitChange() {}
        void prepareForGame() {}
        
        Move play() {
            // 自分のプレーについての変数を更新
            ClockMicS clms;
            clms.start();
            Move ret = playSub();
            return ret;
        }
        Move playSub() { // ここがプレー関数
            const auto& gameLog = shared.record.latestGame();
            
            Move playMove = MOVE_NONE;
            
            RootInfo root;
            
            // ルート合法手生成バッファ
            MoveInfo mv[N_MAX_MOVES + 256];
            
            const int myPlayerNum = record.myPlayerNum;
            
            Field field;
            setFieldFromClientLog(gameLog, myPlayerNum, &field);
            DERR << "tp = " << gameLog.current.turn() << endl;
            ASSERT_EQ(field.turn(), myPlayerNum);
            
            const Hand& myHand = field.getHand(myPlayerNum);
            const Hand& opsHand = field.getOpsHand(myPlayerNum);
            const Cards myCards = myHand.cards;
            const Cards opsCards = opsHand.cards;
            const Board b = field.board;
            CERR << b << endl;
            FieldAddInfo& fieldInfo = field.fieldInfo;
            
            // サーバーの試合進行バグにより無条件支配役が流れずに残っている場合はリジェクトにならないようにパスしておく
            if (b.isInvalid()) return MOVE_PASS;

#ifdef RARE_PLAY
            // レアプレーを行っていない場合は行う
            if (b.isNull() && !rare_play_flag.test(0)) { // 空場パス
                rare_play_flag.set(0);
                return MOVE_PASS;
            }
#endif
                
            // 合法着手生成
            int NMoves = genMove(mv, myCards, b);
            if (NMoves <= 0) { // 合法着手なし
                cerr << "No valid move." << endl;
                return MOVE_PASS;
            }
            if (NMoves == 1) {
                // 合法着手1つ。パスのみか、自分スタートで手札1枚
                // 本当はそういう場合にすぐ帰ると手札がばれるのでよくない
                if (monitor) {
                    if (!mv[0].isPASS()) cerr << "final move. " << mv[0] << endl;
                    else cerr << "no chance. PASS" << endl;
                }
                if (!mv[0].isPASS())shared.setMyMate(field.getBestClass()); // 上がり
                return mv[0];
            }
            
            // 合法着手生成(特別着手)
            int NSpecialMoves = 0;
            if (b.isNull()) {
                (mv + NMoves + NSpecialMoves)->setPASS();
                NSpecialMoves++;
            }
            if (containsJOKER(myCards) && containsS3(opsCards)) {
                if (b.isGroup() && b.qty() > 1) {
                    NSpecialMoves += genJokerGroup(mv + NMoves + NSpecialMoves, myCards, opsCards, b);
                }
            }
            NMoves += NSpecialMoves;
            
            // 着手多様性確保のため着手をランダムシャッフル
            for (int i = NMoves; i > 1; --i) {
                std::swap(mv[dice.rand() % i], mv[i - 1]);
            }
            
            // 場の情報をまとめる
            field.prepareForPlay(true);
            
            // 着手追加情報を設定
            for (int i = 0; i < NMoves; i++) {
                MoveInfo& move = mv[i];
                // 支配性
                if (move.qty() > fieldInfo.getMaxNCardsAwake()
                    || dominatesCards(move, opsCards, b)) {
                    move.setDO(); // 他支配
                }
                if (move.isPASS() || move.qty() > myCards.count() - move.qty()
                    || dominatesCards(move, myCards - move.cards(), b)) {
                    move.setDM(); // 自己支配
                }
                
                if (Settings::MateSearchOnRoot) { // 多人数必勝判定
                    if (checkHandMate(1, searchBuffer, move, myHand, opsHand, b, fieldInfo)) {
                        move.setMPMate(); fieldInfo.setMPMate();
                    }
                }
                if (Settings::L2SearchOnRoot) {
                    if (field.getNAlivePlayers() == 2) { // 残り2人の場合はL2判定
                        L2Judge lj(400000, searchBuffer);
                        int l2Result = (b.isNull() && move.isPASS()) ? L2_LOSE : lj.start_check(move, myHand, opsHand, b, fieldInfo);
                        if (l2Result == L2_WIN) { // 勝ち
                            DERR << "l2win!" << endl;
                            move.setL2Mate(); fieldInfo.setL2Mate();
                            DERR << fieldInfo << endl;
                        } else if (l2Result == L2_LOSE) {
                            move.setL2GiveUp();
                        }
                    }
                }
            }
            
            // 判定結果を報告
            if (Settings::L2SearchOnRoot) {
                if (field.getNAlivePlayers() == 2) {
                    // L2の際、結果不明を考えなければ、MATEが立っていれば勝ち、立っていなければ負けのはず
                    // ただしL2探索を行う場合のみ
                    CERR << fieldInfo << endl;
                    if (fieldInfo.isL2Mate()) {
                        shared.setMyL2Mate();
                    } else {
                        fieldInfo.setL2GiveUp();
                        shared.setMyL2GiveUp();
                    }
                }
            }
            if (Settings::MateSearchOnRoot) {
                if (fieldInfo.isMPMate()) shared.setMyMate(field.getBestClass());
            }

            if (monitor) {
                // 着手候補一覧表示
                cerr << "My Cards : " << myCards << endl;
                cerr << b << " " << fieldInfo << endl;
                for (int i = 0; i < NMoves; i++) {
                    cerr << mv[i] << toInfoString(mv[i], b) << endl;
                }
            }
            
            // ルートノード設定
            int limitSimulations = std::min(5000, (int)(pow((double)NMoves, 0.8) * 700));
            root.setPlay(mv, NMoves, field, shared, limitSimulations);
            
            // 方策関数による評価(必勝のときも行う, 除外された着手も考慮に入れる)
            double score[N_MAX_MOVES + 256];
            calcPlayPolicyScoreSlow<0>(score, mv, NMoves, field, shared.basePlayPolicy);
            root.feedPolicyScore(score, NMoves);
            
#ifndef POLICY_ONLY
            // モンテカルロ法による評価(結果確定のとき以外)
            if (!fieldInfo.isMate() && !fieldInfo.isGiveUp()) {
                for (int th = 0; th < N_THREADS; th++) {
                    threadTools[th].gal.clear();
                }
#ifdef USE_POLICY_TO_ROOT
                root.addPolicyScoreToMonteCarloScore();
#endif
                // モンテカルロ開始
                if (Settings::NPlayThreads > 1) {
                    std::vector<std::thread> thr;
                    for (int ith = 0; ith < Settings::NPlayThreads; ith++) {
                        thr.emplace_back(std::thread(&MonteCarloThread, ith, &root, &field, &shared, &threadTools[ith]));
                    }
                    for (auto& th : thr) th.join();
                } else {
                    MonteCarloThread(0, &root, &field, &shared, &threadTools[0]);
                }
            }
#endif
            // 着手決定のための情報が揃ったので着手を決定する
            
            // 方策とモンテカルロの評価
            root.sort();
            
            // 以下必勝を判定したときのみ
            if (fieldInfo.isMate()) {
                int next = root.candidates;
                // 1. 必勝判定で勝ちのものに限定
                next = root.binary_sort(next, [](const RootAction& a) { return a.move.isMate(); });
                assert(next > 0);
                // 2. 即上がり
                next = root.binary_sort(next, [](const RootAction& a) { return a.move.isFinal(); });
                // 3. 完全勝利
                next = root.binary_sort(next, [](const RootAction& a) { return a.move.isPW(); });
                // 4. 多人数判定による勝ち
                next = root.binary_sort(next, [](const RootAction& a) { return a.move.isMPMate(); });
#ifdef DEFEAT_RIVAL_MATE
                if (next > 1 && !isNoRev(myCards)) {
                    // 5. ライバルに不利になるように革命を起こすかどうか
                    int prefRev = Heuristics::preferRev(field, myPlayerNum, field.getRivalPlayersFlag(myPlayerNum));
                    if (prefRev > 0) // 革命優先
                        next = root.sort(next, [myCards](const RootAction& a) {
                            return a.move.isRev() ? 2 :
                            (isNoRev(maskCards(myCards, a.move.cards())) ? 0 : 1);
                        });
                    if (prefRev < 0) // 革命しないことを優先
                        next = root.sort(next, [myCards](const RootAction& a) {
                            return a.move.isRev() ? -2 :
                            (isNoRev(maskCards(myCards, a.move.cards())) ? 0 : -1);
                        });
                }
#endif
                // 必勝時の出し方の見た目をよくする
                if (fieldInfo.isUnrivaled()) {
                    // 6. 独断場のとき最小分割数が減るのであればパスでないものを優先
                    //    そうでなければパスを優先
                    int division = computeDivision(searchBuffer, myCards);
                    for (int i = 0; i < NMoves; i++) {
                        Cards nextCards = myCards - root.child[i].move.cards();
                        root.child[i].nextDivision = computeDivision(searchBuffer, nextCards);
                    }
                    next = root.sort(next, [](const RootAction& a) { return a.nextDivision; });
                    if (root.child[0].nextDivision < division) {
                        next = root.binary_sort(next, [](const RootAction& a) { return a.move.isPASS(); });
                    }
                }
                // 7. 枚数が大きいものを優先
                next = root.sort(next, [](const RootAction& a) { return a.move.qty(); });
                // 8. 即切り役を優先
                next = root.binary_sort(next, [](const RootAction& a) { return a.move.domInevitably(); });
                // 9. 自分を支配していないものを優先
                next = root.binary_sort(next, [](const RootAction& a) { return !a.move.isDM(); });
                
                playMove = root.child[0].move; // 必勝手から選ぶ
            }
            
#ifdef POLICY_ONLY
            if (playMove == MOVE_NONE && Settings::temperaturePlay > 0) {
                // 確率的に選ぶ場合
                BiasedSoftmaxSelector<double> selector(score, root.candidates,
                                                       Settings::simulationTemperaturePlay,
                                                       Settings::simulationAmplifyCoef,
                                                       Settings::simulationAmplifyExponent);
                // rootは着手をソートしているので元の着手生成バッファから選ぶ
                playMove = mv[selector.select(dice.drand())];
            }
#endif
            if (playMove == MOVE_NONE) {
                // 最高評価の着手を選ぶ
                playMove = root.child[0].move;
            }
            if (monitor) {
                cerr << root.toString();
                cerr << "\033[1m";
                cerr << "\033[" << 31 << "m";
                cerr << "Best Move : " << playMove << endl;
                cerr << "\033[" << 39 << "m";
                cerr << "\033[0m";
            }
            
            return playMove;
        }
        void closeGame() {
            shared.closeGame();
        }
        void closeMatch() {
            for (int th = 0; th < N_THREADS; th++) threadTools[th].close();
            shared.closeMatch();
        }
        ~WisteriaEngine() {
            closeMatch();
        }
    };
}