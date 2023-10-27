#pragma once

// 思考部メイン
// 実際のゲームでの局面更新など低次な処理はmain関数にて

#include <thread>
#include "../settings.h"
#include "data.hpp"
#include "reward.hpp"
#include "../core/dominance.hpp"
#include "mate.hpp"
#include "last2.hpp"
#include "policy.hpp"
#include "estimation.hpp"
#include "simulation.hpp"
#include "monteCarlo.hpp"

namespace Settings {
    const bool changeHeuristicsOnRoot = true;
    const bool addPolicyOnRoot = true;
    const bool L2SearchOnRoot = true;
    const bool mateSearchOnRoot = true;
    const bool defeatRivalMate = true;
}

class WisteriaEngine {
private:
    ThreadTools rootTools;
    std::vector<ThreadTools> threadTools;
    RootInfo root;
public:
    SharedData shared;
    decltype(shared.record)& record = shared.record;
    bool monitor = false;

    void setRandomSeed(uint64_t s) {
        // 乱数系列を初期化
        rootTools.dice.seed(s);
    }

    void initMatch(int playerNum) {
        // サイコロ初期化
        // シード指定の場合はこの後に再設定される
        setRandomSeed((uint32_t)time(NULL));
        shared.initMatch(playerNum);
        auto& playPolicy = shared.basePlayPolicy;
        auto& changePolicy = shared.baseChangePolicy;

        shared.basePlayPolicy.bin(DIRECTORY_PARAMS_IN + "play_policy.bin");
        shared.baseChangePolicy.bin(DIRECTORY_PARAMS_IN + "change_policy.bin");
        loadEstimationParams(DIRECTORY_PARAMS_IN + "est_score.bin");
    }
    void initGame() {
        // 汎用変数の設定
        shared.initGame();
        // 報酬設定
        // ランク初期化まで残り何試合か
        // 公式には未定だが、多くの場合100試合だろうから100試合としておく
        const int gbci = numGamesBeforeClassInit(record.getLatestGameNum());
        shared.gameReward = standardReward(std::min(gbci + 1, N_REWARD_CALCULATED_GAMES)).back();
        root.clear();
        L2::init();
    }
    void startMonteCarlo(RootInfo& root, const Field& field, int numThreads) {
        // 1スレッドの場合は関数で、そうでなければスレッドで開く
        if (numThreads > 1) {
            // スレッド数に合わせたThreadToolsを準備
            if ((int)threadTools.size() < numThreads - 1) threadTools.resize(numThreads - 1);
            for (int i = 1; i < numThreads; i++) threadTools[i - 1].dice.seed(rootTools.dice() + i);
            std::vector<std::thread> threads;
            for (int i = 0; i < numThreads; i++) {
                threads.emplace_back(&MonteCarloThread, i, numThreads, &root, &field, &shared,
                                     i == 0 ? &rootTools : &threadTools[i - 1]);
            }
            for (auto& t : threads) t.join();
        } else {
            MonteCarloThread(0, 1, &root, &field, &shared, &rootTools);
        }
    }
    Cards change(unsigned changeQty) { // 交換関数
        const auto& game = record.latestGame();
        const int myPlayerNum = record.myPlayerNum;

        Cards changeCards = CARDS_NULL;
        const Cards myCards = game.dealtCards[myPlayerNum];
        if (monitor) std::cerr << "My Cards : " << myCards << std::endl;

        // 手札レベルで枝刈りする場合
        Cards tmp = myCards;
        if (Settings::changeHeuristicsOnRoot) {
            tmp.mask(CARDS_8 | CARDS_2 | CARDS_JOKER | CARDS_D3);
            if (myCards.joker() > 0) tmp.mask(CARDS_S3);
            if (tmp.count() < changeQty) tmp = myCards; // キャンセル
        }

        if (tmp.count() == changeQty) return tmp;

        // 合法交換候補生成
        std::array<Cards, N_MAX_CHANGES> change;
        int numChanges = genChange(change.data(), tmp, changeQty);
        if (numChanges == 1) return change[0];
        std::shuffle(change.begin(), change.begin() + numChanges, rootTools.dice);

        // 必勝チェック&枝刈り
        for (int i = 0; i < numChanges; i++) {
            // 交換後の自分のカード
            Cards restCards = myCards - change[i];

            // D3を持っている場合、自分からなので必勝チェック
            if (Settings::mateSearchOnRoot && containsD3(restCards)) {
                const Board b = OrderToNullBoard(0); // 通常オーダーの空場
                FieldAddInfo fieldInfo;
                fieldInfo.init();
                fieldInfo.setFlushLead();
                fieldInfo.setMinNumCardsAwake(10);
                fieldInfo.setMinNumCards(10);
                fieldInfo.setMaxNumCardsAwake(11);
                fieldInfo.setMaxNumCards(11);
                Hand mine, ops;
                mine.set(restCards);
                ops.set(CARDS_ALL - restCards);
                if (judgeHandMate(1, rootTools.mbuf, mine, ops, b, fieldInfo)) {
                    CERR << "CHANGE MATE!" << std::endl;
                    shared.setMyMate(DAIFUGO);
                    return change[i]; // 必勝
                }
            }
        }

        if (Settings::changeHeuristicsOnRoot) {
            for (int i = 0; i < numChanges; i++) {
                Cards restCards = myCards - change[i];
                // 残り札に革命が無い場合,Aもダメ
                if (isNoRev(restCards)) {
                    if (change[i] & CARDS_A) std::swap(change[i], change[--numChanges]);
                }
                if (numChanges == 1) return change[0];
            }
        }

        // ルートノード設定
        Field field;
        field.fromRecord(game, myPlayerNum, -1);
        int limitSimulations = Settings::fixedSimulationCount > 0 ? Settings::fixedSimulationCount
                               : std::min(10000, (int)(pow((double)numChanges, 0.8) * 700));
        root.setChange(change.data(), numChanges, field, shared, &rootTools, limitSimulations);

        // 方策関数による評価
        double score[N_MAX_CHANGES];
        changePolicyScore(score, change.data(), numChanges, myCards, changeQty, shared.baseChangePolicy);
        root.feedPolicyScore(score, numChanges);

        // モンテカルロ法による評価
        if (!Settings::policyMode && changeCards == CARDS_NULL) {
            if (Settings::addPolicyOnRoot) root.addPolicyScoreToMonteCarloScore();
            startMonteCarlo(root, field, Settings::numChangeThreads);
        }
        root.sort();

        // 最高評価の交換を選ぶ
        if (changeCards == CARDS_NULL) changeCards = root.child[0].changeCards;

        assert(countCards(changeCards) == changeQty);
        assert(holdsCards(myCards, changeCards));
        if (monitor) {
            std::cerr << root.toString();
            std::cerr << "\033[1m\033[" << 34 << "m";
            std::cerr << "Best Change : " << changeCards << std::endl;
            std::cerr << "\033[" << 39 << "m\033[0m";
        }
        return changeCards;
    }

    Move play() { // ここがプレー関数
        const auto& game = shared.record.latestGame();
        Move playMove = MOVE_NONE;

        // ルート合法手生成バッファ
        std::array<MoveInfo, N_MAX_MOVES> mbuf;

        const int myPlayerNum = record.myPlayerNum;

        Field field;
        field.fromRecord(game, myPlayerNum);
        if (monitor) std::cerr << field.toString();
        assert(field.turn() == myPlayerNum);

        const Hand& myHand = field.getHand(myPlayerNum);
        const Hand& opsHand = field.getOpsHand(myPlayerNum);
        const Cards myCards = myHand.cards;
        const Cards opsCards = opsHand.cards;
        const Board b = field.board;
        FieldAddInfo& fieldInfo = field.fieldInfo;

        // サーバーの試合進行バグにより無条件支配役が流れずに残っている場合はリジェクトにならないようにパスしておく
        if (b.isInvalid()) return MOVE_PASS;

        // 合法着手生成
        int numMoves = genMove(mbuf.data(), myCards, b);
        if (numMoves <= 0) { // 合法着手なし
            std::cerr << "No valid move." << std::endl;
            return MOVE_PASS;
        }
        std::shuffle(mbuf.begin(), mbuf.begin() + numMoves, rootTools.dice);

        // 即上がりチェック
        for (int i = 0; i < numMoves; i++) {
            if (mbuf[i].qty() >= myHand.qty) {
                if (monitor) {
                    std::cerr << "\033[1m\033[31m";
                    std::cerr << "Final Move : " << mbuf[i];
                    std::cerr << "\033[39m\033[0m" << std::endl;
                }
                shared.setMyMate(field.bestClass()); // 上がり
                shared.setMyL2Result(1);
                return mbuf[i];
            }
        }

        if (numMoves == 1) {
            // 合法着手1つ。パスのみ
            // 本当はそういう場合にすぐ帰ると手札がばれるのでよくない
            if (monitor) std::cerr << "no chance. PASS" << std::endl;
            return mbuf[0];
        }

        // 合法着手生成(特別着手)
        if (b.isNull()) {
            mbuf[numMoves++].setPASS();
        }
        if (containsJOKER(myCards) && containsS3(opsCards)) {
            if (b.isGroup() && b.qty() > 1) {
                numMoves += genJokerGroup(mbuf.data() + numMoves, myCards, opsCards, b);
            }
        }
        std::shuffle(mbuf.begin(), mbuf.begin() + numMoves, rootTools.dice);

        // 着手追加情報を設定
        bool l2failure = false;
        for (int i = 0; i < numMoves; i++) {
            MoveInfo& move = mbuf[i];
            // 支配性
            if (move.qty() > fieldInfo.maxNumCardsAwake()
                || dominatesCards(move, opsCards, b)) {
                move.setDO(); // 他支配
            }
            if (move.isPASS() || move.qty() > myCards.count() - move.qty()
                || dominatesCards(move, myCards - move.cards(), b)) {
                move.setDM(); // 自己支配
            }
            if (Settings::mateSearchOnRoot) { // 多人数必勝判定
                if (checkHandMate(1, rootTools.mbuf, move, myHand, opsHand, b, fieldInfo)) {
                    move.setMPMate(); fieldInfo.setMPMate();
                }
            }
            if (Settings::L2SearchOnRoot) {
                if (field.numPlayersAlive() == 2) { // 残り2人の場合はL2判定
                    int nodeLimit = Settings::policyMode ? 200000 : 2000000;
                    int l2Result = L2_LOSE;
                    if (!(b.isNull() && move.isPASS())) l2Result = checkLast2(rootTools.mbuf, move, myHand, opsHand, b, fieldInfo, nodeLimit);
                    if (l2Result == L2_WIN) { // 勝ち
                        DERR << "l2win!" << std::endl;
                        move.setL2Mate(); fieldInfo.setL2Mate();
                        DERR << fieldInfo << std::endl;
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
            if (field.numPlayersAlive() == 2) {
                // L2探索の結果MATEが立っていれば勝ち
                // 立っていなければ判定失敗か負け
                if (fieldInfo.isL2Mate()) {
                    shared.setMyL2Result(1);
                } else if (l2failure) {
                    shared.setMyL2Result(0);
                } else {
                    fieldInfo.setL2GiveUp();
                    shared.setMyL2Result(-1);
                }
            }
        }
        if (Settings::mateSearchOnRoot) {
            if (fieldInfo.isMPMate()) shared.setMyMate(field.bestClass());
        }

        if (monitor) {
            // 着手候補一覧表示
            std::cerr << "My Cards : " << myCards << std::endl;
            std::cerr << b << " " << fieldInfo << std::endl;
            for (int i = 0; i < numMoves; i++) {
                std::cerr << mbuf[i] << toInfoString(mbuf[i], b) << std::endl;
            }
        }

        // ルートノード設定
        int limitSimulations = Settings::fixedSimulationCount > 0 ? Settings::fixedSimulationCount
                               : std::min(5000, (int)(pow((double)numMoves, 0.8) * 700));
        root.setPlay(mbuf.data(), numMoves, field, shared, &rootTools, limitSimulations);

        // 方策関数による評価(必勝のときも行う, 除外された着手も考慮に入れる)
        double score[N_MAX_MOVES];
        playPolicyScore(score, mbuf.data(), numMoves, field, shared.basePlayPolicy);
        root.feedPolicyScore(score, numMoves);

        // モンテカルロ法による評価(結果確定のとき以外)
        if (!Settings::policyMode
            && !fieldInfo.isMate() && !fieldInfo.isGiveUp()) {
            if (Settings::addPolicyOnRoot) root.addPolicyScoreToMonteCarloScore();
            startMonteCarlo(root, field, Settings::numPlayThreads);
        }
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
            if (Settings::defeatRivalMate) {
                if (next > 1 && !isNoRev(myCards)) {
                    // 5. ライバルに不利になるように革命を起こすかどうか
                    int prefRev = positionPreferRevolution(field, myPlayerNum);
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
            }
            // 必勝時の出し方の見た目をよくする
            if (fieldInfo.isUnrivaled()) {
                // 6. 独断場のとき最小分割数が減るのであればパスでないものを優先
                //    そうでなければパスを優先
                int division = divisionCount(rootTools.mbuf, myCards);
                for (int i = 0; i < numMoves; i++) {
                    Cards nextCards = myCards - root.child[i].move.cards();
                    root.child[i].nextDivision = divisionCount(rootTools.mbuf, nextCards);
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
            std::cerr << root.toString();
            std::cerr << "\033[1m\033[31m";
            std::cerr << "Best Move : " << playMove;
            std::cerr << "\033[39m\033[0m" << std::endl;
        }
        return playMove;
    }
    void closeGame() {
        // プレーヤーモデル更新
        const auto& record = shared.record;
        shared.playerModel.update(record, record.games.size() - 1, record.myPlayerNum, shared, rootTools.mbuf);
        shared.closeGame();
    }
    void closeMatch() {
        shared.closeMatch();
    }
    ~WisteriaEngine() {
        closeMatch();
    }
};