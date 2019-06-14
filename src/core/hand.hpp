#pragma once

#include "daifugo.hpp"

/**************************手札表現**************************/

// 手札表現の基本
// 単に集合演算がしたい場合はCards型のみでやるが
// 着手生成や支配、必勝判定等行う場合はこちらが便利

struct Hand {
    Cards cards; // 通常型
    uint32_t qty; // 総枚数
    uint32_t jk; // ジョーカー枚数
    BitCards seq; // 3枚階段型

    CardArray qr; // ランク枚数型
    Cards pqr; // ランク枚数位置型
    Cards sc; // 圧縮型
    Cards nd[2]; // 無支配型(通常、革命)

    uint64_t key; // ハッシュ値

    // 情報を使う早さにより、前後半(keyだけは例外としてその他)に分ける
    // allが付く進行はkeyも含めて更新する

    // 前半
    // cards, qty, jk, seq, qr, pqr
    // 後半
    // sc, nd[2]
    // その他
    // key

    constexpr operator Cards() const { return cards; }
    bool holds(Cards c) const { return cards.holds(c); }
    constexpr bool any() const { return cards.any(); }
    void setKey(uint64_t k) { key = k; }

    void set1stHalf(Cards c, unsigned q) {
        assert(c.count() == q);
        cards = c;
        jk = c.joker();
        qty = q;
        BitCards plain = c.plain();
        qr = CardsToQR(plain);
        seq = polymRanks(plain, jk, 3);
        pqr = QRToPQR(qr);
        assert(exam1stHalf());
    }
    void set(Cards c, unsigned q) {
        // ハッシュ値はsetKeyにて別に設定
        set1stHalf(c, q);
        sc = PQRToSC(pqr);
        PQRToND(pqr, jk, nd);
        assert(exam());
    }
    void setAll(Cards c, unsigned q, uint64_t k) {
        set(c, q);
        setKey(k);
        assert(exam_key());
    }
    void set1stHalf(Cards c) { set1stHalf(c, c.count()); }
    void set(Cards c) { set(c, c.count()); }
    void setAll(Cards c) { setAll(c, c.count(), CardsToHashKey(c)); }
    void init() {
        cards = seq = CARDS_NULL;
        qr = 0ULL;
        pqr = sc = CARDS_NULL;
        qty = jk = 0;
        key = 0ULL;
    }
    void makeMove(Move m);
    void makeMoveAll(Move m);
    void makeMoveAll(Move m, Cards dc, int dq, uint64_t dk);
    void makeMove1stHalf(Move m);
    void makeMove1stHalf(Move m, Cards dc, int dq);

    // カード集合単位(役の形をなしていない)の場合
    void add(Cards dc, int dq) {
        set(cards + dc, qty + dq);
    }
    void add(Cards dc) {
        add(dc, dc.count());
    }
    void addAll(Cards dc, int dq, uint64_t dk) {
        setAll(cards + dc, qty + dq, addCardKey(key, dk));
    }
    void addAll(Cards dc) {
        addAll(dc, dc.count(), CardsToHashKey(dc));
    }

    void subtr(Cards dc, int dq) {
        set(cards - dc, qty - dq);
    }
    void subtr(Cards dc) {
        subtr(dc, dc.count());
    }
    void subtrAll(Cards dc, int dq, uint64_t dk) {
        setAll(cards - dc, qty - dq, subCardKey(key, dk));
    }
    void subtrAll(Cards dc) {
        subtrAll(dc, dc.count(), CardsToHashKey(dc));
    }

    // validator
    // 無視する部分もあるので、その場合は部分ごとにチェックする
    // 当然だが基本のcardsがおかしかったらどうにもならない
    bool exam_cards() const {
        if (!holdsCards(CARDS_ALL, cards)) {
            std::cerr << "Hand : exam_cards() <<" << cards << std::endl;
            return false;
        }
        return true;
    }
    bool exam_key() const {
        if (key != CardsToHashKey(cards)) {
            std::cerr << "Hand : exam_key()" << cards << " <-> ";
            std::cerr << std::hex << key << std::dec << std::endl;
            return false;
        }
        return true;
    }
    bool exam_qty() const {
        if (qty != cards.count()) {
            std::cerr << "Hand : exam_qty() " << cards << " <-> " << qty << std::endl;
            return false;
        }
        return true;
    }
    bool exam_jk() const {
        if (jk != cards.joker()) {
            std::cerr << "Hand : exam_jk()" << std::endl;
            return false;
        }
        return true;
    }
    bool exam_qr() const {
        for (int r = RANK_IMG_MIN; r <= RANK_IMG_MAX; r++) {
            Cards rc = RankToCards(r) & cards;
            uint32_t rq = rc.count();
            uint32_t rqr = qr[r];
            if (rqr != rq) {
                std::cerr << "Hand : exam_qr()" << cards << " -> " << BitArray64<4>(qr) << std::endl;
                return false;
            } // 枚数型があってない
        }
        if (qr & ~CARDS_ALL) {
            std::cerr << "Hand : exam_qr()" << cards << " -> " << BitArray64<4>(qr) << std::endl;
            return false;
        }
        return true;
    }
    bool exam_pqr() const {
        for (int r = RANK_IMG_MIN; r <= RANK_IMG_MAX; r++) {
            Cards rc = RankToCards(r) & cards;
            uint32_t rq = rc.count();
            uint32_t rpqr = pqr[r];
            if (anyCards(rc)) {
                if (1U << (rq - 1) != rpqr) {
                    std::cerr << "Hand : exam_pqr()" << cards << " -> " << pqr << std::endl;
                    return false;
                } // pqrの定義
            } else {
                if (rpqr) {
                    std::cerr << "Hand : exam_pqr()" << cards << " -> " << pqr << std::endl;
                    return false;
                }
            }
        }
        if (pqr & ~CARDS_PLAIN_ALL) {
            std::cerr << "Hand : exam_pqr()" << cards << " -> " << pqr << std::endl;
            return false;
        }
        return true;
    }
    bool exam_sc() const {
        for (int r = RANK_IMG_MIN; r <= RANK_IMG_MAX; r++) {
            Cards rc = RankToCards(r) & cards;
            uint32_t rq = rc.count();
            uint32_t rsc = sc[r];
            if ((1U << rq) - 1U != rsc) {
                std::cerr << "Hand : exam_sc()" << std::endl;
                return false;
            } // scの定義
        }
        if (sc & ~CARDS_PLAIN_ALL) {
            std::cerr << "Hand : exam_sc()" << std::endl;
            return false;
        }
        return true;
    }
    bool exam_seq() const {
        if (seq != polymRanks(cards.plain(), cards.joker(), 3)) {
            std::cerr << "Hand : exam_seq()" << std::endl;
            return false;
        }
        return true;
    }
    bool exam_nd_by_pqr() const {
        // 無支配型をpqrからの変形によって確かめる
        // pqr -> nd は正確と仮定
        Cards tmpnd[2];
        PQRToND(pqr, containsJOKER(cards) ? 1 : 0, tmpnd);
        if (nd[0] != tmpnd[0]) {
            std::cerr << "Hand : exam_nd_by_pqr() nd[0]" << std::endl;
            return false;
        }
        if (nd[1] != tmpnd[1]) {
            std::cerr << "Hand : exam_nd_by_pqr() nd[1]" << std::endl;
            return false;
        }
        return true;
    }
    bool exam_nd() const {
        if (!exam_pqr()) return false;
        if (!exam_nd_by_pqr()) return false;
        return true;
    }

    bool exam1stHalf() const {
        if (!exam_cards()) return false;
        if (!exam_jk()) return false;
        if (!exam_qty()) return false;
        if (!exam_pqr()) return false;
        if (!exam_seq()) return false;
        if (!exam_qr()) return false;
        return true;
    }
    bool exam2ndHalf() const {
        if (!exam_sc()) return false;
        if (!exam_nd()) return false;
        return true;
    }
    bool exam() const {
        if (!exam1stHalf()) return false;
        if (!exam2ndHalf()) return false;
        return true;
    }
    bool examAll() const {
        if (!exam()) return false;
        if (!exam_key()) return false;
        return true;
    }

    std::string toDebugString() const {
        std::ostringstream oss;
        oss << "cards = " << cards << std::endl;
        oss << "qty = " << qty << std::endl;
        oss << "jk = " << jk << std::endl;
        oss << "seq = " << seq << std::endl;
        oss << "qr = " << CardArray(qr) << std::endl;
        oss << "pqr = " << CardArray(pqr) << std::endl;
        oss << "sc = " << CardArray(sc) << std::endl;
        oss << "nd[0] = " << CardArray(nd[0]) << std::endl;
        oss << "nd[1] = " << CardArray(nd[1]) << std::endl;
        oss << std::hex << key << std::dec << std::endl;

        oss << "correct data : " << std::endl;
        Hand tmp;
        tmp.setAll(cards);
        oss << "qty = " << tmp.qty << std::endl;
        oss << "jk = " << tmp.jk << std::endl;
        oss << "seq = " << tmp.seq << std::endl;
        oss << "qr = " << CardArray(tmp.qr) << std::endl;
        oss << "pqr = " << CardArray(tmp.pqr) << std::endl;
        oss << "sc = " << CardArray(tmp.sc) << std::endl;
        oss << "nd[0] = " << CardArray(tmp.nd[0]) << std::endl;
        oss << "nd[1] = " << CardArray(tmp.nd[1]) << std::endl;
        oss << std::hex << tmp.key << std::dec << std::endl;

        return oss.str();
    }
};

inline std::ostream& operator <<(std::ostream& out, const Hand& hand) { // 出力
    out << hand.cards << "(" << hand.qty << ")";
    return out;
}

template <bool HALF = false>
inline void makeMove(const Hand& arg, Hand *const dst, Move m, Cards dc, uint32_t dq) {
    // 普通、パスやカードが0枚になるときはこの関数には入らない。

    // 更新するものは最初にチェック
    if (HALF) assert(arg.exam1stHalf());
    else assert(arg.exam());
    assert(!m.isPASS());
    assert(arg.cards.holds(dc));

    int djk = dc.joker();
    int r = m.rank();

    dst->cards = arg.cards - dc; // 通常型は引けば良い
    dst->qty = arg.qty - dq; // 枚数進行
    dst->jk = arg.jk - djk; // ジョーカー枚数進行

    BitCards plain = dst->cards.plain();
    dst->seq = polymRanks(plain, dst->jk, 3);

    if (!HALF) {
        // 無支配型(共通処理)
        dst->nd[0] = arg.nd[0];
        dst->nd[1] = arg.nd[1];

        if (djk) {
            // ジョーカーが無くなった事で、1枚分ずれる
            // ただしもともと同ランク4枚があった場合にはそこだけ変化しない
            dst->nd[0] = (dst->nd[0] & PQR_234) >> 1;
            dst->nd[1] = (dst->nd[1] & PQR_234) >> 1;

            Cards quad = arg.pqr & PQR_4;
            if (quad) {
                IntCard ic0 = quad.highest();
                dst->nd[0] |= ORQ_NDTable[0][IntCardToRank(ic0) - 1][3];
                IntCard ic1 = quad.lowest();
                dst->nd[1] |= ORQ_NDTable[1][IntCardToRank(ic1) + 1][3];
            }
        }
    }

    if (dc & CARDS_PLAIN_ALL) {
        if (!m.isSeq()) {
            // ジョーカーの存在により少し処理が複雑に
            dq -= djk; // ジョーカーの分引く

            Cards mask = RankToCards(r); // 当該ランクのマスク
            Cards opqr = arg.pqr & mask; // 当該ランクの元々のpqr

            // 枚数型は当該ランクの枚数を引く
            dst->qr = arg.qr - (BitCards(dq) << (r << 2));

            // 枚数位置型、圧縮型は当該ランクのみ枚数分シフト
            // 0枚になったときに、シフトだけでは下のランクにはみ出す事に注意
            dst->pqr = (((arg.pqr & mask) >> dq) & mask) | (arg.pqr & ~mask);

            if (!HALF) {
                BitCards orgsc = arg.sc;
                dst->sc  = (((arg.sc  & mask) >> dq) & mask) | (arg.sc  & ~mask);

                // 無支配型
                if (dst->jk) { // まだジョーカーが残っている
                    // ジョーカーなしに戻す
                    dst->nd[0] &= PQR_234;
                    dst->nd[0] >>= 1;
                    dst->nd[1] &= PQR_234;
                    dst->nd[1] >>= 1;
                }
                BitCards dmask = orgsc ^ dst->sc; // 取り去る分のマスク

                // 通常オーダー
                if (!(opqr & dst->nd[0])) { // 元々そのランクが無支配ゾーンに関係しているので更新の必要あり
                    BitCards dmask0 = dmask & ~dst->nd[0];
                    while (1) {
                        dmask0 >>= 4;
                        dst->nd[0] ^= dmask0;
                        dmask0 &= ~dst->sc; // 現にあるカード集合の分はもう外れない
                        if (!(dmask0 & CARDS_ALL)) break;
                    }
                }
                // 逆転オーダー
                if (!(opqr & dst->nd[1])) { // 元々そのランクが無支配ゾーンに関係しているので更新の必要あり
                    BitCards dmask1 = dmask & ~dst->nd[1];
                    while (1) {
                        dmask1 <<= 4;
                        dst->nd[1] ^= dmask1;
                        dmask1 &= ~dst->sc; // 現にあるカード集合の分はもう外れない
                        if (!(dmask1 & CARDS_ALL)) break;
                    }
                }
                if (dst->jk) {
                    dst->nd[0] <<= 1;
                    dst->nd[0] |= PQR_1;
                    dst->nd[1] <<= 1;
                    dst->nd[1] &= PQR_234;
                    dst->nd[1] |= PQR_1;
                }
            }
        } else {
            // 階段
            Cards mask = RankRangeToCards(r, r + dq - 1); // 当該ランクのマスク
            Cards dqr = dc;

            if (djk) {
                Cards jkmask = RankToCards(m.jokerRank()); // ジョーカーがある場合のマスク
                mask &= ~jkmask; // ジョーカー部分はマスクに関係ないので外す
                dqr = maskJOKER(dqr);
            }

            dqr >>= SuitToSuitNum(m.suits()); // スートの分だけずらすと枚数を表すようになる

            // 枚数型はジョーカー以外の差分を引く
            dst->qr = arg.qr - dqr;
            // 枚数位置型、圧縮型は当該ランクを1枚分シフト
            // ただしグループと違って元々1枚の場合に注意
            dst->pqr = ((arg.pqr & mask & PQR_234) >> 1) | (arg.pqr & ~mask);

            if (!HALF) {
                dst->sc  = ((arg.sc  & mask & PQR_234) >> 1) | (arg.sc & ~mask);
                // めんどいのでその場計算
                PQRToND(dst->pqr, dst->jk, dst->nd);
            }
        }
    } else { // ジョーカーだけ無くなった場合のコピー処理
        dst->qr = arg.qr;
        dst->pqr = arg.pqr;
        if (!HALF) dst->sc = arg.sc;
    }
    if (HALF) assert(dst->exam1stHalf());
    else assert(dst->exam());
}

inline void makeMove(const Hand& arg, Hand *const dst, Move m) {
    makeMove(arg, dst, m, m.cards(), m.qty());
}
inline void makeMove1stHalf(const Hand& arg, Hand *const dst, Move m, Cards dc, int dq) {
    makeMove<true>(arg, dst, m, dc, dq);
}
inline void makeMove1stHalf(const Hand& arg, Hand *const dst, Move m) {
    makeMove1stHalf(arg, dst, m, m.cards(), m.qty());
}
inline void makeMoveAll(const Hand& arg, Hand *const dst, Move m, Cards dc, int dq, uint64_t dk) {
    makeMove(arg, dst, m, dc, dq);
    dst->key = subCardKey(arg.key, dk);
    assert(dst->exam_key());
}
inline void makeMoveAll(const Hand& arg, Hand *const dst, Move m) {
    Cards dc = m.cards();
    makeMoveAll(arg, dst, m, dc, m.qty(), CardsToHashKey(dc));
}
inline void Hand::makeMove1stHalf(Move m, Cards dc, int dq) {
    ::makeMove1stHalf(*this, this, m, dc, dq);
}
inline void Hand::makeMove1stHalf(Move m) {
    ::makeMove1stHalf(*this, this, m);
}
inline void Hand::makeMove(Move m) {
    ::makeMove(*this, this, m);
}
inline void Hand::makeMoveAll(Move m, Cards dc, int dq, uint64_t dk) {
    ::makeMoveAll(*this, this, m, dc, dq, dk);
}
inline void Hand::makeMoveAll(Move m) {
    ::makeMoveAll(*this, this, m);
}