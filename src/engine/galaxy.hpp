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
        world_t* pickRand(const int line, const int limit, dice_t& dice) {
            // activeである世界のどれかにランダムアクセスする
            int w = line + (dice() % limit);
            
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
    
    template <class glxy_t, int N = N_THREADS>
    struct GalaxyAnalyzer {
        
       using galaxy_t = glxy_t;
       using world_t = typename glxy_t::world_t;
        
        // 世界推定力を検討
        galaxy_t *pgal[N];
        
        // 世界推定力パラメータ
        double population, survivals;
        double mean, score, variance;
        
        GalaxyAnalyzer(): population(0.0), survivals(0.0),
        mean(0.0), score(0.0), variance(0.0) {}
        
        ~GalaxyAnalyzer() { close(); }
        void close() {
            double lsuv, lscore, lvariance, lmean, stddev, zscore;
            
            if (population > 0) {
                
                lsuv = survivals / population;
                lscore = score / population;
                lvariance = variance / population;
                lmean = mean / population;
                stddev = sqrt(lvariance);
                
                if (stddev > 0) {
                    zscore = (lscore - lmean) / stddev;
                } else {
                    zscore = 0.0;
                }
                
            } else {
                lsuv = 0.0;
                lscore = 0.0;
                lvariance = 0.0;
                lmean = 0.0;
                stddev = 0.0;
                zscore = 0,0;
            }
            CERR << endl;
            CERR << "Galaxies : whole survival ratio - " << lsuv << std::endl;
            CERR << "Galaxies : ideal score - " << lmean << " (stddev: " << stddev << " ) real score - " << lscore << std::endl;
            CERR << "Galaxies : z score - " << zscore << " standard score - " << (50.0 + 10.0 * zscore) << std::endl;
        }
        
        void set(int g, galaxy_t *gal) {
            assert(0 <= g && g < N);
            pgal[g] = gal;
        }
        
        void proceed(int p, const Move& mv, const double compRatio) {
            assert(compRatio > 0);
            Cards c = mv.cards();
            if (anyCards(c)) {
                for (int g = 0; g < N; g++) {
                    if (pgal[g]->actives > 0) {
                        double sp = 0.0;
                        double pops = (double)pgal[g]->actives;
                        
                        int dieds = 0;
                        for (int w = 0; w < pgal[g]->size(); ++w) {
                            world_t& tmpW = *pgal[g]->access(w);
                            if (tmpW.isActive()) {
                                //完全矛盾による世界の自然死を判定
                                if (!holdsCards(tmpW.getCards(p), c)) {//矛盾
                                    DERR << "World " << w << " died..." << endl;
                                    tmpW.clear();//そんな世界は存在しなかった
                                    dieds++;
                                } else {
                                    sp += compRatio;
                                    score++;
                                }
                            }
                        }
                        pgal[g]->actives -= dieds;
                        population += pops;
                        survivals += sp;
                        mean += pops * 1 / compRatio;
                        variance += pops * 1 / compRatio * (1 - 1 / compRatio);
                        CERR << "Galaxy : survival point " << sp / pops << std::endl;
                    }
                }
            }
        }
    };
}
