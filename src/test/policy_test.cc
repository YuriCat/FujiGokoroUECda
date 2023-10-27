
// 方策関数のテスト

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/policy.hpp"
#include "../engine/simulation.hpp"
#include "test.h"

using namespace std;

static XorShift64 dice((unsigned int)time(NULL));

static ChangePolicy changePolicy;
static PlayPolicy playPolicy;

int outputParams() {
    // 方策関数中で気になるパラメータを出力
    int f0 = PlayPolicySpace::FEA_IDX(PlayPolicySpace::FEA_GR_CARDS)
                             + 0 * (16 * 2) * (16) * N_PATTERNS_SUITS_SUITS
                             + RANK_6 * (2) * (16) * N_PATTERNS_SUITS_SUITS
                             + 1 * (16) * N_PATTERNS_SUITS_SUITS
                             + RANK_A * N_PATTERNS_SUITS_SUITS
                             + SSIndex[SUITS_S][SUITS_S];
    int f1 = PlayPolicySpace::FEA_IDX(PlayPolicySpace::FEA_SEQ_CARDS)
                             + 0 * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_4 * (3) * (16) * N_PATTERNS_SUIT_SUITS
                             + min(int(3) - 3, 2) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_7 * N_PATTERNS_SUIT_SUITS
                             + sSIndex[SUITS_H][SUITS_CH];
    int f2 = PlayPolicySpace::FEA_IDX(PlayPolicySpace::FEA_SEQ_CARDS)
                             + 0 * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_4 * (3) * (16) * N_PATTERNS_SUIT_SUITS
                             + min(int(4) - 3, 2) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_7 * N_PATTERNS_SUIT_SUITS
                             + sSIndex[SUITS_H][SUITS_C];

    cerr << f0 << " " << playPolicy.param(f0) << endl;
    cerr << f1 << " " << playPolicy.param(f1) << endl;
    cerr << f2 << " " << playPolicy.param(f2) << endl;

    //PlayPolicySpace::commentToPolicyParam(cerr, playPolicy.param_);
    return 0;
}

int testChangePolicyWithRecord(const MatchRecord& match) {
    // 棋譜中の交換との一致率計算
    // 棋譜ファイルは1つのみ受ける(相手が変わる場合には最初からなのでまたこの関数を呼ぶ)

    int sameCount[N_PLAYERS][2] = {0};
    int trials[N_PLAYERS][2] = {0};
    uint64_t time[N_PLAYERS][2] = {0};

    cerr << "change policy : " << endl;

    Field field;
    for (const auto& game : match.games) {
        for (ChangeRecord ch : ChangeRoller(field, game)) {
            Cards change[N_MAX_CHANGES + 1];
            const int cl = field.classOf(ch.from);
            if (cl < MIDDLE) {
                const Cards myCards = field.getCards(ch.from);
                const int changeQty = N_CHANGE_CARDS(cl);
                const int numChanges = genChange(change, myCards, changeQty);

                Clock clock;
                clock.start();
                int index = changeWithBestPolicy(change, numChanges, myCards, changeQty, changePolicy, dice);
                time[ch.from][cl] += clock.stop();

                Cards p = change[index];

                if (ch.cards == p) sameCount[ch.from][cl] += 1;
                trials[ch.from][cl] += 1;
            }
        }
    }
    for (int p = 0; p < N_PLAYERS; p++) {
        cerr << match.playerName[p];
        double sum = 0;
        for (int i = 0; i < 2; i++) {
            double prob = sameCount[p][i] / (double)trials[p][i];
            cerr << " " << prob << " (" << sameCount[p][i] << " / " << trials[p][i] << ")";
            cerr << " in " << time[p][i] / (double)trials[p][i] << " clock";
            sum += prob;
        }
        sum /= 2;
        cerr << " (" << sum << ")" << endl;
    }
    return 0;
}

int testPlayPolicyWithRecord(const MatchRecord& match) {
    // 棋譜中の役提出との一致率計算
    int sameCount[N_PLAYERS] = {0};
    int trials[N_PLAYERS] = {0};
    uint64_t time[N_PLAYERS] = {0};

    cerr << "play policy : " << endl;

    Field field;
    for (const auto& game : match.games) {
        for (Move move : PlayRoller(field, game)) {
            int turn = field.turn();
            auto moves = genMove(field);

            Clock clock;
            clock.start();
            int index = playWithBestPolicy(moves.data(), moves.size(), field, playPolicy, dice);
            time[turn] += clock.stop();
            Move p = moves[index];

            if (move == p) sameCount[turn] += 1;
            trials[turn] += 1;
        }
    }
    for (int p = 0; p < N_PLAYERS; p++) {
        cerr << match.playerName[p];
        double prob = sameCount[p] / (double)trials[p];
        cerr << " " << prob << " (" << sameCount[p] << " / " << trials[p] << ")";
        cerr << " in " << time[p] / (double)trials[p] << " clock" << endl;
    }
    return 0;
}

int testSelector(const MatchRecord& match) {
    // 方策の最終段階の実験
    double sameProb[4][7][5] = {0}; // 確率ベースでの一致率
    double entropy[4][7][5] = {0}; // 方策エントロピー
    int trials = 0;
    uint64_t time[4] = {0};
    Clock clock;

    cerr << "play policy with selector : " << endl;

    Field field;
    for (const auto& game : match.games) {
        for (Move move : PlayRoller(field, game)) {
            MoveInfo play[N_MAX_MOVES];
            double score[N_MAX_MOVES];

            const int turnPlayer = field.turn();
            const int numMoves = genMove(play, field.getCards(turnPlayer), field.board);

            playPolicyScore(score, play, numMoves, field, playPolicy);

            int recordIndex = searchMove(play, numMoves, move);

            // ここから条件を少しずつ変更
            for (int i = 0; i < 7; i++) {
                {
                    // softmax
                    double tscore[N_MAX_MOVES];
                    memcpy(tscore, score, sizeof(double) * (numMoves + 1));

                    double temp = 0.7 + 0.1 * i;

                    clock.start();
                    SoftmaxSelector<double> selector(tscore, numMoves, temp);
                    if (recordIndex >= 0) {
                        sameProb[0][i][0] += selector.prob(recordIndex);
                    }
                    entropy[0][i][0] += selector.entropy();
                    time[0] += clock.stop();
                }
                for (int j = 0; j < 5; j++) {
                    {
                        // truncated
                        double tscore[N_MAX_MOVES];
                        memcpy(tscore, score, sizeof(double) * (numMoves + 1));

                        double temp = 0.7 + 0.1 * i;
                        double threshold = (4 - j) * 0.01;

                        clock.start();
                        ThresholdSoftmaxSelector<double> selector(tscore, numMoves, temp, threshold);
                        if (recordIndex >= 0) {
                            sameProb[1][i][j] += selector.prob(recordIndex);
                        }
                        entropy[1][i][j] += selector.entropy();
                        time[1] += clock.stop();
                    }
                    {
                        // biased
                        double tscore[N_MAX_MOVES];
                        memcpy(tscore, score, sizeof(double) * (numMoves + 1));

                        double temp = 0.8 + 0.1 * i;
                        double coef = 0.4 - 0.1 * j;
                        double rate = 2; // エンジン設定が変化した場合注意

                        clock.start();
                        BiasedSoftmaxSelector<double> selector(tscore, numMoves, temp, coef, rate);
                        if (recordIndex >= 0) {
                            sameProb[2][i][j] += selector.prob(recordIndex);
                        }
                        entropy[2][i][j] += selector.entropy();
                        time[2] += clock.stop();
                    }
                    {
                        // exp-biased
                        double tscore[N_MAX_MOVES];
                        memcpy(tscore, score, sizeof(double) * (numMoves + 1));

                        double temp = 0.8 + 0.1 * i;
                        double coef = 0.4 - 0.1 * j;
                        double etemp = 1 / log(2);
                        //double etemp = 0.1 * pow(4, j);
                        clock.start();
                        ExpBiasedSoftmaxSelector<double> selector(tscore, numMoves, temp, coef, etemp);
                        if (recordIndex >= 0) {
                            sameProb[3][i][j] += selector.prob(recordIndex);
                        }
                        entropy[3][i][j] += selector.entropy();
                        time[3] += clock.stop();
                    }
                }
            }
            trials += 1;
        }
    }

    cerr << "Softmax Selector : " << time[0] / (trials * 7) << " clock" << endl;
    for (int i = 0; i < 7; i++) {
        cerr << "(" << sameProb[0][i][0] / trials << ", " << entropy[0][i][0] / trials << ") " << endl;
    }
    cerr << "Truncated Softmax Selector : " << time[1] / (trials * 7 * 5) << " clock" << endl;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            cerr << "(" << sameProb[1][i][j] / trials << ", " << entropy[1][i][j] / trials << ") ";
        }
        cerr << endl;
    }
    cerr << "Polynomially Biased Softmax Selector : " << time[2] / (trials * 7 * 5) << " clock" << endl;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            cerr << "(" << sameProb[2][i][j] / trials << ", " << entropy[2][i][j] / trials << ") ";
        }
        cerr << endl;
    }
    cerr << "Exponentially Biased Softmax Selector : " << time[3] / (trials * 7 * 5) << " clock" << endl;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 5; j++) {
            cerr << "(" << sameProb[3][i][j] / trials << ", " << entropy[3][i][j] / trials << ") ";
        }
        cerr << endl;
    }
    return 0;
}

bool PolicyTest(const vector<string>& recordFiles) {
    changePolicy.bin(DIRECTORY_PARAMS_IN + "change_policy.bin");
    playPolicy.bin(DIRECTORY_PARAMS_IN + "play_policy.bin");

    outputParams();

    for (string file : recordFiles) {
        MatchRecord record(file);
        testChangePolicyWithRecord(record);
        testPlayPolicyWithRecord(record);
        testSelector(record);
    }
    return true;
}