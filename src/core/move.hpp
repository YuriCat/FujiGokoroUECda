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
    
    constexpr int MOVE_LCT_TMPORD  = 0;
    constexpr int MOVE_LCT_PRMORD  = 1;
    constexpr int MOVE_LCT_QTY     = 4;
    constexpr int MOVE_LCT_RANK    = 8;
    constexpr int MOVE_LCT_SUITS   = 12;
    constexpr int MOVE_LCT_JKRANK  = 20;
    constexpr int MOVE_LCT_JKSUITS = 24;
    
    // ランク4倍型
    constexpr int MOVE_LCT_RANK4X   = MOVE_LCT_RANK - 2;
    constexpr int MOVE_LCT_JKRANK4X = MOVE_LCT_JKRANK - 2;
    
    constexpr uint32_t MOVE_NONE           = 0xFFFFFFFF; // 存在しない着手

    constexpr uint32_t MOVE_PASS           = 0U;
    constexpr uint32_t MOVE_NULL           = 0U; // ある着手について何も情報が無い状態を示す
    
    constexpr uint32_t MOVE_FLAG_TMPORD    = 0x00000001;
    constexpr uint32_t MOVE_FLAG_PRMORD    = 0x00000002;
    
    constexpr uint32_t MOVE_FLAG_SUITSLOCK = 0x00000004;
    constexpr uint32_t MOVE_FLAG_RANKLOCK  = 0x00000008;
    
    constexpr uint32_t MOVE_FLAG_QTY       = 0x000000F0;
    
    constexpr uint32_t MOVE_FLAG_RANK      = 0x00000F00;
    constexpr uint32_t MOVE_FLAG_SUITS     = 0x0000F000;
    
    constexpr uint32_t MOVE_FLAG_SINGLE    = 0x00010000;
    constexpr uint32_t MOVE_FLAG_GROUP     = 0x00020000;
    constexpr uint32_t MOVE_FLAG_SEQ       = 0x00040000;
    
    constexpr uint32_t MOVE_FLAG_JKRANK    = 0x00F00000;
    constexpr uint32_t MOVE_FLAG_JKSUITS   = 0x0F000000;
        
    constexpr uint32_t MOVE_FLAG_CONDDOM   = 0x10000000;
    constexpr uint32_t MOVE_FLAG_INEVITDOM = 0x20000000;
    
    constexpr uint32_t MOVE_FLAG_ORD       = MOVE_FLAG_TMPORD    | MOVE_FLAG_PRMORD;
    constexpr uint32_t MOVE_FLAG_LOCK      = MOVE_FLAG_SUITSLOCK | MOVE_FLAG_RANKLOCK;
    constexpr uint32_t MOVE_FLAG_FORM      = MOVE_FLAG_SINGLE    | MOVE_FLAG_GROUP     | MOVE_FLAG_SEQ;
    constexpr uint32_t MOVE_FLAG_TYPE      = MOVE_FLAG_FORM      | MOVE_FLAG_QTY;
    constexpr uint32_t MOVE_FLAG_CHARA     = MOVE_FLAG_TYPE      | MOVE_FLAG_RANK      | MOVE_FLAG_SUITS; // 下16ビットさえあれば性質が分かる
    constexpr uint32_t MOVE_FLAG_JK        = MOVE_FLAG_JKRANK    | MOVE_FLAG_JKSUITS;
    constexpr uint32_t MOVE_FLAG_MELD      = MOVE_FLAG_CHARA     | MOVE_FLAG_JK;
    constexpr uint32_t MOVE_FLAG_DOM       = MOVE_FLAG_CONDDOM   | MOVE_FLAG_INEVITDOM;
    
    constexpr uint32_t MOVE_FLAG_STATUS    = MOVE_FLAG_ORD | MOVE_FLAG_LOCK;
    constexpr uint32_t MOVE_FLAG_EFFECTS   = MOVE_FLAG_ORD | MOVE_FLAG_LOCK | MOVE_FLAG_DOM;
    
    constexpr uint32_t MOVE_SINGLEJOKER    = (1U << MOVE_LCT_QTY) | MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM | (SUITS_ALL << MOVE_LCT_JKSUITS);
    constexpr uint32_t MOVE_S3FLUSH        = (1U << MOVE_LCT_QTY) | MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM | (SUIT_S << MOVE_LCT_SUITS) | (RANK_3 << MOVE_LCT_RANK);
    
    template <typename data_t>
    class MoveInRegister {
    public:
        constexpr MoveInRegister() : i_() {}
        constexpr MoveInRegister(data_t arg) : i_(arg) {}
        
        constexpr operator data_t() const { return i_; }
        constexpr data_t data() const { return i_.data(); }
        constexpr BitSetInRegister<data_t> bits() const { return i_; }
        
        void setNULL()                    { i_ = MOVE_NULL; }
        void setPASS()                    { i_ = MOVE_PASS; }
        void setSingleJOKER()             { i_ = MOVE_SINGLEJOKER; } // シングルジョーカーのランクは未定義
        void setS3Flush()                 { i_ = MOVE_S3FLUSH; } // スペ3切りの場合のみ
        void setRev()                     { i_ ^= MOVE_FLAG_ORD; }
        void setBack()                    { i_ ^= MOVE_FLAG_TMPORD; }
        void setEight()                   { i_ |= MOVE_FLAG_INEVITDOM; }
        void setSingle()                  { i_ |= MOVE_FLAG_SINGLE; }
        void setGroup()                   { i_ |= MOVE_FLAG_GROUP; }
        void setSeq()                     { i_ |= MOVE_FLAG_SEQ; }
        void setQty(uint32_t qty)         { i_ |= qty     << MOVE_LCT_QTY; }
        void setRank(uint32_t r)          { i_ |= r       << MOVE_LCT_RANK; }
        void setRank4x(uint32_t r4x)      { i_ |= r4x     << MOVE_LCT_RANK4X; } // 4倍型
        void setSuits(uint32_t s)         { i_ |= s       << MOVE_LCT_SUITS; }
        void setJokerRank(uint32_t r)     { i_ |= r       << MOVE_LCT_JKRANK; }
        void setJokerRank4x(uint32_t r4x) { i_ |= r4x     << MOVE_LCT_JKRANK4X; } // 4倍型
        void setJokerSuits(uint32_t s)    { i_ |= s       << MOVE_LCT_JKSUITS; }
        void setSpecialJokerSuits()       { i_ |= SUITS_ALL << MOVE_LCT_JKSUITS; }
        
        // 特殊効果
        void resetEffects() { i_ &= ~MOVE_FLAG_EFFECTS; }
        void setEffects() {
            // 普通は合法着手生成の段階で付ける
            // 棋譜から読んだ着手にはここで付ける
            if (isSeq()) {
                if (qty() >= 5) { setRev(); }
                if (isEightSeqRank(rank(), qty())) {
                    setEight();
                }
            } else {
                if (rank() == RANK_3 && suits() == SUITS_S && qty() == 1) {
                    i_ = MOVE_S3FLUSH;
                } else if (qty() == 1 && suits() == SUITS_NULL) {
                    i_ = MOVE_SINGLEJOKER;
                } else {
                    if (qty() >= 4) { setRev(); }
                    if (rank() == RANK_8) { setEight(); }
                }
            }
        }
        
        // タイプを指定してまとめて処理
        // 特殊効果フラグはここでの分岐処理は避け、呼び出し元で対応
        template <typename rank_t, typename suits_t>
        void setSingle(rank_t rank, suits_t suits) {
            setSingle(); setQty(1); setRank(rank); setSuits(suits);
        }
        template <typename rank4x_t, typename suits_t>
        void setSingleByRank4x(rank4x_t r4x, suits_t suits) {
            setSingle(); setQty(1); setRank4x(r4x); setSuits(suits);
        }
        template <typename qty_t, typename rank_t, typename suits_t>
        void setGroup(qty_t qty, rank_t rank, suits_t suits) {
            setGroup(); setQty(qty); setRank(rank); setSuits(suits);
        }
        template <typename qty_t, typename rank4x_t, typename suits_t>
        void setGroupByRank4x(qty_t qty, rank4x_t r4x, suits_t suits) {
            setGroup(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }
        template <typename qty_t, typename rank_t, typename suits_t>
        void setSeq(qty_t qty, rank_t rank, suits_t suits) {
            setSeq(); setQty(qty); setRank(rank); setSuits(suits);
        }
        template <typename qty_t, typename rank4x_t, typename suits_t>
        void setSeqByRank4x(qty_t qty, rank4x_t r4x, suits_t suits) {
            setSeq(); setQty(qty); setRank4x(r4x); setSuits(suits);
        }

        // IntCard型からシングル着手をセットする(特殊効果フラグ以外)
        void setSingleByIntCard(IntCard ic) {
            setSingle(IntCardToRank(ic), IntCardToSuits(ic));
        }
        
        // 情報を得る
        constexpr uint32_t suits()       const { return (i_ >> MOVE_LCT_SUITS)    & 15U; }
        constexpr uint32_t qty()         const { return (i_ >> MOVE_LCT_QTY)      & 15U; }
        constexpr uint32_t rank()        const { return (i_ >> MOVE_LCT_RANK)     & 15U; }
        constexpr uint32_t jokerRank()   const { return (i_ >> MOVE_LCT_JKRANK)   & 15U; }
        constexpr uint32_t jokerSuits()  const { return (i_ >> MOVE_LCT_JKSUITS)  & 15U; }
        constexpr uint32_t rank4x()      const { return (i_ >> MOVE_LCT_RANK4X)   & (15U << 2); } // 4倍型
        constexpr uint32_t jokerRank4x() const { return (i_ >> MOVE_LCT_JKRANK4X) & (15U << 2); } // 4倍型
        
        // 部分に着目する
        constexpr uint32_t orderPart()       const { return i_ & MOVE_FLAG_ORD;   }
        constexpr uint32_t exceptOrderPart() const { return i_ & ~MOVE_FLAG_ORD;  }
        constexpr uint32_t suitsPart()       const { return i_ & MOVE_FLAG_SUITS; }
        constexpr uint32_t rankPart()        const { return i_ & MOVE_FLAG_RANK;  }
        constexpr uint32_t qtyPart()         const { return i_ & MOVE_FLAG_QTY;   }
        constexpr uint32_t typePart()        const { return i_ & MOVE_FLAG_TYPE;  } // サイズ + 形式
        constexpr uint32_t jokerPart()       const { return i_ & MOVE_FLAG_JK;    } // ジョーカー関連
        constexpr uint32_t exceptJokerPart() const { return i_ & ~MOVE_FLAG_JK;   } // ジョーカー関連以外
        constexpr uint32_t charaPart()       const { return i_ & MOVE_FLAG_CHARA; } // 性質決定のための必要十分条件
        constexpr uint32_t meldPart()        const { return i_ & MOVE_FLAG_MELD;  } // 性質決定のための必要十分条件
        
        // True or False
        constexpr bool isPASS() const { return (uint32_t)i_ == MOVE_PASS; }
        constexpr uint32_t isSeq() const { return i_ & MOVE_FLAG_SEQ; }
        constexpr uint32_t isGroup() const { return i_ & MOVE_FLAG_GROUP; }
        constexpr uint32_t isSingle() const { return i_ & MOVE_FLAG_SINGLE; }
        constexpr uint32_t isSingleOrGroup() const { return i_ & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP); }
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
        
        constexpr uint32_t containsJOKER() const { return i_ & MOVE_FLAG_JK; }
        
        constexpr bool isSingleJOKER() const { return (((uint32_t)i_) & (MOVE_FLAG_SINGLE | MOVE_FLAG_RANK)) == MOVE_FLAG_SINGLE; }
        constexpr bool isS3Flush() const { return holdsBits<data_t>(i_, (MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM)); }
        
        constexpr bool isEqualRankSuits(uint32_t r, uint32_t s) const {
            // rank と スートが一致するか
            return ((uint32_t)i_ & (MOVE_FLAG_RANK | MOVE_FLAG_SUITS)) == ((r << MOVE_LCT_RANK) | (s << MOVE_LCT_SUITS));
        }

        constexpr uint32_t domInevitably() const { return i_ & MOVE_FLAG_INEVITDOM; }
        
        constexpr uint32_t flipsPrmOrder() const { return i_ & MOVE_FLAG_PRMORD; }
        constexpr uint32_t flipsTmpOrder() const { return i_ & MOVE_FLAG_TMPORD; }
        
        constexpr uint32_t changesPrmState() const { return flipsPrmOrder(); }
        
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
        
        template <int IS_PASS = _BOTH>
        Cards cards() const { // カード集合を得る
            // 本来は着手表現からの計算で得るのは止めた方が良いかもしれない
            Cards res;
            if (IS_PASS == _YES || (IS_PASS != _NO && isPASS())) return CARDS_NULL;
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
                    subtrCards(&res, Rank4xSuitsToCards(jokerRank4x(), s));
                    addJOKER(&res);
                }
            }
            //tock();
            //cout<<m<<OutCards(res);
            
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
    protected:
        BitSetInRegister<data_t> i_;
    };
    
    using Move = MoveInRegister<uint32_t>;
    using Move16 = MoveInRegister<uint16_t>;
    
    class MeldChar : public Move {
        // Moveと全く同じデータなのだが、構成するカード集合に言及する気がないときにこっちで表記
    public:
        MeldChar(const Move& arg) : Move(arg.data()) {}
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
        out << MeldChar(m) << OutCards(m.cards());
        return out;
    }
    
    class LogMove : public Move {
        // ログ出力用
    public:
        LogMove(const Move& arg) : Move(arg.data()) {}
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
        m.setEffects();
        DERR << "pointer &m = " << (uint64_t)(&m) << endl;
        DERR << "chara " << OutCards(chara) << " used " << OutCards(used) << " -> " << MeldChar(m) << endl;
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
        mv.setEffects();
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