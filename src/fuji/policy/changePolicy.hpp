/*
 changePolicy.hpp
 Katsuki Ohto
 */

// カード交換方策

#ifndef UECDA_POLICY_CHANGEPOLICY_HPP_
#define UECDA_POLICY_CHANGEPOLICY_HPP_

#include "../../structure/primitive/prim.hpp"
#include "../../structure/primitive/prim2.hpp"

#include "../../generator/changeGenerator.hpp"

namespace UECda{

    /**************************交換方策点**************************/
    
    namespace ChangePolicySpace{
        enum{
            // 交換
            POL_CHANGE_HAND_1PQR_SEQ = 0, // JK以外
            POL_CHANGE_HAND_2PQR_SEQ, // nj
            POL_CHANGE_HAND_REV_1PQR,
            
            POL_CHANGE_HAND_D3,
            POL_CHANGE_HAND_JOKER_S3,
            
            POL_CHANGE_HAND_MAX1_RANK,
            POL_CHANGE_HAND_MAX2_RANK,
            POL_CHANGE_HAND_MIN1_RANK,
            POL_CHANGE_HAND_MIN2_RANK,
            
            POL_CHANGE_DOUBLE, // 2枚捨ての場合にダブルをあげるかどうか
            POL_CHANGE_PART_SEQ, // 2枚捨ての場合に3階段の部分集合をあげるかどうか
            
            POL_CHANGE_CC,
            //POL_CHANGE_CONV,
            
            POL_ALL,
        };
        
        constexpr int polNumTable[POL_ALL] = {
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
            
            16 * 16 * N_PATTERNS_SUITS_SUITS,
            //16 * 16 * N_PATTERNS_SUITS_SUITS_SUITS
        };
        constexpr int POL_NUM(unsigned int fea){
            return polNumTable[fea];
        }
        constexpr int POL_IDX(unsigned int fea){
            return (fea == 0) ? 0 : (POL_IDX(fea - 1) + POL_NUM(fea - 1));
        }
        constexpr int POL_NUM_ALL = POL_IDX(POL_ALL);
        
#define LINEOUT(feature, str) {out << str << endl; int base = POL_IDX(feature); for (int i = 0; i < POL_NUM(feature); ++i){ os(base + i); }out << endl;}
        
        template<typename T>
        int commentToPolicyParam(std::ostream& out, const T param[POL_NUM_ALL]){
            auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };
            
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
            //LINEOUT(,);
            return 0;
        }
#undef LINEOUT
    }
        
    //using ChangePolicy = SoftmaxPolicy<ChangePolicySpace::POL_NUM_ALL, 2>;
    //using ChangePolicyLearner = SoftmaxPolicyLearner<ChangePolicy>;
    template<typename T> using ChangePolicy = SoftmaxClassifier<ChangePolicySpace::POL_NUM_ALL, 2, 1, T>;
    template<typename T> using ChangePolicyLearner = SoftmaxClassifyLearner<ChangePolicy<T>>;
    
    template<class T>
    int foutComment(const ChangePolicy<T>& pol, const std::string& fName){
        std::ofstream ofs(fName, std::ios::out);
        return ChangePolicySpace::commentToPolicyParam(ofs, pol.param_);
    }
  
#define Foo(i) s += pol.param(i);\
    if(M){ pol.feedFeatureScore(m, (i), 1.0); }
    
#define FooX(i, x) s += pol.param(i) * (x);FASSERT(x,);\
    if(M){ pol.feedFeatureScore(m, (i), (x)); }
    
    template<int M = 1, class field_t, class policy_t>
    int calcChangePolicyScoreSlow(double *const dst,
                                  const Cards *const change,
                                  const int NChanges,
                                  const Cards myCards,
                                  const int NChangeCards,
                                  const field_t& field,
                                  policy_t& pol){ // learnerとして呼ばれうるため const なし
        
        using namespace ChangePolicySpace;
        
        pol.template initCalculatingScore(NChanges);
        
        if(M){
            if(dst != nullptr){
                dst[0] = 0;
            }
        }else{
            dst[0] = 0;
        }
        
        const Cards pqr = CardsToPQR(myCards);
        
        // インデックス計算用
        //bits256_t myCardsL = bits256_t::zero();
        //for(int r = 0; r < 4; ++r)myCardsL[r] = pdep64(myCards >> (r * 16), 0x0F0F0F0F);
        
        //bits256_t ccIndex = bits256_t::zero();
        //bits128_t dstIndex = bits256_t::filled16();
        
        //typename policy_t::real_t *baseAddress = pol.param_ + POL_IDX(POL_CHANGE_CC) + ;
        
        
        for(int m = 0; m < NChanges; ++m){
            
            pol.template initCalculatingCandidateScore();
            
            typename policy_t::real_t s = 0;
            int i;
            const Cards changeCards = change[m];
            const Cards changePlainCards = maskJOKER(changeCards);
            const Cards afterCards = maskCards(myCards, changeCards);
            const Cards afterPlainCards = maskJOKER(afterCards);
            
            const Cards afterPqr = CardsToPQR(afterPlainCards);
            
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
                                
                                i = base + IntCardToRank(ic) - RANK_3;
                                Foo(i);
                                
                                maskCards(&seq5, extractRanks<5>(IntCardToCards(ic)));
                                if(!seq5){ break; }
                            }
                        }
                        if(seq4){
                            maskCards(&tmp, extractRanks<4>(seq4));
                            maskCards(&seq3, extractRanks<2>(seq4));
                            while(1){
                                IntCard ic = popIntCard(&seq4);
                                
                                i = base + 9 + IntCardToRank(ic) - RANK_3;
                                Foo(i);
                                
                                if(!seq4){ break; }
                            }
                        }
                    }
                    if(seq3){
                        maskCards(&tmp, extractRanks<3>(seq3));
                        while(1){
                            IntCard ic = popIntCard(&seq3);
                            i = base + 19 + IntCardToRank(ic) - RANK_3;
                            Foo(i);
                            if(!seq3){ break; }
                        }
                    }
                }
                
                if(tmp){
                    base += 30 - 4;
                    //CERR << OutCards(tmp);
                    tmp = CardsToPQR(tmp); // 枚数位置型に変換
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
                    hr1 = IntCardToRank(ic);
                }
                const int hr2 = IntCardToRank(pickIntCardHigh(tmpPqr));
                const int lr1 = IntCardToRank(popIntCardLow(&tmpPqr));
                const int lr2 = IntCardToRank(pickIntCardLow(tmpPqr));
                
                FooX(POL_IDX(POL_CHANGE_HAND_MAX1_RANK), hr1);
                FooX(POL_IDX(POL_CHANGE_HAND_MAX2_RANK), hr2);
                FooX(POL_IDX(POL_CHANGE_HAND_MIN1_RANK), lr1);
                FooX(POL_IDX(POL_CHANGE_HAND_MIN2_RANK), lr2);
            }
            
            { // DOUBLE PRESENT, PART SEQ PRESENT
                if(NChangeCards == 2){
                    if(!any2Cards(CardsToER(changePlainCards))){ // 同じ階級のカードをあげる
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
                }
                
                // 交換するランクのカードと他のカードとの関係
                const Cards diffRanks = CardsToER(changeCards);
                Cards tmp = diffRanks;
                while(tmp){
                    IntCard ic = popIntCard(&tmp);
                    unsigned int r4x = IntCardToRank4x(ic);
                    unsigned int rank = r4x / 4;
                    // プレーンカード同士の関係
                    for(int r = RANK_MIN; r <= RANK_MAX; ++r){
                        FooX(base
                             + rank * 16 * N_PATTERNS_SUITS_SUITS
                             + r * N_PATTERNS_SUITS_SUITS
                             + getSuitsSuitsIndex((myCards >> r4x) & SUITS_ALL,
                                                  (myCards >> (r * 4)) & SUITS_ALL),
                             -1);
                        Foo(base
                            + rank * 16 * N_PATTERNS_SUITS_SUITS
                            + r * N_PATTERNS_SUITS_SUITS
                            + getSuitsSuitsIndex((afterCards >> r4x) & SUITS_ALL,
                                                 (afterCards >> (r * 4)) & SUITS_ALL));
                    }
                    
                    // ジョーカーとの関係
                    FooX(base
                         + rank * 16 * N_PATTERNS_SUITS_SUITS
                         + (RANK_MAX + 2) * N_PATTERNS_SUITS_SUITS
                         + ((pqr >> r4x) & SUITS_ALL),
                         -1);
                    Foo(base
                        + rank * 16 * N_PATTERNS_SUITS_SUITS
                        + (RANK_MAX + 2) * N_PATTERNS_SUITS_SUITS
                        + ((afterPqr >> r4x) & SUITS_ALL));
                }
            }
            
            double exps = exp(s / pol.temperature());
            
            pol.template feedCandidateScore(m, exps);
            
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
