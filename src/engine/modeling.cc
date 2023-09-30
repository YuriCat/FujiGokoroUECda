#include "../core/action.hpp"
#include "mate.hpp"
#include "policy.hpp"
#include "modeling.hpp"

using namespace std;

#define F(index) { if (v != nullptr) v->push_back(make_pair(index, 1.0f)); score += table[index]; }

double PlayerModel::biasScore(const Field& field, int player, Move move,
                              vector<pair<int, float>> *v) const {
    const double *table = bias[player];
    double score = 0;
    if (move.isPASS()) F(0)
    if (move.isSingle()) F(1)
    if (move.isGroup()) F(2)
    if (move.isSeq()) F(3)
    if (move.containsJOKER()) F(4)
    if (move.isRev()) F(5)
    if (move.domInevitably()) F(6)
    if (field.board.domConditionally(move)) F(7)
    if (field.board.locksSuits(move)) F(8)

    return score;
}

#undef F

void PlayerModel::updateGame(const GameRecord& record, int playerNum,
                             const PlayPolicy<policy_value_t>& playPolicy, MoveInfo *const buf, bool computeStats) {
    Field field;
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
        double score[N_MAX_MOVES], score2[N_MAX_MOVES];
        playPolicyScore(score, buf, numMoves, field, playPolicy, 0);
        for (int i = 0; i < numMoves; i++) score2[i] = score[i];

        // プレーヤーに対応したバイアスを足す
        vector<pair<int, float>> features[N_MAX_MOVES];
        for (int i = 0; i < numMoves; i++) {
            score[i] += biasScore(field, turn, buf[i], features);
        }

        SoftmaxSelector<double> selector(score, numMoves, 1);
        double prob[N_MAX_MOVES];
        for (int i = 0; i < numMoves; i++) prob[i] = selector.prob(i);

        // バイアスパラメータ更新
        updator.update(bias[turn], prob, numMoves, index, features);

        // スタッツ情報
        if (!computeStats) continue;
        tmpStats.count[turn] += 1;
        tmpStats.updatedEntropy[turn] += selector.entropy();
        tmpStats.updatedEntropy[turn] += -log2(selector.prob(index));
        tmpStats.updatedAccuracy[turn] += index == (max_element(prob, prob + numMoves) - prob) ? 1.0 : 0.0;
        {
            SoftmaxSelector<double> selector2(score2, numMoves, 1);
            tmpStats.baseEntropy[turn] += selector2.entropy();
            tmpStats.baseCrossEntropy[turn] += -log2(selector2.prob(index));
            tmpStats.baseAccuracy[turn] += index == (max_element(score2, score2 + numMoves) - score2) ? 1.0 : 0.0;
            double kldiv = 0;
            for (int i = 0; i < numMoves; i++) {
                kldiv += selector2.prob(i) * (log2(selector2.prob(i)) - log2(selector.prob(i)));
            }
            tmpStats.KLDivergence[turn] += kldiv;
        }
    }
}

void PlayerModel::update(const MatchRecord& record, int gameNum, int playerNum,
                         const PlayPolicy<policy_value_t>& playPolicy, MoveInfo *const buf) {
    for (int i = 0; i < 1; i++) {
        int g = gameNum - i;
        if (g >= 0) updateGame(record.games[g], playerNum, playPolicy, buf, i == 0);
    }
    games += 1;
    if (games % 100 == 0) {
        stats_.push_back(tmpStats);
        tmpStats = {0};
    }
}

void PlayerModel::stats() const {
    cerr << "Modeling Stats:" << endl;
    for (int p = 0; p < N_PLAYERS; p++) cerr << "Player " << p << "     ";
    cerr << endl;

    int block = games < 1000 ? 1 : 10;
    for (int i = 0; i < stats_.size(); i += block) {
        PlayerModelStats t = {0};
        for (int j = i; j < min(i + block, (int)stats_.size()); j++) {
            for (int k = 0; k < t.v_.size(); k++) t.v_[k] += stats_[j].v_[k];
        }
        for (int p = 0; p < N_PLAYERS; p++) {
            double n = 1e-6 + t.count[p];
            cerr << std::fixed << std::setprecision(3) << t.updatedAccuracy[p] / n
            << "(" << t.baseAccuracy[p] / n << ") " << std::defaultfloat;
        }
        cerr << endl;
    }
}

void GradientUpdator::update(double *value, double *prob, int numMoves, int index, const vector<pair<int, float>> *features) {
    // 統計情報を更新
    for (int i = 0; i < numMoves; i++) {
        for (auto f : features[i]) {
            sum[f.first] += f.second;
            sum2[f.first] += f.second * f.second;
            count[f.first] += 1;
        }
        totalCount += 1;
    }
    for (int j = 0; j < BIAS_FEATURES; j++) {
        sum[j] *= 1 - stats_decay;
        sum2[j] *= 1 - stats_decay;
        count[j] *= 1 - stats_decay;
    }
    totalCount *= 1 - stats_decay;

    // 選ばれた手の確率を上げ、全体の手の確率を下げる
    for (int i = 0; i < numMoves; i++) {
        for (auto f : features[i]) {
            double diff = i == index ? (1 - prob[i]) : -prob[i];
            value[f.first] += lr * diff / (1e-3 + var(f.first)) * f.second;
            value[f.first] *= pow(1 - decay, 1 / (1e-3 + freq(i)));
        }
    }
}