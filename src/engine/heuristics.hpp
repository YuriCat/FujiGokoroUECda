#pragma once

// 機械学習の対象とする部分以外のヒューリスティクス

namespace Heuristics {
    int preferRev(const Field& field,
                  int myPlayerNum, uint32_t rivals) {
        
        int allClassSum = 0;
        int rivalClassSum = 0;
        int numRivals = 0;
        
        for (int p = 0; p < N_PLAYERS; p++) {
            if (field.isAlive(p) && myPlayerNum != p) {
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
            int rivalBetter = (rivalClassSum * field.getNAlivePlayers()
                               < allClassSum * numRivals) ? 1 : 0;
            
            // オーダー通常 & RPが良い階級　またはその逆の時革命優先
            return (rivalBetter ^ field.board.order()) ? 1 : -1;
        }
        return 0;
    }
}