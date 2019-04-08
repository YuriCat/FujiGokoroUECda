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
    
    /**************************カード集合に対する支配**************************/
    
    // カード集合の付加情報が計算されていない場合の支配性判定

    inline bool dominatesCards(const Move m, const Cards oc, const Board b) {
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
            return !canMakeSeq(oc & zone, oc.joker(), m.qty());
        }
    }
    
    // 引数として場を取った場合
    // パスの時は場を更新してから判定しても仕方ないので注意
    bool dominatesCards(const Board b, const Cards oc) {
        if (b.isNull()) return false;
        if (b.domInevitably()) return true;

        if (b.isSingleJOKER()) return !containsS3(oc);
        if (!b.isSeq() && b.qty() <= oc.joker()) return false;
        
        Cards zone = -1;
        if (b.suitsLocked()) zone &= SuitsToCards(b.suits());

        if (!b.isSeq()) {
            zone &= ORToGValidZone(b.order(), b.rank());
            return !canMakeGroup(oc & zone, b.qty() - oc.joker());
        } else {
            zone &= ORQToSCValidZone(b.order(), b.rank(), b.qty());
            return !canMakeSeq(oc & zone, oc.joker(), b.qty());
        }
    }
    
    inline bool dominatesHand(const Move m, const Hand& oh, const Board b) {
        // Hand型への支配(着手を引数に取る場合)
        // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
        assert(oh.exam_jk());
        assert(oh.exam_seq());
        assert(oh.exam_nd());
        
        if (m.isPASS()) return false;
        if (m.domInevitably()) return true;
        if (b.domConditionally(m)) return true;

        if (m.isSingleJOKER()) return !containsS3(oh);
        if (!m.isSeq() && m.qty() <= oh.jk) return false;
        
        if (!m.isSeq()) {
            int aftTmpOrd = b.afterTmpOrder(m);
            if (m.qty() > 4) return true;
            if (!(m.charaPQR() & oh.nd[aftTmpOrd])) return true; // 無支配型と交差なし
            if (b.locksSuits(m)) { // スートロックの場合はまだ支配可能性あり
                Cards zone = ORToGValidZone(aftTmpOrd, m.rank());
                zone &= SuitsToCards(m.suits());
                return !canMakeGroup(oh.cards & zone, m.qty() - oh.jk);
            }
        } else {
            if (!oh.seq) return true;
            Cards zone = ORQToSCValidZone(b.afterTmpOrder(m), m.rank(), m.qty());
            if (b.locksSuits(m)) zone &= SuitsToCards(m.suits());
            return !canMakeSeq(oh.cards & zone, oh.jk, m.qty());
        }
        return false;
    }
    
    inline bool dominatesHand(const Board b, const Hand& oh) {
        // Hand型への支配(場を引数に取る場合)
        // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
        assert(oh.exam_jk());
        assert(oh.exam_seq());
        assert(oh.exam_nd());
        
        if (b.isNull()) return false;
        if (b.domInevitably()) return true;

        if (b.isSingleJOKER()) return !containsS3(oh);
        if (!b.isSeq() && b.qty() <= oh.jk) return false;
        
        if (!b.isSeq()) { // グループ
            if (b.qty() > 4) return true;
            Move m = Move(b);
            Cards pqr = m.charaPQR();
            if (!(pqr & oh.nd[b.order()])) return true; // 無支配型と交差なし
            if (b.suitsLocked()) { // スートロックの場合はまだ支配可能性あり
                Cards zone = ORToGValidZone(b.order(), b.rank());
                zone &= SuitsToCards(b.suits());
                return !canMakeGroup(oh.cards & zone, b.qty() - oh.jk);
            }
        } else { // 階段
            if (!oh.seq) return true;
            int qty = b.qty();
            Cards zone = ORQToSCValidZone(b.order(), b.rank(), qty);
            if (b.suitsLocked()) zone &= SuitsToCards(b.suits());
            return !canMakeSeq(oh.cards & zone, oh.jk, qty);
        }
        return false;
    }
    
    template <class move_t, class sbjField_t>
    void setDomState(move_t *const buf, const int candidates, const sbjField_t& field) {
        // 支配関係のパラメータを一括計算しPlaySpaceに入れる
        // クライアント用に詳しく計算。
        const int turnPlayer = field.turn();
        const Cards myCards = field.getCards(turnPlayer);
        const int NMyCards = field.getNCards(turnPlayer);
        const Cards opsCards = field.getOpsCards(turnPlayer);
        
        const Board& b = field.board;
        const FieldAddInfo& fieldInfo = field.fieldInfo;
        
        const int ord = b.order();
        
        if (fieldInfo.isOrderSettled()) {
            // オーダー固定
            
            // 現在オーダーについてのみ調べる
            for (int i = 0; i < candidates; i++) {
                move_t *const m = &buf[i];
                
                // 無条件
                if (m->domInevitably()) { // 永続的完全支配
                    m->setP_DALL();
                    continue;
                }
                
                if (m->cards() == (CARDS_S3 | CARDS_JOKER)) { m->setP_NFDALL_only(); continue; } // 永続的完全支配と等価(特別)
                
                const uint32_t qty = m->qty();
                
                // 空場
                if ((qty > fieldInfo.getMaxNCards())
                   || dominatesCards(m->mv(), opsCards, OrderToNullBoard(ord))) {
                    m->setP_NFDO(); // 永続的空場他支配
                }
                //if ((qty > fieldInfo.getMaxNCards()) || ::dominatesCards(m,subtrCards(myCards,m.extract()),makeBoardNF(ord))) {
                // mInfo->setP_NFDmv();//永続的空場自己支配
                //}
            }
            
        } else {
            // オーダー固定でない
            
            // 現在オーダーと逆転オーダーの両方を調べる
             for (int i = 0; i < candidates; i++) {
                move_t *const m = &buf[i];
                
                // 無条件
                if (m->domInevitably()) { // 永続的完全支配
                    m->setP_DALL();
                    continue;
                }
                
                if (m->cards() == (CARDS_S3 | CARDS_JOKER)) { m->setP_NFDALL_only(); continue; } // 永続的完全支配と等価(特別)
                
                const uint32_t qty = m->qty();
                if (qty > fieldInfo.getMaxNCards()) {
                    m->setP_NFDO();
                }else if (dominatesCards(m->mv(), opsCards, OrderToNullBoard(ord))) {
                    if (dominatesCards(m->mv(), opsCards, OrderToNullBoard(flipOrder(ord)))) {
                        m->setP_NFDO(); // 永続的空場他支配
                    } else {
                        m->setE_NFDO(); // 一時的空場他支配
                    }
                }
            }
            
            
        }
        
        // 当座
        // 他の情報と被るので無駄がある
        for (int i = 0; i < candidates; i++) {
            move_t *const m = &buf[i];
            
            const uint32_t qty = m->qty();
            
            if (qty > fieldInfo.getMaxNCardsAwake()
               || dominatesCards(m->mv(), opsCards, b)) {
                m->setDO(); // 当座他支配
            }
            if (m->isPASS()
               || qty > NMyCards - qty
               || dominatesCards(m->mv(), myCards - m->cards(), b)) {
                m->setDM(); // 当座自己支配
            }
        }
    }
}
