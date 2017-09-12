/*
 dominance.hpp
 Katsuki Ohto
 */

// 支配性の判定

#ifndef UECDA_LOGIC_DOMINANCE_HPP_
#define UECDA_LOGIC_DOMINANCE_HPP_

namespace UECda{
    
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
    
    // 使用機会は少ないと思われるが、カード集合の付加情報が計算されていない場合の支配性判定
    
    constexpr bool dominatesCards_PASS(const Cards cards){ return false; }
    bool dominatesCards_SINGLEJOKER(const Cards cards){ return !containsS3(cards); }
    bool dominatesCards_NF_Group(const int order, const int rank, int qty, const Cards opsCards){
        if(containsJOKER(opsCards)){ --qty; }
        if(qty == 0)return false;
        return !judgeSuitComp(andCards(opsCards, ORToGValidZone(order, rank)), qty);
    }
    
    bool dominatesCards_NF_Single(const Move mv, const Cards opsCards, const Board bd){
        if(mv.isSingleJOKER()){
            return dominatesCards_SINGLEJOKER(opsCards);
        }else{
            if(containsJOKER(opsCards))return false;
            return !andCards(opsCards, ORToGValidZone(bd.afterTmpOrder(mv), mv.rank()));
        }
    }
    
    bool dominatesCards_Single(const Move mv, const Cards opsCards, const Board bd){
        if(mv.isSingleJOKER()){
            return dominatesCards_SINGLEJOKER(opsCards);
        }else{
            if(containsJOKER(opsCards))return false;
            if(bd.afterSuitsLocked(mv)){
                return !andCards(opsCards, ORSToGValidZone(bd.afterTmpOrder(mv), mv.rank(), mv.suits()));
            }else{
                return !andCards(opsCards, ORToGValidZone(bd.afterTmpOrder(mv), mv.rank()));
            }
        }
    }
    
    bool dominatesCards_NF_Group(const Move mv, const Cards opsCards, const Board bd){
        uint32_t qty = mv.qty();
        Cards zone;
        if(containsJOKER(opsCards)){ --qty; }
        zone = ORToGValidZone(bd.afterTmpOrder(mv), mv.rank());
        return !judgeSuitComp(andCards(opsCards, zone), qty);
    }
    
    bool dominatesCards_Group(const Move mv, const Cards opsCards, const Board bd){
        uint32_t qty = mv.qty();
        Cards zone;
        if(containsJOKER(opsCards)){ --qty; }
        if(bd.afterSuitsLocked(mv)){
            zone = ORSToGValidZone(bd.afterTmpOrder(mv), mv.rank(), mv.suits());
        }else{
            zone = ORToGValidZone(bd.afterTmpOrder(mv), mv.rank());
        }
        return !judgeSuitComp(andCards(opsCards, zone), qty);
    }
    
    bool dominatesCards_NF_Seq(const Move mv, const Cards opsCards, const Board bd){
        Cards zone;
        int qty = mv.qty();
        zone = ORQToSCValidZone(bd.afterTmpOrder(mv), mv.rank(), mv.qty());
        return !canMakeSeq(andCards(opsCards, zone) | andCards(opsCards, CARDS_JOKER), qty);
    }
    
    bool dominatesCards_Seq(const Move mv, const Cards opsCards, const Board bd){
        Cards zone;
        int qty = mv.qty();
        if(bd.afterSuitsLocked(mv)){
            zone = ORSQToSCValidZone(bd.afterTmpOrder(mv), mv.rank(), mv.suits(), mv.qty());
        }else{
            zone = ORQToSCValidZone(bd.afterTmpOrder(mv), mv.rank(), mv.qty());
        }
        return !canMakeSeq(andCards(opsCards, zone) | andCards(opsCards, CARDS_JOKER), qty);
    }
    
    bool dominatesCards(const Move argMove, const Cards opsCards, const Board argBoard){
        // カード集合に対する支配性の判定
        
        if(argMove.isPASS())return dominatesCards_PASS(opsCards);
        if(argMove.domInevitably())return true;
        if(argBoard.domConditionally(argMove)){
            DERR << "CONDITIONAL DOM" << endl;
            return true;
        }
        
        if(argMove.isSingle()){
            return dominatesCards_Single(argMove, opsCards, argBoard);
        }else if(!argMove.isSeq()){
            return dominatesCards_Group(argMove, opsCards, argBoard);
        }else{
            return dominatesCards_Seq(argMove, opsCards, argBoard);
        }
        return false;
    }
    
    bool dominatesCards_NF(const Move argMove, const Cards opsCards, const Board argBoard){
        // カード集合に対する支配性の判定
        if(argMove.isPASS())return dominatesCards_PASS(opsCards);
        if(argMove.domInevitably())return true;
        
        if(argMove.isSingle()){
            return dominatesCards_NF_Single(argMove, opsCards, argBoard);
        }else if(!argMove.isSeq()){
            return dominatesCards_NF_Group(argMove, opsCards, argBoard);
        }else{
            return dominatesCards_NF_Seq(argMove, opsCards, argBoard);
        }
        return false;
    }
    
    // 引数として場を取った場合
    // パスの時は場を更新してから判定しても仕方ないので注意
    bool dominatesCards(const Board bd, const Cards cards){
        
        if(bd.isNF()){ return false; }
        if(bd.domInevitably()){ return true; } // ジョーカースペ3の場もこれで判定出来るようにしている
        
        uint32_t qty = bd.qty();
        Cards zone;
        
        if(bd.isSeq()){
            if(bd.suitsLocked()){ // スートロック
                zone = ORSQToSCValidZone(bd.tmpOrder(), bd.rank(), bd.suits(), qty);
            }else{
                zone = ORQToSCValidZone(bd.tmpOrder(), bd.rank(), qty);
            }
            return !canMakeSeq(andCards(cards, zone) | andCards(cards, CARDS_JOKER), qty);
        }else{
            if(qty > 4){ return true; }
            if(qty == 1){
                if(bd.isSingleJOKER()){
                    if(containsS3(cards)){
                        return false;
                    }else{
                        return true;
                    }
                }else{
                    if(containsJOKER(cards)){
                        return false;
                    }
                }
            }
            
            if(containsJOKER(cards)){ --qty; }
            if(bd.suitsLocked()){
                zone = ORSToGValidZone(bd.tmpOrder(), bd.rank(), bd.suits());
            }else{
                zone = ORToGValidZone(bd.tmpOrder(), bd.rank());
            }
            return !judgeSuitComp(andCards(cards, zone), qty);
        }
        UNREACHABLE;
    }
    
    bool dominatesHand(const Move mv, const Hand& opsHand, const Board bd){
        // Hand型への支配(着手を引数に取る場合)
        // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
        assert(opsHand.exam_jk());
        assert(opsHand.exam_seq());
        assert(opsHand.exam_nd());
        
        if(mv.isPASS()){ return false; }
        if(mv.domInevitably()){ return true; }
        if(bd.domConditionally(mv)){ return true; }
        
        if(!mv.isSeq()){
            // Joker -> S3 を判定
            if(mv.isSingleJOKER()){
                if(containsS3(opsHand.cards)){
                    return false;
                }else{
                    return true;
                }
            }
            int aftTmpOrd = bd.afterTmpOrder(mv);
            if(mv.qty() > 4){ return true; }
            if(mv.charaPQR() & opsHand.nd[aftTmpOrd]){ // 無支配型と交差あり
                if(bd.locksSuits(mv)){ // スートロックの場合はまだ支配可能性あり
                    uint32_t qty = mv.qty();
                    qty -= opsHand.jk;
                    Cards zone = ORSToGValidZone(aftTmpOrd, mv.rank(), mv.suits());
                    if(qty){
                        return !judgeSuitComp(andCards(opsHand.cards, zone), qty);
                    }
                }
                return false;
            }else{
                return true;
            }
        }else{
            assert(mv.isSeq());
            if(anyCards(opsHand.seq)){
                uint32_t qty = mv.qty();
                Cards zone;
                int aftTmpOrd = bd.afterTmpOrder(mv);
                if(bd.locksSuits(mv)){ // スートロック
                    zone = ORSQToSCValidZone(aftTmpOrd, mv.rank(), mv.suits(), qty);
                }else{
                    zone = ORQToSCValidZone(aftTmpOrd, mv.rank(), qty);
                }
                return !canMakeSeq(andCards(opsHand.cards, zone) | andCards(opsHand.cards, CARDS_JOKER), qty);
            }else{
                return true;
            }
        }
        UNREACHABLE;
    }
    
    bool dominatesHand(const Board bd,const Hand& opsHand){
        // Hand型への支配(場を引数に取る場合)
        // ops.jk, ops.nd, ops.seq が計算されている事が必要あり
        assert(opsHand.exam_jk());
        assert(opsHand.exam_seq());
        assert(opsHand.exam_nd());
        
        if(bd.isNF()){ return false; }
        if(bd.domInevitably()){ return true; }
        
        if(!bd.isSeq()){ // グループ
            if(bd.isSingleJOKER()){
                if(containsS3(opsHand.cards)){
                    return false;
                }else{
                    return true;
                }
            }
            if(bd.qty() > 4){ return true; }
            Move mv = Move(bd);
            Cards pqr = mv.charaPQR();
            if(pqr & opsHand.nd[bd.tmpOrder()]){ // 無支配型と交差あり
                if(bd.suitsLocked()){ // スートロックの場合はまだ支配可能性あり
                    uint32_t qty = bd.qty();
                    qty -= opsHand.jk;
                    Cards zone = ORSToGValidZone(bd.tmpOrder(), bd.rank(), bd.suits());
                    if(qty){
                        return !judgeSuitComp(andCards(opsHand.cards, zone) ,qty);
                    }
                }
                return false;
            }else{
                return true;
            }
        }else{ // 階段
            if(anyCards(opsHand.seq)){
                Cards zone;
                uint32_t qty = bd.qty();
                if(bd.suitsLocked()){ // スートロック
                    zone = ORSQToSCValidZone(bd.tmpOrder(), bd.rank(), bd.suits(), qty);
                }else{
                    zone = ORQToSCValidZone(bd.tmpOrder(), bd.rank(), qty);
                }
                return !canMakeSeq(andCards(opsHand.cards, zone) | andCards(opsHand.cards, CARDS_JOKER), qty);
            }else{
                return true;
            }
        }
        UNREACHABLE;
    }
    
    template<class move_t, class sbjField_t>
    void setDomState(move_t *const buf, const int candidates, const sbjField_t& field){
        // 支配関係のパラメータを一括計算しPlaySpaceに入れる
        // クライアント用に詳しく計算。
        const int turnPlayer = field.getTurnPlayer();
        const Cards myCards = field.getCards(turnPlayer);
        const int NMyCards = field.getNCards(turnPlayer);
        const Cards opsCards = field.getOpsCards(turnPlayer);
        
        const Board& bd = field.getBoard();
        const FieldAddInfo& fieldInfo = field.fieldInfo;
        
        const int ord = bd.tmpOrder();
        
        if(fieldInfo.isTmpOrderSettled()){
            // オーダー固定
            
            // 現在オーダーについてのみ調べる
            for(int m = candidates - 1; m >= 0; --m){
                move_t *const mv = &buf[m];
                
                // 無条件
                if(mv->domInevitably()){ // 永続的完全支配
                    mv->setP_DALL();
                    continue;
                }
                
                if(mv->cards() == (CARDS_S3 | CARDS_JOKER)){ mv->setP_NFDALL_only(); continue; } // 永続的完全支配と等価(特別)
                
                const uint32_t qty = mv->qty();
                
                // 空場
                if((qty > fieldInfo.getMaxNCards())
                   || dominatesCards(mv->mv(), opsCards, OrderToNullBoard(ord))){
                    mv->setP_NFDO(); // 永続的空場他支配
                }
                //if((qty > fieldInfo.getMaxNCards()) || ::dominatesCards(mv,subtrCards(myCards,mv.extract()),makeBoardNF(ord))){
                // mInfo->setP_NFDM();//永続的空場自己支配
                //}
            }
            
        }else{
            // オーダー固定でない
            
            // 現在オーダーと逆転オーダーの両方を調べる
            for(int m = candidates - 1; m >= 0; --m){
                move_t *const mv = &buf[m];
                
                // 無条件
                if(mv->domInevitably()){ // 永続的完全支配
                    mv->setP_DALL();
                    continue;
                }
                
                if(mv->cards() == (CARDS_S3 | CARDS_JOKER)){ mv->setP_NFDALL_only(); continue; } // 永続的完全支配と等価(特別)
                
                const uint32_t qty = mv->qty();
                if(qty > fieldInfo.getMaxNCards()){
                    mv->setP_NFDO();
                }else if(dominatesCards(mv->mv(), opsCards, OrderToNullBoard(ord))){
                    if(dominatesCards(mv->mv(), opsCards, OrderToNullBoard(flipOrder(ord)))){
                        mv->setP_NFDO(); // 永続的空場他支配
                    }else{
                        mv->setE_NFDO(); // 一時的空場他支配
                    }
                }
            }
            
            
        }
        
        // 当座
        // 他の情報と被るので無駄がある
        for(int m = candidates - 1; m >= 0; --m){
            move_t *const mv = &buf[m];
            
            const uint32_t qty = mv->qty();
            
            if(qty > fieldInfo.getMaxNCardsAwake()
               || dominatesCards(mv->mv(), opsCards, bd)){
                mv->setDO(); // 当座他支配
            }
            if(mv->isPASS()
               || qty > NMyCards - qty
               || dominatesCards(mv->mv(), myCards - mv->cards(), bd)){
                mv->setDM(); // 当座自己支配
            }
        }
    }
}

#endif // UECDA_LOGIC_DOMINANCE_HPP_
