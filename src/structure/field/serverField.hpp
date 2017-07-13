/*
 serverField.hpp
 Katsuki Ohto
 */

#ifndef UECDA_STRUCTURE_SERVERFIELD_HPP_
#define UECDA_STRUCTURE_SERVERFIELD_HPP_

#include "commonField.hpp"

namespace UECda{
    
    struct ServerField : public CommonField{
        // サーバーの持つ情報
        
        // プレーヤーの名前
        std::array<std::string, N_PLAYERS> playerName;
        
        // カード集合
        std::array<Cards, N_PLAYERS> dealtCards; // 配られたカード
        std::array<Cards, N_PLAYERS> orgCards; // 交換後のカード
        std::array<Cards, N_PLAYERS> cards; // 現在持っているカード
        std::array<Cards, N_PLAYERS> changeCards; // カード交換で出したカード
        
        // 残りプレーヤー数
        int np;
        
        std::string getPlayerName(int p)const{
            return playerName[p];
        }
        void setPlayerName(int p, const std::string& astr){
            playerName[p] = astr;
        }
        
        // カード集合関連
        void setDealtCards(int atp, Cards ac){
            dealtCards[atp] = ac;
        }
        void setOrgCards(int atp, Cards ac){
            orgCards[atp] = ac;
        }
        void setChangeCards(int atp, Cards ac){
            changeCards[atp] = ac;
        }
        void setCards(int atp, Cards ac){
            cards[atp] = ac;
        }
        void removeCards(int atp, Cards ac){
            assert(0 <= atp && atp < N_PLAYERS);
            addCards(&(CommonField::usedCards[atp]), ac);
            maskCards(&(CommonField::remCards), ac);
            maskCards(&(cards[atp]), ac);
        }
        
        Cards getDealtCards(int p)const{
            return dealtCards[p];
        }
        Cards getOrgCards(int p)const{
            return orgCards[p];
        }
        Cards getChangeCards(int p)const{
            return changeCards[p];
        }
        Cards getCards(int p)const{
            return cards[p];
        }
        
        void procHand(uint32_t tp, Move mv, Cards dc, uint32_t dq){
            removeCards(tp, dc);
            subtrNCards(tp, dq);
        }
        
        uint32_t procPass(){
            if (isSoloAwake()){
                // renew処理
                flush();
            }else{
                setPlayerAsleep(getTurnPlayer());
                rotateTurnPlayer();
            }
            addTurnNum();
            
            return getTurnPlayer();
        }
        
        uint32_t proc(Move mv){
            uint32_t tp = getTurnPlayer();
            
            if(mv.isPASS()){ // パス
                return procPass();
            }else{ // パスでない
                
                Cards dc = mv.c<_NO>();
                uint32_t dq = mv.q();
                
                procHand(tp, mv, dc, dq);
                
                bd.proc(mv);
                
                if (getNCards(tp) <= 0){ // あがり
                    //cerr << "agari!" << endl;
                    setPlayerDead(tp);
                    if (isSoloAlive()){ // 残り一人
                        setLastPlayerDead(searchL1Player());
                        addTurnNum();
                        return 0xFFFFFFFF;
                    }
                    setPMOwner(tp);
                    if (getNAwakePlayers() <= 0){
                        flush();
                    }else if(bd.isNF()){
                        flushState();
                    }else{
                        rotateTurnPlayer(tp);
                    }
                }else{
                    setPMOwner(tp);
                    
                    if(bd.isNF()){
                        flushState();
                    }else{
                        rotateTurnPlayer(tp);
                    }
                }
                addTurnNum();
                return getTurnPlayer();
            }
        }
        
        // 初期化
        void initMatch(){
            CommonField::initMatch();
            np = 0;
        }
        
        void initGame(){
            CommonField::initGame();
            cards.fill(CARDS_NULL);
            dealtCards.fill(CARDS_NULL);
            orgCards.fill(CARDS_NULL);
            changeCards.fill(CARDS_NULL);
        }
        
        template<class dice_t>
        void dealCards(dice_t *const pdice){
            Cards tc[4] = {0};
            dist64<N_PLAYERS>(tc, getRemCards(), CommonField::infoNDealtCards, pdice);
            for(int p = 0; p < N_PLAYERS; ++p){
                setNCards(p, getNDealtCards(p));
                setNOrgCards(p, getNDealtCards(p));
                dealtCards[p] = tc[p];
                CERR << "dealt cards : " << p << dealtCards[p] << endl;
            }
        }
        
        template<class dice_t>
        void shuffleSeats(dice_t *const pdice){
            // 席替え
            int n = N_PLAYERS;
            int player[N_PLAYERS];
            for(int p = 0; p < N_PLAYERS; ++p){
                player[p] = p;
            }
            for(int p = 0; p < N_PLAYERS - 1; ++p){
                int r = pdice->rand() % n;
                setPlayerSeat(player[r], p);
                player[r] = player[--n];
            }
            setPlayerSeat(player[0], N_PLAYERS - 1);
        }
    };
}

#endif // UECDA_STRUCTURE_SERVERFIELD_HPP_
