/*
 commonField.hpp
 Katsuki Ohto
 */

#ifndef UECDA_STRUCTURE_COMMONFIELD_HPP_
#define UECDA_STRUCTURE_COMMONFIELD_HPP_

#include "../primitive/prim.hpp"
#include "../primitive/prim2.hpp"

namespace UECda{
    
    struct CommonField{
        // 共通の情報
        
        int gameNum; // ゲーム数
        int turnNum; // ターン
        int blockNum; // ブロック(空場~また空場まで)
        BitArray32<4, N_PLAYERS> cycleNum;  // サイクル(あるプレーヤーの空場~また空場まで)
        
        // 場の情報
        Board bd;
        
        // プレーヤー一時情報
        PlayersState ps;
        // 試合の固定情報と進行情報
        GamePhase phase;
        
        BitArray32<4> infoSpecialPlayer; // 0-3 TurnPlayer, 4-7 PMOwner, 8-11 L1Player, 12-15 FTPlayer
        
        BitArray32<4, N_PLAYERS> infoNDealtCards;
        BitArray32<4, N_PLAYERS> infoNOrgCards;
        BitArray32<4, N_PLAYERS> infoNCards;
        
        BitArray32<4, N_PLAYERS> infoClass;
        BitArray32<4, N_PLAYERS> infoClassPlayer; // 初期化ゲームでは未定義
        BitArray32<4, N_PLAYERS> infoNewClass;
        BitArray32<4, N_PLAYERS> infoNewClassPlayer;
        BitArray32<4, N_PLAYERS> infoSeat;
        BitArray32<4, N_PLAYERS> infoSeatPlayer;
        
        // カード集合
        Cards remCards;
        std::array<Cards, N_PLAYERS> usedCards;
        
        // スタッツ
        BitArray32<4, N_PLAYERS> infoPosition; // 総合順位
        std::array<int, N_PLAYERS> score; // 合計得点
        std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS> classDestination; //階級到達回数
        std::array<std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS>, N_PLAYERS> classTransition; // 階級遷移回数
        
        static bool examPlayerNum(int n)noexcept{ return (0 <= n && n < N_PLAYERS); }
        
        Board getBoard()const noexcept{ return bd; }
        
        uint32_t getTurnPlayer()const noexcept{ return infoSpecialPlayer[0]; }
        uint32_t getPMOwner()const noexcept{ return infoSpecialPlayer[1]; }
        uint32_t getFirstTurnPlayer()const noexcept{ return infoSpecialPlayer[3]; }
        
        int setTurnPlayer(const uint32_t p)noexcept{
            if(!examPlayerNum(p)){
                return -1;
            }
            infoSpecialPlayer.assign(0, p);
            return 0;
        }
        void setPMOwner(const uint32_t p)noexcept{
            infoSpecialPlayer.assign(1, p);
        }
        void setL1Player(const uint32_t p)noexcept{
            infoSpecialPlayer.assign(2, p);
        }
        int setFirstTurnPlayer(const uint32_t p)noexcept{
            if(!examPlayerNum(p)){
                return -1;
            }
            infoSpecialPlayer.assign(3, p);
            return 0;
        }
        
        // 永続、進行情報
        int getGameNum()const noexcept{ return gameNum; }
        void setGameNum(int an)noexcept{ gameNum = an; }
        
        int getTurnNum()const noexcept{ return turnNum; }
        int getBlockNum()const noexcept{ return blockNum; }
        uint32_t getCycleNum(int p)const{ return cycleNum[p]; }
        
        void addTurnNum()noexcept{ ++turnNum; }
        void addBlockNum()noexcept{ ++blockNum; }
        void addCycleNum(int p){ cycleNum.plus(p, 1U); }
        
        // カード集合関連
        void removeCards(const int tp, const Cards c){
            assert(0 <= tp && tp < N_PLAYERS);
            addCards(&usedCards[tp], c);
            subtrCards(&remCards, c);
        }
        Cards getRemCards()const noexcept{ return remCards; }
        Cards getUsedCards(int p)const{
            return usedCards[p];
        }
        
        void setInitGame()noexcept{ phase.setInitGame(); }
        void resetInitGame()noexcept{ phase.resetInitGame(); }
        uint32_t isInitGame()const{ return phase.isInitGame(); }
        
        void setFirstTurn()noexcept{ phase.setFirstTurn(); }
        void resetFirstTurn()noexcept{ phase.resetFirstTurn(); }
        uint32_t isFirstTurn()const{return phase.isFirstTurn(); }
        
        void setInChange(){ phase.setInChange();}
        void resetInChange(){ phase.resetInChange();}
        uint32_t isInChange()const{ return phase.isInChange();}
        uint32_t isInPlay()const noexcept{ return phase.isInPlay(); }
        
        void fixPrmOrder(int ord){ bd.fixPrmOrder(ord); }
        void fixTmpOrder(int ord){ bd.fixTmpOrder(ord); }
        uint32_t getPrmOrder()const{ return bd.prmOrder(); }
        uint32_t getTmpOrder()const{ return bd.tmpOrder(); }
        void flipTmpOrder(){ bd.flipPrmOrder(); }
        void flipPrmOrder(){ bd.flipTmpOrder(); }
        
        bool isNF()const noexcept{ return bd.isNF(); }
        
        bool locksSuits(const Move& m)const{
            return bd.locksSuits(m);
        };
        
        void setNOrgCards(int p, uint32_t num){ infoNOrgCards.assign(p, num); }
        void setNCards(int p, uint32_t num){ infoNCards.assign(p, num); }
        void setNDealtCards(int p, uint32_t num){ infoNDealtCards.assign(p, num); }
        uint32_t getNCards(int p)const{ return infoNCards[p]; }
        uint32_t getNOrgCards(int p)const{ return infoNOrgCards[p]; }
        uint32_t getNDealtCards(int p)const{ return infoNDealtCards[p]; }
        
        uint32_t getNRemCards()const noexcept{ return countCards(remCards); }
        void subtrNCards(int p, uint32_t qty){ infoNCards.minus(p, qty); }
        
        uint32_t getPlayerSeat(int p)const{ return infoSeat[p]; }
        uint32_t getSeatPlayer(int s)const{ return infoSeatPlayer[s]; }
        uint32_t getPlayerClass(int p)const{ return infoClass[p]; }
        uint32_t getClassPlayer(int p)const{ return infoClassPlayer[p]; }
        uint32_t getPlayerNewClass(int p)const{ return infoNewClass[p]; }
        uint32_t getNewClassPlayer(int p)const{ return infoNewClassPlayer[p]; }
        
        uint32_t getPosition(int p)const{ return infoPosition[p]; }
        int getScore(int p)const{ return score[p]; }
        
        uint32_t getNextSeatPlayer(int p)const{
            return getSeatPlayer(getNextSeat<N_PLAYERS>(getPlayerSeat(p)));
        }
        
        uint32_t isAlive(int p)const{ return ps.isAlive(p); }
        uint32_t isAwake(int p)const{ return ps.isAwake(p); }
        uint32_t getNAwakePlayers()const noexcept{ return ps.getNAwake(); }
        uint32_t getNAlivePlayers()const noexcept{ return ps.getNAlive(); }
        
        void setNAwakePlayers(int num)noexcept{
            ps.setNAwake(num);
        }
        void setNAlivePlayers(int num)noexcept{
            ps.setNAlive(num);
        }
        
        uint32_t getBestClass()const noexcept{ return (N_PLAYERS - getNAlivePlayers()); }
        uint32_t getWorstClass()const noexcept{ return (N_PLAYERS - 1); }
        
        void setPlayerClass(int p, int cl){
            infoClass.assign(p, cl);
            if(!isInitGame()){ // 初期化ゲーム以外のみ
                infoClassPlayer.assign(cl, p);
            }
        }
        void setPlayerNewClass(int p, int cl){
            infoNewClass.assign(p, cl);
            infoNewClassPlayer.assign(cl, p);
        }
        void setPlayerSeat(int p, int s){
            infoSeat.assign(p, s);
            infoSeatPlayer.assign(s, p);
        }
        
        bool isSoloAwake()const noexcept{ return ps.isSoloAwake(); }
        bool isSoloAlive()const noexcept{ return ps.isSoloAlive(); }
        
        uint32_t searchL1Player()const noexcept{
            return ps.searchL1Player();
        }
        
        void rotateTurnPlayer(){
            uint32_t tp = getTurnPlayer();
            do{
                tp = getNextSeatPlayer(tp);
            }while(!isAwake(tp));
            setTurnPlayer(tp);
        }
        
        void rotateTurnPlayer(uint32_t tp){
            do{
                tp = getNextSeatPlayer(tp);
            }while(!isAwake(tp));
            setTurnPlayer(tp);
        }
        
        void setPlayerAsleep(uint32_t p){
            ASSERT(ps.isAwake(p), cerr << p << endl;); // 現在Awakeであったことは保証されているとする
            ps.setAsleep(p);
        }
        
        void setPlayerDead(const uint32_t p){
            // プレーヤーがあがった(天に召された)
            setPlayerNewClass(p,getBestClass()); // 順位
            ASSERT(ps.isAlive(p) && ps.isAwake(p),
                   cerr << p << endl;); // 現在AliveかつAwakeであったことは保証されているとする
            ps.setDead(p);
        }
        
        void setLastPlayerDead(const uint32_t p){
            // 最後に残ったプレーヤーのあがり処理
            setPlayerNewClass(p,getWorstClass()); // 順位
            ASSERT(ps.isAlive(p), cerr << p << endl;); // 現在Aliveであったことは保証されているとする
            ps.setDeadOnly(p);
        }
        
        void flushTurnPlayer(){
            uint32_t tp = getPMOwner();
            if(isAlive(tp)){
                setTurnPlayer(tp);
            }else{
                rotateTurnPlayer(tp);
            }
        }
        
        uint32_t getFlushLeadPlayer()const{
            // 全員パスの際に誰から始まるか
            if(isNF()){return getTurnPlayer();}
            uint32_t own = getPMOwner();
            DERR << " PM Owner :: " << own << endl;
            if(!isAlive(own)){ // すでにあがっている
                // own~tp間のaliveなプレーヤーを探す
                while(1){
                    own = getNextSeatPlayer(own);
                    if(isAlive(own)){ break; }
                }
            }
            return own;
        }
        
        uint32_t searchMinNCards()const{ // 最小枚数
            uint32_t nmin = 99;
            for (int p = 0; p < N_PLAYERS; ++p){
                if (isAlive(p)){
                    nmin = min(nmin, getNCards(p));
                }
            }
            return nmin;
        }
        uint32_t searchMinNCardsAwake()const{ // Awakeなプレーヤーの最小枚数
            uint32_t nmin = 99;
            for(int p = 0; p < N_PLAYERS; ++p){
                if(isAwake(p)){
                    nmin = min(nmin, getNCards(p));
                }
            }
            return nmin;
        }
        uint32_t searchMaxNCards()const{ // 最大枚数
            uint32_t nmax = 0;
            for(int p = 0; p < N_PLAYERS; ++p){
                if(isAlive(p)){
                    nmax = max(nmax, getNCards(p));
                }
            }
            return nmax;
        }
        uint32_t searchMaxNCardsAwake()const{ // Awakeなプレーヤーの最小枚数
            uint32_t nmax = 0;
            for(int p = 0; p < N_PLAYERS; ++p){
                if(isAwake(p)){
                    nmax = max(nmax, getNCards(p));
                }
            }
            return nmax;
        }

        // 試合進行
        void lockSuits(){
            bd.lockSuits();
        }
        void flush(){
            bd.flush();
            flushState();
            
            addBlockNum(); // 次のブロックへ
            addCycleNum(getTurnPlayer()); // ターンを取ったプレーヤーは次のサイクルへ
        }
        void flushState(){
            ps.flush();
            flushTurnPlayer();
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
                
                Cards dc = mv.cards<_NO>();
                uint32_t dq = mv.qty();
                
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
        
        void feedResult(int p, int cl, int ncl){
            assert(0 <= p && p < N_PLAYERS);
            assert(0 <= cl && cl < N_PLAYERS);
            if(!(0 <= ncl && ncl < N_PLAYERS))return;
            
            int sc = REWARD(ncl);
            
            score[p] += sc;
            ++classDestination[p][ncl];
            ++classTransition[p][cl][ncl];
        }
        void feedResult()noexcept{
            for(int p = 0; p < N_PLAYERS; ++p){
                feedResult(p, getPlayerClass(p), getPlayerNewClass(p));
            }
        }
        
        // 初期化
        void initMatch(){
            gameNum = 0;
            infoPosition.clear();
            score.fill(0);
            for(int p = 0; p < N_PLAYERS; ++p){
                classDestination[p].fill(0);
                for(int cl = 0; cl < N_PLAYERS; ++cl){
                    classTransition[p][cl].fill(0);
                }
            }
        }
        void initGame(){
            
            bd.init();
            ps.init();
            
            turnNum = 0;
            blockNum = 0;
            cycleNum.clear();
            
            infoNDealtCards.clear();
            infoNCards.clear();
            infoNOrgCards.clear();
            
            infoSeat.clear();
            infoSeatPlayer.clear();
            infoClass.clear();
            infoClassPlayer.clear();
            infoNewClass.clear();
            infoNewClassPlayer.clear();
            infoSpecialPlayer.clear();
            
            phase.init();
            
            remCards = CARDS_ALL;
            usedCards.fill(CARDS_NULL);
        }
        
        // 後処理
        void closeGame(){
            // 現在順位計算
            infoPosition.clear();
            for(int p0 = 0; p0 < N_PLAYERS; ++p0){
                for(int p1 = p0 + 1; p1 < N_PLAYERS; ++p1){
                    if(score[p0] < score[p1]){
                        infoPosition.add(p0, 1);
                    }else if(score[p0] > score[p1]){
                        infoPosition.add(p1, 1);
                    }
                }
            }
            ++gameNum;
        }
        void closeMatch(){
            // スタッツ表示
            CERR << "Players Stats" << endl;
            for(int p = 0; p < N_PLAYERS; ++p){
                CERR << "Player " << p << " :" << endl;
                CERR << "R*NR 0  1  2  3  4  total" << endl;
                for(int cl = 0; cl < N_PLAYERS; ++cl){
                    CERR << "  " << cl << "  ";
                    for(int ncl = 0; ncl < N_PLAYERS; ++ncl){
                        CERR << classTransition[p][cl][ncl] << "  ";
                    }
                    CERR << classDestination[p][cl] << endl;
                }
            }
        }
        
        //validator
        bool examBeforePlay()const{
            uint32_t tp = getTurnPlayer();
            if(!isAlive(tp)){
                cerr << "CommonField : turn player(" << tp << ") is dead" << endl;
                return false;
            }
            if(!isAwake(tp)){
                cerr << "CommonField : turn player(" << tp << ") is asleep" << endl;
                return false;
            }
            return true;
        }
    };
}

#endif // UECDA_STRUCTURE_COMMONFIELD_HPP_
