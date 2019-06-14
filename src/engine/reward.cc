#include "reward.hpp"
#include "../UECda.h"

using namespace std;

// 順位遷移確率の基本モデル
const float standardTransition[5][5] = {
    0.568095,  0.243926,  0.0994531, 0.0488888, 0.0396379,
    0.291119,  0.356628,  0.185829,  0.098451,  0.0679728,
    0.0871187, 0.190049,  0.298731,  0.241864,  0.182237,
    0.0293166, 0.113268,  0.235138,  0.328354,  0.293923,
    0.0243511, 0.0961298, 0.180848,  0.282442,  0.416229,
};

vector<array<double, N_PLAYERS>> standardReward(int games) {
    // 階級のみ考慮するシンプルなモデル
    vector<array<double, N_PLAYERS>> reward(games); // 状態価値 (残り試合 x 階級)
    reward[0].fill(0);
    for (int g = 0; g < games; g++) {
        if (g > 0) {
            // 遷移価値計算
            for (int i = 0; i < N_PLAYERS; i++) { // この試合の順位
                for (int j = 0; j < N_PLAYERS; j++) { // 次の試合の順位
                    // 以降の試合の相対報酬を加算
                    reward[g][i] += standardTransition[i][j] * reward[g - 1][j];
                }
            }
        }
        // 順位価値加算
        for (int i = 0; i < N_PLAYERS; i++) { // この試合の順位
            reward[g][i] = REWARD(i) + reward[g][i];
        }
        // 最下位が0になるように全体をずらす
        for (int i = 0; i < N_PLAYERS; i++) { // この試合の順位
            reward[g][i] -= reward[g][N_PLAYERS - 1];
        }
    }
    return reward;
}

uint32_t rivalPlayers(const Field& field, int playerNum) {
    // ライバルプレーヤー集合を得る
    uint32_t ret = 0U;
    int best = 99999;
    for (int p = 0; p < N_PLAYERS; p++) {
        if (p == playerNum) continue;
        int pos = field.positionOf(p);
        if (pos < best) {
            ret = 1U << p;
            best = pos;
        } else if (pos == best) {
            ret |= 1U << p;
        }
    }
    assert(ret != 0U);
    return ret;
}

int positionPreferRevolution(const Field& field, int playerNum) {

    uint32_t rivals = rivalPlayers(field, rivals);
    int allClassSum = 0;
    int rivalClassSum = 0;
    int numRivals = 0;
    
    for (int p = 0; p < N_PLAYERS; p++) {
        if (field.isAlive(p) && playerNum != p) {
            int cl = field.classOf(p);
            allClassSum += cl;
            if (rivals & (1U << p)) {
                rivalClassSum += cl;
                numRivals++;
            }
        }
    }
    if (numRivals > 0) { // ライバルプレーヤーがもういないときはどうでも良い
        // 革命優先かの判断
        int rivalBetter = (rivalClassSum * field.numPlayersAlive()
                           < allClassSum * numRivals) ? 1 : 0;
        
        // オーダー通常 & RPが良い階級　またはその逆の時革命優先
        return (rivalBetter ^ field.board.order()) ? 1 : -1;
    }
    return 0;
}