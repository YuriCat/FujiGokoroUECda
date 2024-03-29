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
    if (b.locksSuits(m)) zone &= SuitsToCards(m.suits());

    if (!m.isSeq()) {
        zone &= ORToGValidZone(b.nextOrder(m), m.rank());
        return !canMakeGroup(oc & zone, m.qty() - oc.joker());
    } else {
        zone &= ORQToSCValidZone(b.nextOrder(m), m.rank(), m.qty());
        return !canMakeSeq(oc & zone, oc.joker(), m.qty());
    }
}

// 引数として場を取った場合
// 場を更新してから判定しても支配役の判定ができないので注意
inline bool dominatesCards(const Board b, const Cards oc) {
    if (b.isNull()) return false;

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

inline bool dominatesHand(const Move m, const Hand& oh, const Board b) {
    // Hand型への支配(着手を引数に取る場合)
    // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
    assert(oh.exam_jk());
    assert(oh.exam_seq());
    assert(oh.exam_nd());

    if (m.isPASS()) return false;
    if (m.domInevitably()) return true;
    if (b.domConditionally(m)) return true;

    if (m.isSingleJOKER()) return !containsS3(oh.cards);
    if (!m.isSeq() && m.qty() <= oh.jk) return false;

    if (!m.isSeq()) {
        if (m.qty() > 4) {
            if (N_JOKERS <= 1) return true;
            if (m.qty() > 4 + oh.jk) return true;
            return CardsToFR(oh.cards & ORToGValidZone(b.nextOrder(m), m.rank()));
        }
        int nextOrder = b.nextOrder(m);
        if (!(m.charaPQR() & oh.nd[nextOrder])) return true; // 無支配型と交差なし
        if (b.locksSuits(m)) { // スートロックの場合はまだ支配可能性あり
            Cards zone = ORToGValidZone(nextOrder, m.rank());
            zone &= SuitsToCards(m.suits());
            return !canMakeGroup(oh.cards & zone, m.qty() - oh.jk);
        }
    } else {
        if (!oh.seq) return true;
        Cards zone = ORQToSCValidZone(b.nextOrder(m), m.rank(), m.qty());
        if (b.locksSuits(m)) zone &= SuitsToCards(m.suits());
        return !canMakeSeq(oh.cards & zone, oh.jk, m.qty());
    }
    return false;
}

inline bool dominatesHand(const Board b, const Hand& oh) {
    // Hand型への支配(場を引数に取る場合)
    // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
    assert(oh.exam_jk());
    assert(oh.exam_seq());
    assert(oh.exam_nd());

    if (b.isNull()) return false;

    if (b.isSingleJOKER()) return !containsS3(oh.cards);
    if (!b.isSeq() && b.qty() <= oh.jk) return false;

    if (!b.isSeq()) { // グループ
        if (b.qty() > 4) {
            if (N_JOKERS <= 1) return true;
            if (b.qty() > 4 + oh.jk) return true;
            return CardsToFR(oh.cards & ORToGValidZone(b.order(), b.rank()));
        }
        if (!(b.charaPQR() & oh.nd[b.order()])) return true; // 無支配型と交差なし
        if (b.suitsLocked()) { // スートロックの場合はまだ支配可能性あり
            Cards zone = ORToGValidZone(b.order(), b.rank());
            zone &= SuitsToCards(b.suits());
            return !canMakeGroup(oh.cards & zone, b.qty() - oh.jk);
        }
    } else { // 階段
        if (!oh.seq) return true;
        Cards zone = ORQToSCValidZone(b.order(), b.rank(), b.qty());
        if (b.suitsLocked()) zone &= SuitsToCards(b.suits());
        return !canMakeSeq(oh.cards & zone, oh.jk, b.qty());
    }
    return false;
}