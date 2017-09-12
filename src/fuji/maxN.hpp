/*
 maxN.hpp
 Katsuki Ohto
 */

// 不完全情報での多人数MaxN探索

#ifndef UECDA_SEARCH_MAXN_HPP_
#define UECDA_SEARCH_MAXN_HPP_

namespace UECda{
    namespace Fuji{
        
        // 全ての手札組み合わせに対して処理を行う
        template<class callback_t>
        void iterateAllHandPatternsSub(int p, Cards dst[],
                                       const Cards c, const int nsum, int n[],
                                       const callback_t& callback){
            if(p == N_PLAYERS){
                for(int p = 0; p < N_PLAYERS; ++p){
                    cerr << OutCards(dst[p]) << BitSet64(dst[p]) << endl;
                }
                callback(dst);
            }else{
                if(n[p] > 0){
                    uint64_t x = (1ULL << n[p]) - 1;
                    const uint64_t fin = (1ULL << nsum);
                    const int nextNsum = nsum - n[p];
                    n[p] = 0;
                    while(!(x & fin)){
                        const Cards dealt = pdep(x, c);
                        dst[p] = dealt;
                        iterateAllHandPatternsSub(p + 1, dst, subtrCards(c, dealt),
                                                  nextNsum, n, callback);
                        
                    }
                    n[p] = nsum - nextNsum;
                }else{
                    iterateAllHandPatternsSub(p + 1, dst, c, nsum, n, callback);
                }
            }
        }
        template<class callback_t>
        void iterateAllHandPatterns(const Cards c, const int nsum, int n[],
                                    const callback_t& callback){
            Cards dst[N_PLAYERS];
            for(int p = 0; p < N_PLAYERS; ++p){
                dst[p] = CARDS_NULL;
            }
            iterateAllHandPatternsSub(0, dst, c, nsum, n, callback);
        }
        
        struct MaxNResult : public BitArray64<8, 8>{
            using base_t = BitArray64<8, 8>;
            ComparableBitSet64 operator [](int p)const noexcept{
                return ComparableBitSet64(base_t::operator [](p));
            }
        };
        
        void searchMaxN(MaxNResult *const presult,
                        Field& field,
                        MoveInfo *const pbuffer){
            const int turnPlayer = field.getTurnPlayer();
            const int moves = genLegal(pbuffer, field.hand[turnPlayer], field.getBoard());
            cerr << field.toDebugString();
            cerr << "moves = " << moves << endl;
            ASSERT(moves > 0,);
            if(moves <= 1){
                int next = field.procSlowest(pbuffer[0]);
                if(next < 0){ // 試合終了
                    for(int p = 0; p < N_PLAYERS; ++p){
                        presult->clear();
                        presult->set_flag(p, field.getPlayerNewClass(p));
                    }
                }else{ // 試合継続
                    searchMaxN(presult, field, pbuffer + moves);
                }
            }else{
                MaxNResult tresult[N_MAX_MOVES];
                for(int i = 0; i < moves; ++i){
                    tresult[i].clear();
                }
                int n[N_PLAYERS];
                for(int p = 0; p < N_PLAYERS; ++p){
                    n[p] = field.getNCards(p);
                }
                n[turnPlayer] = 0;
                const Cards rem = subtrCards(field.getRemCards(), field.getCards(turnPlayer));
                const int numRem = field.getNRemCards() - field.getNCards(turnPlayer);
                iterateAllHandPatterns(rem, numRem, n, [&](const Cards *const c)->void{
                    // 手札設定
                    for(int i = 0; i < moves; ++i){
                        if(field.getNAwakePlayers() == 1
                           && field.isNF() && pbuffer[i].isPASS()){
                            continue; // 千日手回避
                        }
                        Field tfield = field;
                        cerr << tfield.toDebugString();
                        for(int p = 0; p < N_PLAYERS; ++p){
                            if(anyCards(c[p])){
                                tfield.setHand(p, c[p]);
                                tfield.setOpsHand(p, subtrCards(field.getRemCards(), c[p]));
                            }
                        }
                        cerr << tfield.toDebugString();
                        int next = tfield.procSlowest(pbuffer[i]);
                        if(next < 0){ // 試合終了
                            for(int p = 0; p < N_PLAYERS; ++p){
                                tresult[i].set_flag(p, tfield.getPlayerNewClass(p));
                            }
                        }else{ // 試合継続
                            searchMaxN(&tresult[i], tfield, pbuffer + moves);
                        }
                    }
                });
                
                // 最善手の決定
                ComparableBitSet32 bestResult;
                presult->clear();
                bestResult.reset().set(N_PLAYERS);
                for(int i = 0; i < moves; ++i){
                    if(tresult[i][turnPlayer] < bestResult){
                        (*presult) = tresult[i];
                        bestResult = tresult[i][turnPlayer].data();
                    }else if(!(tresult[i][turnPlayer] > bestResult)){
                        (*presult) |= tresult[i];
                        bestResult |= tresult[i][turnPlayer].data();
                    }
                }
            }
        }
    }
}

#endif // UECDA_SEARCH_MAXN_HPP_
