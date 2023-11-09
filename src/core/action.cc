#include "action.hpp"

// ぱおーん氏、beersong.cのコードをベースにしている部分があります

// 交換生成

int genChange(Cards *const pc0, const Cards myCards, const int changeQty) {
    Cards *pc = pc0;
    Cards tmp = myCards;
    if (changeQty == 1) {
        while (tmp.any()) *(pc++) = tmp.popLowestCard();
    } else if (changeQty == 2) {
        while (tmp.any()) {
            Cards c = tmp.popLowestCard();
            Cards tmp2 = tmp;
            while (tmp2.any()) *(pc++) = c | tmp2.popLowestCard();
        }
    }
    return pc - pc0;
}

// 階段生成

#define GEN_BASE(q, oc, op) {\
for (IntCard ic : (oc)) {\
int r = IntCardToRank(ic);\
unsigned s = IntCardToSuits(ic);\
mv->setSeq(q, r, s);\
op; mv++; }}

#define GEN(q, oc) GEN_BASE(q, oc,)
#define GEN_J(q, oc, jdr) GEN_BASE(q, oc, mv->setJokerRank(r + jdr); mv->setJokerSuits(s))

int genAllSeqWithJoker(Move *const mv0, const Cards x) {
    Cards c = maskJOKER(x);
    if (!c) return 0;
    Move *mv = mv0;

    // 3枚階段
    const Cards c1_1 = polymJump(c);
    const Cards c2 = polymRanks<2>(c);
    const Cards c3 = c1_1 & c2;
    // プレーンを作成
    const Cards seq3 = c3;
    GEN(3, seq3);
    // 1_1型を作成
    const Cards seq1_1 = c1_1 & ~seq3;
    GEN_J(3, seq1_1, 1);
    // 2型を両側で作成
    const Cards seq2_ = c2 & ~seq3;
    const Cards seq_2 = (c2 >> 4) & ~seq3;
    GEN_J(3, seq_2, 0);
    GEN_J(3, seq2_, 2);

    // 4枚階段
    const Cards c2_1 = c2 & (c1_1 >> 4);
    const Cards c1_2 = c1_1 & (c2 >> 8);
    const Cards c4 = c3 & c2_1;
    // プレーンを作成
    const Cards seq4 = c4;
    GEN(4, seq4);
    // 2_1型と1_2型を作成
    const Cards seq2_1 = c2_1 & ~seq4;
    const Cards seq1_2 = c1_2 & ~seq4;
    GEN_J(4, seq2_1, 2);
    GEN_J(4, seq1_2, 1);
    // 3型を両側で作成
    const Cards seq3_ = c3 & ~seq4;
    const Cards seq_3 = (c3 >> 4) & ~seq4;
    GEN_J(4, seq_3, 0);
    GEN_J(4, seq3_, 3);

    // 5枚階段
    const Cards c3_1 = c3 & (c2_1 >> 4);
    const Cards c1_3 = c1_2 & (c3 >> 8);
    const Cards c2_2 = c2_1 & (c1_2 >> 4);
    const Cards c5 = c4 & c3_1;
    // プレーンを作成
    const Cards seq5 = c5;
    GEN(5, seq5);
    // 2_2型を作成
    const Cards seq2_2 = c2_2 & ~seq5;
    GEN_J(5, seq2_2, 2);
    // 3_1型と1_3型を作成
    const Cards seq3_1 = c3_1 & ~seq5;
    const Cards seq1_3 = c1_3 & ~seq5;
    GEN_J(5, seq3_1, 3);
    GEN_J(5, seq1_3, 1);
    // 4型を両側で作成
    const Cards seq4_ = c4 & ~seq5;
    const Cards seq_4 = (c4 >> 4) & ~seq5;
    GEN_J(5, seq_4, 0);
    GEN_J(5, seq4_, 4);

    // 6枚階段
    if (c3) {
        const Cards c4_1 = c4 & (c3_1 >> 4);
        const Cards c1_4 = c1_3 & (c4 >> 8);
        const Cards c3_2 = c3_1 & (c2_2 >> 4);
        const Cards c2_3 = c2_2 & (c1_3 >> 4);
        const Cards c6 = c5 & c4_1;
        // プレーンを作成
        const Cards seq6 = c6;
        GEN(6, seq6);
        // 3_2型と2_3型を作成
        const Cards seq3_2 = c3_2 & ~seq6;
        const Cards seq2_3 = c2_3 & ~seq6;
        GEN_J(6, seq3_2, 3);
        GEN_J(6, seq2_3, 2);
        // 4_1型と1_4型を作成
        const Cards seq4_1 = c4_1 & ~seq6;
        const Cards seq1_4 = c1_4 & ~seq6;
        GEN_J(6, seq4_1, 4);
        GEN_J(6, seq1_4, 1);
        // 5型を両側で作成
        const Cards seq5_ = c5 & ~seq6;
        const Cards seq_5 = (c5 >> 4) & ~seq6;
        GEN_J(6, seq_5, 0);
        GEN_J(6, seq5_, 5);
    }

    // 7枚以上階段は未実装
    return mv - mv0;
}

int genAllPlainSeq(Move *const mv0, const Cards x) {
    // ジョーカーを入れない階段のみ生成
    // 生成ランク制限無し
    Move *mv = mv0;
    Cards c = polymRanks<3>(x);
    for (int q = 3; anyCards(c); q++) {
        for (IntCard ic : c) {
            int rank = IntCardToRank(ic);
            unsigned suit = IntCardToSuits(ic);
            (mv++)->setSeq(q, rank, suit);
        }
        // 次の枚数を考える
        c = polymRanks<2>(c); // ランク a かつランク a + 1 の n 枚階段があればランク a の n + 1 枚階段が出来る
    }
    return mv - mv0;
}

int genAllSeq(Move *const mv, const Cards c) {
    if (!containsJOKER(c)) return genAllPlainSeq(mv, c);
    else return genAllSeqWithJoker(mv, c);
}

int genFollowPlainSeq(Move *const mv0, Cards c, Board b) {
    int r = b.rank();
    unsigned q = b.qty();
    assert(q >= 3);
    if (b.order() == 0) { // 通常
        if (r + (q << 1) > RANK_MAX + 1) return 0;
        c &= RankRangeToCards(r + q, RANK_MAX);
    } else { // オーダー逆転中
        if (r < RANK_MIN + q) return 0;
        c &= RankRangeToCards(RANK_MIN, r - 1);
    }
    c = polymRanks<3>(c);
    if (!c) return 0;
    if (q > 3) c = polymRanks(c, q - 3 + 1);
    if (!c) return 0;
    if (b.suitsLocked()) c &= SuitsToCards(b.suits());
    Move *mv = mv0;
    for (IntCard ic : c) {
        int r = IntCardToRank(ic);
        unsigned s = IntCardToSuits(ic);
        (mv++)->setSeq(q, r, s);
    }
    return mv - mv0;
}

int genFollowSeqWithJoker(Move *const mv0, const Cards x, const Board b) {
    unsigned r = b.rank();
    unsigned qty = b.qty();
    assert(qty >= 3);
    Cards c = x.plain();
    Cards rCards; // 合法カードゾーン
    Cards validSeqZone; // 合法階段ゾーン
    if (b.order() == 0) { // 通常
        if (r + (qty << 1) > RANK_MAX + 2) return 0;
        rCards = RankRangeToCards(r + qty, RANK_MAX + 1);
        validSeqZone = RankRangeToCards(r + qty, RANK_MAX + 1);
    } else { // オーダー逆転中
        if (r < qty + RANK_MIN - 1) return 0;
        rCards = RankRangeToCards(RANK_MIN - 1, r - 1);
        validSeqZone = RankRangeToCards(RANK_MIN - 1, r - qty);
    }
    c &= rCards;
    if (b.suitsLocked()) c &= SuitsToCards(b.suits());
    if (!c) return 0;
    Move *mv = mv0;
    switch (qty) {
        case 0: break;
        case 1: break;
        case 2: break;
        case 3: {
            const Cards c1_1 = polymJump(c);
            const Cards c2 = polymRanks<2>(c);

            const Cards c3 = c1_1 & c2;

            // プレーンを作成
            const Cards seq3 = c3 & validSeqZone;
            GEN(3, seq3);
            // 1_1型を作成
            const Cards seq1_1 = c1_1 & validSeqZone & ~seq3;
            GEN_J(3, seq1_1, 1);
            // 2型を両側で作成
            const Cards seq2_ = c2 & validSeqZone & ~seq3;
            const Cards seq_2 = (c2 >> 4) & validSeqZone & ~seq3;
            GEN_J(3, seq_2, 0);
            GEN_J(3, seq2_, 2);
        } break;
        case 4: {
            const Cards c1_1 = polymJump(c);
            const Cards c2 = polymRanks<2>(c);

            const Cards c3 = c1_1 & c2;
            const Cards c2_1 = c2 & (c1_1 >> 4);
            const Cards c1_2 = c1_1 & (c2 >> 8);

            const Cards c4 = c3 & c2_1;

            // プレーンを作成
            const Cards seq4 = c4 & validSeqZone;
            GEN(4, seq4);
            // 2_1型と1_2型を作成
            const Cards seq2_1 = c2_1 & validSeqZone & ~seq4;
            const Cards seq1_2 = c1_2 & validSeqZone & ~seq4;
            GEN_J(4, seq2_1, 2);
            GEN_J(4, seq1_2, 1);
            // 3型を両側で作成
            const Cards seq3_ = c3 & validSeqZone & ~seq4;
            const Cards seq_3 = (c3 >> 4) & validSeqZone & ~seq4;
            GEN_J(4, seq3_, 3);
            GEN_J(4, seq_3, 0);
        } break;
        case 5: {
            const Cards c1_1 = polymJump(c);
            const Cards c2 = polymRanks<2>(c);

            const Cards c3 = c1_1 & c2;

            if (!c3) break;

            const Cards c2_1 = c2 & (c1_1 >> 4);
            const Cards c1_2 = c1_1 & (c2 >> 8);

            const Cards c4 = c3 & c2_1;
            const Cards c3_1 = c3 & (c2_1 >> 4);
            const Cards c1_3 = c1_2 & (c3 >> 8);
            const Cards c2_2 = c2_1 & (c1_2 >> 4);

            const Cards c5 = c4 & c3_1;

            // プレーンを作成
            const Cards seq5 = c5 & validSeqZone;
            GEN(5, seq5);
            // 2_2型を作成
            const Cards seq2_2 = c2_2 & validSeqZone & ~seq5;
            GEN_J(5, seq2_2, 2);
            // 3_1型と1_3型を作成
            const Cards seq3_1 = c3_1 & validSeqZone & ~seq5;
            const Cards seq1_3 = c1_3 & validSeqZone & ~seq5;
            GEN_J(5, seq3_1, 3);
            GEN_J(5, seq1_3, 1);
            // 4型を両側で作成
            const Cards seq4_ = c4 & validSeqZone & ~seq5;
            const Cards seq_4 = (c4 >> 4) & validSeqZone & ~seq5;
            GEN_J(5, seq_4, 0);
            GEN_J(5, seq4_, 4);
        } break;
        default: break; // 6枚以上階段は未実装
    }
    return mv - mv0;
}

#undef GEN_BASE
#undef GEN
#undef GEN_J

int genFollowSeq(Move *const mv, const Cards c, const Board b) {
    if (!containsJOKER(c)) return genFollowPlainSeq(mv, c, b);
    else return genFollowSeqWithJoker(mv, c, b);
}

// フォローグループ生成

inline BitCards genNRankInGenFollowGroup(BitCards c, BitCards valid, int q) {
    BitCards vqr = CardsToQR(c) & valid;
    if (q == 4) return vqr & PQR_3;
    return QRToNR(vqr, q);
}

int genFollowSingle(Move *const mv0, const Cards myCards, const Board b) {
    Cards c = myCards;
    Move *mv = mv0;
    if (b.isSingleJOKER()) {
        if (containsS3(c)) {
            mv->setSingle(INTCARD_S3);
            return 1;
        }
        return 0;
    }
    if (containsJOKER(c)) {
        (mv++)->setSingleJOKER();
        c = c.plain();
    }
    if (b.suitsLocked()) c &= SuitsToCards(b.suits());
    if (c) {
        int br = b.rank();
        if (b.order() == 0) { // normal order
            c &= RankRangeToCards(br + 1, RANK_MAX);
        } else { // reversed
            c &= RankRangeToCards(RANK_MIN, br - 1);
        }
        for (IntCard ic : c) (mv++)->setSingle(ic);
    }
    return mv - mv0;
}

#define GEN_BASE(q, s, op) { mv->setGroup(q, r, s); op; mv++; }
#define GEN(q, s) GEN_BASE(q, s,)
#define GEN_J(q, s, js) GEN_BASE(q, s, mv->setJokerSuits(js))

int genFollowDouble(Move *const mv0, const Cards c, const Board b) {
    const int br = b.rank();
    Cards valid;
    if (b.order() == 0) { // 通常
        valid = RankRangeToCards(br + 1, RANK_MAX);
    } else { // オーダー逆転中
        valid = RankRangeToCards(RANK_MIN, br - 1);
    }
    Move *mv = mv0;
    if (!b.suitsLocked()) { // スートしばりなし
        Cards c4 = genNRankInGenFollowGroup(c, valid, 4);
        // 4枚ある箇所から各スートで生成
        for (IntCard ic : c4) {
            int r = IntCardToRank(ic);
            GEN(2, SUITS_CD);
            GEN(2, SUITS_CH);
            GEN(2, SUITS_CS);
            GEN(2, SUITS_DH);
            GEN(2, SUITS_DS);
            GEN(2, SUITS_HS);
        }
        Cards c3 = genNRankInGenFollowGroup(c, valid, 3);
        Cards c2 = genNRankInGenFollowGroup(c, valid, 2);
        if (!containsJOKER(c)) {
            // 3枚だけの箇所を生成
            for (IntCard ic : c3) {
                int r = IntCardToRank(ic);
                unsigned suits = c[r];
                unsigned s0 = lsb(suits);
                unsigned s1 = lsb(suits - s0);
                unsigned s2 = suits - s0 - s1;

                GEN(2, s0 | s1);
                GEN(2, s1 | s2);
                GEN(2, s2 | s0);
            }
            // 2枚だけの箇所を生成
            for (IntCard ic : c2) {
                int r = IntCardToRank(ic);
                GEN(2, c[r]);
            }
        } else {
            // ジョーカーあり
            for (IntCard ic : c3) {
                int r = IntCardToRank(ic);
                unsigned suits = c[r];
                unsigned s0 = lsb(suits);
                unsigned s1 = lsb(suits - s0);
                unsigned s2 = suits - s0 - s1;

                // プレーン
                GEN(2, s0 | s1);
                GEN(2, s1 | s2);
                GEN(2, s2 | s0);

                // ジョーカー使用
                unsigned isuits = SUITS_ALL - suits;
                GEN_J(2, s0 | isuits, isuits);
                GEN_J(2, s1 | isuits, isuits);
                GEN_J(2, s2 | isuits, isuits);
            }
            for (IntCard ic : c2) {
                int r = IntCardToRank(ic);
                unsigned suits = c[r];
                unsigned s0 = lsb(suits);
                unsigned s1 = suits - s0;
                unsigned isuits = SUITS_ALL - suits;
                unsigned is0 = lsb(isuits);
                unsigned is1 = isuits - is0;

                // プレーン
                GEN(2, suits);

                // ジョーカー使用
                GEN_J(2, s0 | is0, is0);
                GEN_J(2, s0 | is1, is1);
                GEN_J(2, s1 | is0, is0);
                GEN_J(2, s1 | is1, is1);
            }
            // 1枚だけの箇所を生成
            Cards c1 = genNRankInGenFollowGroup(c, valid, 1);
            for (IntCard ic : c1) {
                int r = IntCardToRank(ic);
                unsigned suits = c[r];
                unsigned isuits = SUITS_ALL - suits;
                unsigned is0 = lsb(isuits);
                unsigned is1 = lsb(isuits - is0);
                unsigned is2 = isuits - is0 - is1;
                // ジョーカー使用
                GEN_J(2, suits | is0, is0);
                GEN_J(2, suits | is1, is1);
                GEN_J(2, suits | is2, is2);
            }
        }
    } else { // スートしばりあり
        const Cards vc = c | SuitsToCards(SUITS_ALL - b.suits());
        // ランクごとに4ビット全て立っているか判定
        Cards c4 = genNRankInGenFollowGroup(vc, valid, 4);
        for (IntCard ic : c4) {
            int r = IntCardToRank(ic);
            GEN(2, b.suits());
        }
        if (containsJOKER(c)) {
            // 3ビット立っている部分からジョーカーを利用して生成
            Cards c3 = genNRankInGenFollowGroup(vc, valid, 3);
            for (IntCard ic : c3) {
                int r = IntCardToRank(ic);
                unsigned suit = c[r] & b.suits();
                GEN_J(2, b.suits(), b.suits() - suit);
            }
        }
    }
    return mv - mv0;
}

int genFollowTriple(Move *const mv0, const Cards c, const Board b) {
    Cards valid;
    if (b.order() == 0) { // 通常
        valid = RankRangeToCards(b.rank() + 1, RANK_MAX);
    } else { // オーダー逆転中
        valid = RankRangeToCards(RANK_MIN, b.rank() - 1);
    }
    Move *mv = mv0;

    if (!b.suitsLocked()) { // スートしばりなし
        Cards c4 = genNRankInGenFollowGroup(c, valid, 4);
        // 4枚ある箇所から各スートで生成
        for (IntCard ic : c4) {
            int r = IntCardToRank(ic);
            for (unsigned s = 1; s < (1 << N_SUITS); s <<= 1) {
                GEN(3, SUITS_ALL - s);
            }
        }
        Cards c3 = genNRankInGenFollowGroup(c, valid, 3);
        if (!containsJOKER(c)) {
            // ジョーカーなし スートロックなし
            // 3枚だけの箇所を生成
            for (IntCard ic : c3) {
                int r = IntCardToRank(ic);
                GEN(3, c[r]);
            }
        } else {
            // ジョーカーあり スートロックなし
            // 3枚だけの箇所を生成
            for (IntCard ic : c3) {
                int r = IntCardToRank(ic);
                unsigned suits = c[r];
                // プレーン
                GEN(3, suits);
                // ジョーカー使用
                unsigned isuits = SUITS_ALL - suits;
                for (unsigned s = 1; s < (1 << N_SUITS); s <<= 1) {
                    if (s != isuits) {
                        unsigned is = SUITS_ALL - s;
                        GEN_J(3, is, isuits);
                    }
                }
            }
            // 2枚だけの箇所を生成
            Cards c2 = genNRankInGenFollowGroup(c, valid, 2);
            for (IntCard ic : c2) {
                int r = IntCardToRank(ic);
                unsigned suits = c[r];
                unsigned isuits = SUITS_ALL - suits;
                unsigned is0 = lsb(isuits);
                unsigned is1 = isuits - is0;
                // ジョーカー使用
                GEN_J(3, suits | is0, is0);
                GEN_J(3, suits | is1, is1);
            }
        }
    } else { // スートしばりあり
        // virtual cardを加えて4枚ある箇所から生成
        const Cards vc = c | SuitsToCards(SUITS_ALL - b.suits());
        // ランクごとに4ビット全て立っているか判定
        Cards c4 = genNRankInGenFollowGroup(vc, valid, 4);
        // ジョーカーあり スートロックあり
        // virtual cardを加えて4枚ある箇所からプレーンで生成
        for (IntCard ic : c4) {
            int r = IntCardToRank(ic);
            GEN(3, b.suits());
        }
        if (containsJOKER(c)) {
            // 3枚だけの箇所をジョーカー込みで生成
            Cards c3 = genNRankInGenFollowGroup(vc, valid, 3);
            for (IntCard ic : c3) {
                int r = IntCardToRank(ic);
                GEN_J(3, b.suits(), SUITS_ALL & ~vc[r]);
            }
        }
    }
    return mv - mv0;
}

int genFollowQuadruple(Move *const mv0, const Cards c, const Board b) {
    Move *mv = mv0;
    const int br = b.rank();
    Cards valid;
    if (b.order() == 0) { // 通常
        valid = RankRangeToCards(br + 1, RANK_MAX);
    } else { // オーダー逆転中
        valid = RankRangeToCards(RANK_MIN, br - 1);
    }
    Cards c4 = genNRankInGenFollowGroup(c, valid, 4);
    for (IntCard ic : c4) {
        int r = IntCardToRank(ic);
        GEN(4, SUITS_CDHS);
    }
    if (containsJOKER(c)) {
        Cards c3 = genNRankInGenFollowGroup(c, valid, 3);
        for (IntCard ic : c3) {
            int r = IntCardToRank(ic);
            GEN_J(4, SUITS_CDHS, Cards(~c)[r]);
        }
    }
    return mv - mv0;
}

int genAllSingle(Move *const mv0, Cards c) {
    Move *mv = mv0;
    if (c.joker()) {
        (mv++)->setSingleJOKER();
        c = c.plain();
    }
    for (IntCard ic : c) (mv++)->setSingle(ic);
    return mv - mv0;
}

int genLead(Move *const mv0, const Cards c) {
    bool jk = containsJOKER(c) ? true : false;
    Move *mv = mv0 + genAllSingle(mv0, c); // シングルはここで生成
    Cards x;
    if (jk) {
        x = maskJOKER(c);
    } else {
        x = BitCards(CardsToQR(c)) & PQR_234;
    }
    while (x) {
        int r = IntCardToRank(pickIntCardLow(x));
        switch (c[r]) {
            case 0: assert(0); break;
            case 1: {
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 2);
                GEN_J(2, 5, 4);
                GEN_J(2, 9, 8);
            } break;
            case 2: {
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 1);
                GEN_J(2, 6, 4);
                GEN_J(2, 10, 8);
            } break;
            case 3: {
                GEN(2, 3);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 5, 4);
                GEN_J(2, 9, 8);
                GEN_J(2, 6, 4);
                GEN_J(2, 10, 8);
                GEN_J(3, 7, 4);
                GEN_J(3, 11, 8);
            } break;
            case 4: {
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 5, 1);
                GEN_J(2, 6, 2);
                GEN_J(2, 12, 8);
            } break;
            case 5: {
                GEN(2, 5);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 2);
                GEN_J(2, 9, 8);
                GEN_J(2, 6, 2);
                GEN_J(2, 12, 8);
                GEN_J(3, 7, 2);
                GEN_J(3, 13, 8);
            } break;
            case 6: {
                GEN(2, 6);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 1);
                GEN_J(2, 10, 8);
                GEN_J(2, 5, 1);
                GEN_J(2, 12, 8);
                GEN_J(3, 7, 1);
                GEN_J(3, 14, 8);
            } break;
            case 7: {
                GEN(2, 3);
                GEN(2, 5);
                GEN(2, 6);
                GEN(3, 7);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 9, 8);
                GEN_J(2, 10, 8);
                GEN_J(2, 12, 8);
                GEN_J(3, 11, 8);
                GEN_J(3, 13, 8);
                GEN_J(3, 14, 8);
                GEN_J(4, 15, 8);
            } break;
            case 8: {
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 9, 1);
                GEN_J(2, 10, 2);
                GEN_J(2, 12, 4);
            } break;
            case 9: {
                GEN(2, 9);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 2);
                GEN_J(2, 5, 4);
                GEN_J(2, 10, 2);
                GEN_J(2, 12, 4);
                GEN_J(3, 11, 2);
                GEN_J(3, 13, 4);
            } break;
            case 10: {
                GEN(2, 10);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 1);
                GEN_J(2, 6, 4);
                GEN_J(2, 9, 1);
                GEN_J(2, 12, 4);
                GEN_J(3, 11, 1);
                GEN_J(3, 14, 4);
            } break;
            case 11: {
                GEN(2, 3);
                GEN(2, 9);
                GEN(2, 10);
                GEN(3, 11);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 5, 4);
                GEN_J(2, 6, 4);
                GEN_J(2, 12, 4);
                GEN_J(3, 7, 4);
                GEN_J(3, 13, 4);
                GEN_J(3, 14, 4);
                GEN_J(4, 15, 4);
            } break;
            case 12: {
                GEN(2, 12);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 5, 1);
                GEN_J(2, 6, 2);
                GEN_J(2, 9, 1);
                GEN_J(2, 10, 2);
                GEN_J(3, 13, 1);
                GEN_J(3, 14, 2);
            } break;
            case 13: {
                GEN(2, 5);
                GEN(2, 9);
                GEN(2, 12);
                GEN(3, 13);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 2);
                GEN_J(2, 6, 2);
                GEN_J(2, 10, 2);
                GEN_J(3, 7, 2);
                GEN_J(3, 11, 2);
                GEN_J(3, 14, 2);
                GEN_J(4, 15, 2);
            } break;
            case 14: {
                GEN(2, 6);
                GEN(2, 10);
                GEN(2, 12);
                GEN(3, 14);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(2, 3, 1);
                GEN_J(2, 5, 1);
                GEN_J(2, 9, 1);
                GEN_J(3, 7, 1);
                GEN_J(3, 11, 1);
                GEN_J(3, 13, 1);
                GEN_J(4, 15, 1);
            } break;
            case 15: {
                GEN(2, 3);
                GEN(2, 5);
                GEN(2, 6);
                GEN(2, 9);
                GEN(2, 10);
                GEN(2, 12);
                GEN(3, 7);
                GEN(3, 11);
                GEN(3, 13);
                GEN(3, 14);
                GEN(4, 15);
                if (!jk) break; // ジョーカーがなければ飛ばす
                GEN_J(5, 15, 15);
            } break;
            default: assert(0); break;
        }
        x = maskCards(x, RankToCards(r));
    }
    mv += genAllSeq(mv, c); // 階段を生成
    return mv - mv0;
}

int genLegalLead(Move *const mv0, const Cards c) {
    int cnt = genLead(mv0, c);
    (mv0 + cnt)->setPASS();
    return cnt + 1;
}

#undef GEN_BASE
#undef GEN
#undef GEN_J

// 特別着手生成
// 通常の着手生成では無視する着手のうち、
// クライアントの手としては検討するものを生成

int genJokerGroup(Move *const mv0, Cards c, const Cards ops, const Board b) {
    // ジョーカーを使わずに作る事が出来る着手を
    // ジョーカーによって出す着手
    // 8か最強ランクであり、
    // 本来そのランクが無くなるはずの着手についてのみ検討する

    assert(containsJOKER(c));
    assert(containsS3(ops));
    assert(b.isGroup() && b.qty() > 1);

    Move *mv = mv0;
    if (b.order() == 0) c &= RankRangeToCards(b.rank() + 1, RANK_MAX);
    else c &= RankRangeToCards(RANK_MIN , b.rank() - 1);

    for (int r = RANK_3; r <= RANK_2; r++) {
        unsigned x = c[r];
        if (b.suitsLocked() && b.suits() != x) continue;
        int q = countSuits(x);
        if (q <= 1 || q != b.qty()) continue;
        if (r != RANK_8) {
            // 相手が返せるかどうかチェック
            if ((b.order() ^ int(x == 15)) == 0) {
                if (!holdsCards(RankRangeToCards(RANK_3, r), ops)) continue;
            } else {
                if (!holdsCards(RankRangeToCards(r, RANK_2), ops)) continue;
            }
        }
        for (unsigned jks = 1; jks < (1 << N_SUITS); jks <<= 1) {
            if (jks & x) {
                mv->setGroup(q, r, x);
                mv->setJokerSuits(jks);
                mv++;
            }
        }
    }
    return mv - mv0;
}

int genGroupDebug(Move *const mv0, Cards c, Board b) {
    // 速度は問わず、全ての合法着手を生成
    Move *mv = mv0;
    int st = RANK_MIN, ed = RANK_MAX;
    if (!b.isNull()) {
        if (b.order() == 0) st = b.rank() + 1;
        else ed = b.rank() - 1;
    }
    for (int r = st; r <= ed; r++) {
        for (unsigned s = 1; s <= SUITS_ALL; s++) {
            if (b.isNull() || b.suits() == s) {
                int q = countSuits(s);
                if (holdsCards(c, RankSuitsToCards(r, s))) {
                    (mv++)->setGroup(q, r, s);
                } else if (containsJOKER(c)) {
                    for (unsigned js = 1; s < (1 << N_SUITS); s <<= 1) {
                        if ((s & js) && holdsCards(c, RankSuitsToCards(r, s - js))) {
                            mv->setGroup(q, r, s);
                            mv->setJokerSuits(js);
                            mv++;
                        }
                    }
                }
            }
        }
    }
    return mv - mv0;
}

int genFollowExceptPASS(Move *const mv, const Cards c, const Board b) {
    // 返り値は、生成した手の数
    if (!b.isSeq()) {
        int cnt;
        switch (b.qty()) {
            case 0: cnt = 0; break;
            case 1: cnt = genFollowSingle(mv, c, b); break;
            case 2: cnt = genFollowDouble(mv, c, b); break;
            case 3: cnt = genFollowTriple(mv, c, b); break;
            case 4: cnt = genFollowQuadruple(mv, c, b); break;
            default: cnt = 0; break;
        }
        return cnt;
    } else return genFollowSeq(mv, c, b);
}