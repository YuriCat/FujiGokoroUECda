#include "../core/action.hpp"
#include "mate.hpp"
#include "policy.hpp"
#include "modeling.hpp"
#include "data.hpp"

using namespace std;

#define F(index) { if (v != nullptr) v->push_back(make_pair(index, 1.0f)); score += table[index]; }

double PlayerModel::playBiasScore(const Field& field, int player, Move move,
                                  vector<pair<int, float>> *v) const {
    const double *table = bias[player];
    double score = 0;
    if (move.isPASS()) F(0);
    if (move.isSingle()) F(1);
    if (move.isGroup()) F(2);
    if (move.isSeq()) F(3);
    if (move.containsJOKER()) F(4);
    if (move.isRev()) F(5);
    if (move.domInevitably()) F(6);
    if (field.board.domConditionally(move)) F(7);
    if (field.board.locksSuits(move)) F(8);
    return score;
}

#undef F

#define F(index) { if (v != nullptr) v->push_back(make_pair(PLAY_BIAS_FEATURES + index, 1.0f)); score += table[index]; }

double PlayerModel::changeBiasScore(int player, const Cards cards, const Cards changeCards,
                                    vector<pair<int, float>> *v) const {
    const double *table = bias[player] + PLAY_BIAS_FEATURES;
    double score = 0;
    Cards afterCards = cards - changeCards;
    if (containsD3(changeCards)) F(0);
    if (containsS3(changeCards)) F(1);
    if (containsS3(changeCards) && containsJOKER(afterCards)) F(2);
    for (IntCard ic : changeCards) {
        int rank = IntCardToRank(ic);
        if (rank == RANK_3) F(3);
        if (rank == RANK_8) F(4);
        if (RankToCards(rank) & afterCards) F(5);
    }
    return score;
}

#undef F

void updateStats(PlayerModelStats& tmpStats, int turn, double *baseScore, double *score, int numMoves, int index) {
    SoftmaxSelector<double> selector(score, numMoves, 1);
    tmpStats.count[turn] += 1;
    tmpStats.updatedEntropy[turn] += selector.entropy();
    tmpStats.updatedCrossEntropy[turn] += -log2(selector.prob(index));
    tmpStats.updatedAccuracy[turn] += index == (max_element(score, score + numMoves) - score) ? 1.0 : 0.0;
    {
        SoftmaxSelector<double> selector2(baseScore, numMoves, 1);
        tmpStats.baseEntropy[turn] += selector2.entropy();
        tmpStats.baseCrossEntropy[turn] += -log2(selector2.prob(index));
        tmpStats.baseAccuracy[turn] += index == (max_element(baseScore, baseScore + numMoves) - baseScore) ? 1.0 : 0.0;
        double kldiv = 0;
        for (int i = 0; i < numMoves; i++) {
            kldiv += selector2.prob(i) * (log2(selector2.prob(i)) - log2(selector.prob(i)));
        }
        tmpStats.KLDivergence[turn] += kldiv;
    }
}

void PlayerModel::updateGame(const GameRecord& record, int playerNum,
                             const SharedData& shared, MoveInfo *const buf, bool computeStats) {
    Field field;
    // 交換
    if (!record.isInitGame()) {
        for (auto change : ChangeRoller(field, record)) {
            // 自分が試合中の場合は、自分が交換下位の場合の交換以外は空のデータが入っている
            if (change.from == playerNum) continue;
            if (change.already) continue;
            if (change.cards == CARDS_NULL) continue;
            assert(playerNum < 0 || change.to == playerNum);

            // ベース方策を計算
            Cards changeCards = change.cards;
            int changeQty = changeCards.count();
            Cards dealtCards = record.orgCards[change.from] + change.cards;
            Cards buf[N_MAX_CHANGES];
            int numChanges = genChange(buf, dealtCards, changeQty);
            int index = find(buf, buf + numChanges, changeCards) - buf;
            if (index == numChanges) continue;

            double baseScore[N_MAX_CHANGES], score[N_MAX_CHANGES];
            changePolicyScore(score, buf, numChanges, dealtCards, changeQty, shared.baseChangePolicy);
            if (computeStats) for (int i = 0; i < numChanges; i++) baseScore[i] = score[i];

            // プレーヤーに対応したバイアスを足す
            vector<pair<int, float>> features[N_MAX_CHANGES];
            for (int i = 0; i < numChanges; i++) {
                score[i] += changeBiasScore(change.from, dealtCards, buf[i], features + i);
            }

            double prob[N_MAX_CHANGES];
            for (int i = 0; i < numChanges; i++) prob[i] = score[i];
            SoftmaxSelector<double> selector(prob, numChanges, 1);
            for (int i = 0; i < numChanges; i++) prob[i] = selector.prob(i);

            // バイアスパラメータ更新
            updator.update(bias[change.from], prob, numChanges, index, features, 3);
            // スタッツ更新
            if (computeStats) updateStats(tmpChangeStats, change.from, baseScore, score, numChanges, index);
        }
    }

    // 着手
    for (Move move : PlayRoller(field, record, record.orgCards)) {
        int turn = field.turn();

        // 除外ルール
        if (turn == playerNum) continue;
        if (field.numPlayersAlive() <= 2) continue;
        if (!field.hand[turn].cards.holds(move.cards())) continue;

        // ゲームが正常終了せずこのプレーヤーの手札が判明しなかったケース
        // ただ、本来は相手手札が判明しなくても統計情報から相手の傾向を判断することはできるはず
        if (record.orgCards[turn].count() != record.numOrgCards[turn]) continue;

        int numMoves = genMove(buf, field.hand[turn].cards, field.board);
        if (numMoves <= 1) continue;

        int index = -1;
        bool mate = false;
        for (int i = 0; i < numMoves; i++) {
            if (buf[i] == move) index = i;
            if (!mate && checkHandMate(0, buf + numMoves, buf[i], field.hand[turn], field.opsHand[turn],
                                       field.board, field.fieldInfo)) mate = true;
        }
        if (index < 0) continue;
        if (mate) continue;

        // ベース方策を計算
        double baseScore[N_MAX_MOVES], score[N_MAX_MOVES];
        playPolicyScore(score, buf, numMoves, field, shared.basePlayPolicy);
        if (computeStats) for (int i = 0; i < numMoves; i++) baseScore[i] = score[i];

        // プレーヤーに対応したバイアスを足す
        vector<pair<int, float>> features[N_MAX_MOVES];
        for (int i = 0; i < numMoves; i++) {
            score[i] += playBiasScore(field, turn, buf[i], features + i);
        }

        double prob[N_MAX_MOVES];
        for (int i = 0; i < numMoves; i++) prob[i] = score[i];
        SoftmaxSelector<double> selector(prob, numMoves, 1);
        for (int i = 0; i < numMoves; i++) prob[i] = selector.prob(i);

        // バイアスパラメータ更新
        updator.update(bias[turn], prob, numMoves, index, features);
        // スタッツ更新
        if (computeStats) updateStats(tmpStats, turn, baseScore, score, numMoves, index);
    }
}

void PlayerModel::update(const MatchRecord& record, int gameNum, int playerNum,
                         const SharedData& shared, MoveInfo *const buf) {
    for (int i = 0; i < 10; i++) {
        int g = gameNum - i;
        if (g >= 0) updateGame(record.games[g], playerNum, shared, buf, i == 0);
    }
    trained = true;
    games += 1;
    if (games % 100 == 0) {
        stats_.push_back(tmpStats);
        tmpStats = {0};
        changeStats_.push_back(tmpChangeStats);
        tmpChangeStats = {0};
    }
}

void PlayerModel::stats() const {
    cerr << "Modeling Stats:" << endl;
    cerr << std::fixed << std::setprecision(3);
    int block = games < 1000 ? 1 : 10;

    // 交換
    cerr << "  ";
    for (int p = 0; p < N_PLAYERS; p++) cerr << "Player " << p << "     ";
    cerr << endl;
    for (int i = 0; i < changeStats_.size(); i += block) {
        PlayerModelStats t = {0};
        for (int j = i; j < min(i + block, (int)changeStats_.size()); j++) {
            for (int k = 0; k < t.v_.size(); k++) t.v_[k] += changeStats_[j].v_[k];
        }
        cerr << "C ";
        for (int p = 0; p < N_PLAYERS; p++) {
            double n = 1e-6 + t.count[p];
            cerr << t.updatedAccuracy[p] / n << "(" << t.baseAccuracy[p] / n << ") ";
        }
        cerr << endl;
    }

    // 着手
    for (int i = 0; i < stats_.size(); i += block) {
        PlayerModelStats t = {0};
        for (int j = i; j < min(i + block, (int)stats_.size()); j++) {
            for (int k = 0; k < t.v_.size(); k++) t.v_[k] += stats_[j].v_[k];
        }
        cerr << "P ";
        for (int p = 0; p < N_PLAYERS; p++) {
            double n = 1e-6 + t.count[p];
            cerr << t.updatedAccuracy[p] / n << "(" << t.baseAccuracy[p] / n << ") ";
        }
        cerr << endl;
    }

    cerr << std::defaultfloat << std::setprecision(6);
}