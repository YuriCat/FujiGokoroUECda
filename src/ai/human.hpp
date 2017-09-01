/*
 human.hpp
 Katsuki Ohto
 */

// 人間用

#ifndef UECDA_HUMAN_HPP_
#define UECDA_HUMAN_HPP_

// 交換着手生成
#include "../generator/changeGenerator.hpp"
#include "../generator/moveGenerator.hpp"

// クライアント空間メイン
#include "../structure/field/clientField.hpp"

#ifdef LOGGING
// 試合ログ
namespace UECda{
    namespace Human{
        // 自分のプレー記録
        bool my_play_time_lock = false;
        uint64_t my_play_time_sum = 0ULL;
        uint32_t my_plays = 0U;
        
#ifdef LOGGING_FILE
        MinMatchLog<MinClientGameLog<MinClientPlayLog<N_PLAYERS>>> match_log;
#endif
        MinClientGameLog<MinClientPlayLog<N_PLAYERS>> game_log;
    }
}
#endif

namespace UECda{
    namespace Human{
        class Client{
        private:
            XorShift64 dice;
        public:
            ClientField field;
            
            void setRandomSeed(uint64_t s)noexcept{
                // 乱数系列を初期化
                dice.srand(s);
            }
            
            void initMatch(){
                dice.srand((uint64_t)time(NULL) + 123ULL);
            }
            void initGame(){}
            
            Cards change(uint32_t change_qty){ // 交換関数
                // 合法交換候補生成
                const Cards myCards = field.getMyDealtCards();
                Cards cand[N_MAX_CHANGES]; // 最大候補数
                const int NCands = genChange(cand, myCards, change_qty);
                
                for(int i = 0; i < NCands; ++i){
                    cerr << i << " : " << OutCards(cand[i]) << endl;
                }
                int chosen = -1;
                while(1){
                    std::cin >> chosen;
                    if(0 <= chosen && chosen < NCands){ break; }
                }
                return cand[chosen];
            }
            
            void afterChange(){}
            void waitChange(){}
            void prepareForGame(){}
            
            Move play(){ // プレー関数
                MoveInfo buf[512];
                const int NMoves = genMove(buf, field.getMyCards(), field.getBoard());
                
                for(int i = 0; i < NMoves; ++i){
                    cerr << i << " : " << buf[i] << endl;
                }
                
                if(NMoves == 1 && buf[0].isPASS()){
                    cerr << "forced pass." << endl;
                    return MOVE_PASS;
                }
                
                int chosen = -1;
                while(1){
                    std::cin >> chosen;
                    if(0 <= chosen && chosen < NMoves){ break; }
                }
                return buf[chosen].mv();
            }
            
            void afterMyPlay(){}
            void afterOthersPlay(){}
            void waitBeforeWon(){}
            void waitAfterWon(){}
            void tellOpponentsCards(){}
            void closeGame(){}
            void closeMatch(){}
            ~Client(){
                closeMatch();
            }
        };
    }
}

#endif // UECDA_HUMAN_HPP_
