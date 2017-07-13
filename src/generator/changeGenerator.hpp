/*
 changeGenerator.hpp
 Katsuki Ohto
 */

#ifndef UECDA_GENERATOR_CHANGEGENERATOR_HPP_
#define UECDA_GENERATOR_CHANGEGENERATOR_HPP_

#include "../structure/primitive/prim.hpp"

// 合法交換候補生成

namespace UECda{
    
    template<class cards_t>
    int genChange(cards_t *const pc0, const BitCards myCards, const int changeQty){
        
        static AtomicAnalyzer<> ana("ChangeGenerator");
        ana.start();
        cards_t *pc = pc0;
        BitCards tmp = myCards;
        if(changeQty == 1){ // 1枚
            while(anyCards(tmp)){
                BitCards c = popLow(&tmp);
                *pc = c;
                ++pc;
            }
        }else{ // 2枚
            while(anyCards(tmp)){
                BitCards c = popLow(&tmp);
                BitCards tmp1 = tmp;
                while(anyCards(tmp1)){
                    BitCards c1 = popLow(&tmp1);
                    *pc = addCards(c, c1);
                    ++pc;
                }
            }
        }
        ana.end();
        return pc - pc0;
    }
    
    template<class cards_t, class callback_t>
    void iterateChange(const BitCards myCards, const int changeQty, const callback_t& callback){
        BitCards tmp = myCards;
        if(changeQty == 1){ // 1枚
            while(anyCards(tmp)){
                BitCards c = popLow(&tmp);
                callback(c);
            }
        }else{ // 2枚
            while(anyCards(tmp)){
                BitCards c = popLow(&tmp);
                BitCards tmp1 = tmp;
                while(anyCards(tmp1)){
                    BitCards c1 = popLow(&tmp1);
                    callback(addCards(c, c1));
                }
            }
        }
    }
}

#endif // UECDA_GENERATOR_CHANGEGENERATOR_HPP_
