#pragma once

#include "daifugo.hpp"
#include "card.hpp"
#include "move.hpp"

namespace UECda {
    
    /**************************場表現**************************/
    
    // 各プレーヤーの情報を持たない場表現
    // 32ビット着手表現と同一の系列で扱えるようにする
    // ジョーカー情報などを残すかは難しいが、現在はほとんどの情報を残したまま
    
    struct Board {
        uint32_t b;
        
        constexpr Board() :b() {}
        constexpr Board(uint32_t arg) : b(arg) {}
        constexpr Board(const Board& arg) : b(arg.b) {}
        
        constexpr operator uint32_t() const { return b; }
        constexpr operator uint64_t() const { return (uint64_t)b; }
        
        void init() { b = 0U; }
        
        // set, fix
        
        void setTmpOrder(const uint32_t ord) { b |= ord << MOVE_LCT_TMPORD; }
        void setPrmOrder(const uint32_t ord) { b |= ord << MOVE_LCT_PRMORD; }
        
        void fixTmpOrder(const uint32_t ord) { b = (b & ~MOVE_FLAG_TMPORD) | (ord << MOVE_LCT_TMPORD); }
        void fixPrmOrder(const uint32_t ord) { b = (b & ~MOVE_FLAG_PRMORD) | (ord << MOVE_LCT_PRMORD); }
        
        void flipTmpOrder() { b ^= MOVE_FLAG_TMPORD; }
        void flipPrmOrder() { b ^= MOVE_FLAG_PRMORD; }
        
        void setExceptOrder(const uint32_t info) { b |= info; }
        void fixExceptOrder(const uint32_t info) { b = (b & MOVE_FLAG_ORD) | info; }
        
        void resetDom() { b &= ~MOVE_FLAG_INVALID; }
        
        // 2体情報をメンバ関数で返す関数
        // 半マスク化みたいな感じ
        constexpr uint32_t domConditionally(Move m) const { return isSingleJOKER() && m.isS3Flush(); }
        
        constexpr bool locksSuits(Move m) const { return suitsPart() == m.suitsPart(); }
        constexpr bool locksRank(Move m) const { return false; } // ルールにない
        
        constexpr uint32_t afterPrmOrder(Move m) const {
            return prmOrder() ^ bool(m.isRev());
        }
        constexpr uint32_t afterTmpOrder(Move m) const {
            return order() ^ bool(m.isRev()) ^ bool(m.isBack());
        }
        
        constexpr uint32_t isAfterTmpOrderReversed(Move m) const { return (b ^ m) & (1U << MOVE_LCT_TMPORD); }
        constexpr uint32_t isAfterPrmOrderReversed(Move m) const { return (b ^ m) & (1U << MOVE_LCT_PRMORD); }
            
        constexpr bool afterSuitsLocked(Move m) const {
            return suitsLocked() || locksSuits(m);
        }
        
        // get
        constexpr int prmOrder()   const { return (b >> MOVE_LCT_PRMORD) & 1U; }
        constexpr int order()      const { return (b >> MOVE_LCT_TMPORD) & 1U; }

        constexpr uint32_t suits() const { return (b >> MOVE_LCT_SUITS) & 15U; }
        constexpr int qty()        const { return (b >> MOVE_LCT_QTY) & 15U; }
        constexpr int rank()       const { return (b >> MOVE_LCT_RANK) & 15U; }
        constexpr int jokerRank()  const { return (b >> MOVE_LCT_JKRANK) & 15U; }
        constexpr uint32_t jokerSuits() const { return (b >> MOVE_LCT_JKSUITS) & 15U; }
        
        // 部分に着目する
        constexpr uint32_t orderPart()     const { return b & MOVE_FLAG_ORD; }
        constexpr uint32_t exceptOrderPart() const { return b & ~MOVE_FLAG_ORD; }
        constexpr uint32_t suitsPart()     const { return b & MOVE_FLAG_SUITS; }
        constexpr uint32_t rankPart()      const { return b & MOVE_FLAG_RANK; }
        constexpr uint32_t qtyPart()       const { return b & MOVE_FLAG_QTY; }
        constexpr uint32_t typePart()      const { return b & MOVE_FLAG_TYPE; } // サイズ＋形式
        constexpr uint32_t jokerPart()     const { return b & MOVE_FLAG_JK; } // ジョーカー関連
        constexpr uint32_t exeptJokerPart() const { return b & ~MOVE_FLAG_JK; } // ジョーカー関連以外
        
        // true or false
        constexpr uint32_t isNotNull() const { return b & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP | MOVE_FLAG_SEQ); }
        constexpr bool isNull() const { return !isNotNull(); }
        constexpr uint32_t suitsLocked() const { return b & MOVE_FLAG_SUITSLOCK; }
        constexpr uint32_t rankLocked() const { return b & MOVE_FLAG_RANKLOCK; }
        
        constexpr bool isRev() const { return b & MOVE_FLAG_PRMORD; }
        
        constexpr uint32_t containsJOKER() const { return b & MOVE_FLAG_JK; }
        
        constexpr bool isSingleJOKER() const { return (b & (MOVE_FLAG_SINGLE | MOVE_FLAG_RANK)) == MOVE_FLAG_SINGLE; }
        //constexpr bool isS3Flush() const { return holdsBits(b, (MOVE_FLAG_SINGLE | MOVE_FLAG_CONDDOM)); }
        constexpr uint32_t domInevitably() const { //return b & MOVE_FLAG_INEVITDOM;
            return Move(b).domInevitably();
        }
        
        constexpr uint32_t isSeq() const { return b & MOVE_FLAG_SEQ; }
        constexpr uint32_t isGroup() const { return b & MOVE_FLAG_GROUP; }
        constexpr uint32_t isSingle() const { return b & MOVE_FLAG_SINGLE; }
        constexpr uint32_t isSingleOrGroup() const { return b & (MOVE_FLAG_SINGLE | MOVE_FLAG_GROUP); }
        bool isQuintuple() const {
            return typePart() == (MOVE_FLAG_GROUP | (5U << MOVE_LCT_QTY));
        }
        bool isInvalid() const {
            return (b & MOVE_FLAG_INVALID) || domInevitably();
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
        // 進行
        void procOrder(Move m) {
            if (m.isRev()) {
                flipTmpOrder();
                flipPrmOrder();
            }
            if (m.isBack()) flipTmpOrder();
        }
        
        void flush() {
            b &= MOVE_FLAG_ORD;
            fixTmpOrder(prmOrder());
        }
        
        void lockSuits() { b |= MOVE_FLAG_SUITSLOCK; }
        
        void procPASS() const {} //何もしない
        
        void proc(Move m) { // プレーヤー等は関係なく局面のみ進める
            if (m.isPASS()) procPASS();
            else {
                if (m.domInevitably() || domConditionally(m)) { // 無条件完全支配
                    if (m.isRev()) flipPrmOrder();
                    flush();
                } else {
                    procOrder(m);
                    if (isNull()) {
                        setExceptOrder(m.exceptOrderPart()); // 一時情報入れ替え
                    } else {
                        // スートロック
                        if (!suitsLocked()) if (locksSuits(m)) lockSuits();
                        // 一時情報入れ替え
                        b = (b & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                            | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD));
                    }
                }
            }
        }
        
        void procAndFlush(Move m) {
            // 局面を更新し、強引に場を流す
            if (!m.isPASS()) procOrder(m); // オーダーフリップ
            flush();
        }
        
        void procExceptFlush(Move m) {
            // 局面を更新するが場を流さない
            procOrder(m);
            // スートロック
            if (!suitsLocked()) {
                // スートが一緒だったらロック処理
                if (locksSuits(m)) lockSuits();
            }
            if (m.domInevitably() || domConditionally(m)) { // Joker->S3のみ
                b = ((b & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                      | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD)));
                // ８切りと同じように無条件支配フラグをたてておく
                b |= MOVE_FLAG_INVALID;
            } else{
                b = (b & (MOVE_FLAG_LOCK | MOVE_FLAG_ORD))
                     | (m & ~(MOVE_FLAG_LOCK | MOVE_FLAG_ORD));
            }
        }
    };
    
    constexpr Board OrderToNullBoard(int o) {
        return Board(o << MOVE_LCT_PRMORD | o << MOVE_LCT_TMPORD);
    }
    
    constexpr Move BoardToMove(const Board& b) {
        // 場->場役へと変化させる
        // 場役へとコンバート出来ない部分は変えない
        return Move(b.b);
    }
    constexpr Board MoveToBoard(const Move& mv) {
        // 場->場役へと変化させる
        return Board(mv.m_);
    }
    
    static std::ostream& operator <<(std::ostream& out, const Board& b) { // Board出力
        if (b.isNull()) {
            out << "NULL";
        } else {
            Move m = BoardToMove(b); // 場役へ変化
            out << m;
        }
        // オーダー...一時オーダーのみ
        out << "  Order : ";
        if (b.order() == 0) {
            out << "NORMAL";
        } else {
            out << "REVERSED";
        }
        out << "  Suits : ";
        if (b.suitsLocked()) {
            out << "LOCKED";
        } else {
            out << "FREE";
        }
        return out;
    }
    
    inline bool isSubjectivelyValid(Board b, Move mv, const Cards& c, const uint32_t q) {
        // 不完全情報の上での合法性判定
        // c はそのプレーヤーが所持可能なカード
        // q はそのプレーヤーの手札枚数（公開されている情報）
        if (mv.isPASS()) return true;
        // 枚数オーバー
        if (mv.qty() > q) return false;
        // 持っていないはずの札を使った場合
        if (!holdsCards(c, mv.cards())) return false;
        if (!b.isNull()) {
            if (b.typePart() != mv.typePart()) return false; // 型違い
            if (b.isSeq()) {
                if (!isValidSeqRank(mv.rank(), b.order(), b.rank(), mv.qty())) {
                    return false;
                }
                if (b.suitsLocked()) {
                    if (b.suits() != mv.suits()) return false;
                }
            } else {
                if (b.isSingleJOKER()) {
                    return mv.isS3Flush();
                }
                if (mv.isSingleJOKER()) {
                    if (!b.isSingle()) return false;
                } else {
                    if (!isValidGroupRank(mv.rank(), b.order(), b.rank())) {
                        return false;
                    }
                    if (b.suitsLocked()) {
                        if (b.suits() != mv.suits()) return false;
                    }
                }
            }
        }
        return true;
    }
}