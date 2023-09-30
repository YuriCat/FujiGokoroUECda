#pragma once

#include "../core/field.hpp"
#include "../core/record.hpp"

const int BIAS_FEATURES = 4 + 5;

struct ProbDiffUpdator {
    static constexpr double stats_decay = 1e-3;
    double sum[BIAS_FEATURES];
    double count[BIAS_FEATURES];

    ProbDiffUpdator() { init(); }

    void init() {
        for (int j = 0; j < BIAS_FEATURES; j++) sum[j] = 0;
        for (int j = 0; j < BIAS_FEATURES; j++) count[j] = 10;
    }

    void update(double *value, double *prob, int numMoves, int index, const std::vector<std::pair<int, float>> *features) {
        // 統計情報を更新
        double probDiff[BIAS_FEATURES] = {0};
        for (int i = 0; i < numMoves; i++) {
            // 特徴量は0,1のみ
            for (auto f : features[i]) probDiff[f.first] += prob[i];
            if (i == index) for (auto f : features[i]) probDiff[f.first] -= 1;
        }

        for (int j = 0; j < BIAS_FEATURES; j++) {
            if (probDiff[j] != 0) {
                sum[j] -= probDiff[j];
                count[j] += 1;

                sum[j] *= 1 - stats_decay;
                count[j] *= 1 - stats_decay;

                double m = sum[j] / count[j];
                value[j] = 2.0 * log((1 + m) / (1 - m));
            }
        }
    }
};

struct GradientUpdator {
    static constexpr double lr = 1e-3;
    static constexpr double decay = 3e-4;
    static constexpr double stats_decay = 1e-5;
    double sum[BIAS_FEATURES];
    double sum2[BIAS_FEATURES];
    double count[BIAS_FEATURES];
    double totalCount;

    GradientUpdator() { init(); }

    double var(int i) const { return sum2[i] / totalCount - (sum[i] / totalCount) * (sum[i] / totalCount); }
    double freq(int i) const { return count[i] / totalCount; }

    void init() {
        totalCount = 1;
        for (int j = 0; j < BIAS_FEATURES; j++) sum[j] = 0;
        for (int j = 0; j < BIAS_FEATURES; j++) sum2[j] = count[j] = 1;
    }

    void update(double *value, double *prob, int numMoves, int index, const std::vector<std::pair<int, float>> *features);
};

union PlayerModelStats {
    struct {
        double count[N_PLAYERS];
        double baseEntropy[N_PLAYERS];
        double updatedEntropy[N_PLAYERS];
        double baseCrossEntropy[N_PLAYERS];
        double updatedCrossEntropy[N_PLAYERS];
        double baseAccuracy[N_PLAYERS];
        double updatedAccuracy[N_PLAYERS];
        double KLDivergence[N_PLAYERS];
    };
    std::array<double, 8 * N_PLAYERS> v_; // 加算を簡単にするために
};

struct PlayerModel {
    double bias[N_PLAYERS][BIAS_FEATURES];
    std::vector<PlayerModelStats> stats_;
    PlayerModelStats tmpStats;
    int games;

    GradientUpdator updator;
    //ProbDiffUpdator updator;

    PlayerModel() { init(); }
    void init() {
        memset(bias, 0, sizeof(double) * N_PLAYERS * BIAS_FEATURES);
        updator.init();
        stats_.clear();
        tmpStats = {0};
        games = 0;
    }
    double biasScore(const Field& field, int player, Move move, std::vector<std::pair<int, float>> *v = nullptr) const;
    void update(const MatchRecord& record, int gameNum, int playerNum, const PlayPolicy<policy_value_t>& playPolicy, MoveInfo *const buf);
    void updateGame(const GameRecord& record, int playerNum, const PlayPolicy<policy_value_t>& playPolicy, MoveInfo *const buf, bool computeStats);
    void stats() const;
};