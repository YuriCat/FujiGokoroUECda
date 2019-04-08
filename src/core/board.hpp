#pragma once

#include "daifugo.hpp"
#include "card.hpp"
#include "move.hpp"

namespace UECda {
    
    /**************************場表現**************************/
    
    // 各プレーヤーの情報を持たない場表現
    // 32ビット着手表現と同一の系列で扱えるようにする
    // ジョーカー情報などを残すかは難しいが、現在はほとんどの情報を残したまま
    
    struct Board : public Move {
        
        void init() { clear(); }
        Move move() const { return Move(*this); }
        
        // set, fix
        
        void setTmpOrder(uint32_t ord) { Move::o = ord; }
        void setPrmOrder(uint32_t ord) { Move::po = ord; }
        
        void fixTmpOrder(uint32_t ord) { setTmpOrder(ord); }
        void fixPrmOrder(uint32_t ord) { setPrmOrder(ord); }
        
        void flipTmpOrder() { Move::o ^= 1; }
        void flipPrmOrder() { Move::po ^= 1; }
        
        void resetDom() { Move::invalid = 0; }
        
        // 2体情報をメンバ関数で返す関数
        // 半マスク化みたいな感じ
        constexpr uint32_t domConditionally(Move m) const { return isSingleJOKER() && m.isS3(); }
        
        constexpr bool locksSuits(Move m) const { return !isNull() && suits() == m.suits(); }
        constexpr bool locksRank(Move m) const { return false; } // ルールにない
        
        constexpr uint32_t afterPrmOrder(Move m) const {
            return prmOrder() ^ bool(m.isRev());
        }
        constexpr uint32_t afterTmpOrder(Move m) const {
            return order() ^ bool(m.isRev()) ^ bool(m.isBack());
        }
        constexpr bool afterSuitsLocked(Move m) const {
            return suitsLocked() || locksSuits(m);
        }
        
        // get
        constexpr int prmOrder()   const { return Move::po; }
        constexpr int order()      const { return Move::o; }
        
        // true or false
        constexpr bool isNull() const { return type() == 0; }
        constexpr bool suitsLocked() const { return Move::sl; }
        constexpr bool rankLocked() const { return Move::rl; }
        
        constexpr bool isRev() const { return po; }
        
        bool isInvalid() const {
            return Move::invalid;
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
            int ord = Move::po;
            init();
            fixPrmOrder(ord);
            fixTmpOrder(ord);
        }
        void lockSuits() { Move::sl = 1; }
        void procPASS() {} //何もしない

        void fixMeld (Move m) {
            Move::s = m.s;
            Move::r = m.r;
            Move::jks = m.jks;
            Move::jkr = m.jkr;
            Move::q = m.q;
            Move::t = m.t;
        }
        
        void proc(Move m) { // プレーヤー等は関係なく局面のみ進める
            if (m.isPASS()) procPASS();
            else {
                if (m.domInevitably() || domConditionally(m)) { // 無条件完全支配
                    if (m.isRev()) flipPrmOrder();
                    flush();
                } else {
                    procOrder(m);
                    if (locksSuits(m)) lockSuits();
                    fixMeld(m);
                }
            }
        }
        
        void procAndFlush(Move m) {
            // 局面を更新し、強引に場を流す
            if (m.isRev()) flipPrmOrder();
            flush();
        }
        
        void procExceptFlush(Move m) {
            // 局面を更新するが場を流さない
            procOrder(m);
            // スートロック
            if (locksSuits(m)) lockSuits();
            fixMeld(m);
            if (m.domInevitably() || domConditionally(m)) {
                invalid = 1;
            }
        }
    };
    
    Board OrderToNullBoard(int o) {
        Board b;
        b.init();
        b.o = o; b.po = o;
        return b;
    }
    
    static std::ostream& operator <<(std::ostream& out, const Board& b) { // Board出力
        if (b.isNull()) out << "NULL";
        else out << b.move();
        // オーダー...一時オーダーのみ
        out << "  Order : ";
        if (b.order() == 0) out << "NORMAL";
        else out << "REVERSED";
        out << "  Suits : ";
        if (b.suitsLocked()) out << "LOCKED";
        else out << "FREE";
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
            if (b.type() != mv.type()) return false; // 型違い
            if (b.isSeq()) {
                if (!isValidSeqRank(mv.rank(), b.order(), b.rank(), mv.qty())) {
                    return false;
                }
                if (b.suitsLocked()) {
                    if (b.suits() != mv.suits()) return false;
                }
            } else {
                if (b.isSingleJOKER()) {
                    return mv.isS3();
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