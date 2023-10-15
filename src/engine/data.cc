#include "data.hpp"
#include "reward.hpp"

using namespace std;

namespace Settings {
    const double rootPriorCoef = 4;
    const double rootPriorExponent = 0.6;
}

void BaseSharedData::closeMatch() {
    // 共通スタッツ表示
    cerr << "Players Stats" << endl;
    for (int p = 0; p < N_PLAYERS; p++) {
        cerr << "Player " << p << " :" << endl;
        cerr << "R*NR 0  1  2  3  4  total" << endl;
        for (int cl = 0; cl < N_PLAYERS; cl++) {
            cerr << "  " << cl << "  ";
            for (int ncl = 0; ncl < N_PLAYERS; ncl++) {
                cerr << classTransition[p][cl][ncl] << "  ";
            }
            cerr << classDestination[p][cl] << endl;
        }
    }
}

void SharedData::closeMatch() {
    // 自己スタッツ表示
    cerr << "My Stats" << endl;
    cerr << "change rejection by server : " << SharedData::changeRejection << endl;
    cerr << "play rejection by server : " << SharedData::playRejection << endl;
    cerr << "L2 result : " << endl;
    cerr << "jwin  " << myL2Result[0][0] << " " << myL2Result[1][0] << endl;
    cerr << "jfail " << myL2Result[0][1] << " " << myL2Result[1][1] << endl;
    cerr << "jlose " << myL2Result[0][2] << " " << myL2Result[1][2] << endl;
    cerr << "jnone " << myL2Result[0][3] << " " << myL2Result[1][3] << endl;
    cerr << "mate result : " << endl;
    for (auto& a : myMateResult) {
        for (auto i : a) cerr << i << " ";
        cerr << endl;
    }
    cerr << endl;
    base_t::closeMatch();
}

string RootAction::toString() const {
    ostringstream oss;
    oss << "size = " << size();
    oss << " mean = " << mean();
    oss << " std = " << std();
    return oss.str();
}

void RootInfo::setCommonInfo(int num, const Field& field, const SharedData& shared, int limSim) {
    candidates = num;
    myPlayerNum = shared.record.myPlayerNum;
    rivalPlayerNum = -1;
    bestReward = shared.gameReward[field.bestClass()];
    worstReward = shared.gameReward[field.worstClass()];
    rewardGap = bestReward - worstReward;
    if (Settings::maximizePosition && shared.record.positionOf(myPlayerNum) < 2) {
        uint32_t rivals = rivalPlayers(field, myPlayerNum);
        if (popcnt(rivals) == 1) {
            int rnum = bsf(rivals);
            if (field.isAlive(rnum)) rivalPlayerNum = rnum;
        }
    }
    limitSimulations = limSim < 0 ? 100000 : limSim;
}
void RootInfo::setChange(const Cards *const a, int num,
                         const Field& field, const SharedData& shared, int limSim) {
    isChange = true;
    for (int i = 0; i < num; i++) child[i].setChange(a[i]);
    setCommonInfo(num, field, shared, limSim);
}
void RootInfo::setPlay(const MoveInfo *const a, int num,
                       const Field& field, const SharedData& shared, int limSim) {
    isChange = false;
    for (int i = 0; i < num; i++) child[i].setPlay(a[i]);
    setCommonInfo(num, field, shared, limSim);
}

void RootInfo::addPolicyScoreToMonteCarloScore() {
    // 方策関数の出力をモンテカルロ結果の事前分布として加算
    // 0 ~ 1 の値にする
    double maxScore = -DBL_MAX, minScore = DBL_MAX;
    for (int i = 0; i < candidates; i++) {
        maxScore = max(maxScore, child[i].policyScore + 0.000001);
        minScore = min(minScore, child[i].policyScore);
    }
    // 初期値として加算
    double n = Settings::rootPriorCoef * pow(double(candidates - 1), Settings::rootPriorExponent);
    for (int i = 0; i < candidates; i++) {
        double ratio = (child[i].policyScore - minScore) / (maxScore - minScore);
        double r = rewardGap * ratio + worstReward;
        child[i].n += n;
        child[i].sum += n * r;
        child[i].sum2 += n * r * r;
        allN += n;
        allSum += n * r;
        allSum2 += n * r * r;
    }
    policyAdded = true;
}

void RootInfo::feedPolicyScore(const double *const score, int num) {
    // 方策関数の出力得点と選択確率を記録
    assert(num <= candidates && candidates > 0 && num > 0);
    double tscore[N_MAX_MOVES];
    for (int i = 0; i < num; i++) {
        child[i].policyScore = tscore[i] = score[i];
    }
    SoftmaxSelector<double> selector(tscore, num, 1);
    for (int i = 0; i < num; i++) {
        child[i].policyProb = selector.prob(i);
    }
}

void RootInfo::feedSimulationResult(int triedIndex, const Field& field, SharedData *const pshared) {
    // シミュレーション結果を記録
    // ロックが必要な演算とローカルでの演算が混ざっているのでこの関数内で排他制御する

    // 新たに得た証拠分布
    BetaDistribution myScore, rivalScore, totalScore;

    double reward = pshared->gameReward[field.newClassOf(myPlayerNum)];
    if (rivalPlayerNum >= 0) {
        double rivalReward = pshared->gameReward[field.newClassOf(rivalPlayerNum)];
        constexpr double RIVAL_RATE = 1 / 16.0; // ライバルの結果を重視する割合 0.5 で半々
        reward = reward * (1 - RIVAL_RATE) + -rivalReward * RIVAL_RATE;
    }

    lock();

    child[triedIndex].simulations += 1;
    child[triedIndex].sum += reward;
    child[triedIndex].sum2 += reward * reward;

    if (rivalPlayerNum >= 0) {
        child[triedIndex].rivalSum += reward;
        child[triedIndex].rivalSum2 += reward * reward;
    }

    allSimulations += 1;
    allSum += reward;
    allSum2 += reward * reward;

    // 参考にする統計量
    child[triedIndex].turnSum += field.turnCount();
    if (allSimulations >= limitSimulations) exitFlag = true;

    unlock();
}

void RootInfo::sort() { // 評価が高い順に候補行動をソート
    stable_sort(child.begin(), child.begin() + candidates,
                [&](const RootAction& a, const RootAction& b)->bool{
                    // モンテカルロが同点(またはモンテカルロをやっていない)なら方策の点で選ぶ
                    // ただしルートで方策の点を使わないときにはそうではない
                    if (a.mean() > b.mean()) return true;
                    if (a.mean() < b.mean()) return false;
                    if (a.policyProb > b.policyProb) return true;
                    if (a.policyProb < b.policyProb) return false;
                    return a.policyScore > b.policyScore;
                });
}

string RootInfo::toString(int num) const {
    if (num == -1) num = candidates; // numで表示最大数を設定
    // 先にソートしておく必要あり
    ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "Reward Zone [ " << worstReward << " ~ " << bestReward << " ] ";
    oss << allSimulations << " trials." << endl;

    for (int i = 0; i < min(candidates, num); i++) {
        double mean = child[i].mean();
        //double nrew = worstReward + child[i].naive_mean() * rewardGap;
        double sem = child[i].sem();

        if (i == 0) oss << "\033[1m";
        oss << i << " ";

        if (isChange) oss << child[i].changeCards;
        else oss << child[i].move;
        oss << " : ";

        if (child[i].simulations > 0) {
            // 総合評価点を表示
            oss << mean << " ( " << (mean - sem) << " ~ " << (mean + sem) << " ) ";
            //oss << "{mc: " <<  << "} ";
            if (rivalPlayerNum >= 0) {
                // 自分とライバルの評価点を表示
                //oss << " [mine = " << int(K * worstReward + child[i].myScore.mean() * rewardGap) << "] ";
                //oss << " [rival = " << int(K * worstReward + child[i].rivalScore.mean() * rewardGap) << "] ";
            }
        }
        oss << "prob = " << child[i].policyProb; // 方策関数の確率出力
        oss << " (pol = " << child[i].policyScore << ") "; // 方策関数のスコア
        if (child[i].simulations > 0) {
            oss << "t = " << child[i].turnSum / (double)child[i].simulations << " "; // 統計量
        }
        oss << child[i].simulations << " trials." << endl;

        if (i == 0) oss << "\033[0m";
    }
    oss << std::defaultfloat << std::setprecision(6);
    return oss.str();
}