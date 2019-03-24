#pragma once

#include "action.hpp"

// 思考エンジンのベース

namespace UECda {
    class BaseEngine {
    public:
        virtual void setRandomSeed(uint64_t) {}
        virtual void initMatch() {}
        virtual void initGame() {}
        virtual Cards change(int) {}
        virtual Move play() {}
        virtual void closeGame() {}
        virtual void closeMatch() {}
        ~BaseEngine() { closeMatch(); }
    };

    class RandomEngine {
        // ランダムなクライアント
    public:
        virtual void setRandomSeed(uint64_t s) {
            // 乱数系列を初期化
            dice.srand(s);
        }
        virtual void initMatch() {
            dice.srand((uint64_t)time(NULL) + 123ULL);
        }
        virtual Cards change(int numChange) {
            // 合法交換候補生成
            const Cards myCards = field.getMyDealtCards();
            Cards candidate[N_MAX_CHANGES]; // 最大候補数
            const int numCandidates = genChange(cand, myCards, numChange);
            
            for (int i = 0; i < numCandidates; i++) {
                CERR << i << " : " << OutCards(candidate[i]) << endl;
            }
            if (numCandidates == 1) return candidate[0];
            else return candidate[dice.rand() % numCandidates];
        }
        
        virtual Move play() {
            MoveInfo buf[1024];
            const int NMoves = genMove(buf, field.getMyCards(), field.getBoard());
            
            for(int i = 0; i < NMoves; ++i) {
                CERR << i << " : " << buf[i] << endl;
            }
            MoveInfo mv;
            if(NMoves == 1 && buf[0].isPASS()) {
                mv = buf[0];
            }else{
                mv = buf[dice.rand() % NMoves];
            }
            CERR << mv << " by " << OutCards(field.getMyCards()) << endl;
            return mv.mv();
        }
        ~RandomEngine() { closeMatch(); }
    };
    private:
        XorShift64 dice;
    };

    class HumanClient {
        // 人間用クライアント
    public:
        virtual Cards change(uint32_t change_qty) { // 交換関数
            // 合法交換候補生成
            const Cards myCards = field.getMyDealtCards();
            Cards cand[N_MAX_CHANGES]; // 最大候補数
            const int NCands = genChange(cand, myCards, change_qty);
            
            for(int i = 0; i < NCands; ++i) {
                cerr << i << " : " << OutCards(cand[i]) << endl;
            }
            int chosen = -1;
            while(1) {
                std::cin >> chosen;
                if(0 <= chosen && chosen < NCands) { break; }
            }
            return cand[chosen];
        }

        virtual Move play() { // プレー関数
            MoveInfo buf[512];
            const int NMoves = genMove(buf, field.getMyCards(), field.getBoard());
            
            for(int i = 0; i < NMoves; ++i) {
                cerr << i << " : " << buf[i] << endl;
            }
            
            if(NMoves == 1 && buf[0].isPASS()) {
                cerr << "forced pass." << endl;
                return MOVE_PASS;
            }
            
            int chosen = -1;
            while(1) {
                std::cin >> chosen;
                if(0 <= chosen && chosen < NMoves) { break; }
            }
            return buf[chosen].mv();
        }
    };
}
