/*
 client.cc
 */

/////////////////////////////////

// UECコンピュータ大貧民大会(UECda)用クライアントメイン
// by Katsuki Ohto

/////////////////////////////////


// ヘッダ
#include "connection.h"

#include "include.h"

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

// クライアント
#ifdef HUMAN_MODE
#include "ai/human.hpp"
using namespace UECda::Human;
#elif RANDOM_MODE
#include "ai/randomAI.hpp"
using namespace UECda::RandomAI;
#else
#include "fuji/FujiGokoro.hpp"
using namespace UECda::Fuji;
#endif

Client client;

int main(int argc, char* argv[]){ // for UECda
    
    // main関数はクライアントの思考の進行役を務める
    // サーバーの情報を、クライアントが用いる共通型に変換して最低限の主観的情報を更新する
    // それ以上高次な演算はクライアントが行う
    // ただしClientFieldに高次な演算結果のスペースがあることについては問題ない
    
    CERR << MY_NAME << " ver. " << MY_VERSION << " trained with " << MY_COACH << "." << endl;
    
    // 時間計測(microsec単位)
    ClockMicS clms;
    uint64_t tmpTime;
    
    // 基本的な初期化
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    // ファイルパスの取得
    {
        std::ifstream ifs("./blauweregen_config.txt");
        if(!ifs){
            cerr << "main() : failed to open config file." << endl;
        }
        if(ifs){ ifs >> DIRECTORY_PARAMS_IN; }
        if(ifs){ ifs >> DIRECTORY_PARAMS_OUT; }
        if(ifs){ ifs >> DIRECTORY_LOGS; }
    }
    
    // 全試合前の初期化
#ifdef LOGGING
#ifdef _ENGINE_FUJI_
    auto& game_log = client.gameLog;
    auto& match_log = client.matchLog;
    match_log.init();
#endif
#endif
    
    auto& field = client.field;
    
    field.initMatch();
    
    bool seedSet = false;
    int seed = -1;
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-i")){ // input directory
            DIRECTORY_PARAMS_IN = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-o")){ // output directory
            DIRECTORY_PARAMS_OUT = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-s")){ // random seed
            seedSet = true;
            seed = atoi(argv[c + 1]);
        }
#ifdef _ENGINE_FUJI_
#ifndef MATCH
        // プレー設定 大会版ビルドでは定数として埋め込む
        else if(!strcmp(argv[c], "-th")){ // num of threads
            int NThreads = atoi(argv[c + 1]);
            Settings::NPlayThreads = NThreads;
            Settings::NChangeThreads = max(1, NThreads / 2);
        }else if(!strcmp(argv[c], "-pm")){ // play modeling
            Settings::simulationPlayModel = true;
        }else if(!strcmp(argv[c], "-npm")){ // no play modeling
            Settings::simulationPlayModel = false;
        }else if(!strcmp(argv[c], "-l2r")){ // L2 search on the root state
            Settings::L2SearchOnRoot = true;
        }else if(!strcmp(argv[c], "-nol2r")){ // no L2 search on the root state
            Settings::L2SearchOnRoot = false;
        }else if(!strcmp(argv[c], "-mater")){ // Mate search on the root state
            Settings::MateSearchOnRoot = true;
        }else if(!strcmp(argv[c], "-nomater")){ // no Mate search on the root state
            Settings::MateSearchOnRoot = false;
        }else if(!strcmp(argv[c], "-t")){ // temperarure
            Settings::simulationTemperaturePlay = atof(argv[c + 1]);
            Settings::temperaturePlay = atof(argv[c + 1]);
            Settings::temperatureChange = atof(argv[c + 1]);
        }else if(!strcmp(argv[c], "-ac")){ // softmax amplify coefficient
            Settings::simulationAmplifyCoef = atof(argv[c + 1]);
        }else if(!strcmp(argv[c], "-ae")){ // softmax amplify exponent
            Settings::simulationAmplifyExponent = atof(argv[c + 1]);
        }else if(!strcmp(argv[c], "-l2s")){ // L2 search in simulations
            Settings::L2SearchInSimulation = true;
        }else if(!strcmp(argv[c], "-nol2s")){ // no L2 search in simulations
            Settings::L2SearchInSimulation = false;
        }else if(!strcmp(argv[c], "-mates")){ // Mate search in simulations
            Settings::MateSearchInSimulation = true;
        }else if(!strcmp(argv[c], "-nomates")){ // no Mate search in simulations
            Settings::MateSearchInSimulation = false;
        }else if(!strcmp(argv[c], "-ss")){ // selector in simulation
            std::string selectorName = std::string(argv[c + 1]);
            if(!strcmp(argv[c + 1], "e")){ // exp
                Settings::simulationSelector = Selector::EXP_BIASED;
            }else if(!strcmp(argv[c + 1], "p")){ // poly
                Settings::simulationSelector = Selector::POLY_BIASED;
            }else if(!strcmp(argv[c + 1], "t")){ // thres
                Settings::simulationSelector = Selector::THRESHOLD;
            }else if(!strcmp(argv[c + 1], "n")){ // naive
                Settings::simulationSelector = Selector::NAIVE;
            }else{
                cerr << " : unknown selector [" << std::string(std::string()) << "] : default selector will be used." << endl;
            }
        }else if(!strcmp(argv[c], "-dt")){ // deal type in estimation
            std::string dealTypeName = std::string(argv[c + 1]);
            if(!strcmp(argv[c + 1], "re")){ // rejection
                Settings::monteCarloDealType = DealType::REJECTION;
            }else if(!strcmp(argv[c + 1], "b")){ // bias
                Settings::monteCarloDealType = DealType::BIAS;
            }else if(!strcmp(argv[c + 1], "s")){ // subjective info
                Settings::monteCarloDealType = DealType::SBJINFO;
            }else if(!strcmp(argv[c + 1], "rn")){ // random
                Settings::monteCarloDealType = DealType::RANDOM;
            }else{
                cerr << " : unknown deal type [" << std::string(std::string()) << "] : default deal type will be used." << endl;
            }
        }
#endif
#endif
    }
    
    checkArg(argc, argv); // 引数のチェック 引数に従ってサーバアドレス、接続ポート、クライアント名を変更
    int my_player_num = entryToGame(); // ゲームに参加
    
    field.setMyPlayerNum(my_player_num);
    client.initMatch(); // ここで呼ばないとプレーヤー番号が反映されない
    
    if(seedSet){
        client.setRandomSeed(seed);
    }
    
#ifdef BROADCAST
    field.broadcastBMatch();
#endif
    
    // 通信変数
    int recv_table[8][15];
    int send_table[8][15];
    int whole_gameend_flag;
    int one_gameend_flag;
    int accept_flag;
    
    clearAll(recv_table);
    clearAll(send_table);
    
    // 試合フェーズ
    while(1){
        // 各試合前の初期化
        field.initGame();
        client.initGame();
        
        // ログ準備
#ifdef _ENGINE_FUJI_
        auto& game_log = client.gameLog;
#endif
        game_log.init();
        
        startGame(recv_table); // 自分のカード、初期情報受け取り
        
        // 自分のプレーヤー番号設定（初期化されて消えてもいいように毎試合やる）
        field.setMyPlayerNum(my_player_num);
        
        // 自分の現在順位（スタッツに記録されているはず）
        field.setMyPosition(field.getPosition(my_player_num));
        
        // 席順設定
        for(int s = 0; s < N_PLAYERS; ++s){
            int p = getSeatPlayer(recv_table, s);
            field.setPlayerSeat(p, s);
            game_log.setPlayerSeat(p, s);
        }
        field.shiftSeats(); // 自分が0になるように座席番号を付け替える
        
        // 自分のカード設定
        Cards c = TableToCards(recv_table, false);
        field.setMyDealtCards(c);
        game_log.setDealtCards(my_player_num, c);
        field.setMyCards(c);
        
        // 階級設定
        // init game cycleが何試合かは未定なため、
        // テーブル情報からinit gameを判定する必要がある。
        CERR << toString(recv_table) << endl;
        
        if(isInitGame(recv_table)){
            // init gameは全員平民としておく
            CERR << "main() : probed init game from table" << endl;
            field.setInitGame();
            game_log.setInitGame();
            for(int p = 0; p < N_PLAYERS; ++p){
                field.setPlayerClass(p, MIDDLE);
                game_log.setPlayerClass(p, MIDDLE);
            }
            field.setMyClass(MIDDLE);
        }else{
            CERR << "main() : probed change game from table" << endl;
            field.resetInitGame();
            game_log.resetInitGame();
            for(int p = 0; p < N_PLAYERS; ++p){
                int cl = getPlayerClass(recv_table, p);
                field.setPlayerClass(p, cl);
                game_log.setPlayerClass(p, cl);
            }
            field.setMyClass(getPlayerClass(recv_table, my_player_num));
        }
        
        // カード枚数設定
        // javaサーバーに引っかからないように、最初にjavaなのかCなのか判定
        // init gameのときは関係ないが、falseになればそれで良い
        bool is_java;
        
        if(!field.isInitGame()
           && getNCards(recv_table, field.getClassPlayer(DAIFUGO)) == 13U
           ){
            is_java = true;
            CERR << "main() : This is java server." << endl;
        }else{
            is_java = false;
        }
        
        for(int p = 0; p < N_PLAYERS; ++p){
            int NCards = getNCards(recv_table, p);
            if(is_java){
                // 高位者は引く
                int cl = field.getPlayerClass(p);
                if(cl <= FUGO)NCards -= N_CHANGE_CARDS(cl);
            }
            field.setNOrgCards(p, NCards);
            game_log.setNOrgCards(p, NCards);
            field.setNCards(p, NCards);
        }
        
#ifdef BROADCAST
        field.broadcastBGame();
#endif
        
        // カード交換フェーズ
        if(!field.isInitGame()){ // カード交換あり
            
            CERR << "main() : change phase" << endl;
            field.setInChange();
            
            // 順位との関係から自分の交換への関与をまとめる
            if(field.getMyClass() != HEIMIN){ // 平民でない
                int change_qty = N_CHANGE_CARDS(field.getMyClass());
                
                DERR << change_qty << endl;
                DERR << field.getMyClass() << endl;
                
                field.setMyChangeImpQty(change_qty);
                if(field.getMyClass() <= FUGO){ // 上位
                    field.setMyChangeReqQty(change_qty);
                }
            }
            
            if(field.isMyImpChange()){ // 自分がカード交換に関与している
                CERR << "main() : my imp for change" << endl;
                if(field.isMyReqChange()){ // 自分があげるカードを選択する必要あり
                    CERR<<"main() : my req for change" << endl;
                    Cards cCards;
                    DERR << field.isMyReqChange() << endl;
                    
                    // 新
                    cCards = client.change(field.getMyChangeReqQty()); // 自分のカード交換選択
                    field.setMySentCards(cCards); // 実際にその交換が行われるかわからないがとりあえず設定
                    CardsToTable(cCards, send_table);
                    
                    sendChangingCards(send_table);
                    
#ifdef BROADCAST
                    field.broadcastMyChange(cCards);
#endif
                }else{
                    //自分はもらう側なのでここでは何もしない
                }
            }else{ // 自分はカード交換に関与しない
                client.prepareForGame(); // 試合前準備
            }
            
            field.resetInChange();
            
        }else{ // カード交換なし
            CERR << "main() : no change game" << endl;
            client.prepareForGame();
        }
        
#ifdef YAOCHO_MODE
        //対戦相手のカードをこっそり教えてもらう（実験用）
        client.tellOpponentsCards();
#endif
        
        // プレーフェーズ
        // javaサーバーがルールと異なった挙動を示す事があり厄介なので、
        // 自分の情報の更新を行う際に試合を進行させないようにする
        // ただし全員パスによる流れは更新しておいた方が良いようだ
        
        field.setFirstTurn(); // 初手フラグオン
        
        while(1){ // ターンループ
            
            clms.start(); // ここで時間計測開始
            // javaサーバーでは、ここで時間計測を初めても自分以外ほぼ0になってしまう。解決策は不明...
            
            // 自分のカードとフィールド情報を受け取り
            // この通信順がおかしい?のか、この後に時間計測を始めても相手の計算時間がわからない(ほぼ0になる)<-c server
            receiveCards(recv_table);
            
            // 自分の情報をサーバーの情報に合わせておく
            int turnPlayer = getTurnPlayer(recv_table);
            
            if(isNF(recv_table) && ((!field.isNF()) || (field.getNAwakePlayers() == 0))){
                // 空場パスに対応するため、既に空場と認識していない状況、
                // またはawakeなプレーヤーが誰もいない状況でのみ流す
                field.flush();
            }
            
            if(suitsLocked(recv_table)){
                field.lockSuits();
            }
            
            field.fixPrmOrder(getPrmOrder(recv_table));
            field.fixTmpOrder(getTmpOrder(recv_table));
            field.setTurnPlayer(turnPlayer);
            
            if(isMyTurn(recv_table)){
                field.setMyTurn();
            }
            
            if(field.isFirstTurn()){ // 初手の特別な処理
                // 局面をサーバーからの情報通りに設定する
                for(int p = 0; p < N_PLAYERS; ++p){
                    int n = getNCards(recv_table, p);
                    field.setNOrgCards(p, n);
                    field.setNCards(p, n);
                    game_log.setNOrgCards(p, n);
                }
                field.setFirstTurnPlayer(turnPlayer);
                field.setPMOwner(turnPlayer); // 初期オーナーは初手プレーヤーとしておく
                field.resetFirstTurn();
                
                if(field.isMyImpChange()){ // 自分がカード交換に関与した場合、ここで初めて結果が分かる
                    
                    Cards newCards = TableToCards(recv_table, false);
                    
                    // 自分の手札、初手プレーヤーに関する情報を得る
                    field.setMyCards(newCards);
                    
                    // 自分が上位の際もカード交換が自分の想定通りに行われたか確認の必要がある
                    
                    Cards sentCards, recvCards;
                    Cards dealtCards = field.getMyDealtCards();
                    
                    if(field.isMyReqChange()){ // 交換要求されていた場合
                        // recvCardsは分からない
                        recvCards = CARDS_NULL;
                        sentCards = maskCards(dealtCards, newCards);
                        if(sentCards != field.getMySentCards()){
                            // 自分がやろうとした交換が出来ていないのでリジェクト報告
                            field.feedChangeRejection();
                            cerr << "main() : My Change was Rejected!" << endl;
                            
                            // 自分が意図した交換の事は忘れ、実際に行われた交換を設定する
                        }
                    }else{ // 要求されていない場合
                        recvCards = maskCards(newCards, dealtCards);
                        if((uint32_t)countCards(recvCards) != field.getMyChangeImpQty()){ // 枚数があっていない
                            // 突き返しがあったはず。サーバーがルール通りの献上(上位n枚)を行っていると信頼
                            sentCards = pickHigh(field.getMyDealtCards(), field.getMyChangeImpQty());
                            recvCards = maskCards(newCards, subtrCards(dealtCards, sentCards));
                            
                            assert((uint32_t)countCards(recvCards) == field.getMyChangeImpQty());
                            
                        }else{ // あってる
                            sentCards = maskCards(dealtCards, subtrCards(newCards, recvCards));
                        }
                    }
                    
                    field.setMyRecvCards(recvCards);
                    field.setMySentCards(sentCards);
                    
                    game_log.setRecvCards(my_player_num, recvCards);
                    game_log.setSentCards(my_player_num, sentCards);
                    
                    client.afterChange();
                    client.prepareForGame();
                    
                    field.setMyOrgCards(newCards);
                    game_log.setOrgCards(my_player_num, newCards);
                    
                }else{ // 自分がカード交換に関与していない
                    field.setMyOrgCards(field.getMyDealtCards());
                    game_log.setOrgCards(my_player_num, field.getMyDealtCards());
                }
            }
            
#ifdef BROADCAST
            field.broadcastBP();
#endif
            
            const bool myTurn = isMyTurn(recv_table);
            uint64_t myPlayTime;
            
            if(myTurn){ // 自分のターン
                
                // サーバーのカード情報と自分の情報とがずれている可能性を考え
                // 自分のカード情報をセットし直す
                field.setMyCards(TableToCards(recv_table, false));
                
                ClockMicS clms_mine;
                clms_mine.start();
                
                // 新
                Move myMove = client.play(); // 自分のプレー
                MoveToTable(myMove, field.getBoard(), send_table);
                
                myPlayTime = clms_mine.stop();
                
                accept_flag = sendCards(send_table);
                if(accept_flag == 9){ // accepted
                }else{ // rejected
                    // rejectの原因を考える
                    if(!myMove.isPASS()){ // UECdaではパスはrejectと同じフラグが返る
                        field.feedPlayRejection();
                        cerr << "main() : My Play was Rejected(" << accept_flag << ")! " << myMove << endl;
                        getchar();
                    }
                }
#ifdef BROADCAST
                field.broadcastMyPlay(myMove);
#endif
            }else{ // 自分のターンでない
                // 他人のプレー中の処理(UECdaでは暗黙の了解として、重い処理はしない)
                if(field.isMyWon()){
                    client.waitAfterWon();
                }else{
                    client.waitBeforeWon();
                }
                
                myPlayTime = 0; // 他人のプレー着手決定実時間はわからないので0とする
            }
            
            // 共通処理
            receiveCards(recv_table);
            tmpTime = clms.stop();
            
            // サーバーが受理した役(場に出ている役)
            
            //cerr << toString(recv_table) << endl;
            
            Move serverMove = TableToMove(recv_table);
            Cards serverUsedCards = TableToCards(recv_table, false);
            DERR << "org server move = " << serverMove << " " << OutCards(serverUsedCards) << endl;
            
            // すでに場に出ていた役が調べることで、パスを判定
            if(serverMove.cards() == BoardToMove(field.getBoard()).cards()){
                serverMove = MOVE_PASS;
                serverUsedCards = CARDS_NULL;
            }
            DERR << "server move = " << serverMove << " " << OutCards(serverUsedCards) << endl;
            field.procByServer(serverMove, serverUsedCards);
            
            //if(serverMove.isSeq() && serverMove.jokerRank() == 15){
            //    cerr << toString(recv_table) << endl;
            //}
            
            MinClientPlayLog playLog;
            playLog.set(serverMove, serverUsedCards, tmpTime, myPlayTime);
            game_log.push_play(playLog);
            
            if(myTurn){
                client.afterMyPlay(); // 自分のプレー後の処理
            }else{
                client.afterOthersPlay();
            }
            
#ifdef BROADCAST
            field.broadcastPlay(turnPlayer, serverMove);
#endif
            
            switch (beGameEnd()){
                case 0:
                    one_gameend_flag = 0; whole_gameend_flag = 0;
                    break;
                case 1:
                    one_gameend_flag = 1; whole_gameend_flag = 0;
                    break;
                default:
                    one_gameend_flag = 1; whole_gameend_flag = 1;
                    break;
            }
            if(one_gameend_flag){ goto ENDED_GameAME; }
            
        }
        
        
    ENDED_GameAME:
        // 試合終了後処理
        // 最後の一人のあがり処理はfieldが勝手にやる
        
        // スタッツ進行
        field.feedResult();
        field.feedMyStats();
        
        game_log.infoNewClass() = field.infoNewClass;
        for(int p = 0; p < N_PLAYERS; ++p){
            game_log.setOrgCards(p, field.getUsedCards(p));
        }
        if(field.getNAlivePlayers() == 0){ // 先日手でなく通常の終了
            int l1pn = field.getNewClassPlayer(DAIHINMIN);
            game_log.addOrgCards(l1pn, field.getRemCards());
            game_log.setTerminated();
        }
#ifdef LOGGING_FILE
        match_log.push_game(game_log);
#endif
        field.closeGame();
        client.closeGame(); // 相手の分析等はここで
        
        if(whole_gameend_flag){ break; }
    }
    
    // 全試合終了後処理
    
    // クライアントの全試合終了処理はデストラクタにて
    
    if(closeSocket() != 0){
        printf("failed to close socket\n");
        exit(1);
    } // ソケットを閉じて終了
    return 0;
}
