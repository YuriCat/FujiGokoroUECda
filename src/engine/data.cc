#include "data.hpp"

using namespace std;

namespace Settings {
    const double rootPlayPriorCoef = 4;
    const double rootPlayPriorExponent = 0.6;
    
    const double rootChangePriorCoef = 4;
    const double rootChangePriorExponent = 0.6;
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
    for (auto& a : myL2Result) {
        for (auto i : a) cerr << i << " ";
        cerr << endl;
    }
    cerr << "mate result : " << endl;
    for (auto& a : myMateResult) {
        for (auto i : a) cerr << i << " ";
        cerr << endl;
    }
    cerr << endl;

    base_t::closeMatch();
}

void RootAction::clear() {
    move = MOVE_NONE;
    changeCards = CARDS_NULL;
    simulations = 0;
    turnSum = 0;
    for (int p = 0; p < N_PLAYERS; p++) {
        for (int cl = 0; cl < N_CLASSES; cl++) {
            classDistribution[p][cl] = 0;
        }
    }
    monteCarloScore.set(1, 1);
    naiveScore.set(0, 0);
    myScore.set(1, 1);
    rivalScore.set(1, 1);
    policyScore = 0;
    policyProb = -1; // 方策計算に含めないものがあれば自動的に-1になるようにしておく
    pruned = false;
}

string RootAction::toString() const {
    ostringstream oss;
    oss << "size = " << size();
    oss << " mean = " << mean();
    oss << " var = " << var();
    return oss.str();
}

void RootInfo::setCommonInfo(int num, const Field& field, const SharedData& shared, int limSim) {
    actions = candidates = num;
    for (int i = 0; i < actions; i++) {
        monteCarloAllScore += child[i].monteCarloScore;
    }
    myPlayerNum = shared.record.myPlayerNum;
    rivalPlayerNum = -1;
    bestClass = field.bestClass();
    worstClass = field.worstClass();
    bestReward = shared.gameReward[bestClass];
    worstReward = shared.gameReward[worstClass];
    rewardGap = bestReward - worstReward;
    uint32_t rivals = field.getRivalPlayersFlag(myPlayerNum);
    if (popcnt(rivals) == 1) {
        int rnum = bsf(rivals);
        if (field.isAlive(rnum)) {
            rivalPlayerNum = rnum;
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

void RootInfo::prune(int m) {
    // m番目の候補を除外し、空いた位置に末尾の候補を代入
    swap(child[m], child[--candidates]);
    child[candidates].pruned = true;
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
    double n = 0;
    if (isChange) {
        n = Settings::rootChangePriorCoef * pow(double(candidates - 1), Settings::rootChangePriorExponent);
    } else {
        n = Settings::rootPlayPriorCoef * pow(double(candidates - 1), Settings::rootPlayPriorExponent);
    }
    for (int i = 0; i < candidates; i++) {
        double r = (child[i].policyScore - minScore) / (maxScore - minScore);
        child[i].monteCarloScore += BetaDistribution(r, 1 - r) * n;
    }
    for (int i = 0; i < candidates; i++) {
        monteCarloAllScore += child[i].monteCarloScore;
    }
    policyAdded = true;
}

void RootInfo::feedPolicyScore(const double *const score, int num) {
    // 方策関数の出力得点と選択確率を記録
    assert(num <= actions && actions > 0 && num > 0);
    double tscore[N_MAX_MOVES + 64];
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
    int myRew = pshared->gameReward[field.newClassOf(myPlayerNum)];
    ASSERT(0 <= myRew && myRew <= bestReward, cerr << myRew << endl;);
    
    // 自分のシミュレーション結果を分布に変換
    BetaDistribution mySc = BetaDistribution((myRew - worstReward) / (double)rewardGap,
                                             (bestReward - myRew) / (double)rewardGap);
    
#ifdef DEFEAT_RIVAL_MC
    if (rivalPlayerNum < 0) { // 自分の結果だけ考えるとき
        lock();
        child[triedIndex].monteCarloScore += mySc;
        monteCarloAllScore += mySc;
    } else {
        // ライバルの結果も考えるとき
        int rivalRew = field.infoReward[rivalPlayerNum];
        ASSERT(0 <= rivalRew && rivalRew <= bestReward, cerr << rivalRew << endl;);
        
        BetaDistribution rivalSc = BetaDistribution((rivalRew - worstReward) / (double)rewardGap,
                                                    (bestReward - rivalRew) / (double)rewardGap);
        
        constexpr double RIVAL_RATE = 1 / 16.0; // ライバルの結果を重視する割合 0.5 で半々
        
        lock();
        child[triedIndex].myScore += mySc);
        child[triedIndex].rivalScore += rivalSc;
        
        mySc *= 1 - RIVAL_RATE;
        rivalSc.mul(RIVAL_RATE).rev();
        
        child[triedIndex].monteCarloScore += mySc + rivalSc;
        monteCarloAllScore += mySc + rivalSc;
    }
#else
    lock();
    child[triedIndex].monteCarloScore += mySc;
    child[triedIndex].naiveScore += mySc;
    monteCarloAllScore += mySc;
#endif
    
    child[triedIndex].simulations += 1;
    allSimulations += 1;
    
    // 以下参考にする統計量
    child[triedIndex].turnSum += field.turnCount();
    
#ifdef FIXED_N_PLAYOUTS
    if (allSimulations >= FIXED_N_PLAYOUTS) exitFlag = true;
#else
    if (allSimulations >= limitSimulations) exitFlag = true;
#endif
    unlock();
}

void RootInfo::sort() { // 評価が高い順に候補行動をソート
    stable_sort(child.begin(), child.begin() + actions,
                     [&](const RootAction& a, const RootAction& b)->bool{
                         // モンテカルロが同点(またはモンテカルロをやっていない)なら方策の点で選ぶ
                         // ただしルートで方策の点を使わないときにはそうではない
                         // いちおう除外されたものに方策の点がついた場合にはその中で並べ替える
                         if (a.pruned && !b.pruned) return false;
                         if (!a.pruned && b.pruned) return true;
                         if (a.mean() > b.mean()) return true;
                         if (a.mean() < b.mean()) return false;
                         if (a.policyProb > b.policyProb) return true;
                         if (a.policyProb < b.policyProb) return false;
                         return a.policyScore > b.policyScore;
                     });
}

string RootInfo::toString(int num) const {
    if (num == -1)num = actions; // numで表示最大数を設定
    ostringstream oss;
    // 先にソートしておく必要あり
    oss << "Reward Zone [ " << worstReward << " ~ " << bestReward << " ] ";
    oss << allSimulations << " trials." << endl;
    for (int i = 0; i < min(actions, num); i++) {
        const int rg = (int)(child[i].mean() * rewardGap);
        const int rew = rg + worstReward;
        const int nrg = (int)(child[i].naive_mean() * rewardGap);
        const int nrew = nrg + worstReward;
        double sem = sqrt(child[i].mean_var());
        const int rewZone[2] = {rew - (int)(sem * rewardGap), rew + (int)(sem * rewardGap)};
        
        if (i == 0) oss << "\033[1m";
        oss << i << " ";
        
        if (isChange) oss << child[i].changeCards;
        else oss << child[i].move;
        
        oss << " : ";
        
        if (child[i].simulations > 0) {
            // まず総合評価点を表示
            oss << rew << " ( " << rewZone[0] << " ~ " << rewZone[1] << " ) ";
            oss << "{mc: " << nrg << "} ";
            if (rivalPlayerNum >= 0) {
                // 自分とライバルの評価点を表示
                oss << child[i].myScore;
                oss << " [mine = " << (worstReward + (int)(child[i].myScore.mean() * (double)rewardGap)) << "] ";
                oss << child[i].rivalScore;
                oss << " [rival's = ~" << (bestReward - (int)(child[i].rivalScore.mean() * (double)rewardGap)) << "] ";
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
    return oss.str();
}