#pragma once

namespace UECda {
    
    constexpr int MAX_N_WORLDS = 128;
    template <class wrd_t, int SIZE = (MAX_N_WORLDS / N_THREADS)>
    struct Galaxy {
        
        using world_t = wrd_t;
        
        // 幾多の世界が詰まっている入れ物のような何か
        int usedLimit;
        int actives;
        int nullLimit;
        world_t world[SIZE];
        
        Galaxy() { clear(); }
        ~Galaxy() { close(); }
        
        int size() const { return SIZE; }
        
        void clear() {
            for (int w = 0; w < SIZE; w++) world[w].clear();
            usedLimit = SIZE;
            actives = 0;
            nullLimit = 0;
        }
        
        void close() { clear();}
        void proceed(const int p, const Move mv, const Cards c, const double compRatio) {}
        void checkRationality() {   }
        
        world_t* access(const int w) {
            assert(0 <= w && w < SIZE);
            return &world[w];
        }
        
        world_t* searchSpace(const int line, const int limit) {
            // 空になっている分割スペースを返す
            for (int w = line; w < line + limit; ++w) {
                if (!world[w].isActive()) {//ここに登録
                    DERR << "WORLD " << w << "IS INACTIVE!" << endl;
                    return &world[w];
                }
            }
            DERR << "NO ACTIVE WORLD..." << endl;
            return nullptr;
        }
        
        template <class dice_t>
        world_t* pickRand(const int line, const int limit, dice_t *const dice) {
            // activeである世界のどれかにランダムアクセスする
            int w = line + (dice->rand() % limit);
            
            if (0 <= w && w < SIZE && world[w].isActive()) {
                return &world[w];
            } else {
                assert(0);
                return nullptr;
            }
        }
        
        int regist(world_t *const wld) {
            if (!(0 <= (wld - world) && (wld - world) < SIZE && (!wld->isActive()))) {
                assert(0);
                return -1;
            }
            wld->activate();
            actives++;
            return 0;
        }
        
        void addActives() { actives++; }
    };
}
