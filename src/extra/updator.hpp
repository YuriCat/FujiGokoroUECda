#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>

struct GradientUpdator {
    // settings
    double lr;
    double lr_decay;
    double weight_decay;
    double stats_decay;
    double ent_reg;

    struct ParameterStats { double count, sum, sum2; };
    std::vector<ParameterStats> s;
    double totalCount;

    double lr_, weight_decay_;

    GradientUpdator(int paramCount) {
        s.resize(paramCount);
        init();
    }

    double var(int i) const { return s[i].sum2 / totalCount - (s[i].sum / totalCount) * (s[i].sum / totalCount); }
    double freq(int i) const { return s[i].count / totalCount; }

    void init(double _lr = 3e-5, double _lr_decay = 1e-6, double _weight_decay = 3e-6, double _stats_decay = 0, double _ent_reg = 0) {
        lr = _lr;
        lr_decay = _lr_decay;
        weight_decay = _weight_decay;
        stats_decay = _stats_decay;
        ent_reg = _ent_reg;

        totalCount = 1;
        for (auto& stat : s) stat = {1, 0, 1};
        lr_ = lr;
        weight_decay_ = weight_decay;
    }

    template <typename T>
    void update(T *value, double *prob, int numActions, int index, const std::vector<std::pair<int, float>> *features, double scale = 1) {
        // 統計情報を更新
        for (int i = 0; i < numActions; i++) {
            for (auto f : features[i]) {
                s[f.first].sum += f.second;
                s[f.first].sum2 += f.second * f.second;
                s[f.first].count += 1;
            }
            totalCount += 1;
        }
        // TODO: stats decay
        if (stats_decay != 0) {
            for (auto& stat : s) {
                stat.sum *= 1 - stats_decay;
                stat.sum2 *= 1 - stats_decay;
                stat.count *= 1 - stats_decay;
            }
            totalCount *= 1 - stats_decay;
        }

        // 選ばれた手の確率を上げ、全体の手の確率を下げる
        double ent = 0;
        if (ent_reg != 0) for (int i = 0; i < numActions; i++) ent += -prob[i] * log2(prob[i]);
        for (int i = 0; i < numActions; i++) {
            for (auto f : features[i]) {
                double diff = i == index ? (1 - prob[i]) : -prob[i];
                double v = value[f.first];
                v += lr_ * scale * diff / (1e-3 + var(f.first)) * f.second;
                if (ent_reg != 0) v += lr_ * scale * ent_reg * prob[i] * (-log2(prob[i]) - ent);
                if (weight_decay_ != 0) v *= pow(1 - weight_decay_, 1 / (1e-3 + freq(i)));
                value[f.first] = v;
            }
        }
        lr_ *= 1 - lr_decay;
        weight_decay_ *= 1 - lr_decay;
    }
};

struct GradientUpdatorStats {
    unsigned long long count, correct;
    double ent, cent;

    GradientUpdatorStats() { clear(); }
    void clear() { count = correct = ent = cent = 0; }
    void update(double *prob, int num, int index) {
        count += 1;
        correct += (std::max_element(prob, prob + num) - prob) == index;
        cent += -log2(prob[index]);
        for (int i = 0; i < num; i++) ent += -prob[i] * log2(prob[i]);
    }
    void stats() {
        std::cerr << "acc: " << correct / (double)count
        << " (" << correct << " / " << count << ")" << " ent: " << ent / count << " cent: " << cent / count << std::endl;
    }
};