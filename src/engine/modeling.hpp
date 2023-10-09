#pragma once

#include "../extra/updator.hpp"
#include "../core/field.hpp"
#include "../core/record.hpp"

const int PLAY_BIAS_FEATURES = 4 + 5;
const int BIAS_FEATURES = PLAY_BIAS_FEATURES + 6;

struct ProbDiffUpdator {
    static constexpr double stats_decay = 1e-4;
    struct ParameterStats { double count, sum; };
    std::vector<ParameterStats> s;

    ProbDiffUpdator(int paramCount) {
        s.resize(paramCount);
        init();
    }

    void init(double _ = 1, double __ = 0, double ___ = 0, double ____ = 0) {
        for (auto& stat : s) {
            stat.sum = 0;
            stat.count = 100;
        }
    }

    void update(double *value, double *prob, int numMoves, int index, const std::vector<std::pair<int, float>> *features, double scale = 1) {
        // 統計情報を更新
        std::vector<double> probDiff(s.size(), 0.0);
        for (int i = 0; i < numMoves; i++) {
            // 特徴量は0,1のみ
            for (auto f : features[i]) probDiff[f.first] += prob[i];
            if (i == index) for (auto f : features[i]) probDiff[f.first] -= 1;
        }

        for (int j = 0; j < (int)s.size(); j++) {
            if (probDiff[j] != 0) {
                s[j].sum -= probDiff[j] * scale;
                s[j].count += scale;

                s[j].sum *= 1 - stats_decay;
                s[j].count *= 1 - stats_decay;

                double m = s[j].sum / s[j].count;
                value[j] = 2.0 * log((1 + m) / (1 - m));
            }
        }
    }
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

struct SharedData;

struct PlayerModel {
    double bias[N_PLAYERS][BIAS_FEATURES];
    std::vector<PlayerModelStats> stats_;
    PlayerModelStats tmpStats;
    std::vector<PlayerModelStats> changeStats_;
    PlayerModelStats tmpChangeStats;

    int games;

    GradientUpdator updator;
    //ProbDiffUpdator updator;

    PlayerModel(): updator(BIAS_FEATURES) { init(); }

    void init() {
        memset(bias, 0, sizeof(double) * N_PLAYERS * BIAS_FEATURES);
        updator.init(1e-3, 0, 1e-4, 1e-5);
        stats_.clear();
        tmpStats = {0};
        changeStats_.clear();
        tmpChangeStats = {0};
        games = 0;
    }
    double playBiasScore(const Field& field, int player, Move move, std::vector<std::pair<int, float>> *v = nullptr) const;
    double changeBiasScore(int player, const Cards cards, const Cards changeCards, std::vector<std::pair<int, float>> *v = nullptr) const;
    void update(const MatchRecord& record, int gameNum, int playerNum, const SharedData& shared, MoveInfo *const buf);
    void updateGame(const GameRecord& record, int playerNum, const SharedData& shared, MoveInfo *const buf, bool computeStats);
    void stats() const;
};