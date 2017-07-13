/*
 staticEval.hpp
 Katsuki Ohto
 */

// 完全情報空場順位予測

#ifndef UECDA_EVAL_STATICEVAL_HPP_
#define UECDA_EVAL_STATICEVAL_HPP_

#include "../../structure/primitive/prim.hpp"
#include "../../structure/primitive/prim2.hpp"

namespace UECda{

    /**************************順位予測**************************/
    
    namespace StaticEvalSpace{
        enum{
            // 交換
            EVAL_GGG,
            EVAL_ALL,
        };
        
        constexpr int evalNumTable[EVAL_ALL] = {
            //交換
            11 + 10 + 9 + 4 * 13,
            
            0,
            
            0, //14,
            
            1,
            3,
            
            1,
            1,
            1,
            1,
            
            1,
            2,
            
            //16 * 16 * 20,
            16 * 16 * N_PATTERNS_SUITS_SUITS,
        };
        constexpr int EVAL_NUM(unsigned int fea){
            return polNumTable[fea];
        }
        constexpr int EVAL_IDX(unsigned int fea){
            return (fea == 0) ? 0 : (EVAL_IDX(fea - 1) + EVAL_NUM(fea - 1));
        }
        constexpr int EVAL_NUM_ALL = EVAL_IDX(EVAL_ALL);
        
#define LINEOUT(feature, str) {out << str << endl; int base = POL_IDX(feature); for (int i = 0; i < POL_NUM(feature); ++i){ os(base + i); }out << endl;}
        
        int commentToParam(std::ostream& out, const double param[EVAL_NUM_ALL]){
            /*auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };
            
            out << "****** CHANGE POLICY ******" << endl;
            
            {
                out << "1PQR or SEQ" << endl;
                int base = POL_IDX(POL_CHANGE_HAND_1PQR_SEQ);
                out << "  SEQ" << endl;
                for (int i = 0; i < 11; ++i){ os(base + i); }out << endl;
                for (int i = 0; i < 10; ++i){ os(base + 11 + i); }out << endl;
                for (int i = 0; i < 9; ++i){ os(base + 21 + i); }out << endl;
                out << "  GROUP" << endl;
                for (int i = 0; i < 13; ++i){
                    for (int j = 0; j < 4; ++j){ os(base + 30 + 4 * i + j); }out << endl;
                }
            }
            
            LINEOUT(POL_CHANGE_HAND_2PQR_SEQ, "2PQR or SEQ");
            LINEOUT(POL_CHANGE_HAND_D3, "D3 BONUS");
            LINEOUT(POL_CHANGE_HAND_JOKER_S3, "S3-JOKER LINK BONUS");
            LINEOUT(POL_CHANGE_HAND_MAX1_RANK, "MAX1 RANK");
            LINEOUT(POL_CHANGE_HAND_MAX2_RANK, "MAX2 RANK");
            LINEOUT(POL_CHANGE_HAND_MIN1_RANK, "MIN1 RANK");
            LINEOUT(POL_CHANGE_HAND_MIN2_RANK, "MIN2 RANK");
            LINEOUT(POL_CHANGE_DOUBLE, "DOUBLE PRESENT");
            LINEOUT(POL_CHANGE_PART_SEQ, "PART SEQ PRESENT");
            //LINEOUT(,);*/
            return 0;
        }
#undef LINEOUT
    }
        
    using StaticEvaluator = SoftmaxPolicy<StaticEvalSpace::EVAL_NUM_ALL, 2>;
    using StaticEvaluatorLearner = SoftmaxPolicyLearner<ChangePolicy>;
    
    int foutComment(const StaticEvaluator& mdl, const std::string& fName){
        std::ofstream ofs(fName, std::ios::out);
        return StaticEvalSpace::commentToParam(ofs, mdl.param_);
    }
  
#define Foo(i) s += pol.param(i);\
    if(M && pol.plearner_ != nullptr){\
        pol.plearner_->vec_.back().emplace_back(std::pair<int, double>((i), 1.0));}
    
#define FooX(i, x) s += pol.param(i) * (x);\
    FASSERT(x,);\
    if(M && pol.plearner_ != nullptr){\
        pol.plearner_->vec_.back().emplace_back(std::pair<int, double>((i), (x)));}
    
    template<int M = 1, class field_t, class policy_t>
    int calcChangePolicyScoreSlow(double *const dst,
                                  const Cards *const change,
                                  const int NChanges,
                                  const Cards myCards,
                                  const int NChangeCards,
                                  const field_t& field,
                                  const policy_t& pol){
        
        using namespace ChangePolicySpace;
        
        pol.template initCalculatingScore<M>(N_PLAYERS);
        
        const Cards pqr = convCards_PQR(myCards);
        
        // 相対プレーヤー番号から実際のプレーヤー番号への変換
        int rp2p[N_PLAYERS], rp = 0;
        for(int s = field.getTurnSeat(); s != field.getTurnSeat(); s = field.getNextSeat()){
            int p = field.getSeatPlayer(s);
            if(field.isAlive(p)){
                rp2p[rp++] = p;
            }
        }
        
        for(int r0 = RANK_MIN; r0 <= RANK_JOKER; ++r0){
            for(int rp0 = 0; rp0 < field.getNAlivePlayers(); ++rp0){
                const int p0 = rp2p[rp0];
                if((field.getCards(p0) >> (r0 * 4)) & 15){
                    for(int r1 = r0 + 1; r1 <= RANK_JOKER; ++r1){
                        for(int rp1 = 0; rp1 < field.getNAlivePlayers(); ++rp1){
                            const int p1 = rp2p[rp1];
                            for(int r2 = r1 + 1; r2 <= RANK_JOKER; ++r2){
                                for(int rp2 = 0; rp2 < field.getNAlivePlayers(); ++rp2){
                                    const int p2 = rp2p[rp2];
                if(field.)
            }
        }
        
        for(int cl = 0; cl < N_PLAYERS; ++cl){
            
            pol.template initCalculatingCandidateScore<M>();
            
            double s = 0;
            int i;
            const Cards changeCards = change[m];
            const Cards changePlainCards = maskJOKER(changeCards);
            const Cards afterCards = maskCards(myCards, changeCards);
            const Cards afterPlainCards = maskJOKER(afterCards);
            
            const Cards afterPqr = convCards_PQR(afterPlainCards);
            
            // 1PQR, SEQ (snowl score except joker)
            {
                int base = POL_IDX(POL_CHANGE_HAND_1PQR_SEQ);
                
                Cards tmp = afterPlainCards;
                Cards seq3 = polymRanks<3>(tmp);
                if(seq3){
                    Cards seq4 = polymRanks<2>(seq3);
                    if(seq4){
                        Cards seq5 = polymRanks<2>(seq4);
                        if(seq5){
                            // 6枚以上の階段も5枚階段と同じパラメータで扱う(ダブルカウントしないように注意)
                            maskCards(&seq4, extractRanks<2>(seq5));
                            maskCards(&seq3, extractRanks<3>(seq5));
                            maskCards(&tmp, extractRanks<5>(seq5));
                            while(1){
                                IntCard ic = pickIntCardLow(seq5);
                                
                                i = base + convIntCard_Rank(ic) - RANK_3;
                                Foo(i);
                                
                                maskCards(&seq5, extractRanks<5>(convCard(ic)));
                                if(!seq5){ break; }
                            }
                        }
                        if(seq4){
                            maskCards(&tmp, extractRanks<4>(seq4));
                            maskCards(&seq3, extractRanks<2>(seq4));
                            while(1){
                                IntCard ic = popIntCard(&seq4);
                                
                                i = base + 9 + convIntCard_Rank(ic) - RANK_3;
                                Foo(i);
                                
                                if(!seq4){ break; }
                            }
                        }
                    }
                    if(seq3){
                        maskCards(&tmp, extractRanks<3>(seq3));
                        while(1){
                            IntCard ic = popIntCard(&seq3);
                            i = base + 19 + convIntCard_Rank(ic) - RANK_3;
                            Foo(i);
                            if(!seq3){ break; }
                        }
                    }
                }
                
                if(tmp){
                    base += 30 - 4;
                    //CERR << OutCards(tmp);
                    tmp = convCards_PQR(tmp); // 枚数位置型に変換
                    //CERR << OutCards(tmp) << endl; getchar();
                    while(1){
                        IntCard ic = popIntCard(&tmp);
                        
                        i = base + ic; // 枚数位置型なのでそのままインデックスになる
                        Foo(i);
                        
                        if(!tmp){break;}
                    }
                }
            }
            
            { // D3 BONUS
                constexpr int base = POL_IDX(POL_CHANGE_HAND_D3);
                if(containsCard(afterCards, CARDS_D3)){
                    i = base;
                    Foo(i);
                }
            }
            
            { // JOKER_S3 BONUS
                constexpr int base = POL_IDX(POL_CHANGE_HAND_JOKER_S3);
                if(containsCard(afterCards, CARDS_S3)){
                    if(containsCard(afterCards, CARDS_JOKER)){ // mine
                        i = base;
                        Foo(i);
                    }else{ // ops
                        i = base + 1;
                        Foo(i);
                    }
                }else if(containsCard(afterCards, CARDS_JOKER)){
                    i = base + 2;
                    Foo(i);
                }
            }
            
            { // MAX, MIN RANK
                Cards tmpPqr = afterPqr;
                int hr1;
                if(containsJOKER(afterCards)){
                    hr1 = RANK_MAX + 1;
                }else{
                    IntCard ic = popIntCardHigh(&tmpPqr);
                    hr1 = getIntCard_Rank(ic);
                }
                const int hr2 = getIntCard_Rank(pickIntCardHigh(tmpPqr));
                const int lr1 = getIntCard_Rank(popIntCardLow(&tmpPqr));
                const int lr2 = getIntCard_Rank(pickIntCardLow(tmpPqr));
                
                FooX(POL_IDX(POL_CHANGE_HAND_MAX1_RANK), hr1);
                FooX(POL_IDX(POL_CHANGE_HAND_MAX2_RANK), hr2);
                FooX(POL_IDX(POL_CHANGE_HAND_MIN1_RANK), lr1);
                FooX(POL_IDX(POL_CHANGE_HAND_MIN2_RANK), lr2);
            }
            
            { // DOUBLE PRESENT, PART SEQ PRESENT
                if(NChangeCards == 2){
                    if(!any2Cards(convCards_ER(changePlainCards))){ // 同じ階級のカードをあげる
                        Foo(POL_IDX(POL_CHANGE_DOUBLE));
                    }else if(polymRanks<2>(changePlainCards)){ // 同じスートの連続した2階級
                        Foo(POL_IDX(POL_CHANGE_PART_SEQ));
                    }else if(polymJump(changePlainCards)){ // 同じスートの1つ飛ばし階級
                        Foo(POL_IDX(POL_CHANGE_PART_SEQ) + 1);
                    }
                }
            }
            
            { // 2 CARDS
                constexpr int base = POL_IDX(POL_CHANGE_CC);
                
                /*const Cards changeRankCards = gatherAll4Bits(changeCards);
                if(!any2Cards(changeCards)){
                    IntCard ic = convIntCard(changeRankCards);
                    unsigned int rx4 = getIntCard_Rankx4(ic);
                    unsigned int rank = rx4 / 4;
                    uint32_t suits = (myCards >> rx4) & SUITS_ALL;
                    for(int r = RANK_MIN; r <= RANK_MAX + 2; ++r){
                        FooX(base + rank * 16 * 20 + r * 20 + get2SuitsIndex(suits, (myCards >> (r * 4)) & SUITS_ALL), -1);
                    }
                    
                    suits = (afterCards >> rx4) & SUITS_ALL;
                    for(int r = RANK_MIN; r <= RANK_MAX + 2; ++r){
                        Foo(base + rank * 16 * 20 + r * 20 + get2SuitsIndex(suits, (afterCards >> (r * 4)) & SUITS_ALL));
                    }
                }else{
                    
                 
                }*/

                //Cards addPqr = maskCards(afterPqr | (afterCards & CARDS_JOKER), pqr | (myCards & CARDS_JOKER));
                //Cards subPqr = maskCards(pqr | (myCards & CARDS_JOKER), afterPqr | (afterCards & CARDS_JOKER));
                
                //assert(countCards(addPqr) <= NChangeCards);
                if(containsJOKER(changeCards)){
                    for(int r = RANK_MIN; r <= RANK_MAX; ++r){
                        FooX(base
                             + (RANK_MAX + 2) * 16 * N_PATTERNS_SUITS_SUITS
                             + r * N_PATTERNS_SUITS_SUITS
                             + ((pqr >> (r * 4)) & SUITS_ALL),
                             -1);
                        Foo(base
                            + (RANK_MAX + 2) * 16 * N_PATTERNS_SUITS_SUITS
                            + r * N_PATTERNS_SUITS_SUITS
                            + ((afterPqr >> (r * 4)) & SUITS_ALL));  // suits - suits のパターン数より少ないのでOK
                    }
                    /*FooX(base
                         + rank * 16 * N_PATTERNS_SUITS_SUITS
                         + (RANK_MAX + 2) * N_PATTERNS_SUITS_SUITS
                         + 1,
                         -1);*/
                    /*Foo(base
                        + rank * 16 * N_PATTERNS_SUITS_SUITS
                        + (RANK_MAX + 2) * N_PATTERNS_SUITS_SUITS
                        + 0);*/
                    // 他のとこでやってくれるからここではいらない
                }
                
                // 交換するランクのカードと他のカードとの関係
                const Cards diffRanks = convCards_ER(changeCards);
                Cards tmp = diffRanks;
                while(tmp){
                    IntCard ic = popIntCard(&tmp);
                    unsigned int rx4 = getIntCard_Rankx4(ic);
                    unsigned int rank = rx4 / 4;
                    // プレーンカード同士の関係
                    for(int r = RANK_MIN; r <= RANK_MAX; ++r){
                        FooX(base
                             + rank * 16 * N_PATTERNS_SUITS_SUITS
                             + r * N_PATTERNS_SUITS_SUITS
                             + getSuitsSuitsIndex((myCards >> rx4) & SUITS_ALL,
                                                  (myCards >> (r * 4)) & SUITS_ALL),
                             -1);
                        Foo(base
                            + rank * 16 * N_PATTERNS_SUITS_SUITS
                            + r * N_PATTERNS_SUITS_SUITS
                            + getSuitsSuitsIndex((afterCards >> rx4) & SUITS_ALL,
                                                 (afterCards >> (r * 4)) & SUITS_ALL));
                    }
                    // ジョーカーとの関係
                    FooX(base
                         + rank * 16 * N_PATTERNS_SUITS_SUITS
                         + (RANK_MAX + 2) * N_PATTERNS_SUITS_SUITS
                         + ((pqr >> rx4) & SUITS_ALL),
                         -1);
                    Foo(base
                        + rank * 16 * N_PATTERNS_SUITS_SUITS
                        + (RANK_MAX + 2) * N_PATTERNS_SUITS_SUITS
                        + ((afterPqr >> rx4) & SUITS_ALL));
                }
            }
            
            double exps = exp(s / pol.temperature());
            
            pol.template feedCandidateScore<M>(exps);
            
            if(M){
                if(dst != nullptr){
                    dst[m + 1] = dst[m] + exps;
                }
            }else{
                dst[m + 1] = dst[m] + exps;
            }
        }
        return 0;
    }
    
#undef Foo
#undef FooX
    
    template<class cards_t, class field_t, class policy_t, class dice_t>
    int changeWithPolicy(const cards_t *const buf, const int NChanges, const Cards myCards, const int NChangeCards,
                             const field_t& field, const policy_t& pol, dice_t *const pdice){
        double score[N_MAX_CHANGES + 1];
        calcChangePolicyScoreSlow<0>(score, buf, NChanges, myCards, NChangeCards, field, pol);
        double r = pdice->drand() * score[NChanges];
        return sortedDAsearch(score, 0, NChanges, r);
    }
    
    template<class cards_t, class field_t, class policy_t, class dice_t>
    int changeWithBestPolicy(const cards_t *const buf, const int NChanges, const Cards myCards, const int NChangeCards,
                             const field_t& field, const policy_t& pol, dice_t *const pdice){
        double score[N_MAX_CHANGES + 1];
        calcChangePolicyScoreSlow<0>(score, buf, NChanges, myCards, NChangeCards, field, pol);
        int bestIndex[N_MAX_CHANGES];
        bestIndex[0] = -1;
        int NBestMoves = 0;
        double bestScore = -DBL_MAX;
        for(int m = 0; m < NChanges; ++m){
            double s = score[m + 1] - score[m];
            //cerr << OutCards(buf[m]) << " : " << log(s) << endl;
            if(s > bestScore){
                bestIndex[0] = m;
                bestScore = s;
                NBestMoves = 1;
            }else if(s == bestScore){
                bestIndex[NBestMoves] = m;
                ++NBestMoves;
            }
        }
        if(NBestMoves <= 1){
            return bestIndex[0];
        }else{
            return bestIndex[pdice->rand() % NBestMoves];
        }
    }
}



#endif // UECDA_POLICY_CHANGEPOLICY_HPP_
