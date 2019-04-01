#pragma once

// 大富豪ゲームの性質を用いた演算
// 機械学習ベースとなったので複雑なロジックが有効な機会は最早少ないかもしれない

#include "daifugo.hpp"

namespace UECda {
    
    /**************************最小分割数計算**************************/
    
    // paoon氏のbeersongのアイデアを利用
    template<class move_t>
    int calcMinNMelds(move_t *const mv, const Cards c) {
        
        int ret = countCards(CardsToER(c)); // 階段なしの場合の最小分割数
        const int cnt = genAllSeq(mv, c);
        if (cnt) {
            // 階段を使った場合の方が分割数を減らせる場合を考慮
            move_t *const new_buf = mv + cnt;
            for (int i = 0; i < cnt; ++i) {
                assert(holdsCards(c, mv[i].cards()));
                
                Cards tmp = subtrCards(c, mv[i].cards());
                int nret = calcMinNMelds(new_buf, tmp) + 1;
                ret = min(ret, nret);
            }
        }
        return ret;
    }
    
    /**************************ゲーム段階**************************/
    
    constexpr bool isNoBack(const Cards mine, const Cards ops) {
        return true; // イレブンバックはUECdaにはない
    }
    constexpr bool isNoBack(const Cards mine) {
        return true; // イレブンバックはUECdaにはない
    }
    
    bool isNoRev(const Cards mine, const Cards ops) {
        // 無革命性の証明
        return !groupCards(ops, 4) && !canMakeSeq(ops, 5)
                && !groupCards(mine, 4) && !canMakeSeq(mine, 5);
    }
    
    bool isNoRev(const Cards mine) {
        // 無革命性の証明
        return !groupCards(mine, 4) && !canMakeSeq(mine, 5);
    }
    
    /**************************支配保証、空場期待**************************/
    
    // 支配保証、空場期待、半支配保証半空場期待についての解析
    // 革命返しのときには間違っていることがある
    
    static inline Cards getAllDWCards(const Cards mine, const Cards ops,
                                      const int order, const uint64_t set) {
        // 全ての単支配保証カードを返す
        // 自分のジョーカーは考えない(滅多に支配保証にならないので)
        Cards dw = mine & CARDS_8;
        
        if (set) {
            if (!containsJOKER(ops)) {
                if (order == ORDER_NORMAL) {
                    IntCard ic = pickIntCardHigh(ops);
                    int r4x = IntCardToRank4x(ic);
                    addCards(&dw, mine & RankRange4xToCards(r4x, RANK_MAX * 4));
                } else {
                    IntCard ic = pickIntCardLow(ops);
                    int r4x = IntCardToRank4x(ic);
                    addCards(&dw, mine & RankRange4xToCards(RANK_MIN * 4, r4x));
                }
            }
        }
        return dw;
    }
    
    // 支配保証ありorなし判定
    static inline bool hasDW(const Cards myCards, const Cards opsCards,
                             const int order, const uint64_t set) {
        // Cards型から
        if (myCards & CARDS_8)return true; // 8切り
        if (!set)return false; // オーダー未固定では8切りオンリー
        
        uint32_t mine = 0;
        uint32_t ops;
        Cards tmp;
        
        if (containsJOKER(opsCards)) { ops = 1; } else { ops = 0; }
        
        if (order == ORDER_NORMAL) {
            //int myR = getHighRankx4(myCards);
            // 上のランクからスキャン
            for (tmp = CARDS_2; tmp > CARDS_8; tmp >>= 4) {
                Cards rc = myCards & tmp;
                if (rc) {
                    if (ops == 0) {
                        return true;
                    } else {
                        mine += countCards(rc);
                        if (mine > ops)return true;
                    }
                }
                if (opsCards & tmp) {
                    ops += countCards(opsCards & tmp);
                }
            }
        } else {
            for (tmp = CARDS_3; tmp < CARDS_8; tmp <<= 4) {
                Cards rc = myCards & tmp;
                if (rc) {
                    if (ops == 0) {
                        return true;
                    } else {
                        mine += countCards(rc);
                        if (mine > ops)return true;
                    }
                }
                if (opsCards & tmp) {
                    ops += countCards(opsCards & tmp);
                }
            }
        }
        return false;
    }
    
    static inline Cards getAllNFHCards(const Cards mine, const Cards ops,
                                       const int order, const uint64_t set) {
        // 全ての単空場期待カードを返す
        if (!set) return CARDS_NULL;
        Cards ret;
        if (order == ORDER_NORMAL) {
            IntCard ic = pickIntCardLow(ops);
            int r4x = IntCardToRank4x(ic);
            ret = mine & RankRange4xToCards(RANK_MIN * 4, r4x);
        } else {
            IntCard ic = pickIntCardHigh(ops);
            int r4x = IntCardToRank4x(ic);
            ret = mine & RankRange4xToCards(r4x, RANK_MAX  *4);
        }
        if (containsS3(ret)) {
            if (containsJOKER(addCards(mine, ops))) ret -= CARDS_S3;
        }
        return ret;
    }
    
    // 完全空場期待
    static inline bool hasNFH(const Cards myCards, const Cards opsCards,
                              const int order, const uint64_t set) {
        
        // Cards型から
        if (!set)return false; // オーダー未固定ではおこらない
        
        uint32_t mine = 0;
        uint32_t ops;
        Cards tmp;
        
        if (containsJOKER(opsCards)) { ops = 1; } else { ops = 0; }
        
        if (order == ORDER_NORMAL) {
            if (ops || containsJOKER(myCards)) {
                if (commonCards(myCards, maskCards(CARDS_3, CARDS_S3)))return true;
            } else {
                if (commonCards(myCards, CARDS_3))return true;
            }
            if (opsCards & CARDS_3) { ops += countCards(opsCards & CARDS_3); }
            
            for (tmp = CARDS_4; tmp < CARDS_8; tmp <<= 4) {
                Cards rc = myCards & tmp;
                if (rc) {
                    if (ops == 0) {
                        return true;
                    } else {
                        mine += countCards(rc);
                        if (mine > ops)return true;
                    }
                }
                if (opsCards & tmp) {
                    ops += countCards(opsCards & tmp);
                }
            }
            
        } else {
            if (commonCards(myCards, CARDS_2))return true;
            for (tmp = CARDS_A; tmp > CARDS_8; tmp >>= 4) {
                Cards rc = myCards & tmp;
                if (rc) {
                    if (ops == 0) {
                        return true;
                    } else {
                        mine += countCards(rc);
                        if (mine > ops)return true;
                    }
                }
                if (opsCards & tmp) {
                    ops += countCards(opsCards & tmp);
                }
            }
        }
        return false;
    }
    
    // 半支配保証 & 半空場期待
    static inline bool hasDWorNFH(const Cards myCards, const Cards opsCards,
                                  const int tmpOrder, const uint64_t set) {
        
        if (set) { // オーダー固定
            if (hasDW(myCards, opsCards, tmpOrder, 1))return true;
            if (hasNFH(myCards, opsCards, tmpOrder, 1))return true;
            if (tmpOrder == ORDER_NORMAL || (!containsJOKER(opsCards))) {
                if (containsS3(myCards))return true; // 通常オーダーか、相手がジョーカーを持たない場合はS3は空場または支配場にしか出せないので
            }
        } else { // オーダー反転可能性あり
            if (myCards & CARDS_8)return true;
            if (!containsJOKER(opsCards)) {
                if (containsS3(myCards))return true; // 相手がジョーカーを持たない場合はS3は空場または支配場にしか出せないので
            }
            if (
               (hasDW(myCards, opsCards, 0, 1) && hasNFH(myCards, opsCards, 1, 1))
               ||
               (hasDW(myCards, opsCards, 1, 1) && hasNFH(myCards, opsCards, 0, 1))
               )return true;
            
        }
        return false;
    }
    
}
