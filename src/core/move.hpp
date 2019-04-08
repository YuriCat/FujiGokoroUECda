#pragma once

#include "daifugo.hpp"
#include "card.hpp"

namespace UECda {
    
    /**************************着手表現**************************/

    // 最小のMove構造
    union Move16 {
        uint16_t m_;
        struct {
            unsigned s       : 4;
            signed   r       : 4;
            unsigned jks     : 4;
            signed   jkr     : 4;
        };
    };

    struct Move {
        unsigned s       : 4;
        unsigned r       : 4;
        unsigned jks     : 4;
        unsigned jkr     : 4;
        unsigned q       : 4;
        unsigned t       : 2;
        unsigned         : 1;
        unsigned invalid : 1;
        unsigned o       : 1;
        unsigned po      : 1;
        unsigned sl      : 1;
        unsigned rl      : 1;
        unsigned reverse : 1;
        unsigned         : 3;
        unsigned flags   :32;

        uint32_t toInt() const {
            return uint32_t(*reinterpret_cast<const uint64_t*>(this));
        }
        
        void clear()                      { Move tmp = {0}; (*this) = tmp; }
        void setPASS()                    { clear(); t = 0; }
        void setSingleJOKER()             { clear(); q = 1; t = 1; jks = SUITS_ALL; } // シングルジョーカーのランクは未定義
        void setS3()                      { setSingle(INTCARD_S3); } // スペ3切りの場合のみ

        void setSingle()                  { t = 1; }
        void setGroup()                   { t = 2; }
        void setSeq()                     { t = 3; }
        void setQty(uint32_t qty)         { q = qty; }
        void setRank(uint32_t rank)       { r = rank; }
        void setSuits(uint32_t suits)     { s = suits; }
        void setJokerRank(uint32_t jr)    { jkr = jr; }
        void setJokerSuits(uint32_t js)   { jks = js; }
        void setSpecialJokerSuits()       { jks = SUITS_ALL; }

        // タイプを指定してまとめて処理
        void setSingle(int rank, int suits) {
            clear();
            setSingle(); setQty(1); setRank(rank); setSuits(suits);
        }
        void setGroup(int qty, int rank, int suits) {
            clear();
            setGroup(); setQty(qty); setRank(rank); setSuits(suits);
        }
        void setSeq(int qty, int rank, int suits) {
            clear();
            setSeq(); setQty(qty); setRank(rank); setSuits(suits);
        }
        // IntCard型からシングル着手をセットする
        void setSingle(IntCard ic) {
            setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        }
        
        // True or False
        constexpr bool isPASS() const { return  t == 0; }
        constexpr uint32_t isSeq() const { return  t == 3; }
        constexpr uint32_t isGroup() const { return  t == 2; }
        constexpr uint32_t isSingle() const { return  t == 1; }
        constexpr uint32_t isSingleOrGroup() const { return  t == 1 ||  t == 2; }
        constexpr bool isQuintuple() const {
            return isGroup() && q == 5;
        }
        constexpr uint32_t containsJOKER() const { return jks || jkr; }
        
        constexpr bool isSingleJOKER() const { return isSingle() && jks == SUITS_ALL; }
        constexpr bool isS3() const { return !isSeq() && rank() == RANK_3 && suits() == SUITS_S; }
        
        constexpr bool isEqualRankSuits(uint32_t r, uint32_t s) const {
            // rank と スートが一致するか
            return rank() == r && suits() == s;
        }

        // 情報を得る
        constexpr uint32_t suits()      const { return s; }
        constexpr int qty()             const { return q; }
        constexpr int rank()            const { return r; }
        constexpr int jokerRank()       const { return jkr; }
        constexpr uint32_t jokerSuits() const { return jks; }
        constexpr uint32_t type()       const { return t; }

        int typeNum() const {
            uint32_t q = qty();
            if (isSeq()) {
                if (q >= 6) return 8;
                return 2 + q;
            } else {
                if (q >= 5) return 8;
                return q;
            }
        }
        
        Cards cards() const { // 構成するカード集合を得る
            if (isPASS()) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            int r = rank();
            uint32_t s = suits();
            if (!isSeq()) {
                Cards c = CARDS_NULL;
                uint32_t jks = jokerSuits();
                if (jks) {
                    c |= CARDS_JOKER;
                    if (jks != SUITS_CDHS) s -= jks; // クインタプル対策
                }
                return c | RankSuitsToCards(r, s);
            } else {
                Cards c = RankSuitsToCards(r, s);
                c = extractRanks(c, qty());
                if (containsJOKER()) {
                    c -= RankSuitsToCards(jokerRank(), s);
                    c |= CARDS_JOKER;
                }
                return c;
            }
        }
        Cards charaCards() const {
            // 性質カードを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            if (isPASS()) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            Cards c = RankSuitsToCards(rank(), suits());
            if (isSeq()) c = extractRanks(c, qty());
            return c;
        }
        
        template <int QTY = 256>
        Cards charaPQR() const {
            static_assert((QTY == 256 || (1 <= QTY && QTY <= 4)), "Move::charaPQR\n");
            // 性質カードのPQRを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            // パスとシングルジョーカーも関係ないし、
            // 階段にも今の所対応していない(意味が無さそう)
            if (QTY == 0) {
                return CARDS_NULL;
            } else if (QTY == 1) {
                return CARDS_HORIZON << (rank() << 2);
            } else if (QTY != 256) {
                constexpr int sft = (QTY - 1) >= 0 ? ((QTY - 1) < 32 ? (QTY - 1) : 31) : 0; // warningに引っかからないように...
                if (1 <= QTY && QTY <= 4) {
                    return Cards(1U << sft) << (rank() << 2);
                } else {
                    return CARDS_NULL;
                }
            } else {
                return Cards(1U << (qty() - 1)) << (rank() << 2);
            }
        }

        constexpr bool domInevitably() const {
            if (isSeq()) return rank() <= RANK_8 && RANK_8 < rank() + qty();
            else return rank() == RANK_8;
        }
        constexpr bool isRev() const {
            if (isSeq()) return qty() >= 5;
            else return qty() >= 4;
        }
        constexpr bool isBack() const {
            return false;
        }
        constexpr uint32_t changesPrmState() const {
            return isRev();
        }
        bool exam() const {
            // 変な値でないかチェック
            int q = qty();
            int r = rank();
            uint32_t s = suits();
            if (!isPASS()) {
                if (q < 0) return false;
                if (isSeq()) {
                    if (q < 3) return false;
                    if (countSuits(s) != 1) return false;
                    if (isEightSeqRank(r, 3)) {
                        if (!domInevitably()) return false;
                    } else {
                        if (domInevitably()) return false;
                    }
                } else if (isSingle()) {
                    if (q != 1) return false;
                    if (countSuits(s) != 1) return false;
                } else {
                    if (!isQuintuple()) {
                        if (q != countSuits(s)) return false;
                    }
                }
            }
            return true;
        }

        bool operator ==(const Move& m) const {
            return toInt() == m.toInt();
        }
    };

    const Move MOVE_NULL = {0};
    const Move MOVE_PASS = {0};
    const Move MOVE_NONE = {15, 15};

    struct MeldChar : public Move {
        MeldChar(Move m): Move(m) {}
    };

    std::ostream& operator <<(std::ostream& out, const MeldChar& m) { // MeldChar出力
        if (m.isPASS()) {
            out << "PASS";
        } else if (m.isSingleJOKER()) {
            out << "JOKER";
        } else {
            // スート
            if (m.isQuintuple()) { // クインタ特別
                out << OutSuits(SUITS_CDHSX);
            } else {
                out << OutSuits(m.suits());
            }
            out << "-";
            
            // ランク
            int r = m.rank();
            if (m.isSeq()) {
                uint32_t q = m.qty();
                out << RankRange(r, r + q - 1);
            } else {
                out << OutRank(r);
            }
        }
        return out;
    }
    
    static std::ostream& operator <<(std::ostream& out, const Move& m) { // Move出力
        out << MeldChar(m) << m.cards();
        return out;
    }
    
    class LogMove : public Move {
        // ログ出力用
    public:
        LogMove(const Move& arg) : Move(arg) {}
    };
    
    static std::ostream& operator <<(std::ostream& out, const LogMove& m) { // LogMove出力
        if (m.isPASS()) {
            out << "p";
        } else if (m.isSingleJOKER()) {
            out << "jk";
        } else {
            int r = m.rank();
            if (m.isSeq()) {
                out << OutSuitsM(m.suits());
                out << "-";
                int q = m.qty();
                out << RankRangeM(r, r + q - 1);
                // ジョーカー
                if (m.containsJOKER()) {
                    out << "(" << OutRankM(m.jokerRank()) << ")";
                }
            } else {
                if (m.isQuintuple()) { // クインタ特別
                    out << OutSuitsM(SUITS_CDHSX);
                } else {
                    out << OutSuitsM(m.suits());
                }
                out << "-";
                out << OutRankM(r);
                if (m.containsJOKER()) {
                    out << "(" << OutSuitsM(m.jokerSuits()) << ")";
                }
            }
        }
        return out;
    }
        
    inline Move CardsToMove(const Cards chara, const Cards used) {
        // 性質 chara 構成札 used のカードから着手への変換
        Move m = MOVE_NULL;
        DERR << "pointer &m = " << (uint64_t)(&m) << endl << m << endl;
        if (chara == CARDS_NULL) return MOVE_PASS;
        if (chara == CARDS_JOKER) {
            m.setSingleJOKER();
            return m;
        }
        IntCard ic = chara.lowest();
        int r = IntCardToRank(ic);
        uint32_t s = chara[r];
        uint32_t ps = used[r]; // ジョーカーなしのスート
        int q = countCards(chara);
        if (!polymRanks<2>(chara)) { // グループ系
            if (q == 1) {
                m.setSingle(r, s);
            } else {
                m.setGroup(q, r, s);
                uint32_t js = s - ps;
                if (js) m.setJokerSuits(js);
            }
        } else { // 階段系
            m.setSeq(q, r, s);
            if (containsJOKER(used)) {
                IntCard jic = Cards(subtrCards(chara, used.plain())).lowest();
                uint32_t jr = IntCardToRank(jic);
                m.setJokerRank(jr);
                m.setJokerSuits(s);
            }
        }
        DERR << "pointer &m = " << (uint64_t)(&m) << endl;
        DERR << "chara " << chara << " used " << used << " -> " << MeldChar(m) << endl;
        return m;
    }
    
    inline Move StringToMoveM(const std::string& str) {
        // 入力文字列からMove型への変更
        Move mv = MOVE_NULL;
        bool jk = false; // joker used
        uint32_t s = SUITS_NULL;
        int rank = RANK_NONE;
        uint32_t ns = 0; // num of suits
        uint32_t nr = 0; // num of ranks
        size_t i = 0;
        
        // special
        if (str == "p") return MOVE_PASS;
        if (str == "jk") {
            mv.setSingleJOKER();
            return mv;
        }
        // suits
        for (; i < str.size(); i++) {
            char c = str[i];
            if (c == '-') { i++; break; }
            int sn = CharToSuitNumM(c);
            if (sn == SUITNUM_NONE) {
                CERR << "illegal suit number" << endl;
                return MOVE_NONE;
            }
            if (sn != SUITNUM_X) {
                s |= SuitNumToSuits(sn);
            } else { // クインタプル
                jk = true;
            }
            ns++;
        }
        // rank
        for (; i < str.size(); i++) {
            char c = str[i];
            if (c == '(') { jk = true; i++; break; }
            int r = CharToRankM(c);
            if (r == RANK_NONE) {
                CERR << "illegal rank" << endl;
                return MOVE_NONE;
            }
            if (rank == RANK_NONE) {
                rank = r;
            }
            ++nr;
        }
        // invalid
        if (s == SUITS_NULL) { CERR << "null suits" << endl; return MOVE_NONE; }
        if (!ns) { CERR << "zero suits" << endl; return MOVE_NONE; }
        if (rank == RANK_NONE) { CERR << "null lowest-rank" << endl; return MOVE_NONE; }
        if (!nr) { CERR << "zero ranks" << endl; return MOVE_NONE; }
        // seq or group?
        if (nr > 1) { // seq
            mv.setSeq(nr, rank, s);
        } else if (ns == 1) { // single
            mv.setSingle(rank, s);
        } else { // group
            mv.setGroup(ns, rank, s);
        }
        // joker
        if (jk) {
            if (!mv.isSeq()) {
                uint32_t jks = SUITS_NULL;
                for (; i < str.size(); i++) {
                    char c = str[i];
                    if (c == ')') break;
                    int sn = CharToSuitNumM(c);
                    if (sn == SUITNUM_NONE) {
                        CERR << "illegal joker-suit" << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jks |= SuitNumToSuits(sn);
                }
                if (jks == SUITS_NULL) jks = SUITS_CDHS; // クインタのときはスート指定なくてもよい
                mv.setJokerSuits(jks);
            } else {
                int jkr = RANK_NONE;
                for (; i < str.size(); i++) {
                    char c = str[i];
                    if (c == ')') break;
                    int r = CharToRankM(c);
                    if (r == RANK_NONE) {
                        CERR << "illegal joker-rank " << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jkr = r;
                }
                mv.setJokerRank(jkr);
                mv.setJokerSuits(mv.suits());
            }
        }
        return mv;
    }
    
    // 生成関数
    Move IntCardToSingleMove(IntCard ic) {
        // IntCard型から対応するシングル役に変換
        // シングルジョーカー、ジョーカーS3は非対応
        Move m = MOVE_NULL;
        m.setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        return m;
    }
    
    template <class move_buf_t>
    int searchMove(const move_buf_t *const buf, const int moves, const move_buf_t& move) {
        // 同じ着手の探索
        for (int m = 0; m < moves; m++) {
            if (buf[m]== move) return m;
        }
        return -1;
    }
    
    template <class move_buf_t, typename callback_t>
    int searchMove(const move_buf_t *const buf, const int moves, const callback_t& callback) {
        // callback を条件とする着手の探索
        for (int m = 0; m < moves; m++) {
            if (callback(buf[m])) return m;
        }
        return -1;
    }
}