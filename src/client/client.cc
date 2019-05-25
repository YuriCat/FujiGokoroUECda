/////////////////////////////////

// UECコンピュータ大貧民大会(UECda)用クライアントメイン

/////////////////////////////////

#include "connection.h"
#include "../UECda.h"
#include "../engine/engine.hpp"

using namespace std;

WisteriaEngine engine;

int main(int argc, char* argv[]) { // for UECda
    
    // main関数はクライアントの思考の進行役を務める
    // サーバーの情報を、クライアントが用いる共通型に変換して最低限の主観的情報を更新する
    // それ以上高次な演算はクライアントが行う

    // 時間計測(microsec単位)
    ClockMicS clms;
    uint64_t tmpTime;
    
    // 基本的な初期化
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // 全試合前の初期化
    auto& record = engine.record;
    auto& shared = engine.shared;
    
    bool seedSet = false;
    int seed = -1;
    bool monitor = false;
    
    for (int c = 1; c < argc; c++) {
        if (!strcmp(argv[c], "-i")) { // input directory
            DIRECTORY_PARAMS_IN = string(argv[c + 1]);
        } else if (!strcmp(argv[c], "-o")) { // output directory
            DIRECTORY_PARAMS_OUT = string(argv[c + 1]);
        } else if (!strcmp(argv[c], "-s")) { // random seed
            seedSet = true;
            seed = atoi(argv[c + 1]);
        } else if (!strcmp(argv[c], "-m")) { // output
            monitor = true;
        } else if (!strcmp(argv[c], "-pol")) { // policy mode
            Settings::policyMode = true;
        } else if (!strcmp(argv[c], "-th")) { // num of threads
            int NThreads = atoi(argv[c + 1]);
            Settings::numPlayThreads = NThreads;
            Settings::numChangeThreads = max(1, NThreads / 2);
        }
    }
    
    checkArg(argc, argv); // 引数のチェック 引数に従ってサーバアドレス、接続ポート、クライアント名を変更
    string name = Settings::policyMode ? MY_POL_NAME : MY_NAME;
    CERR << name << " ver. " << MY_VERSION << endl;
    int myPlayerNum = entryToGame(name.c_str()); // ゲームに参加
    
    record.myPlayerNum = myPlayerNum;
    engine.initMatch(); // ここで呼ばないとプレーヤー番号が反映されない 
    if (seedSet) engine.setRandomSeed(seed); // シード指定
    engine.monitor = monitor;
    
    // 通信変数
    int whole_gameend_flag;
    int recv_table[8][15];
    int send_table[8][15];
    
    clearAll(recv_table);
    clearAll(send_table);
    
    // 試合フェーズ
    while (1) {
        // 各試合前の初期化
        engine.initGame();
        record.initGame();
        auto& game = record.latestGame();
        
        startGame(recv_table); // 自分のカード、初期情報受け取り
        CERR << toString(recv_table) << endl;

        // 席順設定
        for (int s = 0; s < N_PLAYERS; s++) game.setSeatOf(seatPlayer(recv_table, s), s);
        // 階級設定 階級初期化が何試合かは通知されないため試合ごとに判定する
        bool initGame = isInitGame(recv_table);
        CERR << "main() : probed " << (initGame ? "init" : "change") << " game from table" << endl;
        if (initGame) {
            game.setInitGame();
            for (int p = 0; p < N_PLAYERS; p++) game.setClassOf(p, MIDDLE);
        } else {
            game.resetInitGame();
            for (int p = 0; p < N_PLAYERS; p++) game.setClassOf(p, classOf(recv_table, p));
        }
        
        // 自分のカード設定
        Cards c = TableToCards(recv_table, false);
        game.dealtCards[myPlayerNum] = c;

        // カード枚数設定
        // javaサーバーに引っかからないように、最初にjavaなのかCなのか判定
        // init gameのときはfalseになれば問題ない
        bool isJava = false;
        if (!game.isInitGame() && getNCards(recv_table, game.classPlayer(DAIFUGO)) == 13) {
            isJava = true;
            CERR << "main() : This is java server." << endl;
        }
        
        for (int p = 0; p < N_PLAYERS; p++) {
            int numCards = getNCards(recv_table, p);
            if (isJava) {
                // javaサーバーでは献上分が加算されているので高位者は引く必要あり
                int cl = classOf(recv_table, p);
                if (cl < MIDDLE) numCards -= N_CHANGE_CARDS(cl);
            }
            game.numDealtCards[p] = numCards;
        }
        
        // カード交換フェーズ
        Cards mySentCards = CARDS_NULL;
        int changeQty = getChangeQty(recv_table);
        if (!game.isInitGame()) {
            // カード交換があるとき自動で行われる献上を空の記録として保存
            for (int cl = DAIFUGO; cl < MIDDLE; cl++) {
                ChangeRecord change;
                int from = game.classPlayer(getChangePartnerClass(cl));
                int to = game.classPlayer(cl);
                change.set(from, to, N_CHANGE_CARDS(cl), CARDS_NULL, true);
            }
        }
        if (changeQty > 0) { // 交換要求があるとき
            Cards cc = engine.change(changeQty); // 自分のカード交換選択
            CardsToTable(cc, send_table);
            mySentCards = cc;
            sendChangingCards(send_table);
        }

        bool firstPlay = true;
        while (1) { // ターンループ
            clms.start(); // ここで時間計測開始
            // javaサーバーでは、ここで時間計測を初めても自分以外ほぼ0になってしまう。解決策は不明...
            
            // 自分のカードとフィールド情報を受け取り
            // Cサーバーではこの後に時間計測を始めても相手の計算時間がわからない(ほぼ0になる)
            receiveCards(recv_table);
            
            // 自分の情報をサーバーの情報に合わせておく
            int turn = turnOf(recv_table);
            if (firstPlay) {
                game.firstTurn = turn;
                firstPlay = false;
                Cards newCards = TableToCards(recv_table, false);
                Cards sentCards = CARDS_NULL, recvCards = CARDS_NULL;
                
                if (!game.isInitGame()) {
                    int myClass = game.classOf(myPlayerNum);
                    if (myClass != HEIMIN) {
                        // 自分がカード交換に関与した場合、ここで初めて結果が分かる
                        // 自分が上位の際もカード交換が自分の想定通りに行われたか確認の必要がある
                        
                        Cards dealtCards = game.dealtCards[myPlayerNum];

                        if (myClass < HEIMIN) {
                            // 自分が交換上位の場合
                            sentCards = dealtCards - newCards;
                            recvCards = CARDS_NULL;
                            if (sentCards != mySentCards) {
                                // 自分がやろうとした交換が出来ていないのでリジェクト報告
                                // 自分が意図した交換の事は忘れ、実際に行われた交換を設定する
                                shared.feedChangeRejection();
                                cerr << "main() : My Change was Rejected!" << endl;
                            }
                        } else {
                            // 自分が交換下位の場合
                            int presentQty = N_CHANGE_CARDS(myClass);
                            recvCards = newCards.masked(dealtCards);
                            sentCards = dealtCards.masked(newCards);
                            if (recvCards.count() != presentQty) { // 枚数があっていない
                                // 突き返しがあったはず。サーバーがルール通りの献上(上位n枚)を行っていると信頼
                                // java版サーバーだと食い違うことがあるがそれで落ちはしない
                                int numKicked = presentQty - countCards(recvCards);
                                Cards kickedCards = (dealtCards.common(newCards)).high(numKicked);
                                sentCards = kickedCards + sentCards;
                                recvCards = kickedCards + recvCards;
                            }
                        }
                    }
                    // 献上についての情報を更新する
                    for (auto& change : game.changes) {
                        if (change.from == myPlayerNum) change.cards = sentCards;
                    }
                    // 上位から下位への交換の情報を追加する
                    for (int cl = DAIFUGO; cl < MIDDLE; cl++) {
                        ChangeRecord change;
                        int from = game.classPlayer(getChangePartnerClass(cl));
                        int to = game.classPlayer(cl);
                        Cards c = CARDS_NULL;
                        if (from == myPlayerNum) c = sentCards;
                        if (to == myPlayerNum) c = recvCards;
                        change.set(from, to, N_CHANGE_CARDS(cl), c, false);
                    }
                    engine.afterChange();
                }
                game.orgCards[myPlayerNum] = newCards;
                engine.prepareForGame();
            }
            
            bool myTurn = isMyTurn(recv_table);
            Field field;
            field.fromRecord(game, myPlayerNum);
            
            if (myTurn) { // 自分のターン
                Move myMove = engine.play(); // 自分のプレー
                MoveToTable(myMove, field.board, send_table);

                int accept_flag = sendCards(send_table);
                if (accept_flag == 9) { // accepted
                } else if (!myMove.isPASS()) { // UECdaではパスはrejectと同じフラグが返る
                    shared.feedPlayRejection();
                    cerr << "main() : My Play was Rejected(" << accept_flag << ")! " << myMove << endl;
                }
            }
            
            // 共通処理
            receiveCards(recv_table);
            unsigned playTime = clms.stop();
            
            // サーバーが受理した役(場に出ている役)
            Move serverMove = TableToMove(recv_table);
            // すでに場に出ていた役が調べることで、パスを判定
            if (serverMove.cards() == field.board.move().cards()) serverMove = MOVE_PASS;
            PlayRecord play;
            play.set(serverMove, playTime);
            game.push_play(play);

            int one_gameend_flag;
            switch (beGameEnd()) {
                case 0: one_gameend_flag = 0; whole_gameend_flag = 0; break;
                case 1: one_gameend_flag = 1; whole_gameend_flag = 0; break;
                default: one_gameend_flag = 1; whole_gameend_flag = 1; break;
            }
            if (one_gameend_flag) break;
        }
        
        // 試合終了後処理
        Field field;
        field.fromRecord(game, myPlayerNum);
        int lastPlayer = field.ps.searchL1Player();
        for (int p = 0; p < N_PLAYERS; p++) {
            game.setNewClassOf(p, field.newClassOf(p));
            game.orgCards[p] = field.usedCards[p];
        }
        if (field.getNAlivePlayers() == 0) { // 先日手でなく通常の終了
            game.orgCards[lastPlayer] += field.remCards;
            game.setTerminated();
        }
        record.closeGame();
        engine.closeGame(); // 相手の分析等はここで
        
        if (whole_gameend_flag) break;
    }
    
    // 全試合終了後処理
    // エンジンの全試合終了処理はデストラクタにて
    if (closeSocket() != 0) {
        printf("failed to close socket\n");
        exit(1);
    }
    return 0;
}
