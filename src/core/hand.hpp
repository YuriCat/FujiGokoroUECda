#pragma once

#include "daifugo.hpp"
#include "hash.hpp"

namespace UECda {
    
    /**************************手札表現**************************/
    
    // 手札表現の基本
    // 単に集合演算がしたい場合はCards型のみでやるが
    // 着手生成や支配、必勝判定等行う場合はこちらが便利
    // 用途によってどの部分を更新するかは要指定
    // 枚数指定をミスると最後まで改善されないので注意
    
    // TODO: pqr の算出は qr からの計算の方が高速と思ったが要検証
    
    class Hand {
    public:
        Cards cards; // 通常型
        uint32_t qty; // 総枚数
        uint32_t jk; // ジョーカー枚数
        Cards seq; // 3枚階段型
        
        CardArray qr; // ランク枚数型
        Cards pqr; // ランク枚数位置型
        Cards sc; // 圧縮型
        Cards nd[2]; // 無支配型(通常、革命)

        uint64_t hash; // ハッシュ値
        
        // 情報を使う早さにより、前後半(ハッシュ値だけは例外としてその他)に分ける。
        // allが付く進行はhash値も含めて更新する。
        
        // 前半
        // cards, qty, jk, seq, qr, pqr
        // 後半
        // sc, nd[2]
        // その他
        // hash
        
        // (未実装)必勝判定のとき
        // cards, qty, jk, seq, pqrを更新
        // qrは使わなさそう?
        
        constexpr operator Cards() const { return cards; }
        
        constexpr uint64_t containsJOKER() const {
            return UECda::containsJOKER(cards);
        }
        constexpr uint64_t containsS3() const {
            return UECda::containsS3(cards);
        }
        constexpr uint64_t containsSeq() const {
            return seq;
        }
        
        bool holds(Cards ac) const {
            return holdsCards(cards, ac);
        }
        bool holds(const Hand& ah) const {
            return holdsCards(cards, ah.cards);
        }
        constexpr bool any() const { return cards.any(); }
        void setHash(uint64_t h) { hash = h; }
        
        void set(const Cards ac, const uint32_t aq) {
            // 手札をセット
            // ハッシュ値はsetHashにて別に設定
            assert(aq > 0); assert(ac.count() == aq);

            cards = ac;
            jk = ac.joker();
            qty = aq;

            Cards plain = ac.plain();
            seq = polymRanks(plain, jk, 3);
            qr = CardsToQR(plain);
            pqr = QRToPQR(qr);
            sc = PQRToSC(pqr);
            PQRToND(pqr, jk, nd);

            assert(exam());
        }
        void set(const Cards ac) { set(ac, countCards(ac)); }
        
        void set1stHalf(const Cards ac, const uint32_t aq) {
            // 手札をセット 前半のみ
            assert(countCards(ac) == aq);
            
            cards = ac;
            jk = ac.joker();
            qty = aq;
            Cards plain = maskJOKER(ac);
            qr = CardsToQR(plain);
            seq = polymRanks(plain, jk, 3);
            pqr = QRToPQR(qr);
            
            assert(exam1stHalf());
        }
        void set1stHalf(const Cards ac) { set1stHalf(ac, countCards(ac)); }
        void setAll(const Cards ac, const uint32_t aq, const uint64_t ahash) {
            set(ac, aq);
            setHash(ahash);
            assert(exam_hash());
        }
        void setAll(const Cards ac, const uint32_t aq) {
            set(ac, aq);
            setAll(ac, aq, CardsToHashKey(ac));
        }
        void setAll(const Cards ac) { setAll(ac, countCards(ac)); }
        void init() {
            cards = seq = CARDS_NULL;
            qr = 0ULL;
            pqr = sc = CARDS_NULL;
            qty = jk = 0;
            hash = 0ULL;
        }
        
        void makeMove1stHalf(const Move mv) { makeMove1stHalf(mv, mv.cards()); }
        void makeMove1stHalf(const Move mv, const Cards dc) { makeMove1stHalf(mv, dc, mv.qty()); }
        void makeMove1stHalf(const Move mv, const Cards dc, uint32_t dq) {
            
            // 更新するものは最初にチェック
            assert(exam1stHalf());
            assert(!mv.isPASS());
            assert(holds(dc));
            
            uint32_t djk = UECda::containsJOKER(dc) ? 1 : 0;
            uint32_t r = mv.rank();
            
            cards -= dc; // 通常型は引けば良い
            qty -= dq; // 枚数進行
            jk -= djk; // ジョーカー枚数進行
            
            Cards plain = maskJOKER(cards);
            seq = polymRanks(plain, jk, 3);
            
            if (dc != CARDS_JOKER) { // ジョーカーだけ無くなった場合はこれで終わり
                if (!mv.isSeq()) {
                    // グループ系統
                    // ジョーカーの存在により少し処理が複雑に
                    dq -= djk; // ジョーカーの分引く
                    Cards mask = RankToCards(r); // 当該ランクのマスク
                    
                    // 枚数型は当該ランクの枚数を引く
                    qr = qr.data() - (BitCards(dq) << (r << 2));
                    
                    // 枚数位置型、圧縮型は当該ランクのみ枚数分シフト
                    pqr = (((pqr & mask) >> dq) & mask) | (pqr & ~mask);
                } else {
                    // 階段
                    Cards mask = RankRangeToCards(r, r + dq - 1); // 当該ランクのマスク
                    
                    Cards dqr = dc;
                    
                    Cards jkmask;
                    
                    if (djk) {
                        jkmask = RankToCards(mv.jokerRank()); // ジョーカーがある場合のマスク
                        mask &= ~jkmask; // ジョーカー部分はマスクに関係ないので外す
                        dqr = maskJOKER(dqr);
                    }
                    dqr >>= SuitToSuitNum(mv.suits()); // スートの分だけずらすと枚数を表すようになる
                    
                    // 枚数型はジョーカー以外の差分を引く
                    qr = qr.data() - dqr;

                    // 枚数位置型、圧縮型は当該ランクを1枚分シフト
                    // ただしグループと違って元々1枚の場合に注意
                    pqr = ((pqr & mask & PQR_234) >> 1) | (pqr & ~mask);
                }
            }
            assert(exam1stHalf());
        }
        
        void makeMove(const Move mv) { makeMove(mv, mv.cards()); }
        void makeMove(const Move mv, const Cards dc) { makeMove(mv, dc, mv.qty()); }
        void makeMove(const Move mv, const Cards dc, uint32_t dq) {
            // 更新するものは最初にチェック
            assert(exam());
            assert(!mv.isPASS());
            ASSERT(holds(dc), cerr << "Hand::makeMove : unholded making-move. " << dc << " from " << cards << endl; );
            
            int djk = UECda::containsJOKER(dc) ? 1 : 0;
            int r = mv.rank();
            
            cards -= dc; // 通常型は引けば良い
            qty -= dq; // 枚数進行
            jk -= djk; // ジョーカー枚数進行
            
            Cards plain = cards.plain();
            seq = polymRanks(plain, jk, 3);

            // 無支配型(共通処理)
            if (djk) {
                // ジョーカーが無くなった事で、1枚分ずれる
                // ただしもともと同ランク4枚があった場合にはそこだけ変化しない
                nd[0] = (nd[0] & PQR_234) >> 1;
                nd[1] = (nd[1] & PQR_234) >> 1;
                
                Cards quad = pqr & PQR_4;
                if (quad) {
                    IntCard ic0 = quad.highest();
                    nd[0] |= ORQ_NDTable[0][IntCardToRank(ic0) - 1][3];
                    IntCard ic1 = quad.lowest();
                    nd[1] |= ORQ_NDTable[1][IntCardToRank(ic1) + 1][3];
                }
            }
            
            Cards orgsc = sc;
            
            if (dc != CARDS_JOKER) { // ジョーカーだけ無くなった場合はこれで終わり
                if (!mv.isSeq()) {
                    
                    // ジョーカーの存在により少し処理が複雑に
                    dq -= djk; // ジョーカーの分引く
                    
                    Cards mask = RankToCards(r); // 当該ランクのマスク
                    Cards opqr = pqr & mask; // 当該ランクの元々のpqr
                    
                    // 枚数型は当該ランクの枚数を引く
                    qr = qr.data() - (BitCards(dq) << (r << 2));
                    
                    // 枚数位置型、圧縮型は当該ランクのみ枚数分シフト
                    // 0枚になったときに、シフトだけでは下のランクにはみ出す事に注意
                    pqr = (((pqr & mask) >> dq) & mask) | (pqr & ~mask);
                    sc  = (((sc  & mask) >> dq) & mask) | (sc  & ~mask);
                    
                    // 無支配型
                    if (jk) { // まだジョーカーが残っている
                        //dmask=(dmask<<1) & PQR_234;//1枚上げる
                        // ジョーカーなしに戻す
                        nd[0] &= PQR_234;
                        nd[0] >>= 1;
                        nd[1] &= PQR_234;
                        nd[1] >>= 1;
                    }
                    Cards dmask = orgsc ^ sc; // 取り去る分のマスク
                    
                    // 通常オーダー
                    if (!(opqr & nd[0])) { // 元々そのランクが無支配ゾーンに関係しているので更新の必要あり
                        Cards dmask0 = dmask & ~nd[0];
                        while (1) {
                            dmask0 >>= 4;
                            nd[0] ^= dmask0;
                            dmask0 &= ~sc; // 現にあるカード集合の分はもう外れない
                            if (!(dmask0 & CARDS_ALL)) break;
                        }
                    }
                    // 逆転オーダー
                    if (!(opqr & nd[1])) { // 元々そのランクが無支配ゾーンに関係しているので更新の必要あり
                        Cards dmask1 = dmask & ~nd[1];
                        while (1) {
                            dmask1 <<= 4;
                            nd[1] ^= dmask1;
                            dmask1 &= ~sc; // 現にあるカード集合の分はもう外れない
                            if (!(dmask1 & CARDS_ALL)) break;
                        }
                    }
                    
                    if (jk) {
                        nd[0] <<= 1;
                        nd[0] |= PQR_1;
                        nd[1] <<= 1;
                        nd[1] &= PQR_234;
                        nd[1] |= PQR_1;
                    }
                    
                } else {
                    // 階段
                    Cards mask = RankRangeToCards(r, r + dq - 1); // 当該ランクのマスク
                    Cards dqr = dc;

                    if (djk) {
                        Cards jkmask = RankToCards(mv.jokerRank()); // ジョーカーがある場合のマスク
                        mask &= ~jkmask; // ジョーカー部分はマスクに関係ないので外す
                        dqr = maskJOKER(dqr);
                    }
                    dqr >>= SuitToSuitNum(mv.suits()); // スートの分だけずらすと枚数を表すようになる
                    
                    //枚数型はジョーカー以外の差分を引く
                    qr = qr.data() - dqr;
                    // 枚数位置型、圧縮型は当該ランクを1枚分シフト
                    // ただしグループと違って元々1枚の場合に注意
                    pqr = ((pqr & mask & PQR_234) >> 1) | (pqr & ~mask);
                    sc  = ((sc  & mask & PQR_234) >> 1) | (sc  & ~mask);
                    // 差分計算で高速化可能かもしれないが、現在はその場計算
                    PQRToND(pqr, jk, nd);
                }
            }
            assert(exam());
        }
        void makeMoveAll(Move mv, Cards dc, uint32_t dq, uint64_t dhash) {
            makeMove(mv, dc, dq);
            hash ^= dhash;
            assert(exam_hash());
        }
        void makeMoveAll(Move mv, Cards dc, uint64_t dhash) {
            makeMoveAll(mv, dc, mv.qty(), dhash);
        }
        void makeMoveAll(Move mv, Cards dc) {
            makeMoveAll(mv, dc, CardsToHashKey(dc));
        }
        void makeMoveAll(Move mv) {
            makeMoveAll(mv, mv.cards());
        }
        
        void unmakeMove(const Move mv, const Cards dc, uint32_t dq) {
            // カードが増えない時は入らない
            // 更新するものは最初にチェック
            assert(exam());
            assert(!mv.isPASS());
            ASSERT(isExclusiveCards(cards, dc),
                   cerr << "Hand::unmakeMove : inclusive unmaking-move. " << dc << " to " << cards << endl; );
            
            int djk = mv.containsJOKER() ? 1 : 0;
            int r = mv.rank();
            
            cards += dc; // 通常型は足せば良い
            qty += dq; // 枚数進行
            
            assert(cards != CARDS_NULL);
            assert(qty > 0);
            
            Cards plain = cards.plain();
            jk = cards.joker();
            seq = polymRanks(plain, jk, 3);

            // 無支配型(共通処理)
            if (djk) {
                // ジョーカーが増えた事で、1枚分ずれる
                // 1枚のところは全て無支配
                nd[0] = ((nd[0] & PQR_123) << 1) | PQR_1;
                nd[1] = ((nd[1] & PQR_123) << 1) | PQR_1;
            }

            if (dc != CARDS_JOKER) { // ジョーカーだけ増えた場合はこれで終わり
                if (!mv.isSeq()) {
                    // ジョーカーの存在により少し処理が複雑に
                    dq -= djk; // ジョーカーの分引く
                    
                    Cards mask = RankToCards(r); // 当該ランクのマスク
                    
                    // 枚数型は当該ランクの枚数を足す
                    qr = qr.data() + (BitCards(dq) << (r << 2));
                    uint32_t nq = qr[r]; // 当該ランクの新しい枚数
                    
                    // 枚数位置型、圧縮型ともに新しい枚数に入れ替える
                    pqr = ((Cards(1U << (nq - 1U))) << (r << 2)) | (pqr & ~mask);
                    sc |= Cards((1U << nq) -1U) << (r << 2);
                    
                    Cards npqr = pqr & mask; // 当該ランクの新しいpqr
                    
                    // 無支配型
                    // グループは増やすのが簡単(ただしテーブル参照あり)
                    if (jk) {
                        // ジョーカーありの場合には1ビット枚数を増やして判定
                        if (npqr & PQR_4) {
                            npqr = (npqr & PQR_4) | ((npqr & PQR_123) << 1);
                        } else {
                            npqr <<= 1;
                        }
                        nq += jk;
                    }
                    
                    // 通常オーダー
                    if (!(npqr & nd[0])) { // 増分が無支配ゾーンに関係するので更新の必要あり
                        nd[0] |= ORQ_NDTable[0][r - 1][nq - 1];
                    }
                    // 逆転オーダー
                    if (!(npqr & nd[1])) { // 増分が無支配ゾーンに関係するので更新の必要あり
                        nd[1] |= ORQ_NDTable[1][r + 1][nq - 1];
                    }
                    
                } else {
                    // 階段
                    Cards mask = RankRangeToCards(r, r + dq - 1); // 当該ランクのマスク
                    Cards dqr = dc >> SuitToSuitNum(mv.suits()); // スートの分だけずらすと枚数を表すようになる
    
                    if (djk) {
                        Cards jkmask = RankToCards(mv.jokerRank()); // ジョーカーがある場合のマスク
                        mask ^= jkmask; // ジョーカー部分はマスクに関係ないので外す
                        dqr &= ~jkmask;
                    }
                    
                    // 枚数型はジョーカー以外の差分を足す
                    qr = qr.data() + dqr;
                    
                    // 枚数位置型は当該ランクを1枚分シフトし、元々無かった所には1枚をうめる
                    // 圧縮型は当該ランクを1枚分シフトし、1枚のところを埋める
                    pqr = ((pqr & mask) << 1) | (~sc & mask & PQR_1) | (pqr & ~mask); // 昔のscを使う
                    sc |= ((sc  & mask) << 1) | (mask & PQR_1);
                    PQRToND(pqr, jk, nd);
                }
            }
            assert(exam());
        }
        
        void unmakeMove(Move mv) {
            Cards dc = mv.cards();
            unmakeMove(mv, dc);
        }
        void unmakeMove(Move mv, Cards dc) {
            unmakeMove(mv, dc, mv.qty());
        }
        void unmakeMoveAll(Move mv) {
            unmakeMoveAll(mv, mv.cards());
        }
        void unmakeMoveAll(Move mv, Cards dc) {
            unmakeMoveAll(mv, dc, CardsToHashKey(dc));
        }
        void unmakeMoveAll(Move mv, Cards dc, uint64_t dhash) {
            unmakeMove(mv, dc);
            hash ^= dhash;
            assert(exam_hash());
        }
        
        // カード集合単位(役の形をなしていない)の場合
        void add(const Cards dc, const int dq) {
            set(addCards(cards, dc), qty + dq);
        }
        void add(const Cards dc) {
            add(dc, countCards(dc));
        }
        void addAll(const Cards dc, const int dq, const uint64_t dhash) {
            setAll(addCards(cards, dc), qty + dq, hash ^ dhash);
        }
        void addAll(const Cards dc, const int dq) {
            addAll(dc, dq, CardsToHashKey(dc));
        }
        void addAll(const Cards dc) {
            addAll(dc, countCards(dc), CardsToHashKey(dc));
        }
        
        void subtr(const Cards dc, const int dq) {
            set(subtrCards(cards, dc), qty - dq);
        }
        void subtr(const Cards dc) {
            subtr(dc, countCards(dc));
        }
        void subtrAll(const Cards dc, const int dq, const uint64_t dhash) {
            setAll(subtrCards(cards, dc), qty - dq, hash ^ dhash);
        }
        void subtrAll(const Cards dc, const int dq) {
            subtrAll(dc, dq, CardsToHashKey(dc));
        }
        void subtrAll(const Cards dc) {
            subtrAll(dc, countCards(dc));
        }
        
        // validator
        // 無視する部分もあるので、その場合は部分ごとにチェックする
        // 当然だが基本のcardsがおかしかったらどうにもならない
        bool exam_cards() const {
            if (!holdsCards(CARDS_ALL, cards)) {
                cerr << "Hand : exam_cards() <<" << cards << endl;
                return false;
            }
            return true;
        }
        bool exam_hash() const {
            if (hash != CardsToHashKey(cards)) {
                cerr << "Hand : exam_hash()" << cards << " <-> ";
                cerr << std::hex << hash << std::dec << endl;
                return false;
            }
            return true;
        }
        bool exam_qty() const {
            if (qty != count()) {
                cerr << "Hand : exam_qty() " << cards << " <-> " << qty << endl;
                return false;
            }
            return true;
        }
        bool exam_jk() const {
            if (jk != (uint32_t)countCards(cards & CARDS_JOKER)) {
                cerr << "Hand : exam_jk()" << endl;
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
                    cerr << "Hand : exam_qr()" << cards << " -> " << BitArray64<4>(qr) << endl;
                    return false;
                } // 枚数型があってない
            }
            if (qr & ~CARDS_ALL) {
                cerr << "Hand : exam_qr()" << cards << " -> " << BitArray64<4>(qr) << endl;
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
                        cerr << "Hand : exam_pqr()" << cards << " -> " << pqr << endl;
                        return false;
                    } // pqrの定義
                } else {
                    if (rpqr) {
                        cerr << "Hand : exam_pqr()" << cards << " -> " << pqr << endl;
                        return false;
                    }
                }
            }
            if (pqr & ~CARDS_ALL_PLAIN) {
                cerr << "Hand : exam_pqr()" << cards << " -> " << pqr << endl;
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
                    cerr << "Hand : exam_sc()" << endl;
                    return false;
                } // scの定義
            }
            if (sc & ~CARDS_ALL_PLAIN) {
                cerr << "Hand : exam_sc()" << endl;
                return false;
            }
            return true;
        }
        bool exam_seq() const {
            if (seq != polymRanks(cards.plain(), cards.joker(), 3)) {
                cerr << "Hand : exam_seq()" << endl;
                return false;
            }
            return true;
        }
        bool exam_nd_by_pqr() const {
            // 無支配型をpqrからの変形によって確かめる
            // pqr -> nd は正確と仮定
            Cards tmpnd[2];
            PQRToND(pqr, UECda::containsJOKER(cards) ? 1 : 0, tmpnd);
            if (nd[0] != tmpnd[0]) {
                cerr << "Hand : exam_nd_by_pqr() nd[0]" << endl;
                return false;
            }
            if (nd[1] != tmpnd[1]) {
                cerr << "Hand : exam_nd_by_pqr() nd[1]" << endl;
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
            if (!exam_hash()) return false;
            return true;
        }
        
        std::string toDebugString() const {
            std::ostringstream oss;
            oss << "cards = " << cards << endl;
            oss << "qty = " << qty << endl;
            oss << "jk = " << jk << endl;
            oss << "seq = " << seq << endl;
            oss << "qr = " << CardArray(qr) << endl;
            oss << "pqr = " << CardArray(pqr) << endl;
            oss << "sc = " << CardArray(sc) << endl;
            oss << "nd[0] = " << CardArray(nd[0]) << endl;
            oss << "nd[1] = " << CardArray(nd[1]) << endl;
            oss << std::hex << hash << std::dec << endl;
            
            oss << "correct data : " << endl;
            Hand tmp;
            tmp.setAll(cards);
            oss << "qty = " << tmp.qty << endl;
            oss << "jk = " << tmp.jk << endl;
            oss << "seq = " << tmp.seq << endl;
            oss << "qr = " << CardArray(tmp.qr) << endl;
            oss << "pqr = " << CardArray(tmp.pqr) << endl;
            oss << "sc = " << CardArray(tmp.sc) << endl;
            oss << "nd[0] = " << CardArray(tmp.nd[0]) << endl;
            oss << "nd[1] = " << CardArray(tmp.nd[1]) << endl;
            oss << std::hex << tmp.hash << std::dec << endl;
            
            return oss.str();
        }
                            
    private:
        int count() const { return countCards(cards); }
                            
    };
    
    inline ostream& operator <<(ostream& out, const Hand& hand) { // 出力
        out << hand.cards << "(" << hand.qty << ")";
        return out;
    }
                            
    inline Cards containsJOKER(const Hand& hand) { return containsJOKER(hand.cards); }
    inline Cards containsS3(const Hand& hand) { return containsS3(hand.cards); }
    
    // 別インスタンスに手札進行
    inline void makeMove1stHalf(const Hand& arg, Hand *const dst, Move mv, Cards dc, uint32_t dq) {
        // 更新するものは最初にチェック
        assert(arg.exam1stHalf());
        assert(!mv.isPASS());
        ASSERT(arg.holds(dc), cerr << arg << dc << endl; );
        
        uint32_t djk = UECda::containsJOKER(dc) ? 1 : 0;
        uint32_t r = mv.rank();
        
        dst->cards = subtrCards(arg.cards, dc); // 通常型は引けば良い
        dst->qty = arg.qty - dq; // 枚数進行
        dst->jk = arg.jk - djk; // ジョーカー枚数進行
        
        Cards plain = dst->cards.plain();
        dst->seq = polymRanks(plain, dst->jk, 3);
        
        if (dc != CARDS_JOKER) { // ジョーカーだけ無くなった場合はこれで終わり
            if (!mv.isSeq()) {
                // グループ系統
                // ジョーカーの存在により少し処理が複雑に
                dq -= djk; // ジョーカーの分引く
                Cards mask = RankToCards(r); // 当該ランクのマスク
                
                // 枚数型は当該ランクの枚数を引く
                dst->qr = arg.qr - (((Cards)(dq)) << (r << 2));
                
                // 枚数位置型、圧縮型は当該ランクのみ枚数分シフト
                dst->pqr = (((arg.pqr & mask) >> dq) & mask) | (arg.pqr & ~mask);
            } else {
                // 階段
                Cards mask = RankRangeToCards(r, r + dq - 1); // 当該ランクのマスク
                Cards dqr = dc;
                
                if (djk) {
                    Cards jkmask = RankToCards(mv.jokerRank()); // ジョーカーがある場合のマスク
                    mask &= ~jkmask; // ジョーカー部分はマスクに関係ないので外す
                    dqr = maskJOKER(dqr);
                }
                dqr >>= SuitToSuitNum(mv.suits()); // スートの分だけずらすと枚数を表すようにな
                // 枚数型はジョーカー以外の差分を引く
                dst->qr = arg.qr - dqr;
                
                // 枚数位置型、圧縮型は当該ランクを1枚分シフト
                // ただしグループと違って元々1枚の場合に注意
                dst->pqr = ((arg.pqr & mask & PQR_234) >> 1) | (arg.pqr & ~mask);
            }
        } else {
            // ジョーカーだけ無くなった場合のコピー処理
            dst->qr = arg.qr;
            dst->pqr = arg.pqr;
        }
        assert(dst->exam1stHalf());
    }
    
    inline void makeMove1stHalf(const Hand& arg, Hand *const dst, const Move mv, const Cards dc) {
        makeMove1stHalf(arg, dst, mv, dc, mv.qty());
    }
    inline void makeMove1stHalf(const Hand& arg, Hand *const dst, const Move mv) {
        makeMove1stHalf(arg, dst, mv, mv.cards());
    }
    
    inline void makeMove(const Hand& arg, Hand *const dst, const Move mv, const Cards dc, uint32_t dq) {
        // 普通、パスやカードが0枚になるときはこの関数には入らない。
        
        // 更新するものは最初にチェック
        assert(arg.exam());
        assert(!mv.isPASS());
        assert(arg.holds(dc));
        
        int djk = UECda::containsJOKER(dc) ? 1 : 0;
        int r = mv.rank();
        
        dst->cards = subtrCards(arg.cards, dc); // 通常型は引けば良い
        dst->qty = arg.qty - dq; // 枚数進行
        dst->jk = arg.jk - djk; // ジョーカー枚数進行
        
        Cards plain = maskJOKER(dst->cards);
        dst->seq = polymRanks(plain, dst->jk, 3);
        
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
        
        Cards orgsc = arg.sc;
        
        if (dc != CARDS_JOKER) {
            if (!mv.isSeq()) {
                // ジョーカーの存在により少し処理が複雑に
                dq -= djk; // ジョーカーの分引く
                
                Cards mask = RankToCards(r); // 当該ランクのマスク
                Cards opqr = arg.pqr & mask; // 当該ランクの元々のpqr
                
                // 枚数型は当該ランクの枚数を引く
                dst->qr = arg.qr - (((Cards)(dq)) << (r << 2));
                
                // 枚数位置型、圧縮型は当該ランクのみ枚数分シフト
                // 0枚になったときに、シフトだけでは下のランクにはみ出す事に注意
                dst->pqr = (((arg.pqr & mask) >> dq) & mask) | (arg.pqr & ~mask);
                dst->sc  = (((arg.sc  & mask) >> dq) & mask) | (arg.sc  & ~mask);
                
                // 無支配型
                if (dst->jk) { // まだジョーカーが残っている
                    //dmask=(dmask<<1) & PQR_234; // 1枚上げる
                    // ジョーカーなしに戻す
                    dst->nd[0] &= PQR_234;
                    dst->nd[0] >>= 1;
                    dst->nd[1] &= PQR_234;
                    dst->nd[1] >>= 1;
                }
                Cards dmask = orgsc ^ dst->sc; // 取り去る分のマスク
                
                // 通常オーダー
                if (!(opqr & dst->nd[0])) { // 元々そのランクが無支配ゾーンに関係しているので更新の必要あり
                    Cards dmask0 = dmask & ~dst->nd[0];
                    while (1) {
                        dmask0 >>= 4;
                        dst->nd[0] ^= dmask0;
                        dmask0 &= ~dst->sc; // 現にあるカード集合の分はもう外れない
                        if (!(dmask0 & CARDS_ALL)) break;
                    }
                }
                // 逆転オーダー
                if (!(opqr & dst->nd[1])) { // 元々そのランクが無支配ゾーンに関係しているので更新の必要あり
                    Cards dmask1 = dmask & ~dst->nd[1];
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
                
            } else {
                // 階段
                Cards mask = RankRangeToCards(r, r + dq - 1); // 当該ランクのマスク
                Cards dqr = dc;

                if (djk) {
                    Cards jkmask = RankToCards(mv.jokerRank()); // ジョーカーがある場合のマスク
                    mask &= ~jkmask; // ジョーカー部分はマスクに関係ないので外す
                    dqr = maskJOKER(dqr);
                }

                dqr >>= SuitToSuitNum(mv.suits()); // スートの分だけずらすと枚数を表すようになる

                // 枚数型はジョーカー以外の差分を引く
                dst->qr = arg.qr - dqr;
                // 枚数位置型、圧縮型は当該ランクを1枚分シフト
                // ただしグループと違って元々1枚の場合に注意
                dst->pqr = ((arg.pqr & mask & PQR_234) >> 1) | (arg.pqr & ~mask);
                dst->sc  = ((arg.sc  & mask & PQR_234) >> 1) | (arg.sc  & ~mask);
                
                // めんどいのでその場計算
                PQRToND(dst->pqr, dst->jk, dst->nd);
            }
        } else { // ジョーカーだけ無くなった場合のコピー処理
            dst->qr = arg.qr;
            dst->pqr = arg.pqr;
            dst->sc = arg.sc;
        }
        assert(dst->exam());
    }
    
    inline void makeMove(const Hand& arg, Hand *const dst, const Move mv, const Cards dc) {
        makeMove(arg, dst, mv, dc, mv.qty());
    }
    inline void makeMove(const Hand& arg, Hand *const dst, const Move mv) {
        makeMove(arg, dst, mv, mv.cards());
    }
    
    inline void makeMoveAll(const Hand& arg, Hand *const dst, Move mv, Cards dc, uint32_t dq, uint64_t dhash) {
        makeMove(arg, dst, mv, dc, dq);
        dst->hash = arg.hash ^ dhash;
        assert(dst->exam_hash());
    }
    inline void makeMoveAll(const Hand& arg, Hand *const dst, Move mv, Cards dc, uint64_t dhash) {
        makeMoveAll(arg, dst, mv, dc, mv.qty(), dhash);
    }
    inline void makeMoveAll(const Hand& arg, Hand *const dst, Move mv, Cards dc) {
        makeMoveAll(arg, dst, mv, dc, CardsToHashKey(dc));
    }
    inline void makeMoveAll(const Hand& arg, Hand *const dst, Move mv) {
        makeMoveAll(arg, dst, mv, mv.cards());
    }
}