#pragma once

#include "../include.h"

// 機械学習の対象とする部分以外のヒューリスティクス

namespace UECda {
    namespace Heuristics {
        Cards pruneCards(const Cards argCards, int changeQty) {
            Cards tmp = argCards;
            // 絶対に渡さないカードを抜く
            // 8, 2, Joker, D3
            maskCards(&tmp , CARDS_8 | CARDS_2 | CARDS_JOKER | CARDS_D3 );
            // ジョーカーを持っているならS3
            if (containsJOKER(argCards)) {
                maskCards(&tmp, CARDS_S3);
            }
            if ((int)countCards(tmp) < changeQty) {
                // 候補が足りなくなった
                // 富豪の時、除外札のみで構成されていた場合のみ
                // S3がいらない。あとので必勝確定
                assert(changeQty == 1);
                return CARDS_S3;
                
                // めんどいので元のまま返す
                //return argCards;
            }
            return tmp;
        }
        
        template <class field_t>
        int preferRev(const field_t& field,
                        int myPlayerNum, uint32_t rivals) {
            
            int allClassSum = 0;
            int rivalClassSum = 0;
            int numRivals = 0;
            
            for (int p = 0; p < N_PLAYERS; ++p) {
                if (field.isAlive(p) && myPlayerNum != p) {
                    int cl = field.getPlayerClass(p);
                    allClassSum += cl;
                    if (rivals & (1U << p)) {
                        rivalClassSum += cl;
                        ++numRivals;
                    }
                }
            }
            if (numRivals > 0) { // ライバルプレーヤーがもういないときはどうでも良い
                // 革命優先かの判断
                int rivalBetter = (rivalClassSum * field.getNAlivePlayers()
                                    < allClassSum * numRivals) ? 1 : 0;
                
                // オーダー通常 & RPが良い階級　またはその逆の時革命優先
                return (rivalBetter ^ field.getBoard().tmpOrder()) ? 1 : -1;
            }
            return 0;
        }
    }
}