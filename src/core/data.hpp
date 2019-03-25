#pragma once

namespace UECda{
    
    struct ThreadTools{
        // 各スレッドの持ち物
        int threadIndex; // スレッド番号
        XorShift64 dice; // サイコロ
        MoveInfo buf[8192]; // 着手生成バッファ
        void init(int index) {
            memset(buf, 0, sizeof(buf));
            threadIndex = index;
        }
        void close() {}
    };
    
    struct SharedData{
        // クライアントの持ち物
        std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS> classDestination; //階級到達回数
        std::array<std::array<std::array<uint32_t, N_PLAYERS>, N_PLAYERS>, N_PLAYERS> classTransition; // 階級遷移回数
        
        ClientMatchLog<ClientGameLog<ClientPlayLog>> matchLog; // 主観的な対戦棋譜
        
        // クライアントの個人的スタッツ
        uint32_t playRejection, changeRejection; // リジェクト
        
        void feedPlayRejection()noexcept{
            playRejection += 1;
        }
        void feedChangeRejection()noexcept{
            changeRejection += 1;
        }
        void feedResult(int p, int cl, int ncl) {
            classDestination[p][ncl] += 1;
            classTransition[p][cl][ncl] += 1;
        }
        
        void initMatch() {
            // スタッツ初期化
            for (int p = 0; p < N_PLAYERS; ++p) {
                classDestination[p].fill(0);
                for (int cl = 0; cl < N_PLAYERS; ++cl) {
                    classTransition[p][cl].fill(0);
                }
            }
            playRejection = changeRejection = 0;
        }
        void initGame() {
            matchLog.initGame();
        }
        template<class gameLog_t>
        void closeGame(const gameLog_t& gLog) {
            // 試合順位の記録
            for (int p = 0; p < N_PLAYERS; ++p) {
                feedResult(p, gLog.getPlayerClass(p), gLog.getPlayerNewClass(p));
            }
        }
        void closeMatch() {
            // 共通スタッツ表示
#ifdef MY_MANE
            cerr << endl << "---" << MY_NAME << "'s seiseki happyou!!!---" << endl;
            cerr << "R*NR";
            for (int cl = 0; cl < N_PLAYERS; ++cl) {
                cerr << " " << cl << " ";
            }
            cerr << " total" << endl;
            for (int cl = 0; cl < N_PLAYERS; ++cl) {
                cerr << "  " << cl << "  ";
                for (int ncl = 0; ncl < N_PLAYERS; ++ncl) {
                    cerr << classTransition[getMyPlayerNum()][cl][ncl] << "  ";
                }
                cerr << classDestination[getMyPlayerNum()][cl] << endl;
            }
            cerr << "Other Players" << endl;
            for (int p = 0; p < N_PLAYERS; ++p) {
                if (p != (int)getMyPlayerNum()) {
                    cerr << "Player " << p << " :" << endl;
                    cerr << "R*NR";
                    for (int cl = 0; cl < N_PLAYERS; ++cl) {
                        cerr << " " << cl << " ";
                    }
                    cerr << " total" << endl;
                    for (int cl = 0; cl < N_PLAYERS; ++cl) {
                        cerr << "  " << cl << "  ";
                        for (int ncl = 0; ncl < N_PLAYERS; ++ncl) {
                            cerr << classTransition[p][cl][ncl] << "  ";
                        }
                        cerr << classDestination[p][cl] << endl;
                    }
                }
            }
#else
            cerr << "Players Stats" << endl;
            for (int p = 0; p < N_PLAYERS; ++p) {
                cerr << "Player " << p << " :" << endl;
                cerr << "R*NR 0  1  2  3  4  total" << endl;
                for (int cl = 0; cl < N_PLAYERS; ++cl) {
                    cerr << "  " << cl << "  ";
                    for (int ncl = 0; ncl < N_PLAYERS; ++ncl) {
                        cerr << classTransition[p][cl][ncl] << "  ";
                    }
                    cerr << classDestination[p][cl] << endl;
                }
            }
#endif
        }
    };
}
