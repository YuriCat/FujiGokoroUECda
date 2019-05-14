#pragma once

#include "../UECda.h"
#include "daifugo.hpp"
#include "hand.hpp"

// 支配性(確実に場を流せるか)の判定

// カード集合の付加情報が計算されていない場合の支配性判定
inline bool dominatesCards(const Move m, const Cards oc, const Board b) {
    // カード集合に対する支配性の判定
    if (m.isPASS()) return false;
    if (m.domInevitably()) return true;
    if (b.domConditionally(m)) return true;

    if (m.isSingleJOKER()) return !containsS3(oc);
    if (!m.isSeq() && m.qty() <= oc.joker()) return false;

    Cards zone = -1;
    if (b.afterSuitsLocked(m)) zone &= SuitsToCards(m.suits());

    if (!m.isSeq()) {
        zone &= ORToGValidZone(b.nextOrder(m), m.rank());
        return !canMakeGroup(oc & zone, m.qty() - oc.joker());
    } else {
        zone &= ORQToSCValidZone(b.nextOrder(m), m.rank(), m.qty());
        return !canMakeSeq(oc & zone, oc.joker(), m.qty());
    }
}

// 引数として場を取った場合
// パスの時は場を更新してから判定しても仕方ないので注意
inline bool dominatesCards(const Board b, const Cards oc) {
    if (b.isNull()) return false;
    if (b.domInevitably()) return true;

    if (b.isSingleJOKER()) return !containsS3(oc);
    if (!b.isSeq() && b.qty() <= oc.joker()) return false;
    
    Cards zone = -1;
    if (b.suitsLocked()) zone &= SuitsToCards(b.suits());

    if (!b.isSeq()) {
        zone &= ORToGValidZone(b.order(), b.rank());
        return !canMakeGroup(oc & zone, b.qty() - oc.joker());
    } else {
        zone &= ORQToSCValidZone(b.order(), b.rank(), b.qty());
        return !canMakeSeq(oc & zone, oc.joker(), b.qty());
    }
}
