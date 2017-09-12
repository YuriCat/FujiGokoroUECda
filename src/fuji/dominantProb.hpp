#ifndef UECDA_DOMINANTPROB_HPP_
#define UECDA_DOMINANTPROB_HPP_

namespace UECda{
    /*
    double estimateDominantProb(Move mv, const Hand& opsHand, Board bd, int nall, int nawake){
        if(judgeHandDominance(mv, opsHand, bd)){ return 1; }
        if(mv.isSeq()){ return 0.99; }
        if(mv.q() >= 4){ return 0.99; }
        if(mv.q() >= 3){ return 0.95; }
        Cards ndMask = (aftOrd == )rankCards()
        Cards ndpqr = opsHand.pqr & ndMask;
        return 
    }
    
    int estimateMateProbNF(Cards myPqr, Cards opsPqr, int ord){
        Cards myPlain = maskJOKER(myCards);
        int dp[4];
        if(order == ORDER_NORMAL){
            for(int r = RANK_MAX; r >= RANK_MIN; --r){
                
                if(myCards & )
            }
        }else{
            
        }
    }*/

    template<class field_t>
    int calcRandDPNoJokerNF(const field_t& field, pqr, int r, uint32_t s, int ao){
        Cards pqr = field.getOpsHand(tp).pqr;
        Cards ndpqr;
        if(ao == ORDER_NORMAL){
            ndpqr = pqr & rankCards(r + 1, RANK_IMG_MAX);
        }else{
            ndpqr = pqr & rankCards(RANK_IMG_MIN, r - 1);
        }
        return countCards(ndpqr);
    }
    
    template<class field_t>
    int calcRandDPNoJoker(const field_t& field, Move mv){
        if(mv.isSeq()){ return 0; }
        int r = mv.r();
        int aftOrd = getAfterTmpOrder(field.getBoard(), mv);
        
        Cards pqr;
        if(bd.locksSuits(mv)){
            Cards opsCards = field.getOpsHand(tp).cards;
            pqr = convCards_PQR((opsCards & suitCards(mv.s())))
        }else{
            pqr = field.getOpsHand(tp).pqr;
        }
        
        Cards ndpqr;
        if(aftOrd == ORDER_NORMAL){
            ndpqr = pqr & rankCards(r + 1, RANK_IMG_MAX);
        }else{
            ndpqr = pqr & rankCards(RANK_IMG_MIN, r - 1);
        }
        Cards nextCards = field.getCards(tp);
        
        return countCards(ndpqr);
    }
    
    template<class field_t>
    double calcRandDP(const field_t& field, int tp,  Move mv){
        
    }
    
    int calcMatePoint(const field_t& field, int tp, Move mv){
        
    }
}

#endif // UECDA_DOMINANTPROB_HPP_
