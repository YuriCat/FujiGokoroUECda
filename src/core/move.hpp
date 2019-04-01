#pragma once

#include "daifugo.hpp"
#include "card.hpp"

namespace UECda {
    
    /**************************着手表現**************************/
    
    // 32ビット着手表現
    // beersongのものと並びは違うが、情報はそれほど変わりはない
    // 支配能については過程を示す（支配して流れた後の情報ではない）
    // 場に依存しない情報のみを載せ、前計算による役生成からの合法性判断にも対応
    
    // 0 一時オーダー逆転(永続、一時双方のオーダー逆転能力を持つ場合は打ち消し合って0になる)
    // 1 永続オーダー逆転
    // 2 スートしばり
    // 3 ランクしばり
    // 4-7 サイズ
    // 8-11 ランク(小さい方)
    // 12-15 スート
    // 16 シングル
    // 17 グループ(シングル除く)
    // 18 階段
    // 19 (空き)
    // 20-23 ジョーカーが占めているランク
    // 24-27 ジョーカーが占めているスート
    // 28 条件付き完全支配(JOKER, S3双方にフラグ)
    // 29 無条件完全支配(8切り)
    
    enum {
        MOVE_LCT_SUITS = 0, MOVE_LCT_RANK = 4,
        MOVE_LCT_JKSUITS = 8, MOVE_LCT_JKRANK = 12,
        MOVE_LCT_QTY = 16,
        MOVE_LCT_TMPORD = 20, MOVE_LCT_PRMORD, MOVE_LCT_SUITSLOCK, MOVE_LCT_RANKLOCK,
        MOVE_LCT_SINGLE = 24, MOVE_LCT_GROUP, MOVE_LCT_SEQ,
        MOVE_LCT_INVALID = 29,
        MOVE_LCT_RANK4X   = MOVE_LCT_RANK - 2,
        MOVE_LCT_JKRANK4X = MOVE_LCT_JKRANK - 2
    };
    
    constexpr uint32_t MOVE_NONE           = 0xFFFFFFFF; // 存在しない着手

    constexpr uint32_t MOVE_PASS           = 0;
    constexpr uint32_t MOVE_NULL           = 0; // ある着手について何も情報が無い状態を示す
    
    constexpr uint32_t MOVE_FLAG_TMPORD    = 1 << MOVE_LCT_TMPORD;
    constexpr uint32_t MOVE_FLAG_PRMORD    = 1 << MOVE_LCT_PRMORD;
    
    constexpr uint32_t MOVE_FLAG_SUITSLOCK = 1 << MOVE_LCT_SUITSLOCK;
    constexpr uint32_t MOVE_FLAG_RANKLOCK  = 1 << MOVE_LCT_RANKLOCK;
    
    constexpr uint32_t MOVE_FLAG_QTY       = 15 << MOVE_LCT_QTY;
    
    constexpr uint32_t MOVE_FLAG_RANK      = 15 << MOVE_LCT_RANK;
    constexpr uint32_t MOVE_FLAG_SUITS     = 15 << MOVE_LCT_SUITS;
    
    constexpr uint32_t MOVE_FLAG_SINGLE    = 1 << MOVE_LCT_SINGLE;
    constexpr uint32_t MOVE_FLAG_GROUP     = 1 << MOVE_LCT_GROUP;
    constexpr uint32_t MOVE_FLAG_SEQ       = 1 << MOVE_LCT_SEQ;
    
    constexpr uint32_t MOVE_FLAG_JKRANK    = 15 << MOVE_LCT_JKRANK;
    constexpr uint32_t MOVE_FLAG_JKSUITS   = 15 << MOVE_LCT_JKSUITS;

    constexpr uint32_t MOVE_FLAG_INVALID = 1 << MOVE_LCT_INVALID;
    
    constexpr uint32_t MOVE_FLAG_ORD       = MOVE_FLAG_TMPORD    | MOVE_FLAG_PRMORD;
    constexpr uint32_t MOVE_FLAG_LOCK      = MOVE_FLAG_SUITSLOCK | MOVE_FLAG_RANKLOCK;
    constexpr uint32_t MOVE_FLAG_FORM      = MOVE_FLAG_SINGLE    | MOVE_FLAG_GROUP     | MOVE_FLAG_SEQ;
    constexpr uint32_t MOVE_FLAG_TYPE      = MOVE_FLAG_FORM      | MOVE_FLAG_QTY;
    constexpr uint32_t MOVE_FLAG_CHARA     = MOVE_FLAG_TYPE      | MOVE_FLAG_RANK      | MOVE_FLAG_SUITS; // 下16ビットさえあれば性質が分かる
    constexpr uint32_t MOVE_FLAG_JK        = MOVE_FLAG_JKRANK    | MOVE_FLAG_JKSUITS;
    constexpr uint32_t MOVE_FLAG_MELD      = MOVE_FLAG_CHARA     | MOVE_FLAG_JK;
    
    constexpr uint32_t MOVE_FLAG_STATUS    = MOVE_FLAG_ORD | MOVE_FLAG_LOCK;
    constexpr uint32_t MOVE_FLAG_EFFECTS   = MOVE_FLAG_ORD | MOVE_FLAG_LOCK;
    
    constexpr uint32_t MOVE_SINGLEJOKER    = (1U << MOVE_LCT_QTY) | MOVE_FLAG_SINGLE | (SUITS_ALL << MOVE_LCT_JKSUITS);
    constexpr uint32_t MOVE_S3FLUSH        = (1U << MOVE_LCT_QTY) | MOVE_FLAG_SINGLE | (SUIT_S << MOVE_LCT_SUITS) | (RANK_3 << MOVE_LCT_RANK);
    

/*
    template <typename T>
    union MoveBase2 {
        T m_;
        struct {
            unsigned s       : 4;
            signed   r       : 4;
            unsigned jks     : 4;
            signed   jkr     : 4;
            signed   q       : 4;
            unsigned group   : 1;
            unsigned sequence: 1;
            unsigned padding :10;
        };

        constexpr MoveBase2() : m_() {}
        constexpr MoveBase2(T m) : m_(m) {}
        
        constexpr operator T() const { return m_; }
        constexpr T data() const { return m_; }
        
        void setNULL()                    { m_ = 0; }
        void setPASS()                    { m_ = 0; }
        void setSingleJOKER()             { m_.jkrank == RANK_O } // シングルジョーカーのランクは未定義
        void setS3Flush()                 { m_ = MOVE_S3FLUSH; } // スペ3切りの場合のみ
        void setRev()                     { m_ ^= MOVE_FLAG_ORD; }
        void setBack()                    { m_ ^= MOVE_FLAG_TMPORD; }
        void setSingle()                  { m_ |= MOVE_FLAG_SINGLE; }
        void setGroup()                   { m_ |= MOVE_FLAG_GROUP; }
        void setSeq()                     { m_ |= MOVE_FLAG_SEQ; }
        void setQty(int qty)         { m_ |= qty     << MOVE_LCT_QTY; }
        void setRank(int r)          { m_ |= r       << MOVE_LCT_RANK; }
        void setSuits(unsigned s)         { suits = s; }
        void setJokerRank(int r)     { jkr = r }
        void setJokerSuits(unsigned s)    { jks = s; }
        void setSpecialJokerSuits()       { m_ |= SUITS_ALL << MOVE_LCT_JKSUITS; }
        
        // 特殊効果
        
        // タイプを指定してまとめて処理
        // 特殊効果フラグはここでの分岐処理は避け、呼び出し元で対応
        void setSingle(int rank, int suits) {
            setSingle(); setQty(1); setRank(rank); setSuits(suits);
        }
        void setSingleByRank4x(int r4x, int suits) {
            setSingle(); setQty(1); setRank4x(r4x); setSuits(suits);
        }
        void setGroup(int qty, int rank, int suits) {
            setGroup(); setQty(qty); setRank(rank); setSuits(suits);
        }
        void setGroupByRank4x(int qty, int r4x, int suits) {
            setGroup(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }
        void setSeq(int qty, int rank, int suits) {
            setSeq(); setQty(qty); setRank(rank); setSuits(suits);
        }
        void setSeqByRank4x(int qty, int r4x, int suits) {
            setSeq(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }

        // IntCard型からシングル着手をセットする(特殊効果フラグ以外)
        void setSingleByIntCard(IntCard ic) {
            setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        }
        
        // 情報を得る
        constexpr uint32_t suits()      const { return (m_ >> MOVE_LCT_SUITS)    & 15U; }
        constexpr int qty()             const { return (m_ >> MOVE_LCT_QTY)      & 15U; }
        constexpr int rank()            const { return (m_ >> MOVE_LCT_RANK)     & 15U; }
        constexpr int jokerRank()       const { return (m_ >> MOVE_LCT_JKRANK)   & 15U; }
        constexpr uint32_t jokerSuits() const { return (m_ >> MOVE_LCT_JKSUITS)  & 15U; }
        constexpr int rank4x()          const { return (m_ >> MOVE_LCT_RANK4X)   & (15U << 2); } // 4倍型
        constexpr int jokerRank4x()     const { return (m_ >> MOVE_LCT_JKRANK4X) & (15U << 2); } // 4倍型
        
        // True or False
        constexpr bool isPASS() const { return m_ == 0; }
        constexpr bool isSeq() const { return sequence; }
        constexpr bool isGroup() const { return group && qty() > 1; }
        constexpr bool isSingle() const { return qroup && qty() == 1; }
        constexpr uint32_t isSingleOrGroup() const { return qroup; }
        constexpr bool isQuadruple() const {
            return typePart() == (MOVE_FLAG_GROUP | (4U << MOVE_LCT_QTY));
        }
        constexpr bool isQuintuple() const {
            return typePart() == (MOVE_FLAG_GROUP | (5U << MOVE_LCT_QTY));
        }
        bool isOver5Seq() const { // 5枚以上階段
            return sequence && qty() >= 5;
        }

        bool isSpecialRankSeq() const {
            if (!sequence) return false;
            uint32_t r = rank(), q = qty();
            return r < RANK_MIN || RANK_MAX < r + q - 1;
        }
        
        constexpr uint32_t containsJOKER() const { return  jks || jkr; }
        
        constexpr bool isSingleJOKER() const { return (((uint32_t)m_) & (MOVE_FLAG_SINGLE | MOVE_FLAG_RANK)) == MOVE_FLAG_SINGLE; }
        constexpr bool isS3Flush() const { return holdsBits<T>(m_, (MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM)); }
        
        constexpr bool isEqualRankSuits(uint32_t r, uint32_t s) const {
            // rank と スートが一致するか
            return ((uint32_t)m_ & (MOVE_FLAG_RANK | MOVE_FLAG_SUITS)) == ((r << MOVE_LCT_RANK) | (s << MOVE_LCT_SUITS));
        }

        constexpr uint32_t domInevitably() const {
            if (sequence) return rank <= RANK_8 && RANK_8 < rank + qty;
            else return rank == RANK_8;
        }
        constexpr uint32_t flipsPrmOrder() const {
            if (sequence) return qty >= 5;
            else return qty >= 4;
        }
        constexpr uint32_t flipsTmpOrder() const {
            return false;
        }
        
        constexpr uint32_t changesPrmState() const { return flipsPrmOrder(); }
        
        int typeNum() const {
            int q = qty();
            if (isSeq()) {
                if (q >= 6) return 8;
                return 2 + q;
            } else {
                if (q >= 5) return 8;
                return q;
            }
        }
        
        Cards cards() const {
            // カード集合を得る
            Cards res;
            if (isPASS()) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            unsigned s = suits;
            
            if (group) {
                if (jks && jks != SUITS_CDHS) s -= jks; // クインタプル対策
                return Cards(RankSuitsToCards(r, s), jks ? 1 : 0);
            } else { // 階段
                Cards res = extractRanks<3>(Rank4xSuitsToCards(rank, s), qty);
                if (containsJOKER()) {
                    res -= RankSuitsToCards(jokerRank(), s);
                    res |= CARDS_JOKER;
                }
                return res;
            }
        }
        Cards charaCards() const {
            // 性質カードを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            if (isPASS()) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            Cards res = RankSuitsToCards(rank, suits);
            if (sequence) {
                switch (qty) {
                    case 0: break;
                    case 1: break;
                    case 2: break;
                    case 3: res = extractRanks<3>(res); break;
                    case 4: res = extractRanks<4>(res); break;
                    case 5: res = extractRanks<5>(res); break;
                    default: res = extractRanks(res, qty); break;
                }
            }
            return res;
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
                return CARDS_HORIZON << rank4x();
            } else if (QTY != 256) {
                constexpr int sft = (QTY - 1) >= 0 ? ((QTY - 1) < 32 ? (QTY - 1) : 31) : 0; // warningに引っかからないように...
                if (1 <= QTY && QTY <= 4) {
                    return Cards(1U << sft) << rank4x();
                } else {
                    return CARDS_NULL;
                }
            } else {
                return Cards(1U << (qty() - 1)) << rank4x();
            }
        }
        
        bool exam() const {
            // 変な値でないかチェック
            // 特殊効果の付け忘れなどに注意
            int q = qty();
            int r = rank();
            uint32_t s = suits();
            if (isPASS()) {
                // TODO: パスの時のチェックがあれば
            } else {
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
                    if (r == RANK_8) {
                        if (!domInevitably()) return false;
                    }
                } else {
                    if (isQuintuple()) {
                    } else {
                        if (q != countSuits(s)) return false;
                    }
                    if (r == RANK_8) {
                        if (!domInevitably()) return false;
                    } else {
                        if (domInevitably()) return false;
                    }
                }
            }
            return true;
        }
    };*/

    template <typename T>
    struct MoveBase {
        T m_;

        constexpr MoveBase() : m_() {}
        constexpr MoveBase(T m) : m_(m) {}
        
        constexpr operator T() const { return m_; }
        constexpr T data() const { return m_; }
        
        void setNULL()                    { m_ = MOVE_NULL; }
        void setPASS()                    { m_ = MOVE_PASS; }
        void setSingleJOKER()             { m_ = MOVE_SINGLEJOKER; } // シングルジョーカーのランクは未定義
        void setS3Flush()                 { m_ = MOVE_S3FLUSH; } // スペ3切りの場合のみ
        void setRev()                     { m_ ^= MOVE_FLAG_ORD; }
        void setBack()                    { m_ ^= MOVE_FLAG_TMPORD; }
        void setSingle()                  { m_ |= MOVE_FLAG_SINGLE; }
        void setGroup()                   { m_ |= MOVE_FLAG_GROUP; }
        void setSeq()                     { m_ |= MOVE_FLAG_SEQ; }
        void setQty(uint32_t qty)         { m_ |= qty     << MOVE_LCT_QTY; }
        void setRank(uint32_t r)          { m_ |= r       << MOVE_LCT_RANK; }
        void setRank4x(uint32_t r4x)      { m_ |= r4x     << MOVE_LCT_RANK4X; } // 4倍型
        void setSuits(uint32_t s)         { m_ |= s       << MOVE_LCT_SUITS; }
        void setJokerRank(uint32_t r)     { m_ |= r       << MOVE_LCT_JKRANK; }
        void setJokerRank4x(uint32_t r4x) { m_ |= r4x     << MOVE_LCT_JKRANK4X; } // 4倍型
        void setJokerSuits(uint32_t s)    { m_ |= s       << MOVE_LCT_JKSUITS; }
        void setSpecialJokerSuits()       { m_ |= SUITS_ALL << MOVE_LCT_JKSUITS; }

        // タイプを指定してまとめて処理
        // 特殊効果フラグはここでの分岐処理は避け、呼び出し元で対応
        void setSingle(int rank, int suits) {
            setSingle(); setQty(1); setRank(rank); setSuits(suits);
        }
        void setSingleByRank4x(int r4x, int suits) {
            setSingle(); setQty(1); setRank4x(r4x); setSuits(suits);
        }
        void setGroup(int qty, int rank, int suits) {
            setGroup(); setQty(qty); setRank(rank); setSuits(suits);
        }
        void setGroupByRank4x(int qty, int r4x, int suits) {
            setGroup(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }
        void setSeq(int qty, int rank, int suits) {
            setSeq(); setQty(qty); setRank(rank); setSuits(suits);
        }
        void setSeqByRank4x(int qty, int r4x, int suits) {
            setSeq(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }

        // IntCard型からシングル着手をセットする(特殊効果フラグ以外)
        void setSingleByIntCard(IntCard ic) {
            setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        }
        
        // 情報を得る
        constexpr uint32_t suits()      const { return (m_ >> MOVE_LCT_SUITS)    & 15U; }
        constexpr int qty()             const { return (m_ >> MOVE_LCT_QTY)      & 15U; }
        constexpr int rank()            const { return (m_ >> MOVE_LCT_RANK)     & 15U; }
        constexpr int jokerRank()       const { return (m_ >> MOVE_LCT_JKRANK)   & 15U; }
        constexpr uint32_t jokerSuits() const { return (m_ >> MOVE_LCT_JKSUITS)  & 15U; }
        constexpr int rank4x()          const { return (m_ >> MOVE_LCT_RANK4X)   & (15U << 2); } // 4倍型
        constexpr int jokerRank4x()     const { return (m_ >> MOVE_LCT_JKRANK4X) & (15U << 2); } // 4倍型
        
        // 部分に着目する
        constexpr uint32_t orderPart()       const { return m_ & MOVE_FLAG_ORD;   }
        constexpr uint32_t exceptOrderPart() const { return m_ & ~MOVE_FLAG_ORD;  }
        constexpr uint32_t suitsPart()       const { return m_ & MOVE_FLAG_SUITS; }
        constexpr uint32_t rankPart()        const { return m_ & MOVE_FLAG_RANK;  }
        constexpr uint32_t qtyPart()         const { return m_ & MOVE_FLAG_QTY;   }
        constexpr uint32_t typePart()        const { return m_ & MOVE_FLAG_TYPE;  } // サイズ + 形式
        constexpr uint32_t jokerPart()       const { return m_ & MOVE_FLAG_JK;    } // ジョーカー関連
        constexpr uint32_t exceptJokerPart() const { return m_ & ~MOVE_FLAG_JK;   } // ジョーカー関連以外
        constexpr uint32_t charaPart()       const { return m_ & MOVE_FLAG_CHARA; } // 性質決定のための必要十分条件
        constexpr uint32_t meldPart()        const { return m_ & MOVE_FLAG_MELD;  } // 性質決定のための必要十分条件
        
        // True or False
        constexpr bool isPASS() const { return (uint32_t)m_ == MOVE_PASS; }
        constexpr uint32_t isSeq() const { return m_ & MOVE_FLAG_SEQ; }
        constexpr uint32_t isGroup() const { return m_ & MOVE_FLAG_GROUP; }
        constexpr uint32_t isSingle() const { return m_ & MOVE_FLAG_SINGLE; }
        constexpr uint32_t isSingleOrGroup() const { return m_ & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP); }
        constexpr bool isQuadruple() const {
            return typePart() == (MOVE_FLAG_GROUP | (4U << MOVE_LCT_QTY));
        }
        constexpr bool isQuintuple() const {
            return typePart() == (MOVE_FLAG_GROUP | (5U << MOVE_LCT_QTY));
        }
        bool isOver5Seq() const { // 5枚以上階段
            return isSeq() && qtyPart() > (4U << MOVE_LCT_QTY);
        }
        template <int IS_SEQ = _BOTH>
        bool isSpecialRankSeq() const {
            if (IS_SEQ == _NO || (IS_SEQ != _YES && !isSeq())) return false;
            uint32_t r = rank(), q = qty();
            return r < RANK_MIN || RANK_MAX < r + q - 1;
        }
        
        constexpr uint32_t containsJOKER() const { return m_ & MOVE_FLAG_JK; }
        
        constexpr bool isSingleJOKER() const { return (((uint32_t)m_) & (MOVE_FLAG_SINGLE | MOVE_FLAG_RANK)) == MOVE_FLAG_SINGLE; }
        constexpr bool isS3Flush() const {
            //return holdsBits<T>(m_, (MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM));
            return isSingle() && rank() == RANK_3 && suits() == SUITS_S;
        }
        
        constexpr bool isEqualRankSuits(uint32_t r, uint32_t s) const {
            // rank と スートが一致するか
            return ((uint32_t)m_ & (MOVE_FLAG_RANK | MOVE_FLAG_SUITS)) == ((r << MOVE_LCT_RANK) | (s << MOVE_LCT_SUITS));
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
        
        Cards cards() const { // カード集合を得る
            // 本来は着手表現からの計算で得るのは止めた方が良いかもしれない
            Cards res;
            if (isPASS()) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            
            uint32_t s = suits(), r4x = rank4x();
            
            if (!isSeq()) { // 階段でない
                res = CARDS_NULL;
                uint32_t jks = jokerSuits();
                if (jks) {
                    addJOKER(&res);
                    if (jks != SUITS_CDHS) { s -= jks; } // クインタプル対策
                }
                addCards(&res, Rank4xSuitsToCards(r4x, s));
            } else { // 階段
                /*
                 uint32_t qty=qty();
                 res=convRx4S_Cards(r4x,suits);
                 switch (qty) {
                 case 0:break;
                 case 1:break;
                 case 2:break;
                 case 3:res=extractRanks<3>(res);break;
                 case 4:res=extractRanks<4>(res);break;
                 case 5:res=extractRanks<5>(res);break;
                 default:res=extractRanks(res,qty);break;
                 }*/
                /*uint32_t qty=qty();
                 res=convRx4S_Cards(r4x,suits);
                 res=extractRanks<3>(res);
                 qty-=3;
                 while (qty) {
                 res=extractRanks<2>(res);
                 --qty;
                 }*/
                uint32_t q = qty();
                assert(q >= 3);
                res = extractRanks<3>(Rank4xSuitsToCards(r4x, s));
                if (q != 3) {
                    res = extractRanks<2>(res);
                    if (q != 4) {
                        res = extractRanks<2>(res);
                        q -= 5;
                        while (q) {
                            res = extractRanks<2>(res);
                            q -= 1;
                        }
                    }
                }
                /*
                 uint32_t qty=qtyPart();
                 res=extractRanks<3>(convRx4S_Cards(r4x,suits));
                 if (qty!=(3<<8)) {
                 res=extractRanks<2>(res);
                 if (qty!=(4<<8)) {
                 res=extractRanks<2>(res);
                 qty-=(5<<8);
                 while (qty) {
                 res=extractRanks<2>(res);
                 qty-=(1<<8);
                 }
                 }
                 }*/
                if (containsJOKER()) {
                    res -= Rank4xSuitsToCards(jokerRank4x(), s);
                    res |= CARDS_JOKER;
                }
            }
            return res;
        }
        
        template <int IS_PASS = _BOTH>
        Cards charaCards() const {
            // 性質カードを返す
            // 性質カードが表現出来ない可能性のある特別スートを用いる役が入った場合には対応していない
            if ((IS_PASS == _YES) || ((IS_PASS != _NO) && isPASS())) return CARDS_NULL;
            if (isSingleJOKER()) return CARDS_JOKER;
            Cards res = Rank4xSuitsToCards(rank4x(), suits());
            if (isSeq()) {
                uint64_t q = qty();
                switch (q) {
                    case 0: break;
                    case 1: break;
                    case 2: break;
                    case 3: res = extractRanks<3>(res); break;
                    case 4: res = extractRanks<4>(res); break;
                    case 5: res = extractRanks<5>(res); break;
                    default: res = extractRanks(res, q); break;
                }
            }
            return res;
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
                return CARDS_HORIZON << rank4x();
            } else if (QTY != 256) {
                constexpr int sft = (QTY - 1) >= 0 ? ((QTY - 1) < 32 ? (QTY - 1) : 31) : 0; // warningに引っかからないように...
                if (1 <= QTY && QTY <= 4) {
                    return Cards(1U << sft) << rank4x();
                } else {
                    return CARDS_NULL;
                }
            } else {
                return Cards(1U << (qty() - 1)) << rank4x();
            }
        }
        
        bool exam() const {
            // 変な値でないかチェック
            // 特殊効果の付け忘れなどに注意
            int q = qty();
            int r = rank();
            uint32_t s = suits();
            if (isPASS()) {
                // TODO: パスの時のチェックがあれば
            } else {
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
                    if (r == RANK_8) {
                        if (!domInevitably()) return false;
                    }
                } else {
                    if (isQuintuple()) {
                    } else {
                        if (q != countSuits(s)) return false;
                    }
                    if (r == RANK_8) {
                        if (!domInevitably()) return false;
                    } else {
                        if (domInevitably()) return false;
                    }
                }
            }
            return true;
        }
    };
    
    using Move = MoveBase<uint32_t>;
    using Move16 = MoveBase<uint16_t>;
    
    class MeldChar : public Move {
        // Moveと全く同じデータなのだが、構成するカード集合に言及する気がないときにこっちで表記
    public:
        MeldChar(const Move& arg) : Move(arg.m_) {}
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
    
    std::ostream& operator <<(std::ostream& out, const Move& m) { // Move出力
        out << MeldChar(m) << m.cards();
        return out;
    }
    
    class LogMove : public Move {
        // ログ出力用
    public:
        LogMove(const Move& arg) : Move(arg.m_) {}
    };
    
    std::ostream& operator <<(std::ostream& out, const LogMove& m) { // LogMove出力
        if (m.isPASS()) {
            out << "p";
        } else if (m.isSingleJOKER()) {
            out << "jk";
        } else {
            int r = m.rank();
            if (m.isSeq()) {
                
                out << OutSuitsM(m.suits());
                out << "-";
                
                uint32_t q = m.qty();
                out << RankRangeM(r, r + q - 1);
                // ジョーカー
                if (m.containsJOKER()) {
                    //cerr << m.jokerRank() << " " << OutRankM(m.jokerRank()) << endl;
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
        
    Move CardsToMove(const Cards chara, const Cards used) {
        // 性質 chara 構成札 used のカードから着手への変換
        Move m = MOVE_NULL;
        DERR << "pointer &m = " << (uint64_t)(&m) << endl << m << endl;
        if (chara == CARDS_NULL)return MOVE_PASS;
        if (chara == CARDS_JOKER)return MOVE_SINGLEJOKER;
        IntCard ic = CardsToLowestIntCard(chara);
        int r4x = IntCardToRank4x(ic);
        uint32_t s = CardsRank4xToSuits(chara, r4x);
        uint32_t ps = CardsRank4xToSuits(used, r4x); // ジョーカーなしのスート
        int q = countCards(chara);
        if (!polymRanks<2>(chara)) { // グループ系
            if (q == 1) {
                m.setSingleByRank4x(r4x, s);
            } else {
                m.setGroupByRank4x(q, r4x, s);
                uint32_t js = s - ps;
                if (js) m.setJokerSuits(js);
            }
        } else { // 階段系
            m.setSeqByRank4x(q, r4x, s);
            if (containsJOKER(used)) {
                IntCard jic = CardsToLowestIntCard(subtrCards(chara, maskJOKER(used)));
                uint32_t jr4x = IntCardToRank4x(jic);
                m.setJokerRank4x(jr4x);
                m.setJokerSuits(s);
            }
        }
        DERR << "pointer &m = " << (uint64_t)(&m) << endl;
        DERR << "chara " << chara << " used " << used << " -> " << MeldChar(m) << endl;
        return m;
    }
    
    Move StringToMoveM(const std::string& str) {
        // 入力文字列からMove型への変更
        Move mv = MOVE_NULL;
        bool jk = false; // joker used
        uint32_t s = SUITS_NULL;
        int rank = RANK_NONE;
        uint32_t ns = 0; // num of suits
        uint32_t nr = 0; // num of ranks
        uint32_t i = 0;
        
        // special
        if (str == "p") { return MOVE_PASS; }
        if (str == "jk") { return MOVE_SINGLEJOKER; }
        // suits
        for (; i < str.size(); ++i) {
            char c = str[i];
            if (c == '-') {
                ++i; break;
            }
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
            ++ns;
        }
        // rank
        for (; i < str.size(); ++i) {
            char c = str[i];
            if (c == '(') { jk = true; ++i; break; }
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
                for (; i < str.size(); ++i) {
                    char c = str[i];
                    if (c == ')') { break; }
                    int sn = CharToSuitNumM(c);
                    if (sn == SUITNUM_NONE) {
                        CERR << "illegal joker-suit" << c << " from " << str << endl;
                        return MOVE_NONE;
                    }
                    jks |= SuitNumToSuits(sn);
                }
                if (jks == SUITS_NULL)jks = SUITS_CDHS; // クインタのときはスート指定なくてもよい
                mv.setJokerSuits(jks);
            } else {
                int jkr = RANK_NONE;
                for (; i < str.size(); ++i) {
                    char c = str[i];
                    if (c == ')') { break; }
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
    constexpr Move IntCardToSingleMove(IntCard ic) {
        // IntCard型から対応するシングル役に変換
        // シングルジョーカー、ジョーカーS3は非対応
        return Move(MOVE_FLAG_SINGLE
                    | (1U << MOVE_LCT_QTY)
                    | (IntCardToSuits(ic) << MOVE_LCT_SUITS)
                    | (IntCardToRank(ic) << MOVE_LCT_RANK));
    }
    
    template <class move_buf_t>
    int searchMove(const move_buf_t *const buf, const int moves, const move_buf_t& move) {
        // 同じ着手の探索
        for (int m = 0; m < moves; ++m) {
            if (buf[m].meldPart() == move.meldPart()) { return m; }
        }
        return -1;
    }
    
    template <class move_buf_t, typename callback_t>
    int searchMove(const move_buf_t *const buf, const int moves, const callback_t& callback) {
        // callback を条件とする着手の探索
        for (int m = 0; m < moves; ++m) {
            if (callback(buf[m])) { return m; }
        }
        return -1;
    }
}