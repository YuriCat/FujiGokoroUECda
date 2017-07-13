/*
 WDI.hpp
 Katsuki Ohto
 */

#ifndef UECDA_WDI_HPP_
#define UECDA_WDI_HPP_

#include "../include.h"
#include "../../../../daifugo/wdi/WDI.h"

namespace UECda{
    
   /* int convWDIString_IntCard(const std::string& str, IntCard *const pic){
        *pic = StringMtoIntCard(str);
        if(!examIntCard(*pic)){
            return -1;
        }
        return 0;
    }
    int convIntCard_WDIString(const IntCard ic, std::string *const pstr){
        std::ostringstream oss;
        oss << OutIntCard(ic);
        *pstr = oss.str();
        return 0;
    }
    int convWDIString_Move(const std::string& str, Move *const pmv){
        *pmv = StringMtoMove(str);
        if(pmv->isILLEGAL()){
            return -1;
        }
        return 0;
    }
    int convMove_WDIString(const Move mv, std::string *const pstr){
        std::ostringstream oss;
        oss << LogMove(mv);
        *pstr = oss.str();
        return 0;
    }
    int convCards_WDIString(const Cards& ac, std::string *const pstr){
        Cards c = ac;
        std::string str = "";
        if(anyCards(c)){
            while(1){
                IntCard ic = popIntCardLow(&c);
                std::string tstr;
                if(convIntCard_WDIString(ic, &tstr) != 0){ return -1; }
                str.append(tstr);
                if(!anyCards(c)){ break; }
                str.append(" ");
            }
        }
        *pstr = str;
        return 0;
    }*/
    
    class WDIDSpace : public WDI::DSpace{
        // UECdaルールに合ったWDI通信用セット
    private:
        template<class t0, class t1>
        struct BiMap{
            std::map<t0, t1> left;
            std::map<t1, t0> right;
            
            struct BiMapValue{
                t0 v0;
                t1 v1;
                BiMapValue(const t0& a0, const t1& a1):v0(a0), v1(a1){}
            };
            
            void insert(const BiMapValue& v){
                left.insert(std::pair<t0, t1>(v.v0, v.v1));
                right.insert(std::pair<t1, t0>(v.v1, v.v0));
            }
            void insertLeft(const BiMapValue& v){ // leftのみ
                left.insert(std::pair<t0, t1>(v.v0, v.v1));
            }
            void insertRight(const BiMapValue& v){ // rightのみ
                right.insert(std::pair<t1, t0>(v.v1, v.v0));
            }
        };
        
    public:
        
        // 必要な関数
        int convWDIString_IntCard(const std::string& str, IntCard *const pic){
            //CERR << str << endl;
            *pic = StringMtoIntCard(str);
            //CERR << (*pic) << endl;
            if(!examIntCard(*pic)){
                return -1;
            }
            return 0;
        }
        int convIntCard_WDIString(const IntCard ic, std::string *const pstr){
            std::ostringstream oss;
            oss << OutIntCard(ic);
            *pstr = oss.str();
            return 0;
        }
        int convWDIString_Move(const std::string& str, move_t *const pmv){
            *pmv = StringMtoMove(str);
            if(pmv->isILLEGAL()){
                return -1;
            }
            return 0;
        }
        int convMove_WDIString(const move_t mv, std::string *const pstr){
            std::ostringstream oss;
            oss << LogMove(mv);
            *pstr = oss.str();
            return 0;
        }
        int convCards_WDIString(const Cards& ac, std::string *const pstr){
            Cards c = ac;
            std::string str = "";
            if(anyCards(c)){
                while(1){
                    IntCard ic = popIntCardLow(&c);
                    std::string tstr;
                    if(convIntCard_WDIString(ic, &tstr) != 0){ return -1; }
                    str.append(tstr);
                    if(!anyCards(c)){ break; }
                    str.append(" ");
                }
            }
            *pstr = str;
            return 0;
        }
        
        WDIDSpace(){}
        
        ~WDIDSpace(){}
    };
} // namespace UECda

#endif // UECDA_WDI_HPP_
