#pragma once

// 支配性(確実に場を流せるか)の判定

namespace UECda {
    
    // 支配性判定の情報系は大きく分けて3種類
    
    // 完全情報支配
    // 主観情報支配
    // 客観情報支配
    
    // さらにUECdaのように独壇場ルールがある場合には、
    
    // 他支配
    // 自己支配
    
    // をいずれも考えるべき
    // 他支配は、特に誰に返されるか等を考えれば、一人一人に対して判定する事も出来る必要があるが
    // 現在は不完全情報ではそこまで考えていない
    
    
    // 主観情報支配では、
    // 全ての情報を加味すると面倒なので
    // 読み抜けを認めて少ない情報で判断することを許容する
    // どの情報を用いているかは他の部分との関連するので注意
    
    // さらに、ロジック発動のため、
    // 当着手時点での支配だけでなく永続的な支配についても考える必要がある
    
    // プリミティブな型のみによる関数はグローバルにしておく
    
    /**************************カード集合に対する支配**************************/
    
    // カード集合の付加情報が計算されていない場合の支配性判定

    bool dominatesCards(const Move m, const Cards oc, const Board b) {
        // カード集合に対する支配性の判定
        if (m.isPASS()) return false;
        if (m.domInevitably()) return true;
        if (b.domConditionally(m)) return true;

        if (m.isSingleJOKER()) return !containsS3(oc);
        if (!m.isSeq() && m.qty() <= oc.joker()) return false;

        Cards zone = -1;
        if (b.afterSuitsLocked(m)) zone &= SuitsToCards(m.suits());
        if (!m.isSeq()) {
            zone &= ORToGValidZone(b.afterTmpOrder(m), m.rank());
            return !canMakeGroup(oc & zone, m.qty() - oc.joker());
        } else {
            zone &= ORQToSCValidZone(b.afterTmpOrder(m), m.rank(), m.qty());
            return !canMakeSeq((oc & zone) | (oc & CARDS_JOKER), m.qty());
        }
        return false;
    }
    
    // 引数として場を取った場合
    // パスの時は場を更新してから判定しても仕方ないので注意
    bool dominatesCards(const Board b, const Cards cards) {
        if (b.isNull()) return false;
        if (b.domInevitably()) return true; // ジョーカースペ3の場もこれで判定出来るようにしている
        
        int qty = b.qty();
        Cards zone = -1;
        
        if (b.isSeq()) {
            if (b.suitsLocked()) { // スートロック
                zone = ORSQToSCValidZone(b.tmpOrder(), b.rank(), b.suits(), qty);
            } else {
                zone = ORQToSCValidZone(b.tmpOrder(), b.rank(), qty);
            }
            return !canMakeSeq(andCards(cards, zone) | andCards(cards, CARDS_JOKER), qty);
        } else {
            if (qty > 4) return true;
            if (qty == 1) {
                if (b.isSingleJOKER()) {
                    return !containsS3(cards);
                } else {
                    if (containsJOKER(cards)) return false;
                }
            }
            
            if (containsJOKER(cards)) qty--;
            if (b.suitsLocked()) {
                zone = ORSToGValidZone(b.tmpOrder(), b.rank(), b.suits());
            } else {
                zone = ORToGValidZone(b.tmpOrder(), b.rank());
            }
            return !canMakeGroup(andCards(cards, zone), qty);
        }
        UNREACHABLE;
    }
    
    bool dominatesHand(const Move mv, const Hand& opsHand, const Board b) {
        // Hand型への支配(着手を引数に取る場合)
        // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
        assert(opsHand.exam_jk());
        assert(opsHand.exam_seq());
        assert(opsHand.exam_nd());
        
        if (mv.isPASS()) return false;
        if (mv.domInevitably()) return true;
        if (b.domConditionally(mv)) return true;
        
        if (!mv.isSeq()) {
            // Joker -> S3 を判定
            if (mv.isSingleJOKER()) {
                return !containsS3(opsHand.cards);
            }
            int aftTmpOrd = b.afterTmpOrder(mv);
            if (mv.qty() > 4) { return true; }
            if (mv.charaPQR() & opsHand.nd[aftTmpOrd]) { // 無支配型と交差あり
                if (b.locksSuits(mv)) { // スートロックの場合はまだ支配可能性あり
                    uint32_t qty = mv.qty();
                    qty -= opsHand.jk;
                    Cards zone = ORSToGValidZone(aftTmpOrd, mv.rank(), mv.suits());
                    if (qty) {
                        return !canMakeGroup(andCards(opsHand.cards, zone), qty);
                    }
                }
                return false;
            } else {
                return true;
            }
        } else {
            assert(mv.isSeq());
            if (anyCards(opsHand.seq)) {
                uint32_t qty = mv.qty();
                Cards zone;
                int aftTmpOrd = b.afterTmpOrder(mv);
                if (b.locksSuits(mv)) { // スートロック
                    zone = ORSQToSCValidZone(aftTmpOrd, mv.rank(), mv.suits(), qty);
                } else {
                    zone = ORQToSCValidZone(aftTmpOrd, mv.rank(), qty);
                }
                return !canMakeSeq(andCards(opsHand.cards, zone) | andCards(opsHand.cards, CARDS_JOKER), qty);
            } else {
                return true;
            }
        }
        UNREACHABLE;
    }
    
    bool dominatesHand(const Board b, const Hand& opsHand) {
        // Hand型への支配(場を引数に取る場合)
        // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
        assert(opsHand.exam_jk());
        assert(opsHand.exam_seq());
        assert(opsHand.exam_nd());
        
        if (b.isNF()) { return false; }
        if (b.domInevitably()) { return true; }
        
        if (!b.isSeq()) { // グループ
            if (b.isSingleJOKER()) {
                return !containsS3(opsHand.cards);
            }
            if (b.qty() > 4) return true;
            Move mv = Move(b);
            Cards pqr = mv.charaPQR();
            if (pqr & opsHand.nd[b.tmpOrder()]) { // 無支配型と交差あり
                if (b.suitsLocked()) { // スートロックの場合はまだ支配可能性あり
                    uint32_t qty = b.qty();
                    qty -= opsHand.jk;
                    Cards zone = ORSToGValidZone(b.tmpOrder(), b.rank(), b.suits());
                    if (qty) {
                        return !canMakeGroup(andCards(opsHand.cards, zone) ,qty);
                    }
                }
                return false;
            } else {
                return true;
            }
        } else { // 階段
            if (anyCards(opsHand.seq)) {
                Cards zone;
                int qty = b.qty();
                if (b.suitsLocked()) { // スートロック
                    zone = ORSQToSCValidZone(b.tmpOrder(), b.rank(), b.suits(), qty);
                } else {
                    zone = ORQToSCValidZone(b.tmpOrder(), b.rank(), qty);
                }
                return !canMakeSeq(andCards(opsHand.cards, zone) | andCards(opsHand.cards, CARDS_JOKER), qty);
            } else {
                return true;
            }
        }
        UNREACHABLE;
    }
    
    template<class move_t, class sbjField_t>
    void setDomState(move_t *const buf, const int candidates, const sbjField_t& field) {
        // 支配関係のパラメータを一括計算しPlaySpaceに入れる
        // クライアント用に詳しく計算。
        const int turnPlayer = field.getTurnPlayer();
        const Cards myCards = field.getCards(turnPlayer);
        const int NMyCards = field.getNCards(turnPlayer);
        const Cards opsCards = field.getOpsCards(turnPlayer);
        
        const Board& b = field.getBoard();
        const FieldAddInfo& fieldInfo = field.fieldInfo;
        
        const int ord = b.tmpOrder();
        
        if (fieldInfo.isTmpOrderSettled()) {
            // オーダー固定
            
            // 現在オーダーについてのみ調べる
            for (int m = candidates - 1; m >= 0; --m) {
                move_t *const mv = &buf[m];
                
                // 無条件
                if (mv->domInevitably()) { // 永続的完全支配
                    mv->setP_DALL();
                    continue;
                }
                
                if (mv->cards() == (CARDS_S3 | CARDS_JOKER)) { mv->setP_NFDALL_only(); continue; } // 永続的完全支配と等価(特別)
                
                const uint32_t qty = mv->qty();
                
                // 空場
                if ((qty > fieldInfo.getMaxNCards())
                   || dominatesCards(mv->mv(), opsCards, OrderToNullBoard(ord))) {
                    mv->setP_NFDO(); // 永続的空場他支配
                }
                //if ((qty > fieldInfo.getMaxNCards()) || ::dominatesCards(mv,subtrCards(myCards,mv.extract()),makeBoardNF(ord))) {
                // mInfo->setP_NFDM();//永続的空場自己支配
                //}
            }
            
        } else {
            // オーダー固定でない
            
            // 現在オーダーと逆転オーダーの両方を調べる
            for (int m = candidates - 1; m >= 0; --m) {
                move_t *const mv = &buf[m];
                
                // 無条件
                if (mv->domInevitably()) { // 永続的完全支配
                    mv->setP_DALL();
                    continue;
                }
                
                if (mv->cards() == (CARDS_S3 | CARDS_JOKER)) { mv->setP_NFDALL_only(); continue; } // 永続的完全支配と等価(特別)
                
                const uint32_t qty = mv->qty();
                if (qty > fieldInfo.getMaxNCards()) {
                    mv->setP_NFDO();
                }else if (dominatesCards(mv->mv(), opsCards, OrderToNullBoard(ord))) {
                    if (dominatesCards(mv->mv(), opsCards, OrderToNullBoard(flipOrder(ord)))) {
                        mv->setP_NFDO(); // 永続的空場他支配
                    } else {
                        mv->setE_NFDO(); // 一時的空場他支配
                    }
                }
            }
            
            
        }
        
        // 当座
        // 他の情報と被るので無駄がある
        for (int m = candidates - 1; m >= 0; --m) {
            move_t *const mv = &buf[m];
            
            const uint32_t qty = mv->qty();
            
            if (qty > fieldInfo.getMaxNCardsAwake()
               || dominatesCards(mv->mv(), opsCards, b)) {
                mv->setDO(); // 当座他支配
            }
            if (mv->isPASS()
               || qty > NMyCards - qty
               || dominatesCards(mv->mv(), myCards - mv->cards(), b)) {
                mv->setDM(); // 当座自己支配
            }
        }
    }
}
