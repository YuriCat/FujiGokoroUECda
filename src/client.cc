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
    auto& matchLog = client.matchLog;
    auto& shared = client.shared;
    
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
    int myPlayerNum = entryToGame(); // ゲームに参加
    
    matchLog.setMyPlayerNum(myPlayerNum);
    client.initMatch(); // ここで呼ばないとプレーヤー番号が反映されない
    
    if(seedSet){ // シード指定の場合
        client.setRandomSeed(seed);
    }
    
#ifdef BROADCAST
    broadcastBMatch();
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
        client.initGame();
        
        // 棋譜準備
        matchLog.initGame();
        auto& gameLog = matchLog.latestGame();
        ClientPlayLog& playLog = gameLog.current; // 盤面の更新等のため
        playLog.initGame();
        
        startGame(recv_table); // 自分のカード、初期情報受け取り

        // 席順設定
        for(int s = 0; s < N_PLAYERS; ++s)
            gameLog.setPlayerSeat(getSeatPlayer(recv_table, s), s);
        
        // 自分のカード設定
        Cards c = TableToCards(recv_table, false);
        gameLog.setDealtCards(myPlayerNum, c);
        
        CERR << toString(recv_table) << endl;
        
        // 階級設定
        // 階級初期化が何試合かは公式には未定なため
        // テーブル情報から判定する必要がある
        if(isInitGame(recv_table)){
            // init gameは全員平民としておく
            CERR << "main() : probed init game from table" << endl;
            gameLog.setInitGame();
            for(int p = 0; p < N_PLAYERS; ++p)
                gameLog.setPlayerClass(p, MIDDLE);
        }else{
            CERR << "main() : probed change game from table" << endl;
            gameLog.resetInitGame();
            for(int p = 0; p < N_PLAYERS; ++p)
                gameLog.setPlayerClass(p, getPlayerClass(recv_table, p));
        }
        
        // カード枚数設定
        // javaサーバーに引っかからないように、最初にjavaなのかCなのか判定
        // init gameのときは関係ないが、falseになればそれで良い
        bool isJava;
        
        if(!gameLog.isInitGame()
           && getNCards(recv_table, gameLog.getClassPlayer(DAIFUGO)) == 13U){
            isJava = true;
            CERR << "main() : This is java server." << endl;
        }else{
            isJava = false;
        }
        
        for(int p = 0; p < N_PLAYERS; ++p){
            int NCards = getNCards(recv_table, p);
            if(isJava){
                // javaサーバーでは献上分が加算されているので高位者は引く必要あり
                int cl = getPlayerClass(recv_table, p);
                if(cl <= FUGO)NCards -= N_CHANGE_CARDS(cl);
            }
            gameLog.setNDealtCards(p, NCards);
        }
#ifdef BROADCAST
        field.broadcastBGame();
#endif
        
        // カード交換フェーズ
        int myClass = gameLog.getPlayerClass(myPlayerNum);
        int changeQty = N_CHANGE_CARDS(myClass);
        
        if(!gameLog.isInitGame()){ // カード交換あり
            
            CERR << "main() : change phase" << endl;
            
            // 順位との関係から自分の交換への関与をまとめる
            if(myClass != HEIMIN){ // 自分がカード交換に関与している
                CERR << "main() : my imp for change" << endl;
                if(myClass < HEIMIN){ // 自分があげるカードを選択する必要あり
                    CERR << "main() : my req for change" << endl;
                    Cards changeCards = client.change(changeQty); // 自分のカード交換選択
                    gameLog.setSentCards(changeCards); // 実際にその交換が行われるかわからないがとりあえず設定
                    CardsToTable(changeCards, send_table);
                    
                    sendChangingCards(send_table);
#ifdef BROADCAST
                    field.broadcastMyChange(cCards);
#endif
                }else{
                    // 自分はもらう側なのでここでは何もしない
                }
            }else{ // 自分はカード交換に関与しない
                client.prepareForGame(); // 試合前準備
            }
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
        
        bool firstTurn = true;
        while(1){ // ターンループ
            clms.start(); // ここで時間計測開始
            // javaサーバーでは、ここで時間計測を初めても自分以外ほぼ0になってしまう。解決策は不明...
            
            // 自分のカードとフィールド情報を受け取り
            // この通信順がおかしい?のか、この後に時間計測を始めても相手の計算時間がわからない(ほぼ0になる)<-c server
            receiveCards(recv_table);
            
            // 自分の情報をサーバーの情報に合わせておく
            int turnPlayer = getTurnPlayer(recv_table);
            
            if(isNF(recv_table) && (!playLog.bd.isNF() || playLog.ps.getNAwake() == 0)){
                // 空場パスに対応するため、既に空場と認識していない状況、
                // またはawakeなプレーヤーが誰もいない状況でのみ流す
                playLog.flush(gameLog.infoSeat); // 席順が必要
            }
            
            if(suitsLocked(recv_table))
                playLog.bd.lockSuits();
            
            playLog.bd.fixPrmOrder(getPrmOrder(recv_table));
            playLog.bd.fixTmpOrder(getTmpOrder(recv_table));
            playLog.setTurnPlayer(turnPlayer);
            
            if(firstTurn){ // 初手の特別な処理
                // 局面をサーバーからの情報通りに設定する
                for(int p = 0; p < N_PLAYERS; ++p)
                    gameLog.setNOrgCards(p, getNCards(recv_table, p));
                    
                playLog.setFirstTurnPlayer(turnPlayer);
                playLog.setPMOwner(turnPlayer); // 初期オーナーは初手プレーヤーとしておく
                firstTurn = false;
                
                if(myClass != HEIMIN){ // 自分がカード交換に関与した場合、ここで初めて結果が分かる
                    
                    Cards newCards = TableToCards(recv_table, false);
                    
                    // 自分が上位の際もカード交換が自分の想定通りに行われたか確認の必要がある
                    
                    Cards sentCards, recvCards;
                    Cards dealtCards = gameLog.getDealtCards(myPlayerNum);
                    
                    if(myClass < HEIMIN){ // 交換要求されていた場合
                        // recvCardsは分からない
                        recvCards = CARDS_NULL;
                        sentCards = maskCards(dealtCards, newCards);
                        if(sentCards != gameLog.getSentCards()){
                            // 自分がやろうとした交換が出来ていないのでリジェクト報告
                            shared.feedChangeRejection();
                            cerr << "main() : My Change was Rejected!" << endl;
                            
                            // 自分が意図した交換の事は忘れ、実際に行われた交換を設定する
                        }
                    }else{ // 交換要求されていない場合
                        recvCards = maskCards(newCards, dealtCards);
                        sentCards = maskCards(dealtCards, newCards);
                        
                        if(countCards(recvCards) != changeQty){ // 枚数があっていない
                            // 突き返しがあったはず。サーバーがルール通りの献上(上位n枚)を行っていると信頼
                            // java版サーバーだと食い違うことがあるがそれで落ちはしない
                            int NKicked = changeQty - countCards(recvCards);
                            Cards kickedCards = pickHigh(commonCards(dealtCards, newCards), NKicked);
                            sentCards = addCards(kickedCards, sentCards);
                            recvCards = addCards(kickedCards, recvCards);
                            
                            assert(countCards(recvCards) == changeQty && countCards(sentCards) == changeQty);
                        }
                    }
                    gameLog.setRecvCards(recvCards);
                    gameLog.setSentCards(sentCards);
                    
                    client.afterChange();
                    client.prepareForGame();
                    
                    gameLog.setOrgCards(myPlayerNum, newCards);
                    
                }else{ // 自分がカード交換に関与していない
                    gameLog.setOrgCards(myPlayerNum, gameLog.getDealtCards(myPlayerNum));
                }
            }
            
            for(int p = 0; p < N_PLAYERS; ++p)
                playLog.setNCards(p, getNCards(recv_table, p));
            
#ifdef BROADCAST
            field.broadcastBP();
#endif
            
            uint64_t myPlayTime;
            bool myTurn = isMyTurn(recv_table);
            
            if(myTurn){ // 自分のターン
                
                // 現時点での所持手札を再度記録
                Cards myCards = TableToCards(recv_table, false);
                
                ClockMicS clms_mine;
                clms_mine.start();
                
                // 新
                Move myMove = client.play(); // 自分のプレー
                MoveToTable(myMove, playLog.bd, send_table);
                
                myPlayTime = clms_mine.stop();
                
                accept_flag = sendCards(send_table);
                if(accept_flag == 9){ // accepted
                }else{ // rejected
                    // rejectの原因を考える
                    if(!myMove.isPASS()){ // UECdaではパスはrejectと同じフラグが返る
                        shared.feedPlayRejection();
                        cerr << "main() : My Play was Rejected(" << accept_flag << ")! " << myMove << endl;
                        getchar();
                    }
                }
#ifdef BROADCAST
                field.broadcastMyPlay(myMove);
#endif
            }else{ // 自分のターンでない
                // 他人のプレー中の処理(UECdaでは暗黙の了解として、重い処理はしない)
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
            if(serverMove.cards() == BoardToMove(playLog.bd).cards()){
                serverMove = MOVE_PASS;
                serverUsedCards = CARDS_NULL;
            }
            DERR << "server move = " << serverMove << " " << OutCards(serverUsedCards) << endl;
            
            //if(serverMove.isSeq() && serverMove.jokerRank() == 15){
            //    cerr << toString(recv_table) << endl;
            //}
            playLog.set(serverMove, serverUsedCards, tmpTime, myPlayTime);
            gameLog.push_play(playLog);
            if(playLog.procByServer(serverMove, serverUsedCards)){
                // あがり処理
                gameLog.setPlayerNewClass(turnPlayer, playLog.ps.getBestClass() - 1);
            }
            
#ifdef BROADCAST
            field.broadcastPlay(turnPlayer, serverMove);
#endif
            
            switch(beGameEnd()){
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
            if(one_gameend_flag)
                break;
        }
        
        // 試合終了後処理
        int lastPlayer = playLog.ps.searchL1Player();
        gameLog.setPlayerNewClass(lastPlayer, DAIHINMIN);
        Field field;
        setFieldFromClientLog(gameLog, myPlayerNum, &field);
        for(int p = 0; p < N_PLAYERS; ++p){
            gameLog.setOrgCards(p, field.getUsedCards(p));
        }
        if(field.getNAlivePlayers() == 0){ // 先日手でなく通常の終了
            gameLog.addOrgCards(lastPlayer, field.getRemCards());
            gameLog.setTerminated();
        }
        matchLog.closeGame();
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
