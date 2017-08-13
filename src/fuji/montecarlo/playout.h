/*
 playout.h
 Katsuki Ohto
 */

#ifndef UECDA_FUJI_PLAYOUT_H_
#define UECDA_FUJI_PLAYOUT_H_

// プレイアウト関連ヘッダー
// 世界設定にも関わってくるので分離

#include "../../structure/primitive/prim.hpp"
#include "../../structure/primitive/prim2.hpp"
#include "../../structure/hand.hpp"
#include "../../structure/hash/hashGenerator.hpp"
#include "../logic/dominance.hpp"
#include "../policy/playPolicy.hpp"

namespace UECda{
    namespace Fuji{

        /**************************プレイアウト結果記入**************************/
        /*
        struct PlayoutScore{
        //１回のプレイアウトの結果について報告

        //BitArray32<4,N_PLAYERS> infoNewClass;//順位

        BitArray64<11,N_PLAYERS> infoReward;//報酬

        uint32_t domFlag;
        uint32_t nNF;

        template<class info_t>
        void setReward(const info_t& arg){
        for(int p=0;p<N_PLAYERS;++p){
        infoReward.set(p,game_reward[arg[p]]);
        }
        }

        constexpr PlayoutScore()
        :infoReward(0ULL),domFlag(0),nNF(0)
        {}
        };
        */

        /**************************プレイアウタ情報空間**************************/

        // complete information of game
        // used in playouts
        struct PlayouterField{

            // tools for playout
            MoveInfo *mv; // buffer of move
            XorShift64 *dice;

            // playout result
            BitArray64<11, N_PLAYERS> infoReward; // rewards
            uint32_t domFlags;
            uint32_t NNullFields;
            std::bitset<32> flags;

            // information for playout
            int depth;
            BitSet32 attractedPlayers; // players we want playout-result
            int NMoves;
            int NActiveMoves;
            MoveInfo playMove; // move chosen by player int playout
            FieldAddInfo fInfo;

            // common information
            int turnNum;
            Board bd;
            PlayersState ps;

            BitArray32<4> infoSpecialPlayer;

            BitArray32<4, N_PLAYERS> infoClass;
            BitArray32<4, N_PLAYERS> infoClassPlayer;
            BitArray32<4, N_PLAYERS> infoSeat;
            BitArray32<4, N_PLAYERS> infoSeatPlayer;
            BitArray32<4, N_PLAYERS> infoNewClass;
            BitArray32<4, N_PLAYERS> infoNewClassPlayer;
            
            BitArray32<4, N_PLAYERS> infoPosition;

            GamePhase phase;

            // common information of cards
            // Cards used[N_PLAYERS];

            uint32_t remQty;
            Cards remCards;
            uint64_t remHash;

            // hash_value
            //uint64_t hash_org;
            //uint64_t hash_prc;
            //uint64_t hash_ri;

            //uint64_t hash_bd;

            // hands
            Hand hand[N_PLAYERS];
            Hand opsHand[N_PLAYERS];
            
            // value for calculating policy
            PlayerPolicySubValue playerPolicyValue[N_PLAYERS];
            PolicySubValue policyValue;
            
            bool isL2Situation()const noexcept{ return getNAlivePlayers() == 2; }
            bool isLnCISituation()const noexcept{
                return hand[getTurnPlayer()].qty > 1U // 1枚なら探索しても意味無し
                && isNF() // この条件邪魔かも
                && remQty < 12U // 計算量制限
                && (!containsJOKER(remCards)); // 計算量制限
            }
            bool isEndGame()const noexcept{ // 末端探索に入るべき局面かどうか。学習にも影響する
#ifdef SEARCH_LEAF_L2
                if(isL2Situation()){ return true; }
#endif
#ifdef SEARCH_LEAF_LNCI
                if(isLnCISituation()){ return true; }
#endif
                return false;
                
            }

            void setMoveBuffer(MoveInfo *const pmv)noexcept{ mv = pmv; }
            void setDice(XorShift64 *const pdice)noexcept{ dice = pdice; }
            void setPlayMove(MoveInfo ami)noexcept{ playMove = ami; }
            
            void addAttractedPlayer(int p)noexcept{ attractedPlayers.set(p); }

            int getDepth()const noexcept{ return depth; }

            Board getBoard()const noexcept{ return bd; }
            bool isNF()const noexcept{ return bd.isNF(); }

            int getTurnNum()const noexcept{ return turnNum; }
            void addTurnNum()noexcept{ ++turnNum; }
            
            void setInitGame()noexcept{ phase.setInitGame(); }
            uint32_t isInitGame()const noexcept{ return phase.isInitGame(); }

            bool isSoloAwake()const noexcept{ return ps.isSoloAwake(); }
            bool isSoloAlive()const noexcept{ return ps.isSoloAlive(); }
            
            uint32_t isAlive(const int p)const noexcept{ return ps.isAlive(p); }
            uint32_t isAwake(const int p)const noexcept{ return ps.isAwake(p); }
            uint32_t getNAwakePlayers()const noexcept{ return ps.getNAwake(); }
            uint32_t getNAlivePlayers()const noexcept{ return ps.getNAlive(); }

            uint32_t searchOpsPlayer(const int p)const noexcept{
                return ps.searchOpsPlayer(p);
            }
            uint32_t getBestClass()const noexcept{ return ps.getBestClass(); }
            uint32_t getWorstClass()const noexcept{ return ps.getWorstClass(); }
            
            void clearSeats()noexcept{
                infoSeat.clear(); infoSeatPlayer.clear();
            }
            void clearClasses()noexcept{
                infoClass.clear(); infoClassPlayer.clear();
            }

            void setPlayerClass(int p, int r)noexcept{ infoClass.set(p, r); }
            void setClassPlayer(int c, int p)noexcept{ infoClassPlayer.set(c, p); }
            void setPlayerNewClass(int p, int c)noexcept{ infoNewClass.set(p, c); }
            void setNewClassPlayer(int c, int p)noexcept{ infoNewClassPlayer.set(c, p); }
            void setPlayerSeat(int p, int s)noexcept{ infoSeat.set(p, s); }
            void setSeatPlayer(int s, int p)noexcept{ infoSeatPlayer.set(s, p); }
            
            uint32_t getPlayerClass(int p)const noexcept{ return infoClass[p]; }
            uint32_t getClassPlayer(int c)const noexcept{ return infoClassPlayer[c]; }
            uint32_t getPlayerNewClass(int p)const noexcept{ return infoNewClass[p]; }
            uint32_t getNewClassPlayer(int c)const noexcept{ return infoNewClassPlayer[c]; }
            uint32_t getSeatPlayer(int s)const noexcept{ return infoSeatPlayer[s]; }
            uint32_t getPlayerSeat(int p)const noexcept{ return infoSeat[p]; }
            
            uint32_t getPosition(int p)const noexcept{ return infoPosition[p]; }

            uint32_t getTurnPlayer()const noexcept{ return infoSpecialPlayer[0]; }
            uint32_t getPMOwner()const noexcept{ return infoSpecialPlayer[1]; }
            uint32_t getFirstTurnPlayer()const noexcept{ return infoSpecialPlayer[3]; }

            void setTurnPlayer(int p)noexcept{
                infoSpecialPlayer.replace(0, p);
            }
            void setPMOwner(int p)noexcept{
                infoSpecialPlayer.replace(1, p);
            }
            void setL1Player(int p)noexcept{
                infoSpecialPlayer.replace(2, p);
            }
            void setFirstTurnPlayer(int p)noexcept{
                infoSpecialPlayer.replace(3, p);
            }

            template<int IS_NF = _BOTH>
            uint32_t getFlushLeadPlayer()const noexcept{
                // 全員パスの際に誰から始まるか
                if(TRI_BOOL_YES(IS_NF, isNF())){ return getTurnPlayer(); }
                uint32_t own = getPMOwner();
                if(!isAlive(own)){ // すでにあがっている
                    // own~tp間のaliveなプレーヤーを探す
                    while (1){
                        own = getNextSeatPlayer(own);
                        if(isAlive(own)){ break; }
                    }
                }
                return own;
            }
            uint32_t getNextSeatPlayer(const int p)const noexcept{
                return getSeatPlayer(getNextSeat<N_PLAYERS>(getPlayerSeat(p)));
            }
            void rotateTurnPlayer(uint32_t tp)noexcept{
                do{
                    tp = getNextSeatPlayer(tp);
                } while (!isAwake(tp));
                setTurnPlayer(tp);
            }

            void flushTurnPlayer()noexcept{
                uint32_t tp = getPMOwner();
                if(isAlive(tp)){
                    setTurnPlayer(tp);
                }
                else{
                    rotateTurnPlayer(tp);
                }
            }

            void setPlayerAsleep(const int p)noexcept{ ps.setAsleep(p); }
            void setAllAsleep()noexcept{ ps.setAllAsleep(); }
            void setPlayerDead(const int p)noexcept{ ps.setDead(p); }
            void setPlayerAwake(const int p)noexcept{ ps.setAwake(p); }
            void setPlayerAlive(const int p)noexcept{ ps.setAlive(p); }

            void flushState()noexcept{
                ps.flush();
                flushTurnPlayer();
            }
            void flush()noexcept{
                bd.flush();
                flushState();
                //flushBoardHash();
            }
            
            uint32_t searchOpsMinNCards(int pn)const{ // 自分以外の最小枚数
                uint32_t nc = N_CARDS;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(isAlive(p) && (p != pn)){
                        nc = min(nc, getNCards(p));
                    }
                }
                return nc;
            }
            uint32_t searchOpsMinNCardsAwake(int pn)const{ // 自分以外のAwakeなプレーヤーの最小枚数
                uint32_t nc = N_CARDS;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(isAwake(p) && (p != pn)){
                        nc = min(nc, getNCards(p));
                    }
                }
                return nc;
            }
            uint32_t searchOpsMaxNCards(int pn)const{ // 自分以外の最大枚数
                uint32_t nc = 0;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(isAlive(p) && (p != pn)){
                        nc = max(nc, getNCards(p));
                    }
                }
                return nc;
            }
            uint32_t searchOpsMaxNCardsAwake(int pn)const{ // 自分以外のAwakeなプレーヤーの最小枚数
                uint32_t nc = 0;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(isAwake(p) && (p != pn)){
                        nc = max(nc, getNCards(p));
                    }
                }
                return nc;
            }
            
            uint32_t getOpsMinNCards(int pn)const{ return searchOpsMinNCards(pn); }
            uint32_t getOpsMinNCardsAwake(int pn)const{ return searchOpsMinNCardsAwake(pn); }
            uint32_t getOpsMaxNCards(int pn)const{ return searchOpsMaxNCards(pn); }
            uint32_t getOpsMaxNCardsAwake(int pn)const{ return searchOpsMaxNCardsAwake(pn); }

            // ハッシュ値更新
            /*void procBoardHash_NP(Move mv)noexcept{
                hash_bd = genprocHash_Board_NP(hash_bd, bd, mv);
            }
            void procBoardHash_P(int tp)noexcept{
                ASSERT(0 <= tp && tp < N_PLAYERS,);
                hash_bd = genprocHash_Board_P(hash_bd, tp);
            }
            void flushBoardHash()noexcept{
                hash_bd = genHash_Board_NF(bd);
            }

            void mergeProcisionHash(int tp, uint64_t hash_dist)noexcept{
                hash_prc = mergeHash_nCards<N_PLAYERS>(hash_prc, tp, hash_dist);
            }
            void procRetInfoHash(int p)noexcept{
                hash_ri = procHash_NCards<N_PLAYERS>(hash_ri, p, hand[p].qty);
            }*/

            void procHand(int tp, Move mv, Cards dc, uint32_t dq)noexcept{
                
                ASSERT(anyCards(dc) && mv.cards() == dc && countCards(dc) == dq,
                       cerr << mv << " " << OutCards(dc) << " " << dq << endl;
                       cerr << toDebugString(););

                uint64_t dhash = CardsToHashKey(dc);

                subtrCards(&remCards, dc);
                remQty -= dq;
                remHash ^= dhash;

                for(int p = 0; p < tp; ++p){
                    if(isAlive(p)){
                        opsHand[p].makeMoveAll(mv, dc, dq, dhash);
                    }
                }
                hand[tp].makeMoveAll(mv, dc, dq, dhash);
                for(int p = tp + 1; p < N_PLAYERS; ++p){
                    if(isAlive(p)){
                        opsHand[p].makeMoveAll(mv, dc, dq, dhash);
                    }
                }

                // proceed hash value
                //mergeProcisionHash(tp, dhash);
                //procRetInfoHash(tp);
            }

            void procAndKillHand(int tp, Move mv, Cards dc, uint32_t dq)noexcept{

                uint64_t dhash = CardsToHashKey(dc);

                subtrCards(&remCards, dc);
                remQty -= dq;
                remHash ^= dhash;

                hand[tp].qty = 0; // set |qty| == 0 for after management

                assert(!isAlive(tp)); // agari player is not alive

                for(int p = 0; p < N_PLAYERS; ++p){
                    if(isAlive(p)){
                        opsHand[p].makeMoveAll(mv, dc, dq, dhash);
                    }
                }

                // proceed hash value
                //mergeProcisionHash(tp, dhash);
                //procRetInfoHash(tp);
            }
            
            // policy 計算
#define Foo(i) s += pol.param(i);
#define FooX(i, x) s += pol.param(i) * (x);
#define IFoo(i) s -= pol.param(i);
#define IFooX(i, x) s -= pol.param(i) * (x);
            
            // policy 1から計算
            template<class policy_t>
            void calcPolicySubValue(const policy_t& pol){
                // 面倒であるが現状計算コードをここにコピーしてくるしかない
                /*for(int p = 0; p < N_PLAYERS; ++p){
                    
                    double s = 0;
                    const Cards c = getCards(p);
                    
                    for(int ord = 0; ord < 2; ++ord){
                        for(int f = 0; f < 16; ++f){
                            for(int j = f + 1; j < 16; ++j){
                                if(((c >> (f * 4)) & 15) && ((c >> (j * 4)) & 15)){
                                    int i = base
                                    + 16 * 16 * 16 * 16 * ord
                                    + f * 16 * 16 * 16
                                    + j * 16 * 16
                                    + ((c >> (f * 4)) & 15) * 16
                                    + ((c >> (j * 4)) & 15);
                                    Foo(i);
                                }
                            }
                        }
                    }
                    playerPolicyValue[p].twoMeldsValue[ord] = s;
                }*/
            }
            
            // policy 差分計算
            template<class policy_t>
            void procPolicySubValue(int tp, Move mv, const policy_t& pol){
                // hand型の更新が終わっていることを仮定する
                /*if(mv.isPASS()){ return; }
                
                const Cards oc = hand[tp].getCards() | mv.cards(); // TODO: 流石に無駄すぎ
                // 2役関係パラメータ差分計算
                const int base = PlayPolicySpace::FEA_IDX(POL_HAND_2PQR);
                
                const Cards er = CardsToER(dc) | (dc & CARDS_JOKER); // joker の更新も必要
                //iterateIntCard(er, [&](IntCard ic)->void{
                //    int r4x = getIntCard_Rankx4(ic);
                // このランクの手札の枚数が変わったのでパラメータを再計算
                //for(int rr4x = 0; rr4x < 16 * 4; rr4x += 4){
                //    if((c >> rr4x) & 15)){
                for(int ord = 0; ord < 2; ++ord){
                    for(int f = 0; f < 16; ++f){
                        if((oc >> (f * 4)) & 15　!= (hand[tp].cards >> (f * 4)) & 15){ // 今回変化した
                            for(int j = f + 1; j < 16; ++j){
                                if((oc >> (j * 4)) & 15){
                                    double s = 0;
                                    // 元のパラメータ
                                    int i = base
                                    + 16 * 16 * 16 * 16 * ord
                                    + f * 16 * 16 * 16
                                    + j * 16 * 16
                                    + ((oc >> (f * 4)) & 15) * 16
                                    + ((oc >> (j * 4)) & 15);
                                    IFoo(i);
                                    
                                    // 新しいパラメータ
                                    i = base
                                    + 16 * 16 * 16 * 16 * ord
                                    + f * 16 * 16 * 16
                                    + j * 16 * 16
                                    + ((c >> (f * 4)) & 15) * 16
                                    + ((c >> (j * 4)) & 15);
                                    Foo(i);
                                }
                            }
                        }
                    }
                    playerPolicyValue[tp].twoMeldsValue[ord] += s;
                }
               // });
                 */
            }
#undef IFooX
#undef IFoo
#undef FooX
#undef Foo

            // 局面更新
            // Move型以外も対応必須?
            template<int IS_PASS = _BOTH, class move_t = Move>
            int procFaster(const int tp, const move_t& mv)noexcept;

            int procPassFaster(const int tp)noexcept{
                //procBoardHash_P(tp);
                if(isSoloAwake()){
                    // renew処理
                    flush();
                }else{
                    setPlayerAsleep(tp);
                    rotateTurnPlayer(tp);
                }
                addTurnNum();
                return getTurnPlayer();
            }
            int proc(const int tp, const MoveInfo mv)noexcept;
            int proc(const int tp, const Move mv)noexcept;
            int procSlowest(const MoveInfo mv)noexcept;
            int procSlowest(const Move mv)noexcept;
            
            void makeChange(int from, int to, Cards dc, int dq)noexcept{
                ASSERT(hand[from].exam(), cerr << hand[from] << endl;);
                ASSERT(hand[to].exam(), cerr << hand[to] << endl;);
                ASSERT(examCards(dc), cerr << OutCards(dc) << endl;);
                ASSERT(holdsCards(hand[from].getCards(), dc), cerr << hand[from] << " -> " << OutCards(dc) << endl;);
                ASSERT(!anyCards(andCards(dc, hand[to].getCards())), cerr << OutCards(dc) << " -> " << hand[to] << endl;)
                uint64_t dhash = CardsToHashKey(dc);
                hand[from].subtrAll(dc, dq, dhash);
                hand[to].addAll(dc, dq, dhash);
                opsHand[from].addAll(dc, dq, dhash);
                opsHand[to].subtrAll(dc, dq, dhash);
                //cerr << "from = " << from << " to = " << to << " dc = " << OutCards(dc) << endl;
                //cerr << toDebugString(); getchar();
            }
            void makeChange(int from, int to, Cards dc)noexcept{
                makeChange(from, to, dc, countCards(dc));
            }
            void makePresents(){
                // 献上を一挙に行う
                for(int cl = 0; cl < MIDDLE; ++cl){
                    const int oppClass = getChangePartnerClass(cl);
                    const int from = getClassPlayer(oppClass);
                    const int to = getClassPlayer(cl);
                    const Cards presentCards = pickHigh(getCards(from), N_CHANGE_CARDS(cl));
                    
                    makeChange(from, to, presentCards, N_CHANGE_CARDS(cl));
                }
            }
            void removePresentedCards(){
                // UECdaにおいてdealt後の状態で献上カードが2重になっているので
                // 献上元のカードを外しておく
                for(int cl = 0; cl < MIDDLE; ++cl){
                    const int oppClass = getChangePartnerClass(cl);
                    const int from = getClassPlayer(oppClass);
                    const int to = getClassPlayer(cl);
                    const Cards dc = andCards(getCards(from), getCards(to));
                    uint64_t dhash = CardsToHashKey(dc);
                    int dq = N_CHANGE_CARDS(cl);
                    
                    hand[from].subtrAll(dc, dq, dhash);
                    opsHand[from].addAll(dc, dq, dhash);
                }
            }

            Cards getCards(int p)const noexcept{ return hand[p].getCards(); }
            Cards getOpsCards(int p)const noexcept{ return opsHand[p].getCards(); }
            uint32_t getNCards(int p)const noexcept{ return hand[p].getQty(); }
            Cards getRemCards()const noexcept{ return remCards; }
            Cards getNRemCards()const noexcept{ return remQty; }
            const Hand& getHand(const int p)const noexcept{ return hand[p]; }
            const Hand& getOpsHand(const int p)const noexcept{ return opsHand[p]; }
            
            uint64_t getRemCardsHash()const noexcept{ return remHash; }
            
            void setHand(int p, Cards ac)noexcept{
                hand[p].setAll(ac);
            }
            void setOpsHand(int p, Cards ac)noexcept{
                opsHand[p].setAll(ac);
            }
            void setRemHand(Cards ac)noexcept{
                remCards = ac;
                remQty = countCards(ac);
                remHash = HASH_CARDS_ALL ^ CardsToHashKey(subtrCards(CARDS_ALL, ac));
            }
            void fillRemHand(Cards ac)noexcept{
                remCards = CARDS_ALL;
                remQty = countCards(CARDS_ALL);
                remHash = HASH_CARDS_ALL;
            }
            
            bool exam()const{
                // validator

                if(!ps.exam()){
                    cerr << "PlayouterField::exam() illegal PlayersState" << endl;
                    cerr << ps << endl; return false;
                }
                if(bd.isNF() && !ps.examSemiNF()){
                    cerr << "PlayouterField::exam() illegal PlayersState on NullField" << endl;
                    cerr << ps << endl; return false;
                }
                // 置換列
                if(!isInitGame()){
                    if(infoClass != invert(infoClassPlayer)){
                        cerr << "PlayouterField::exam() illegal PlayerClass <-> ClassPlayer" << endl;
                        return false;
                    }
                }
                if(infoSeat != invert(infoSeatPlayer)){
                    cerr << "PlayouterField::exam() illegal PlayerSeat <-> SeatPlayer" << endl;
                    return false;
                }

                // 手札
                Cards sum = CARDS_NULL;
                int NSum = 0;
                Cards r = remCards;
                int NR = remQty;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(isAlive(p)){
                        // 上がっていないのに手札が無い場合どうか
                        if(!hand[p].any()){
                            cerr<<"PlayouterField::exam() alive but no card"<<endl;
                            return false;
                        }
                        if(!hand[p].exam()){
                            cerr<<"PlayouterField::exam() alive but invalid hand"<<endl;
                            return false;
                        }

                        Cards c = hand[p].getCards();
                        // 排他性
                        if(!isExclusiveCards(sum, c)){
                            cerr << OutCards(sum) << endl;
                            cerr << hand[p] << endl;
                            cerr << "hand[" << p << "]excl" << endl;
                            return false;
                        }
                        // 包括性
                        if(!holdsCards(r, c)){
                            cerr << OutCards(remCards) << endl;
                            cerr << hand[p] << endl;
                            cerr << "hand[" << p << "]hol" << endl;
                            return false;
                        }

                        addCards(&sum, c);
                        NSum += hand[p].getQty();
                    }else{
                        // 上がっているのに手札がある場合があるかどうか(qtyは0にしている)
                        if(hand[p].qty > 0){
                            cerr << "dead but qty > 0"<<endl;
                        }
                    }
                }

                if(sum != r){

                    for(int p = 0; p < N_PLAYERS; ++p){
                        cerr << OutCards(hand[p].cards) << endl;
                    }

                    cerr << "sum cards - rem cards" << endl;
                    return false;
                }
                if(NR != NSum){
                    cerr << "nsum cards - nrem cards" << endl;
                    return false;
                }
                return true;
            }

            void initForPlayout()noexcept{
                flags.reset();
            }

            void prepareForPlay()noexcept{
                
                int tp = getTurnPlayer();
                
                fInfo.init();
                
                fInfo.setMinNCardsAwake(getOpsMinNCardsAwake(tp));
                fInfo.setMinNCards(getOpsMinNCards(tp));
                fInfo.setMaxNCardsAwake(getOpsMaxNCardsAwake(tp));
                fInfo.setMaxNCards(getOpsMaxNCards(tp));
                
                if(isNF()){
                    if(getNAlivePlayers() == getNAwakePlayers()){ // 空場パスがない
                        fInfo.setFlushLead();
                    }
                }else{
                    if(getPMOwner() == tp){ // セルフフォロー
                        fInfo.setSelfFollow();
                    }else{
                        if(isSoloAwake()){ // SF ではないが LA
                            fInfo.setLastAwake();
                        }
                        uint32_t fLPlayer = getFlushLeadPlayer<_NO>();
                        if(fLPlayer == tp){ // 全員パスしたら自分から
                            fInfo.setFlushLead();
                            if(fInfo.isLastAwake()){
                            }else{
                                if(dominatesHand(getBoard(), opsHand[tp])){
                                    // 場が全員を支配しているので、パスをすれば自分から
                                    fInfo.setBDO();
                                    fInfo.setPassDom(); // fl && bdo ならパス支配
                                }
                            }
                        }
                    }
                }
            }
            
            void init1G()noexcept{
                bd.init();
                ps.init();
                
                turnNum = 0;
                attractedPlayers.reset();
                
                infoSeat.clear();
                infoSeatPlayer.clear();
                infoClass.clear();
                infoClassPlayer.clear();
                infoNewClass.clear();
                infoNewClassPlayer.clear();
                
                infoSpecialPlayer.clear();
                
                phase.init();
                
                remCards = CARDS_ALL;
                remQty = countCards(remCards);
                remHash = HASH_CARDS_ALL;
                
                //hash_prc = 0ULL;
                //hash_bd = 0ULL;
            }
            
            void prepareAfterChange()noexcept{
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(containsD3(hand[p].getCards())){
                        setTurnPlayer(p);
                        setFirstTurnPlayer(p);
                        setPMOwner(p);
                        break;
                    }
                }
                ASSERT(exam(), cerr << toDebugString() << endl;);
            }
            
            std::string toString()const{
                std::ostringstream oss;
                for(int p = 0; p < N_PLAYERS; ++p){
                    oss << p << (isAwake(p) ? " " : "*") << ": ";
                    if(hand[p].qty){
                        oss << hand[p];
                    }else{ // qty だけ 0 にしているため
                        Hand thand;
                        thand.setAll(CARDS_NULL);
                        oss << thand;
                    }
                    oss << endl;
                }
                return oss.str();
            }
            
            std::string toDebugString()const{
                std::ostringstream oss;
                oss << "turn = " << getTurnNum() << endl;
                oss << "player = " << getTurnPlayer() << endl;
                oss << "class = " << infoClass << endl;
                oss << "seat = " << infoSeat << endl;
                oss << "board = " << bd << endl;
                oss << "state = " << ps << endl;
                oss << "hand = " << endl;
                for(int p = 0; p < N_PLAYERS; ++p){
                    oss << p << " : " << hand[p] << endl;
                }
                return oss.str();
            }
        };

        template<>int PlayouterField::procFaster<_YES, MoveInfo>(const int tp, const MoveInfo& mv)noexcept{
            return procPassFaster(tp);
        }
        template<>int PlayouterField::procFaster<_NO, MoveInfo>(const int tp, const MoveInfo& mv)noexcept{
            // パスでない場合
            Move move = mv.mv();

            //procBoardHash_NP(move);

            bd.proc<_BOTH, _NO>(move);
            setPMOwner(tp);
            addTurnNum();
            if(bd.isNF()){ // 流れた
                flushState();
            }else{
                if(mv.isDO()){
                    if(mv.isDM()){
                        // 全員を支配。流れる
                        flush();
                        if(!isAlive(tp)){
                            rotateTurnPlayer(tp);
                        }
                    }else{
                        // 自分以外は支配
                        if(isAlive(tp)){
                            // 自分を除いてasleepにする
                            setAllAsleep();
                            setPlayerAwake(tp);
                        }else{
                            // 流れる
                            flush();
                            rotateTurnPlayer(tp);
                        }
                    }
                }else{
                    // 支配してない
                    rotateTurnPlayer(tp);
                }
            }
            return getTurnPlayer();
        }

        template<>int PlayouterField::procFaster<_BOTH, MoveInfo>(const int tp, const MoveInfo& mv)noexcept{
            if(mv.isPASS()){
                return procFaster<_YES, MoveInfo>(tp, mv);
            }else{
                return procFaster<_NO, MoveInfo>(tp, mv);
            }
        }

        int PlayouterField::proc(const int tp, const MoveInfo mv)noexcept{
            // 丁寧に局面更新
            ASSERT(exam(), cerr << toDebugString() << endl;); // should be valid before Play
            if(mv.isPASS()){
                //procBoardHash_P(tp); // proceed hash value by pass
                if(isSoloAwake()){
                    flush();
                }else{
                    setPlayerAsleep(tp);
                    rotateTurnPlayer(tp);
                }
                addTurnNum();
            }else{
                if(mv.isMate() || mv.qty() >= hand[tp].qty){ // mate or agari
                    if(attractedPlayers.is_only(tp)){
                        // every atrcted player "agari"
                        // we can finish this playout.
                        setPlayerNewClass(tp, getBestClass());
                        return -1;
                    }else if(getNAlivePlayers() == 2){
                        // this game was finished
                        setPlayerNewClass(tp, getWorstClass() - 1);
                        setPlayerNewClass(ps.searchOpsPlayer(tp), getWorstClass());
                        return -1;
                    }else{
                        // only mate or agari deposition
                        if(mv.qty() >= hand[tp].qty){ // agari
                            setPlayerNewClass(tp, getBestClass());
                            ps.setDead(tp);
                            attractedPlayers.reset(tp);
                            procAndKillHand(tp, mv.mv(), mv.cards<_NO>(), mv.qty());
                        }else{
                            procHand(tp, mv.mv(), mv.cards<_NO>(), mv.qty());
                        }
                    }
                }else{
                    procHand(tp, mv.mv(), mv.cards<_NO>(), mv.qty());
                }
                //procBoardHash_NP(mv.mv()); // proceed hash value by non-pass move

                bd.proc<_BOTH, _NO>(mv.mv());
                setPMOwner(tp);
                addTurnNum();
                if(bd.isNF()){ // 流れた
                    flushState();
                }else{
                    if(mv.isDO()){
                        if(mv.isDM()){ // dominates all players. flushes
                            flush();
                            if(!isAwake(tp)){
                                rotateTurnPlayer(tp);
                            }
                        }else{ // dominates players except me.
                            if(isAwake(tp)){
                                // set asleep except me
                                setAllAsleep();
                                setPlayerAwake(tp);
                            }else{
                                // flushes
                                flush();
                                rotateTurnPlayer(tp);
                            }
                        }
                    }else{ // no dominance
                        if(ps.anyAwake()){
                            rotateTurnPlayer(tp);
                        }else{
                            flush();
                            rotateTurnPlayer(tp);
                        }
                    }
                }
            }
            ASSERT(exam(), cerr << toDebugString() << endl;); // should be valid for after plays
            return getTurnPlayer();
        }
        int PlayouterField::proc(const int tp, const Move mv)noexcept{
            return proc(tp, (MoveInfo)mv);
        }
        
        int PlayouterField::procSlowest(const Move mv)noexcept{
            const int tp = getTurnPlayer();
            // 丁寧に局面更新
            ASSERT(exam(), cerr << toDebugString() << endl;); // should be valid before Play
            if(mv.isPASS()){
                //procBoardHash_P(tp); // proceed hash value by pass
                if(isSoloAwake()){
                    flush();
                }else{
                    setPlayerAsleep(tp);
                    rotateTurnPlayer(tp);
                }
                addTurnNum();
            }else{
                if(mv.qty() >= hand[tp].qty){ // agari
                    setPlayerNewClass(tp, getBestClass());
                    ps.setDead(tp);
                    if(ps.isSoloAlive()){
                        setPlayerNewClass(ps.searchL1Player(), getBestClass());
                        return -1;
                    }
                    procAndKillHand(tp, mv, mv.cards<_NO>(), mv.qty());
                }else{
                    procHand(tp, mv, mv.cards<_NO>(), mv.qty());
                }
                //procBoardHash_NP(mv); // proceed hash value by non-pass move
                
                //CERR << bd << " -> ";
                bd.proc<_BOTH, _NO>(mv);
                //CERR << bd << endl;
                setPMOwner(tp);
                addTurnNum();
                if(bd.isNF()){ // 流れた
                    flushState();
                }else{
                    if(ps.anyAwake()){
                        rotateTurnPlayer(tp);
                    }else{
                        flush();
                        rotateTurnPlayer(tp);
                    }
                }
            }
            assert(exam()); // should be valid after Play
            return getTurnPlayer();
        }
        int PlayouterField::procSlowest(const MoveInfo mv)noexcept{
            return procSlowest(mv.mv());
        }
        // 局面情報コンバート
        // WorldFieldとその他の情報(ClientFieldやDoppelgangerField)からPlayouterFieldを設定
        /*template<class wField_t, class addField_t>
        void convField(const wField_t& wField, const addField_t& addField, PlayouterField *const dst)noexcept{

            dst->depth = wField.depth;
            
            // game info
            dst->turnNum = addField.getTurnNum();
            dst->bd = addField.getBoard();
            dst->ps = addField.ps;
            
            dst->infoSeat = addField.infoSeat;
            dst->infoSeatPlayer = addField.infoSeatPlayer;
            dst->infoNewClass = addField.infoNewClass;
            dst->infoNewClassPlayer = addField.infoNewClassPlayer;
            dst->infoClass = addField.infoClass;
            dst->infoClassPlayer = addField.infoClassPlayer;
            
            dst->infoPosition = addField.infoPosition;

            dst->infoSpecialPlayer = addField.infoSpecialPlayer;
            dst->phase = addField.phase;

            // hash_value
            dst->hash_org = addField.hash_org;
            dst->hash_prc = addField.hash_prc;
            dst->hash_ri = addField.hash_ri;
            dst->hash_bd = addField.hash_bd;

            for(int p = 0; p < N_PLAYERS; ++p){
                if(wField.isAlive(p)){
                    dst->hand[p] = wField.hand[p];
                    dst->opsHand[p] = wField.opsHand[p];
                }
            }
            //dst->rHand=wField.rHand;
            dst->remCards = wField.rHand.cards;
            dst->remQty = wField.rHand.qty;
            dst->remHash = wField.rHand.hash;

        }*/

        // copy PlayouterField arg to dst before playout
        void copyField(const PlayouterField& arg, PlayouterField *const dst)noexcept{

            dst->depth = arg.depth;
            
            // playout result
            dst->infoReward = 0ULL;
            dst->domFlags = 0;
            dst->NNullFields = 0;

            // playout info
            dst->attractedPlayers = arg.attractedPlayers;
            dst->mv = arg.mv;
            dst->dice = arg.dice;
            dst->depth = arg.depth;

            // game info
            dst->turnNum = arg.turnNum;
            dst->bd = arg.bd;
            dst->ps = arg.ps;

            dst->infoSeat = arg.infoSeat;
            dst->infoSeatPlayer = arg.infoSeatPlayer;
            dst->infoNewClass = arg.infoNewClass;
            dst->infoNewClassPlayer = arg.infoNewClassPlayer;
            dst->infoClass = arg.infoClass;
            dst->infoClassPlayer = arg.infoClassPlayer;
            
            dst->infoPosition = arg.infoPosition;

            dst->infoSpecialPlayer = arg.infoSpecialPlayer;

            dst->phase = arg.phase;

            // copy hash_value
            //dst->hash_org = arg.hash_org;
            //dst->hash_prc = arg.hash_prc;
            //dst->hash_ri = arg.hash_ri;
            //dst->hash_bd = arg.hash_bd;

            // we don't have to copy each player's hand,
            // because card-position will be set in the opening of playout.

            // in UECda, remained cards is knowable.
            dst->remCards = arg.remCards;
            dst->remQty = arg.remQty;
            dst->remHash = arg.remHash;
        }

        // initialize and set PlayouterField by subjevtive information
        template<class sbjField_t>
        void setSubjectiveField(const sbjField_t& arg, PlayouterField *const dst)noexcept{
            
            dst->depth = arg.depth + 1;
            
            // playout info
            dst->attractedPlayers.reset();
            dst->mv = nullptr;
            
            // game info
            dst->turnNum = arg.turnNum;
            dst->bd = arg.bd;
            dst->ps = arg.ps;
            
            dst->infoSeat = arg.infoSeat;
            dst->infoSeatPlayer = arg.infoSeatPlayer;
            dst->infoNewClass = arg.infoNewClass;
            dst->infoNewClassPlayer = arg.infoNewClassPlayer;
            dst->infoClass = arg.infoClass;
            dst->infoClassPlayer = arg.infoClassPlayer;
            
            dst->infoPosition = arg.infoPosition;
            
            dst->infoSpecialPlayer = arg.infoSpecialPlayer;
            
            dst->phase = arg.phase;
            
            // copy hash_value
            //dst->hash_org = arg.hash_org;
            //dst->hash_prc = arg.hash_prc;
            //dst->hash_ri = arg.hash_ri;
            //dst->hash_bd = arg.hash_bd;
            
            // in UECda, remained cards is knowable.
            dst->remCards = arg.getRemCards();
            dst->remQty = arg.getNRemCards();
            dst->remHash = CardsToHashKey(arg.getRemCards());
            
            // we don't have to copy each player's hand,
            // because card-position will be set in the opening of each playout.
            for(int p = 0; p < N_PLAYERS; ++p){
                dst->hand[p].qty = arg.getNCards(p);
                dst->opsHand[p].qty = arg.getNRemCards() - arg.getNCards(p);
            }
            dst->hand[arg.getMyPlayerNum()].setAll(arg.getMyCards());
            dst->opsHand[arg.getMyPlayerNum()].setAll(arg.getOpsCards());
        }
        
        template<class sbjField_t, class policy_t>
        void setSubjectiveField(const sbjField_t& arg, const policy_t& pol, PlayouterField *const dst)noexcept{
            setSubjectiveField(arg, pol);
            // 方策計算用パラメータをセット
            calcPolicySubValue(pol);
        }

        // set estimated information
        template<class sbjField_t, class world_t>
        void setWorld(const sbjField_t& field, const world_t& world, PlayouterField *const dst)noexcept{

            Cards remCards = field.getRemCards();
            uint64_t remHash = field.getRemCardsHash();

            for(int p = 0; p < N_PLAYERS; ++p){
                if(field.isAlive(p)){
                    // only alive players
                    uint64_t myHash = world.getCardsHash(p);

                    dst->hand[p].set(world.getCards(p));
                    dst->hand[p].setHash(myHash);
                    dst->opsHand[p].set(subtrCards(remCards, world.getCards(p)));
                    dst->opsHand[p].setHash(remHash ^ myHash);
                }else{
                    // alive でないプレーヤーも手札枚数だけセットしておく
                    dst->hand[p].qty = 0;
                }
            }
        }
        
        template<class sbjField_t, class world_t, class policy_t>
        void setWorld(const sbjField_t& field, const world_t& world, const policy_t& pol, PlayouterField *const dst)noexcept{
            setWorld(field, world, dst);
            calcPolicySubValue(pol);
        }
        
        void convField(const PlayouterField& field, const uint32_t player, int table[8][15]){
            // conversion from PlayouterField to 8x15 table
            clearAll(table);
            
            // board move
            const Board bd = field.getBoard();
            MoveToTable(Move(bd), bd, table);
            
            // game state
            table[5][2] = (field.getTurnPlayer() == player) ? 1 : 0;
            table[5][3] = field.getTurnPlayer();
            table[5][4] = bd.isNF() ? 1 : 0;
            table[5][5] = bd.prmOrder() ^ bd.tmpOrder();
            table[5][6] = bd.prmOrder();
            table[5][7] = bd.suitsLocked() ? 1 : 0;
            
            for(int p = 0; p < N_PLAYERS; ++p){
                table[6][p] = field.getNCards(p); // num of cards
                table[6][5 + p] = field.getPlayerClass(p); // class
            }
            for(int s = 0; s < N_PLAYERS; ++s){
                table[6][10 + s] = field.getSeatPlayer(s); // seat player
            }
        }
        void convAfterField(const PlayouterField& field, const uint32_t player, const Move& mv, int table[8][15]){
            // conversion from PlayouterField(after Move) to 8x15table
            clearAll(table);
            
            // board move
            const Board bd = field.getBoard();
            MoveToTable(mv, bd, table);

            // game state
            table[5][2] = (field.getTurnPlayer() == player) ? 1 : 0;
            table[5][3] = field.getTurnPlayer();
            table[5][4] = bd.isNF() ? 1 : 0;
            table[5][5] = bd.afterPrmOrder(mv) ^ bd.afterTmpOrder(mv);
            table[5][6] = bd.afterPrmOrder(mv);
            table[5][7] = bd.locksSuits(mv) ? 1 : 0;
            
            for(int p = 0; p < N_PLAYERS; ++p){
                table[6][p] = field.getNCards(p); // num of cards
                table[6][5 + p] = field.getPlayerClass(p); // class
            }
            for(int s = 0; s < N_PLAYERS; ++s){
                table[6][10 + s] = field.getSeatPlayer(s); // seat player
            }
            
            table[6][field.getTurnPlayer()] -= mv.qty();
        }
        
        void convField(const int table[8][15], const uint32_t player, PlayouterField *const pfield){
            // conversion 8x15table to PlayouterField
            // board move
            pfield->bd = TableToBoard(table);
            
            // game state
            pfield->setTurnPlayer(getTurnPlayer(table));
            pfield->setHand(player, TableToCards(table));
            
            pfield->clearSeats();
            pfield->clearClasses();
            
            for(int p = 0; p < N_PLAYERS; ++p){
                pfield->hand[p].qty = getNCards(table, p); // num of cards
                
                int cl = getPlayerClass(table, p);
                pfield->setPlayerClass(p, cl);
                pfield->setClassPlayer(cl, p);
            }
            
            for(int s = 0; s < N_PLAYERS; ++s){
                int p = getSeatPlayer(table, s);
                pfield->setPlayerSeat(p, s);
                pfield->setSeatPlayer(s, p);
            }
        }

        /**************************仮想世界**************************/

        struct ImaginaryWorld{
            //仮想世界
            constexpr static int ACTIVE = 0;
            constexpr static int USED = 1;

            std::bitset<32> flags;

            double weight; // この世界の存在確率比
            int builtTurn; // この世界がセットされたターン
            uint64_t hash; // 世界識別ハッシュ。着手検討中ターンにおいて世界を識別出来れば形式は問わない

            int depth;
            Cards cards[N_PLAYERS];
            uint64_t hash_cards[N_PLAYERS];

            Cards getCards(int p)const{ return cards[p]; }
            uint64_t getCardsHash(int p)const{ return hash_cards[p]; }
            int getDepth()const{ return depth; }

            void clear(){
                flags.reset();
                weight = 1.0;
            }
            void activate(){
                flags.set(ACTIVE);
                DERR << "ACTIVEATED!" << endl;
            }

            int isActive()const{ return flags.test(ACTIVE); }

            template<class sbjField_t>
            void set(const sbjField_t& field, const Cards argCards[]){
                // worldfieldのセット
                // 手札組み合わせを設定
                /*wField.rHand=field.getRemHand();
                for(int p=0;p<N_PLAYERS;++p){
                if(field.isAlive(p)){
                wField.hand[p].set( argCards[p] );

                uint64_t chash=genHash_Cards(argCards[p]);
                wField.hand[p].setHash( chash );
                wField.opsHand[p].set( maskCards(wField.rHand.cards,argCards[p]) );
                wField.opsHand[p].setHash( wField.rHand.hash ^ chash );
                }
                //wField.used[p]=field.getUsedCards(p);
                }

                wField.ps=field.ps;

                */
                depth = field.getDepth() + 1; // 1深くなる

                clear();
                for(int p = 0; p < N_PLAYERS; ++p){
                    cards[p] = argCards[p];
                    hash_cards[p] = CardsToHashKey(argCards[p]);
                }
                builtTurn = field.getTurnNum();
            }

            void proc(const int p, const Move mv, const Cards dc){
                // 世界死がおきずに進行した
                //uint64_t dhash = CardsToHashKey(dc);
                //uint32_t dq = mv.qty();

                /*wField.rHand.makeMoveAll(mv,dc,dq,dhash);

                wField.hand[p].makeMove(mv,dc);
                wField.hand[p].hash^=dhash;
                wField.opsHand[p].makeMove(mv,dc);
                wField.opsHand[p].hash^=dhash;*/
            }

            int checkRationality(){
                // 生き残っている世界の合理性を判断する
                // ドッペルゲンガーを通じた過去の完全勝利判断
                return 0;
            }
            ImaginaryWorld(){ clear(); }
            ~ImaginaryWorld(){ clear(); }
        };
    }
}

#endif // UECDA_FUJI_PLAYOUT_H_

