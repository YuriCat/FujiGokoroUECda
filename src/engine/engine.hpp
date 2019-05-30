#pragma once

// 思考部メイン
// 実際のゲームでの局面更新など低次な処理はmain関数にて

#include <thread>
#include "../settings.h"
#include "data.hpp"
#include "value.hpp"
#include "../core/dominance.hpp"
#include "mate.hpp"
#include "last2.hpp"
#include "heuristics.hpp"
#include "policy.hpp"
#include "simulation.hpp"
#include "monteCarlo.hpp"

namespace Settings {
    const bool changeHeuristicsOnRoot = true;
    const bool addPolicyOnRoot = true;
    const bool L2SearchOnRoot = true;
    const bool MateSearchOnRoot = true;
}

class WisteriaEngine {
private:
    ThreadTools threadTools[N_THREADS];
    
    // 0番スレッドのサイコロをメインサイコロとして使う
    decltype(threadTools[0].dice)& dice = threadTools[0].dice;
    // 先読み用バッファはスレッド0のもの
    Move *const searchBuffer = threadTools[0].mbuf;
    
public:
    SharedData shared;
    decltype(shared.record)& record = shared.record;
    bool monitor = false;
    
    void setRandomSeed(uint64_t s) {
        // 乱数系列を初期化
        for (int th = 0; th < N_THREADS; th++) {
            threadTools[th].dice.srand(s + th);
        }
    }
    
    void initMatch(int playerNum) {
        // サイコロ初期化
        // シード指定の場合はこの後に再設定される
        setRandomSeed((uint32_t)time(NULL));
        // スレッドごとのデータ初期化
        for (int th = 0; th < N_THREADS; th++) {
            threadTools[th].init(th);
        }
        shared.initMatch(playerNum);
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
        // 報酬設定
        // ランク初期化まで残り何試合か
        // 公式には未定だが、多くの場合100試合だろうから100試合としておく
        const int gamesForCIG = getNGamesForClassInitGame(record.getLatestGameNum());
        for (int cl = 0; cl < N_PLAYERS; cl++) {
            shared.gameReward[cl] = int(standardReward(gamesForCIG, cl) * 100);
        }
        L2::init();
    }
    void startMonteCarlo(RootInfo& root, const Field& field, int numThreads) {
        if (numThreads > 1) {
            std::vector<std::thread> threads;
            for (int i = 0; i < numThreads; i++) {
                threads.emplace_back(std::thread(&MonteCarloThread, i, numThreads, &root, &field, &shared, &threadTools[i]));
            }
            for (auto& t : threads) t.join();
        } else {
            MonteCarloThread(0, 1, &root, &field, &shared, &threadTools[0]);
        }
    }
    Cards change(unsigned changeQty) { // 交換関数
        const auto& game = record.latestGame();
        const int myPlayerNum = record.myPlayerNum;
        
        RootInfo root;
        Cards changeCards = CARDS_NULL;
        const Cards myCards = game.dealtCards[myPlayerNum];
        if (monitor) cerr << "My Cards : " << myCards << endl;

        // 手札レベルで枝刈りする場合
        Cards tmp = myCards;
        if (Settings::changeHeuristicsOnRoot) {
            tmp.mask(CARDS_8 | CARDS_2 | CARDS_JOKER | CARDS_D3);
            if (myCards.joker() > 0) tmp.mask(CARDS_S3);
            if (tmp.count() < changeQty) tmp = myCards; // キャンセル
        }
        
        if (tmp.count() == changeQty) return tmp;

        // 合法交換候補生成
        std::array<Cards, N_MAX_CHANGES> cand;
        int numCands = genChange(cand.data(), tmp, changeQty);
        if (numCands == 1) return cand[0];
        std::shuffle(cand.begin(), cand.begin() + numCands, dice);
        
        // 必勝チェック&枝刈り
        for (int i = 0; i < numCands; i++) {
            // 交換後の自分のカード
            Cards restCards = myCards - cand[i];

            // D3を持っている場合、自分からなので必勝チェック
            if (containsD3(restCards)) {
                Board b = OrderToNullBoard(0); // 通常オーダーの空場
                b.initInfo();
                b.setFlushLead();
                b.setMinNCardsAwake(10);
                b.setMinNCards(10);
                b.setMaxNCardsAwake(11);
                b.setMaxNCards(11);
                Hand mine, ops;
                mine.set(restCards);
                ops.set(CARDS_ALL - restCards);
                if (judgeHandMate(1, searchBuffer, mine, ops, b)) {
                    CERR << "CHANGE MATE!" << endl;
                    return cand[i]; // 必勝
                }
            }
        }

        if (Settings::changeHeuristicsOnRoot) {
            for (int i = 0; i < numCands; i++) {
                Cards restCards = myCards - cand[i];
                // 残り札に革命が無い場合,Aもダメ
                if (isNoRev(restCards)) {
                    if (cand[i] & CARDS_A) std::swap(cand[i], cand[--numCands]);
                }
                if (numCands == 1) return cand[0];
            }
        }

        // ルートノード設定
        Field field;
        field.fromRecord(game, myPlayerNum, -1);
        int limitSimulations = std::min(10000, (int)(pow((double)numCands, 0.8) * 700));
        root.setChange(cand.data(), numCands, field, shared, limitSimulations);
        
        // 方策関数による評価
        double score[N_MAX_CHANGES];
        changePolicyScore(score, cand.data(), numCands, myCards, changeQty, shared.baseChangePolicy, 0);
        root.feedPolicyScore(score, numCands);
        
        // モンテカルロ法による評価
        if (!Settings::policyMode && changeCards == CARDS_NULL) {
            if (Settings::addPolicyOnRoot) root.addPolicyScoreToMonteCarloScore();
            startMonteCarlo(root, field, Settings::numChangeThreads);
        }
        root.sort();

        // 最高評価の交換を選ぶ
        if (changeCards == CARDS_NULL) changeCards = root.child[0].changeCards;
        
    DECIDED_CHANGE:
        assert(countCards(changeCards) == changeQty);
        assert(holdsCards(myCards, changeCards));
        if (monitor) {
            cerr << root.toString();
            cerr << "\033[1m\033[" << 34 << "m";
            cerr << "Best Change : " << changeCards << endl;
            cerr << "\033[" << 39 << "m\033[0m";
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
        Move ret = playImpl();
        return ret;
    }
    Move playImpl() { // ここがプレー関数
        const auto& game = shared.record.latestGame();
        
        Move playMove = MOVE_NONE;
        RootInfo root;
        
        // ルート合法手生成バッファ
        std::array<Move, N_MAX_MOVES + 256> mbuf;
        
        const int myPlayerNum = record.myPlayerNum;
        
        Field field;
        field.fromRecord(game, myPlayerNum);
        if (monitor) cerr << field.toString();
        field.setMoveBuffer(mbuf.data());
        assert(field.turn() == myPlayerNum);
        
        const Hand& myHand = field.getHand(myPlayerNum);
        const Hand& opsHand = field.getOpsHand(myPlayerNum);
        const Cards myCards = myHand.cards;
        const Cards opsCards = opsHand.cards;
        Board& b = field.board;
        
        // サーバーの試合進行バグにより無条件支配役が流れずに残っている場合はリジェクトにならないようにパスしておく
        if (b.isInvalid()) return MOVE_PASS;
            
        // 合法着手生成
        int NMoves = genMove(mbuf.data(), myCards, b);
        if (NMoves <= 0) { // 合法着手なし
            cerr << "No valid move." << endl;
            return MOVE_PASS;
        }
        if (NMoves == 1) {
            // 合法着手1つ。パスのみか、自分スタートで手札1枚
            // 本当はそういう場合にすぐ帰ると手札がばれるのでよくない
            if (monitor) {
                if (!mbuf[0].isPASS()) cerr << "final move. " << mbuf[0] << endl;
                else cerr << "no chance. PASS" << endl;
            }
            if (!mbuf[0].isPASS()) {
                shared.setMyMate(field.getBestClass()); // 上がり
                shared.setMyL2Result(1);
            }
            return mbuf[0];
        }
        
        // 合法着手生成(特別着手)
        if (b.isNull()) {
            mbuf[NMoves++].setPASS();
        }
        if (containsJOKER(myCards) && containsS3(opsCards)) {
            if (b.isGroup() && b.qty() > 1) {
                NMoves += genJokerGroup(mbuf.data() + NMoves, myCards, opsCards, b);
            }
        }
        std::shuffle(mbuf.begin(), mbuf.begin() + NMoves, dice);
        
        // 場の情報をまとめる
        field.prepareForPlay();
        
        // 着手追加情報を設定
        bool l2failure = false;
        for (int i = 0; i < NMoves; i++) {
            Move& move = mbuf[i];
            // 支配性
            if (move.qty() > b.getMaxNCardsAwake()
                || dominatesCards(move, opsCards, b)) {
                move.setDO(); // 他支配
            }
            if (move.isPASS() || move.qty() > myCards.count() - move.qty()
                || dominatesCards(move, myCards - move.cards(), b)) {
                move.setDM(); // 自己支配
            }
            if (Settings::MateSearchOnRoot) { // 多人数必勝判定
                if (checkHandMate(1, searchBuffer, move, myHand, opsHand, b)) {
                    move.setMate(); b.setMate();
                }
            }
            if (Settings::L2SearchOnRoot) {
                if (field.getNAlivePlayers() == 2) { // 残り2人の場合はL2判定
                    L2Judge lj(Settings::policyMode ? 200000 : 2000000, searchBuffer);
                    int l2Result = (b.isNull() && move.isPASS()) ? L2_LOSE : lj.start_check(move, myHand, opsHand, b);
                    if (l2Result == L2_WIN) { // 勝ち
                        DERR << "l2win!" << endl;
                        move.setL2Mate(); b.setL2Mate();
                    } else if (l2Result == L2_LOSE) {
                        move.setL2GiveUp();
                    } else {
                        l2failure = true;
                    }
                }
            }
        }
        
        // 判定結果を報告
        if (Settings::L2SearchOnRoot) {
            if (field.getNAlivePlayers() == 2) {
                // L2探索の結果MATEが立っていれば勝ち
                // 立っていなければ判定失敗か負け
                if (b.isL2Mate()) {
                    shared.setMyL2Result(1);
                } else if (l2failure) {
                    shared.setMyL2Result(-3);
                } else {
                    b.setL2GiveUp();
                    shared.setMyL2Result(-1);
                }
            }
        }
        if (Settings::MateSearchOnRoot) {
            if (b.isMate() && !b.isL2Mate()) shared.setMyMate(field.getBestClass());
        }

        if (monitor) {
            // 着手候補一覧表示
            cerr << "My Cards : " << myCards << endl;
            cerr << b << toInfoString(b) << endl;
            for (int i = 0; i < NMoves; i++) {
                cerr << mbuf[i] << toInfoString(mbuf[i], b) << endl;
            }
        }
        
        // ルートノード設定
        int limitSimulations = std::min(5000, (int)(pow((double)NMoves, 0.8) * 700));
        root.setPlay(mbuf.data(), NMoves, field, shared, limitSimulations);
        
        // 方策関数による評価(必勝のときも行う, 除外された着手も考慮に入れる)
        double score[N_MAX_MOVES + 256];
        playPolicyScore(score, mbuf.data(), NMoves, field, shared.basePlayPolicy, 0);
        root.feedPolicyScore(score, NMoves);

        // モンテカルロ法による評価(結果確定のとき以外)
        if (!Settings::policyMode
            && !b.isMate() && !b.isGiveUp()) {
            if (Settings::addPolicyOnRoot) root.addPolicyScoreToMonteCarloScore();
            startMonteCarlo(root, field, Settings::numPlayThreads);
        }
        // 着手決定のための情報が揃ったので着手を決定する
        
        // 方策とモンテカルロの評価
        root.sort();
        
        // 以下必勝を判定したときのみ
        if (b.isMate()) {
            int next = root.candidates;
            // 1. 必勝判定で勝ちのものに限定
            next = root.binary_sort(next, [](const RootAction& a) { return a.move.isMate(); });
            assert(next > 0);
            // 2. 即上がり
            next = root.binary_sort(next, [](const RootAction& a) { return -a.move.qty(); });
            // 3. 完全勝利
            //next = root.binary_sort(next, [](const RootAction& a) { return a.move.isPW(); });
            // 4. 多人数判定による勝ち
            next = root.binary_sort(next, [](const RootAction& a) { return a.move.isMate() && !a.move.isL2Mate(); });
#ifdef DEFEAT_RIVAL_MATE
            if (next > 1 && !isNoRev(myCards)) {
                // 5. ライバルに不利になるように革命を起こすかどうか
                int prefRev = Heuristics::preferRev(field, myPlayerNum, field.getRivalPlayersFlag(myPlayerNum));
                if (prefRev > 0) { // 革命優先
                    next = root.sort(next, [myCards](const RootAction& a) {
                        return a.move.isRev() ? 2 :
                        (isNoRev(maskCards(myCards, a.move.cards())) ? 0 : 1);
                    });
                }
                if (prefRev < 0) { // 革命しないことを優先
                    next = root.sort(next, [myCards](const RootAction& a) {
                        return a.move.isRev() ? -2 :
                        (isNoRev(maskCards(myCards, a.move.cards())) ? 0 : -1);
                    });
                }
            }
#endif
            // 必勝時の出し方の見た目をよくする
            if (b.isUnrivaled()) {
                // 6. 独断場のとき最小分割数が減るのであればパスでないものを優先
                //    そうでなければパスを優先
                int division = divisionCount(searchBuffer, myCards);
                for (int i = 0; i < NMoves; i++) {
                    Cards nextCards = myCards - root.child[i].move.cards();
                    root.child[i].nextDivision = divisionCount(searchBuffer, nextCards);
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
            next = root.binary_sort(next, [](const RootAction& a) { return !a.move.dominatesMe(); });
            
            playMove = root.child[0].move; // 必勝手から選ぶ
        }

        // 最高評価の着手を選ぶ
        if (playMove == MOVE_NONE) playMove = root.child[0].move;

        if (monitor) {
            cerr << root.toString();
            cerr << "\033[1m\033[" << 31 << "m";
            cerr << "Best Move : " << playMove << endl;
            cerr << "\033[" << 39 << "m\033[0m";
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