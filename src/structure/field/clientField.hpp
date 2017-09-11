/*
 clientField.hpp
 Katsuki Ohto
 */

#ifndef UECDA_STRUCTURE_CLIENTFIELD_HPP_
#define UECDA_STRUCTURE_CLIENTFIELD_HPP_

#include "commonField.hpp"

namespace UECda{
    
    // クライアントの主観的視点からの情報
    // クライアントが持つ情報は全てここから参照出来るようにする
    // 低次な演算はメンバ関数でサポート
    // メモリ節約をする必要は無いけれども
    // 諸々の互換性のためビット表現としている
    
    // コマンドライン入力で思考する場合にはルールに沿わない入力が来ることがあるので
    // セットする関数が返り値を持つようにしている
    
    struct ClientField : public CommonField{
        // 設定
        
        // 現実世界の深さは0
        static constexpr int depth = 0;
        
        // 直近のプレーヤーの着手(パス含む)
        Move lastMove;
        uint32_t lastTurnPlayer;
        
        Cards myCards;
        Cards myDealtCards;
        Cards myOrgCards;
        
        Cards sentCards; // カード交換にて自分があげたカード（現ルールでは必ず初期に確定）
        Cards recvCards; // カード交換にて交換相手がくれたカード（現ルールでは自分が下位の場合のみ初期に確定）
        
        // 手札
        Cards opsCards; // 自分以外
        Cards remCards; // 全体
        
        // 宣言
        int mateClass; // 初めてMATEと判定した階級
        int L2Class; // L2において判定した階級
        
        // 自分の決定手を記録(現在未使用)
        Stack<Move, 5> nextMoves;
        
        // 自分のスタッツ
        MiniStats myStats;
        
        // 以下クライアントの個人的スタッツ
        uint32_t playRejects, changeRejects; // リジェクト
        std::array<std::array<uint32_t, 3>, 2> myL2Result; // (勝利宣言, 無宣言, 敗戦宣言) × (勝利, 敗戦)
        std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS> myMateResult; // MATEの宣言結果
        
        std::array<int, 64> myScore64; // ここ64試合の自分の得点。index は game_num % 64
        int myScore64Sum; // 合計
        
        int getDepth()const{ return depth; }
        
        // カード集合関連
        int setMyDealtCards(Cards cards){
            myDealtCards = cards;
            return 0;
        }
        int setMyOrgCards(Cards cards){
            myOrgCards = cards;
            return 0;
        }
        int setMyCards(Cards cards){
            myCards = cards;
            opsCards = maskCards(remCards, cards);
            return 0;
        }
        
        Cards getMyCards()const noexcept{ return myCards; }
        Cards getMyDealtCards()const noexcept{ return myDealtCards; }
        
        int setMyRecvCards(Cards cards)noexcept{
            if((int)countCards(cards) != N_CHANGE_CARDS(getMyClass())){
                //cerr << "ClientField : illegal recv cards." << endl;
                return -1;
            }
            recvCards = cards;
            return 0;
        }
        int setMySentCards(Cards cards)noexcept{
            if((int)countCards(cards) != N_CHANGE_CARDS(getMyClass())){
                //cerr << "ClientField : illegal sent cards." << endl;
                return -1;
            }
            sentCards = cards;
            return 0;
        }
        
        Cards getMySentCards()const noexcept{ return sentCards; }
        Cards getMyRecvCards()const noexcept{ return recvCards; }
        
        Cards getOpsCards()const noexcept{ return subtrCards(getRemCards(), getMyCards()); }
        
        void pushNextMove(Move m)noexcept{ nextMoves.push(m); }
        Move getNextMove()noexcept{ return nextMoves.pop_get(); }
        
        int setMyPlayerNum(int num)noexcept{
            if(!examPlayerNum(num)){
                cerr << "ClientField : illegal my player number." << endl;
                return -1;
            }
            myStats.setMyPlayerNum(num);
            return 0;
        }
        int setMyPosition(int pos)noexcept{
            if(!examPlayerNum(pos)){
                cerr << "ClientField : illegal my position." << endl;
                return -1;
            }
            myStats.setMyPosition(pos);
            return 0;
        }
        int setMyClass(int cl)noexcept{
            if(!examPlayerNum(cl)){
                cerr << "ClientField : illegal my class." << endl;
                return -1;
            }
            myStats.setMyClass(cl);
            return 0;
        }
        void setMyTurn()noexcept{ myStats.setMyTurn(); }
        void resetMyTurn()noexcept{ myStats.resetMyTurn(); }
        void setMyWon()noexcept{ myStats.setMyWon(); }
        
        uint32_t getMyPlayerNum()const noexcept{ return myStats.getMyPlayerNum(); }
        uint32_t getMyPosition()const noexcept{ return myStats.getMyPosition(); }
        uint32_t getMyClass()const noexcept{ return myStats.getMyClass(); }
        uint32_t isMyTurn()const noexcept{ return myStats.isMyTurn(); }
        uint32_t isMyWon()const noexcept{ return myStats.isMyWon(); }
        
        void setMyChangeImpQty(int qty)noexcept{ myStats.setMyChangeImpQty(qty); }
        void setMyChangeReqQty(int qty)noexcept{ myStats.setMyChangeReqQty(qty); }
        // 関与枚数
        uint32_t getMyChangeImpQty()const noexcept{ return myStats.getMyChangeImpQty(); }
        uint32_t getMyChangeReqQty()const noexcept{ return myStats.getMyChangeReqQty(); }
        // 関与するかどうか?
        uint32_t isMyImpChange()const noexcept{ return myStats.isMyImpChange(); }
        uint32_t isMyReqChange()const noexcept{ return myStats.isMyReqChange(); }
        
        void setMyNewClass(int r)noexcept{ myStats.setNewClass(r); }
        uint32_t getMyNewClass()const noexcept{ return myStats.getNewClass(); }
        
        
        void setMyMate()noexcept{ // 詰み宣言
            if(mateClass == -1)mateClass = getBestClass();
        }
        void setMyL2Mate()noexcept{ // L2詰み宣言
            if(L2Class == -1)L2Class = N_PLAYERS - 2;
        }
        void setMyL2GiveUp()noexcept{ // L2敗北宣言
            if(L2Class == -1)L2Class = N_PLAYERS - 1;
        }
        
        // 客観情報関連
        uint32_t getRivalPlayersFlag()const noexcept{
            // ライバルプレーヤー(自分以外のプレーヤーの中の最上位)集合を得る
            uint32_t ret = 0U;
            int best = 99999;
            for(int p = 0; p < N_PLAYERS; ++p){
                if(p != (int)getMyPlayerNum()){
                    int pos = getPosition(p);
                    if(pos < best){
                        ret = (1U << p);
                        best = pos;
                    }else if(pos == best){
                        ret |= (1U << p);
                    }
                }
            }
            assert(ret != 0U);
            return ret;
        }
        
        uint32_t getNMyCards()const noexcept{ return getNCards(getMyPlayerNum()); }
        
        void shiftSeats(){ // 自分が0になるように座席番号を付け替える
            DERR << infoSeatPlayer << endl;
            infoSeatPlayer.rotate(getPlayerSeat(getMyPlayerNum()));
            infoSeat.clear();
            DERR << infoSeatPlayer << endl;
            for(int s = 0; s < N_PLAYERS; ++s){
                infoSeat.set(getSeatPlayer(s), s);
            }
            DERR << infoSeat << endl;
        }
        
        void setPlayerDead(const uint32_t p){
            // プレーヤーがあがった（天に召された）
            if(p == getMyPlayerNum()){
                setMyWon();
                setMyNewClass(getBestClass());
                DERR << "My Class = " << getMyNewClass() << endl;
            }
            // 順位
            setPlayerNewClass(p, getBestClass());
            assert(ps.isAlive(p) && ps.isAwake(p)); // 現在AliveかつAwakeであったことは保証されているとする
            ps.setDead(p);
            if(ps.isSoloAlive()){
                setL1Player(ps.searchL1Player());
            }
        }
        
        void setLastPlayerDead(const uint32_t p){
            // 最後に残ったプレーヤーのあがり処理
            if(p == getMyPlayerNum()){
                setMyWon();
                setMyNewClass(getWorstClass());
                DERR << "My Class = " << getMyNewClass() << endl;
            }
            // 順位
            setPlayerNewClass(p,getWorstClass());
            assert(ps.isAlive(p)); // 現在Aliveであったことは保証されているとする
            ps.setDeadOnly(p);
        }
        
        void procHand(const uint32_t tp, const Move mv, const Cards used, const uint32_t qty){
            removeCards(tp, used);
            if(tp == getMyPlayerNum()){
                subtrCards(&myCards, used);
            }else{
                subtrCards(&opsCards, used);
            }
            DERR << "make move started." << endl;
            subtrNCards(tp, qty);
        }
        
        uint32_t proc(const Move m){
            uint32_t tp = getTurnPlayer();
            
            lastMove = m;
            lastTurnPlayer = tp;
            
            if(m.isPASS()){ // パス
                return procPass();
            }else{ // パスでない
                Cards dc = m.cards<_NO>();
                uint32_t dq = m.qty();
                
                procHand(tp, m, dc, dq);
                bd.proc(m);
                setPMOwner(tp);
                
                if(bd.isNF()){ // 流れた
                    flushState();
                }
                
                if(getNCards(tp) <= 0){ // あがり
                    setPlayerDead(tp);
                    if(getNAwakePlayers() <= 0){
                        if(!bd.isNF()){
                            bd.flush(); flushState();
                        }
                    }
                    if(isSoloAlive()){
                        setLastPlayerDead(searchL1Player());
                        return 0xFFFFFFFF;
                    }
                }
                addTurnNum();
                if(!bd.isNF()){
                    rotateTurnPlayer(tp);
                }
                return getTurnPlayer();
            }
        }
        
        void procPassByServer(){
            uint32_t tp = getTurnPlayer();
            setPlayerAsleep(tp);
            addTurnNum();
        }
        
        void procByServer(const Move m, const Cards cards){
            // サーバーからの情報を受けた場合に、場の情報を更新するが場を流したりはしない処理
            // サーバーの役表現バグがあった場合のため、場の更新とカード集合の更新を別々に行う
            uint32_t tp = getTurnPlayer();
            
            lastMove = m;
            lastTurnPlayer = tp;
            
            if(m.isPASS()){ // パス
                procPassByServer();
            }else{ // パスでない
                uint32_t dq = m.qty();
                
                procHand(tp, m, cards, dq);
                bd.procExceptFlush(m);
                if(getNCards(tp) <= 0){//あがり
                    setPlayerDead(tp);
                    if(isSoloAlive()){
                        setLastPlayerDead(searchL1Player());
                        return;
                    }
                }
                setPMOwner(tp);
                addTurnNum();
            }
        }
        
        void feedPlayRejection()noexcept{ ++playRejects; }
        void feedChangeRejection()noexcept{ ++changeRejects; }
        void feedL2Result(int realClass){
            int index = (L2Class == -1) ? 1 : (L2Class == N_PLAYERS - 1 ? 2 : 0);
            if(L2Class != -1){
                if(L2Class > realClass)cerr << "L2 Lucky!" << endl;
                if(L2Class < realClass)cerr << "L2 Miss!" << endl;
            }
            myL2Result[realClass - (N_PLAYERS - 2)][index] += 1;
        }
        void feedMyMateClass(int realClass){
            if(mateClass != realClass){ // MATE宣言失敗
                cerr << "Mate Miss! DCL:" << mateClass;
                cerr << " REAL:" << realClass << endl;
            }
            myMateResult[realClass][mateClass] += 1;
        }
        
        void feedMyStats(){
            if(myStats.getNewClass() >= N_PLAYERS - 2){ // ラスト2人
                feedL2Result(myStats.getNewClass());
            }
            if(mateClass != -1){ // MATE宣言あり
                feedMyMateClass(myStats.getNewClass());
            }
        }
        
        void feedResult()noexcept{
            CommonField::feedResult();
            // 自分の64試合分の結果を更新
            int sc = REWARD(getPlayerNewClass(getMyPlayerNum()));
            int index = getGameNum() % 64;
            if(myScore64[index] != -1){
                myScore64Sum -= myScore64[index];
            }
            myScore64[index] = sc;
            myScore64Sum += sc;
        }
        
        // コンソール表示
        // 部分空間のメンバ関数であることに違和感はあるが楽なので
        // 結構主観的な内容
        void broadcastBMatch()const;
        void broadcastBGame()const;
        void broadcastPlayerState()const;
        void broadcastBP()const;
        void broadcastAP()const;
        void broadcastPlay(uint32_t, Move)const;
        void broadcastMyChange(Cards)const;
        void broadcastMyPlay(Move)const;
        
        // 初期化
        void initMatch(){
            CommonField::initMatch();
            playRejects = changeRejects = 0;
            
            for(auto& a : myMateResult)a.fill(0);
            for(auto& a : myL2Result)a.fill(0);
            
            myScore64Sum = 0;
            myScore64.fill(-1); // 結果無しを表現
            
            nextMoves.clear();
        }
        void initGame(){
            CommonField::initGame();
            
            mateClass = L2Class = -1;
            
            myStats.init();

            myDealtCards = myCards = CARDS_NULL;
            sentCards = recvCards = CARDS_NULL;
            remCards = CARDS_ALL;

            nextMoves.init();
        }
        
        // 後処理
        void closeGame(){
            CommonField::closeGame();
        }
        void closeMatch(){
//#ifdef MONITOR
            // スタッツ表示
            cerr << endl << "---" << MY_NAME << "'s seiseki happyou!!!---" << endl;
            cerr << "R*NR";
            for(int cl = 0; cl < N_PLAYERS; ++cl){
                cerr << " " << cl << " ";
            }
            cerr << " total" << endl;
            for(int cl = 0; cl < N_PLAYERS; ++cl){
                cerr << "  " << cl << "  ";
                for(int ncl = 0; ncl < N_PLAYERS; ++ncl){
                    cerr << classTransition[getMyPlayerNum()][cl][ncl] << "  ";
                }
                cerr << classDestination[getMyPlayerNum()][cl] << endl;
            }
            cerr << "Other Players" << endl;
            for(int p = 0; p < N_PLAYERS; ++p){
                if(p != (int)getMyPlayerNum()){
                    cerr << "Player " << p << " :" << endl;
                    cerr << "R*NR";
                    for(int cl = 0; cl < N_PLAYERS; ++cl){
                        cerr << " " << cl << " ";
                    }
                    cerr << " total" << endl;
                    for(int cl = 0; cl < N_PLAYERS; ++cl){
                        cerr << "  " << cl << "  ";
                        for(int ncl = 0; ncl < N_PLAYERS; ++ncl){
                            cerr << classTransition[p][cl][ncl] << "  ";
                        }
                        cerr << classDestination[p][cl] << endl;
                    }
                }
            }
            
            cerr << "My Stats" << endl;
            
            cerr << "change rejection by server : " << changeRejects << endl;
            cerr << "play rejection by server : " << playRejects << endl;
            
            cerr << "L2 result : " << endl;
            for(auto& a : myL2Result){
                for(auto i : a)cerr << i << " ";
                cerr << endl;
            }
            cerr << "mate result : " << endl;
            for(auto& a : myMateResult){
                for(auto i : a)cerr << i << " ";
                cerr << endl;
            }cerr << endl;
//#endif
        }
        
        ClientField(){}
        ~ClientField(){}
    };
    
    //表示中身
    void ClientField::broadcastBMatch()const{ // 全試合開始前実況
        // 自分のプロフィールや設定を確認する
        cerr << "Client's Name : " << MY_NAME << " " << MY_VERSION << endl;
        cerr << "Client's Coach :" << MY_COACH << endl;
        cerr << "Thinking Level : " << THINKING_LEVEL << "  (value of time = " << VALUE_PER_SEC << " pts/sec)" << endl;
        cerr << "My Common Number = " << getMyPlayerNum() << endl;
    }
    
    void ClientField::broadcastBGame()const{ // 1ゲーム開始前実況
        cerr << "Game " << getGameNum() << " is about to start." << endl;
        cerr << "Game Type...";
        if(isInitGame()){
            cerr << "INIT GAME" << endl;
        }else{
            cerr << "ADVANTAGE GAME  " << endl;
        }
        for(int s = 0; s < N_PLAYERS; ++s){
            int pNum = getSeatPlayer(s);
            int pos = getPosition(pNum) + 1; // 最上位を1位に変更
            cerr << "[Seat " << s << "] : Player " << pNum << "  Class " << getPlayerClass(pNum) << "  Score " << getScore(pNum) << " ( " << pos;
            switch(pos % 100){
                case 11: case 12:
                    cerr << "th"; break;
                default:
                    switch(pos % 10){
                        case 1: cerr << "st"; break;
                        case 2: cerr << "nd"; break;
                        case 3: cerr << "rd"; break;
                        default: cerr << "th"; break;
                    }
                    break;
            }
            cerr << " place)";
            if(pNum == getMyPlayerNum()){ cerr << " <- Me"; }
            cerr << endl;
        }
    }
    
    void ClientField::broadcastPlayerState()const{ // プレーヤー状態実況
        for(int s = 0; s < N_PLAYERS; ++s){
            int pNum = getSeatPlayer(s);
            cerr << "[Seat " << s << "] : Player " << pNum << "  Class " << getPlayerClass(pNum);
            if(ps.isAlive(pNum)){
                if(isAwake(pNum)){
                    cerr << "          ";
                }else{
                    cerr << "  ASLEEP  ";
                }
            }else if(ps.isExcluded(pNum)){
                cerr << " EXCLUDED ";
            }else{
                cerr << "  DEAD(";
                cerr << getPlayerNewClass(pNum);
                cerr << ") ";
            }
            cerr << "last " << getNCards(pNum) << " cards ,";
            cerr << " used " << OutCards(usedCards[pNum]);
            cerr << endl;
        }
        cerr << "Players alive : " << getNAlivePlayers() << "  Players awake : " << getNAwakePlayers() << endl;
    }
    
    void ClientField::broadcastBP()const{ // プレー前実況
        cerr << "********** Before Play **********";
        cerr << "<Game> " << getGameNum() << " <Turn> " << getTurnNum();
        cerr <<  "  TurnPlayer : " << getTurnPlayer() << "  Owner : " << getPMOwner() << endl;
        cerr << "Board : " << bd << endl;
        cerr << Out2CardTables(getMyCards(), subtrCards(getRemCards(), getMyCards()));
        broadcastPlayerState();
    }
    
    void ClientField::broadcastAP()const{ // プレー後実況
        cerr << "********** After Play **********";
        cerr << "<Game> " << getGameNum() << " <Turn> " << getTurnNum();
        cerr << "  TurnPlayer : " << getTurnPlayer() << "  Owner : " << getPMOwner() << endl;
        cerr << "Board : " << bd << endl;
        cerr << Out2CardTables(getMyCards(), subtrCards(getRemCards(), getMyCards()));
        broadcastPlayerState();
    }
    
    void ClientField::broadcastPlay(uint32_t p, Move move)const{ // プレー実況
        cerr << "[PLAY] ";
        if(p == getMyPlayerNum()){
            cerr << "MY PLAY  ";
        }else{
            cerr << "Player " << p << "  ";
        }
        cerr << move;
        int qty = getNCards(p);
        if(qty <= 0){
            cerr << " passed away...  Class " << getPlayerNewClass(p);
        }else{
            if(qty == 1){
                cerr << " 1 card";
            }else{
                cerr << " " << qty << " cards";
            }
            cerr << " still remains.";
        }
        cerr << endl;
    }
    
    void ClientField::broadcastMyChange(Cards cards)const{ // 自分のプレー決定実況
        cerr << "My decided change : " << OutCards(cards) << endl;
    }
    
    void ClientField::broadcastMyPlay(Move move)const{ // 自分のプレー決定実況
        cerr << "My decided move : ";
        cerr << move;
        //決定手が決まっている場合はそれも表示
        for(int m = nextMoves.size() - 1; m >= 0; --m){
            cerr << " -> " << nextMoves.getData(m);
        }
        cerr << endl;
    }
}

#endif // UECDA_STRUCTURE_CLIENTFIELD_HPP_
