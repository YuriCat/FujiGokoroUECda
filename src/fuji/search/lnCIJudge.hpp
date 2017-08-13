/*
 lnCIJudge.hpp
 Katsuki Ohto
 */

// 完全情報仮定での多人数MaxN探索

#ifndef UECDA_SEARCH_LNCIJUDGE_HPP_
#define UECDA_SEARCH_LNCIJUDGE_HPP_

namespace UECda{
    namespace Fuji{
        namespace LnCI{
            
#ifdef USE_LNCIBOOK
            constexpr int BOOK_SIZE = (1 << 18) - 3;
            const std::string book_name = "LnCIBook";
            BitBook<11 * N_PLAYERS, BOOK_SIZE> book(book_name);
#endif
        }
        
        struct LnField{ // スタックに積んでいく
            Board bd;
            PlayersState ps;
            int turnSeat;
            int PMOwnerSeat;
            
            bool isSoloAlive()const noexcept{ return ps.isSoloAlive(); }
            bool isSoloAwake()const noexcept{ return ps.isSoloAwake(); }
            uint32_t isAlive(const int p)const{ return ps.isAlive(p); }
            uint32_t isAwake(const int p)const{ return ps.isAwake(p); }
            uint32_t getNAwakePlayers()const noexcept{ return ps.getNAwake(); }
            uint32_t getNAlivePlayers()const noexcept{ return ps.getNAlive(); }
            
            bool exam()const noexcept{
                if(!ps.exam()){ return false; }
                if(bd.isNF() && !ps.examNF()){ return false; }
                return true;
            }
        };
        
        class LnCIJudge{
            // 多人数の完全情報探索
            // rewardには報酬値(順位や、順位点ではない)をセットする
            
        private:
            static AtomicAnalyzer<1, 8, Analysis::TYPE_SEARCH> ana;
            
            const static int nodes_limit = 12000;
            const static int depth_limit = 30;
            
        public:
            
            int nodes;
            int childs;
            
            MoveInfo *const mv_buf;
            
            // 席順。
            // 局面ハッシュ値計算を簡単にするため、上がったプレーヤーの分を詰めていく
            // 5人〜2人
            BitArray32<4, N_PLAYERS> infoSeatPlayer5;
            BitArray32<4, N_PLAYERS> infoSeatPlayer4;
            BitArray32<4, N_PLAYERS> infoSeatPlayer3;
            BitArray32<4, N_PLAYERS> infoSeatPlayer2;
            
            // 報酬に関係ある部分のマスク
            // それまでにあがったプレーヤーの報酬については、
            // それ以下のプレーヤーの結果には無関係とするので、
            // 置換表にはこのマスク該当部分のみ入れる
            BitArray64<11, N_PLAYERS> REWARD_MASK;
            
            template<int N>
            int getSeatPlayer(int s){
                static_assert(2 <= N && N <= N_PLAYERS, "LnCIJudge : getSeatPlayer()\n");
                switch(N){
                    case 2: return infoSeatPlayer2[s]; break;
                    case 3: return infoSeatPlayer3[s]; break;
                    case 4: return infoSeatPlayer4[s]; break;
                    case 5: return infoSeatPlayer5[s]; break;
                    default: UNREACHABLE; break;
                }
            }
            
            template<int N>
            void setSeatPlayer(int s){
                // 一人減ったときに席順を詰めて設定する
                // Nは減る前の人数
                static_assert(3 <= N && N <= N_PLAYERS, "LnCIJudge : setSeatPlayer()\n");
                switch(N){
                    case 3:
                        infoSeatPlayer2 = infoSeatPlayer3;
                        infoSeatPlayer2.remove(s);
                        break;
                    case 4:
                        infoSeatPlayer3 = infoSeatPlayer4;
                        infoSeatPlayer3.remove(s);
                        break;
                    case 5:
                        infoSeatPlayer4 = infoSeatPlayer5;
                        infoSeatPlayer4.remove(s);
                        break;
                    default: UNREACHABLE; break;
                }
            }
            
            LnCIJudge(MoveInfo *const arg_buf)
            :mv_buf(arg_buf)
            {}
            
            ~LnCIJudge(){}
            
            // 人数、空場 or not、上位クラスから引き継いだ情報空間
            template<int N, int IS_NF, class field_t>
            int turn(BitArray64<11, N_PLAYERS> *const reward, const int depth, MoveInfo *const buf, const LnField& lnf, field_t& field, const BitSet32 conPlayers){
                
                static_assert(3 <= N && N <= N_PLAYERS ,"LnCIJudge : turn()\n"); // 2で呼ばれる事は無い
                
                assert(!(*reward)); // 検討が終わるまでrewardには書き込まれない設定
                assert(lnf.exam());
                
                constexpr int bestClass = N_PLAYERS - N;
                const int bestRew = game_reward[bestClass];
                
                const int ts = lnf.turnSeat;
                const int tp = getSeatPlayer<N>(ts);
                
                assert(bestClass < N_PLAYERS);
                assert(bestRew != 0);
                assert(ts < N_PLAYERS);
                assert(tp < N_PLAYERS);
                assert(lnf.isAlive(tp));
                assert(lnf.isAwake(tp));
                
                ASSERT(holdsBits((uint64_t)REWARD_MASK, uint64_t(1023ULL << (tp * 11))), cerr << REWARD_MASK << endl;);
                
                const Board& bd = lnf.bd;
                const PlayersState& ps = lnf.ps;
                
                int level;
                
                
                // 必勝判定を使うかどうか?
                // 最後のプレーヤーか、オーナーのalphaカットが出来る可能性のあるプレーヤーなら判定を使う。
                // alphaカットは現在未実装
                if(IS_NF == _YES){
                    if(!conPlayers.any_except(tp)){
                        level = 1;
                    }else{
                        level = 2;
                    }
                }else{
                    level = 2;
                }
                
                
                uint32_t tmp;
                
                switch(level){
                    case 0:
                    case 1:
                    {
                        // 必勝判定
                        // 全てのプレーヤーに対して支配性を求めてしまっているので遅い
                        // そのかわり、opsHand(自分以外の手札)の更新を省いている
                        
                        // 必勝が出た場合には後のプレーヤーは関係ない。
                        const uint32_t pla_flag = ((uint32_t)(lnf.ps) & 255U) ^ (1U << tp); // 自分以外のプレーヤー
                        
                        tmp = pla_flag;
                        do{
                            int p = bsf32(tmp);
                            if(!judgeHandPW_NF(field.hand[tp], field.hand[p],bd)){
                                goto JUDGE_SEQ;
                            }
                            tmp &= (tmp - 1);
                        }while(tmp);
                        
                        DERR << Space(depth) << "p:" << tp << " CIPW" << endl;
                        reward->set(tp, game_reward[bestClass]);
                        return 1;
                        
                    JUDGE_SEQ:
                        if(field.hand[tp].seq){
                            int NMoves = genAllSeq(buf, field.hand[tp].cards);
                            for(int m = 0; m < NMoves; ++m){
                                const MoveInfo& mv = buf[m];
                                if(mv.q() >= field.hand[tp].qty){
                                    DERR << Space(depth) << "p:" << tp << " CIPW" << endl;
                                    reward->set(tp, game_reward[bestClass]);
                                    return 1;//1手なので支配判定いらず
                                }else{
                                    Board nbd = bd;
                                    
                                    Hand nextHand = field.hand[tp];
                                    nextHand.makeMove1stHalf(mv.mv());
                                    nbd.procOrder(mv.mv());
                                    
                                    // 階段の支配性判定
                                    if(mv.q() < 4){ // 高速化のため、4枚以上なら支配とする
                                        tmp = pla_flag;
                                        do{
                                            int p = bsf32(tmp);
                                            if(mv.q() > field.hand[p].qty){
                                                tmp &= (tmp - 1);
                                                continue;
                                            }
                                            if(!dominatesHand(mv.mv(), field.hand[p], bd)){
                                                // 支配できてないのでPPW判定へ
                                                goto JUDGE_PPW;
                                            }
                                            tmp &= (tmp - 1);
                                        }while(tmp);
                                    }
                                    
                                    // 必勝判定
                                    tmp = pla_flag;
                                    do{
                                        int p = bsf32(tmp);
                                        if(!judgeHandPW_NF(nextHand, field.hand[p], nbd)){
                                            goto NOT_MATE;
                                        }
                                        tmp &= (tmp - 1);
                                    }while(tmp);
                                    DERR << Space(depth) << "p:" << tp << " CIPW" << endl;
                                    reward->set(tp, game_reward[bestClass]);
                                    return 1;
                                    
                                    // PPW判定
                                JUDGE_PPW:
                                    
                                    tmp = pla_flag;
                                    do{
                                        int p = bsf32(tmp);
                                        if(judgeHandPPW_NF(nextHand.cards, nextHand.pqr, nextHand.jk, field.hand[p].nd, bd)){
                                            DERR << Space(depth) << "p:" << tp << " CIPW" << endl;
                                            reward->set(tp, game_reward[bestClass]);
                                            return 1;
                                        }
                                        tmp &= (tmp - 1);
                                    }while(tmp);
                                }
                            }
                        }
                    NOT_MATE:;
                    }
                    case 2:
                    {
                        // 探索
                        int tmpBestRew = -1;
                        int rew;
                        //int brcount;
                        BitArray64<11, N_PLAYERS> retReward;
                        
                        int NMoves = gMIG.genMove<IS_NF>(buf, field.hand[tp].getCards(), bd);
                        
                        assert(NMoves >= 1);
                        
                        // それぞれの着手を探索
                        for(int m = NMoves - 1; m >= 0; --m){
                            const MoveInfo& mv = buf[m];
                            
                            // ヒューリスティック枝刈り...未実装
                            
                            DERR << Space(depth) << "p:" << tp << " " << mv << " " << OutCards(field.hand[tp].getCards()) << endl;
                            
                            BitSet32 newConPlayers;
                            
                            LnField nf;
                            
                            if(!mv.isPASS()){
                                // パスでない手
                                if(mv.q() >= field.hand[tp].getQty()){ // あがり
                                    
                                    DERR << Space(depth) << "p:" << tp << " won." << endl;
                                    
                                    newConPlayers = conPlayers;
                                    newConPlayers.reset(tp);
                                    
                                    if(!newConPlayers.any()){ // 結果を返す必要の有るプレーヤーが全員上がったため、上の階層に戻る
                                        reward->set(tp, (uint64_t)game_reward[bestClass]);
                                        return 1;
                                    }else{
                                        
                                        // 場を更新
                                        nf.ps = ps;
                                        nf.bd = bd;
                                        
                                        nf.ps.setDead(tp);
                                        nf.bd.proc(mv.mv());
                                        
                                        // プレーヤーが減るので、座席を詰める
                                        setSeatPlayer<N>(ts);
                                        
                                        int s = getRemovedNextSeat<N>(ts);
                                        //DERR << "ts:" << ts << endl;
                                        if(nf.bd.isNF()){ // 流れた
                                            assert(nf.ps.anyAlive());//誰かalive
                                            nf.ps.flush(); // 流す操作
                                        }else{ // 流れていない
                                            if(nf.ps.anyAwake()){ // 誰かawake
                                                while(1){
                                                    //DERR << "s:" << s;
                                                    
                                                    int p = getSeatPlayer<N - 1>(s);
                                                    if(nf.isAwake(p)){
                                                        if(nf.bd.getQty() > field.hand[p].qty || dominatesHand(nf.bd, field.hand[p])){
                                                            // 支配しているのでasleepにする
                                                            assert(lnf.ps.isAwake(p)); // 元々awake
                                                            
                                                            nf.ps.setAsleep(p);
                                                            if(!nf.ps.anyAwake()){
                                                                // 全員を支配。場を流す
                                                                nf.bd.flush();
                                                                nf.ps.flush();
                                                                // aliveなプレーヤーを探す
                                                                assert(nf.ps.anyAlive()); // 誰かalive
                                                                s = getRemovedNextSeat<N>(ts);
                                                                break;
                                                            }
                                                            
                                                        }else{
                                                            // 支配できない。
                                                            break;
                                                        }
                                                    }
                                                    s = getNextSeat<N - 1>(s);
                                                }
                                            }else{
                                                // 誰もawakeなプレーヤーがいない。当然流れる
                                                nf.bd.flush();
                                                nf.ps.flush();
                                                // aliveなプレーヤーを探す
                                                assert(nf.ps.anyAlive()); // 誰かalive
                                            }
                                        }
                                        
                                        ASSERT(0 <= s && s < N - 1, cerr << s << endl;);
                                        
                                        if(N - 1 == 2){
                                            // ラスト2人になるのでL2探索に移行。
                                            L2Judge lj(30000, buf + NMoves);
                                            
                                            int newtp = getSeatPlayer<N - 1>(s);
                                            int opsp = getSeatPlayer<N - 1>(1 - s); // 2人なので
                                            
                                            //DERR<<"to L2 ( "<<newtp<<" , "<<opsp<<" )"<<endl;
                                            
                                            assert(nf.isAlive(newtp));
                                            
                                            ASSERT((*reward)[newtp] == 0ULL, cerr << (*reward)[newtp] << endl);
                                            ASSERT((*reward)[opsp] == 0ULL, cerr << (*reward)[opsp] << endl);
                                            
                                            assert(newtp != opsp);
                                            
                                            FieldAddInfo fieldInfo;
                                            fieldInfo.init();
                                            
                                            // 流れたときどちらがターンを取るか等指定しておく必要がある
                                            if(nf.bd.isNF()){
                                                // 流れている場合、newtp
                                                fieldInfo.setFlushLead();
                                            }else{
                                                // 流れていないので、tpの次に来るaliveなプレーヤー
                                                // aliveなプレーヤーを探す
                                                
                                                assert(nf.ps.anyAlive()); // 誰かalive
                                                
                                                int ss;
                                                if(ts == N - 1){
                                                    ss = 0;
                                                }else{
                                                    ss = s;
                                                }
                                                
                                                if(s == ss){ // sはnewtpの座席
                                                    fieldInfo.setFlushLead();
                                                }
                                                if(nf.isSoloAwake()){
                                                    fieldInfo.setLastAwake();
                                                }
                                            }
                                            
                                            int res = lj.start_judge(field.hand[newtp], field.hand[opsp], nf.bd, fieldInfo); // L2探索
                                            if(res == L2_WIN){
                                                reward->set(newtp, (uint64_t)game_reward[N_PLAYERS - 2]);
                                            }else if(res == L2_LOSE){
                                                reward->set(opsp, (uint64_t)game_reward[N_PLAYERS - 2]);
                                            }else{
                                                // L2判定失敗
                                                // 報酬山分け
                                                uint64_t r = (uint64_t)(game_reward[N_PLAYERS - 2] / 2);
                                                reward->set(newtp, r);
                                                reward->set(opsp, r);
                                            }
                                            // 最下位プレーヤーの報酬は0なのでそのままでいい
                                            
                                            // 場合によっては上がり着手が複数ある可能性もあるが、
                                            // 即あがりの手は他プレーヤーに影響を与えにくいのでこの着手で確定としよう
                                            reward->set(tp, (uint64_t)game_reward[bestClass]);
                                            
                                            DERR << "to L2 " << *reward << endl;
                                            
                                            return 1;
                                        }else{
                                            // まだ3人以上いるので探索を続ける
                                            
                                            nf.turnSeat = s;
                                            nf.PMOwnerSeat = (ts == N - 1) ? 0 : ts; // 自分の席が無くなったので、次のプレーヤーが出した事にしておく
                                            
                                            ASSERT(holdsBits((uint64_t)REWARD_MASK, uint64_t(1023ULL << (tp * 11))),
                                                   cerr << REWARD_MASK << endl;);
                                            
                                            int ret;
                                            if(nf.bd.isNF()){ // 流れた
#ifdef USE_LNCIBOOK
                                                // ここで置換表を見る
                                                // 次のターンはN-1人になっているはず
                                                uint64_t next_hash;
                                                switch(N - 1){
                                                        assert(N - 1 != 2); // L2なのにL2メソッドに入っていない
                                                    case 3:{
                                                        next_hash = knitHash_L3CI_NF(field.hand[getSeatPlayer<N - 1>(s)].hash,
                                                                                     field.hand[getSeatPlayer<N - 1>((s + 1) % (N - 1))].hash,
                                                                                     field.hand[getSeatPlayer<N - 1>((s + 2) % (N - 1))].hash,
                                                                                     nf.bd);
                                                    }break;
                                                    case 4:{
                                                        next_hash = knitHash_L4CI_NF(field.hand[getSeatPlayer<N - 1>(s)].hash,
                                                                                     field.hand[getSeatPlayer<N - 1>((s + 1) % (N - 1))].hash,
                                                                                     field.hand[getSeatPlayer<N - 1>((s + 2) % (N - 1))].hash,
                                                                                     field.hand[getSeatPlayer<N - 1>((s + 3) % (N - 1))].hash,
                                                                                     nf.bd);
                                                    }break;
                                                    default: UNREACHABLE; break;
                                                }
                                                
                                                // 置換表を見る。read関数では、インデックスとハッシュ値の上位9(64-11*5)ビットを認証
                                                uint64_t bookrew = LnCI::book.read(next_hash);
                                                if(bookrew && holdsBits((uint64_t)REWARD_MASK, bookrew)){
                                                    // 報酬列にビット包括関係があれば、同じ局面だろうと判定
                                                    // 現在、プレーヤーの並びも同一である必要あり。
                                                    // 異なるプレーヤーが同じ手札になった時の局面を同一視するためには、修正が必要
                                                    *reward = bookrew;
                                                    reward->set(tp, (uint64_t)game_reward[bestClass]);
                                                    
                                                    DERR << Space(depth) << "BOOK " << (*reward) << endl;
                                                    
                                                    return 1;
                                                }
                                                
#endif
                                                
                                                REWARD_MASK ^= (1023ULL << (tp * 11)); // 上がったプレーヤー分を外す
                                                
                                                ret = turn<cmax<int>(N - 1, 3), _YES>(reward, depth + 1, buf + NMoves, nf, field, newConPlayers);
                                                
#ifdef USE_LNCIBOOK
                                                if(newConPlayers.count() >= N - 1 - 1){
                                                    // 置換表に結果を登録
                                                    DERR << Space(depth) << "REGIST " << *reward << endl;
                                                    LnCI::book.regist((uint64_t)*reward, next_hash);
                                                }
#endif
                                            }else{ // 流れていない
                                                REWARD_MASK ^= (1023ULL << (tp * 11)); // 上がったプレーヤー分を外す
                                                ret = turn<cmax<int>(N - 1, 3), _NO>(reward, depth + 1, buf + NMoves, nf, field, newConPlayers);
                                            }
                                            
                                            assert(isExclusive((uint64_t)REWARD_MASK, uint64_t(1023ULL << (tp * 11))));
                                            
                                            REWARD_MASK ^= (1023ULL << (tp * 11)); // 上がったプレーヤー分を戻す
                                            
                                            // 場合によっては上がり着手が複数ある可能性もあるが、
                                            // 即あがりの手は他プレーヤーに影響を与えにくいのでこの着手で確定としよう
                                            reward->set(tp, (uint64_t)game_reward[bestClass]);
                                            return ret;
                                        }
                                    }
                                    
                                }else{
                                    // 上がりではない
                                    
                                    nf.bd = bd;
                                    nf.ps = lnf.ps;
                                    
                                    nf.bd.proc(mv.mv());
                                    
                                    int s = ts;
                                    if(nf.bd.isNF()){ // 流れた
                                        nf.ps.flush();
                                    }else{ // 流れていない
                                        while(1){
                                            s = getNextSeat<N>(s);
                                            int p = getSeatPlayer<N>(s);
                                            
                                            if(nf.isAwake(p)){
                                                if(nf.bd.getQty() > field.hand[p].qty || dominatesHand(nf.bd, field.hand[p])){
                                                    // 支配しているのでasleepにする
                                                    assert(lnf.ps.isAlive(p) && lnf.ps.isAwake(p)); // 元々alive&&awake
                                                    
                                                    nf.ps.setAsleep(p);
                                                    if(!nf.ps.anyAwake()){
                                                        // 全員を支配。場を流す
                                                        nf.bd.flush();
                                                        nf.ps.flush();
                                                        s = ts;
                                                        break;
                                                    }
                                                    
                                                }else{
                                                    // 支配できない
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    
                                    ASSERT(0 <= s && s < N, cerr << s << endl;);
                                    
                                    nf.turnSeat = s;
                                    newConPlayers = conPlayers;
                                    newConPlayers.set(tp);
                                    nf.PMOwnerSeat = ts;
                                }
                                
                            }else{ // パスの時
                                nf.bd = bd;
                                nf.ps = lnf.ps;
                                
                                if(lnf.isSoloAwake()){
                                    // 残り1人 場を流す
                                    nf.bd.flush();
                                    nf.ps.flush();
                                    
                                    assert(nf.ps.anyAlive()); // 誰かalive
                                    
                                    int s = lnf.PMOwnerSeat;
                                    nf.turnSeat = s;
                                    
                                }else{
                                    // 残り1人でない asleepにする
                                    // 場が支配していないプレーヤーのところまで進める
                                    assert(lnf.ps.isAlive(tp) && lnf.ps.isAwake(tp)); // 元々alive&&awake
                                    
                                    nf.ps.setAsleep(tp);
                                    
                                    assert(nf.ps.anyAwake()); // 誰かawake
                                    
                                    int s = ts;
                                    while(1){
                                        s = getNextSeat<N>(s);
                                        int p = getSeatPlayer<N>(s);
                                        
                                        if(nf.isAwake(p)){
                                            if(nf.bd.getQty() > field.hand[p].qty || dominatesHand(nf.bd, field.hand[p])){
                                                // 支配しているのでasleepにする
                                                
                                                assert(lnf.ps.isAlive(p) && lnf.ps.isAwake(p));//元々alive&&awake
                                                
                                                nf.ps.setAsleep(p);
                                                
                                                if(!nf.ps.anyAwake()){
                                                    // 全員を支配。場を流す
                                                    nf.bd.flush();
                                                    nf.ps.flush();
                                                    
                                                    // aliveなプレーヤーを探す
                                                    
                                                    assert(nf.ps.anyAlive()); // 誰かalive
                                                    
                                                    s = lnf.PMOwnerSeat;
                                                    
                                                    assert(s < N && lnf.isAlive(getSeatPlayer<N>(s)));
                                                    
                                                    break;
                                                }
                                            }else{
                                                // 支配できない
                                                break;
                                            }
                                        }
                                    }
                                    
                                    ASSERT(0 <= s && s < N, cerr << s << endl;);
                                    
                                    nf.turnSeat = s;
                                    
                                    assert(nf.isAlive(getSeatPlayer<N>(s)));
                                }
                                newConPlayers = conPlayers;
                                newConPlayers.set(tp);
                            }
                            
                            BitArray64<11, N_PLAYERS> newReward = 0ULL;
                            
                            int ret;
                            if(nf.bd.isNF()){ // 流れた
                                int s = nf.turnSeat;
                                
                                // ここで置換表を見る
                                
                                // 手札のハッシュ値のみ更新
                                uint64_t next_hash, hash_dist;
                                Cards dc;
                                
                                if(!mv.isPASS()){
                                    dc = mv.c<_NO>();
                                    hash_dist = genHash_Cards(dc);
                                    field.hand[tp].hash ^= hash_dist;
                                }else{
                                    hash_dist = 0ULL; // warning回避
                                    dc = CARDS_NULL; // warning回避
                                }
                                
#ifdef USE_LNCIBOOK
                                
                                switch(N){
                                        assert(3 <= N && N <= N_PLAYERS); // L2なのにL2メソッドに入っていない等
                                    case 3:{
                                        next_hash = knitHash_L3CI_NF(field.hand[getSeatPlayer<N>(s)].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 1) % (N))].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 2) % (N))].hash,
                                                                     nf.bd);
                                    }break;
                                    case 4:{
                                        next_hash = knitHash_L4CI_NF(field.hand[getSeatPlayer<N>(s)].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 1) % (N))].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 2) % (N))].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 3) % (N))].hash,
                                                                     nf.bd);
                                    }break;
                                    case 5:{
                                        next_hash = knitHash_L5CI_NF(field.hand[getSeatPlayer<N>(s)].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 1) % (N))].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 2) % (N))].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 3) % (N))].hash,
                                                                     field.hand[getSeatPlayer<N>((s + 4) % (N))].hash,
                                                                     nf.bd);
                                    }break;
                                    default: UNREACHABLE; break;
                                }
#endif
                                
                                // 置換表を見る。read関数では、インデックスとハッシュ値の上位9(64-11*5)ビットを認証
#ifdef USE_LNCIBOOK
                                uint64_t bookrew = LnCI::book.read( next_hash );
#else 
                                constexpr uint64_t bookrew = 0ULL;
#endif
                                if(bookrew && holdsBits((uint64_t)REWARD_MASK, bookrew)){ // 結果あり
                                    // 報酬列にビット包括関係があれば、同じ局面だろうと判定
                                    // 現在、プレーヤーの並びも同一である必要あり。
                                    // 異なるプレーヤーが同じ手札になった時の局面を同一視するためには、修正が必要
                                    newReward = bookrew;
                                    
                                    DERR << Space(depth) << "BOOK " << (*reward) << endl;
                                    
                                    // 手札ハッシュ値を元に戻すのを忘れないよう注意
                                    if(!mv.isPASS()){
                                        field.hand[tp].hash ^= hash_dist;
                                    }
                                }else{
                                    
                                    if(!mv.isPASS()){
                                        field.hand[tp].makeMove(mv.mv(), dc); // 手札(ハッシュ値以外)更新
                                    }
                                    ret = turn<N, _YES>(&newReward, depth + 1, buf + NMoves, nf, field, newConPlayers);
                                    if(!mv.isPASS()){
                                        field.hand[tp].unmakeMoveAll(mv.mv(), dc, hash_dist); // 手札を戻す
                                    }
                                    if(ret == -1){ return -1; }//打ち切り
#ifdef USE_LNCIBOOK
                                    if(newConPlayers.count() >= N - 1){
                                        // 置換表に結果を登録
                                        DERR << Space(depth) << "RESIST " << newReward << endl;
                                        LnCI::book.regist((uint64_t)newReward, next_hash);
                                    }
#endif
                                }
                            }else{ // 流れていない
                                uint64_t hash_dist;
                                Cards dc;
                                if(!mv.isPASS()){
                                    dc = mv.c<_NO>();
                                    hash_dist = genHash_Cards(dc);
                                    field.hand[tp].makeMoveAll(mv.mv(), dc, hash_dist); // 手札更新
                                }else{
                                    hash_dist = 0ULL; // warning回避
                                    dc = CARDS_NULL; // warning回避
                                }
                                ret = turn<N, _NO>(&newReward, depth + 1, buf + NMoves, nf, field, newConPlayers);
                                if(!mv.isPASS()){
                                    // 手札を戻す
                                    field.hand[tp].unmakeMoveAll(mv.mv(), dc, hash_dist);
                                }
                            }
                            
                            rew = (int)(newReward[tp]);
                            
                            if(rew == bestRew){
                                // この手で自分が最高報酬を得た
                                // 本当は他のも検討したいが、ここで終了とする
                                DERR << Space(depth) << "bestRew : " << tp << " ( " << rew << " )" << endl;
                                *reward |= newReward;
                                DERR << *reward << endl;
                                return 0;
                            }else if(rew == tmpBestRew){ // これまでの最大報酬と同じだった
                                if((uint64_t)retReward == (uint64_t)newReward){
                                    //++brcount;
                                }else{
                                    //if( brcount <= 4 ){
                                    
                                    //DERR<<retReward<<" * "<<brcount<<" + "<<newReward<<" = ";
                                    
                                    //retReward=(((uint64_t)(retReward)) * brcount + ((uint64_t)newReward)) / ( brcount+1 );//平均報酬更新
                                    
                                    //コンパイル時に計算してほしい
                                    BitArray64<11, N_PLAYERS> mask = 0ULL;
                                    for(int p = 0; p < N_PLAYERS; ++p){
                                        mask.set(p, 1023ULL);
                                    }
                                    
                                    retReward = (((uint64_t)(retReward) + ((uint64_t)newReward)) >> 1) & mask; // 平均報酬更新
                                    
                                    //DERR<<retReward<<endl;
                                    //++brcount;
                                    //}else{
                                    //同報酬のものが多い時はもう何もしない
                                    //}
                                }
                            }else if(rew > tmpBestRew){ // これまでの最大報酬より上だった
                                //brcount = 1;
                                tmpBestRew = rew;
                                retReward = newReward;
                            }
                        } // 着手ループここまで
                        
                        *reward |= retReward;
                        return 0;
                        break;
                    }
                    default: UNREACHABLE; break;
                }
                UNREACHABLE;
            }
            
            
            template<class field_t>
            int start(BitArray64<11, N_PLAYERS> *const reward, field_t& field, const BitSet32 conPlayers){
                // 開始
                
                ana.start();
                
                LnField lnf;
                
                lnf.bd = field.bd;
                lnf.ps = field.ps;
                lnf.PMOwnerSeat = field.getPMOwner();
                lnf.turnSeat = field.getPlayerSeat(field.getTurnPlayer());
                
                assert(lnf.exam());
                
                nodes = 0;
                
                DERR << "CI-SEARCH START" << endl;
                for(int p = 0; p < N_PLAYERS; ++p){
                    if(field.isAlive(p)){
                        DERR << p << OutCards(field.hand[p].getCards());
                    }
                }
                DERR << endl;
                
                // コンパイル時に計算してほしい
                BitArray64<11, N_PLAYERS> mask = 0ULL;
                for(int p = 0; p < N_PLAYERS; ++p){
                    mask.set(p, 1023ULL);
                }
                REWARD_MASK = mask;
                
                int ret;
                
                // 置換表で見つかった場合はすぐに帰る
                // 見つからなかった場合、置換表に結果登録
                
                
                switch(lnf.getNAlivePlayers()){
                    case 3:{
                        infoSeatPlayer3 = field.infoSeatPlayer;
                        
                        // すでに上がったブレーヤーの席を詰める
                        
                        int s;
                        for(s = N_PLAYERS - 1; s >= 0; --s){
                            int p = infoSeatPlayer3[s];
                            if(!lnf.isAlive(p)){
                                infoSeatPlayer3.remove(s);
                                
                                assert(holdsBits((uint64_t)REWARD_MASK, uint64_t(1023ULL << (p * 11))));
                                
                                REWARD_MASK ^= (1023ULL << (p * 11));
                                if(lnf.PMOwnerSeat > s){
                                    --lnf.PMOwnerSeat;
                                }
                                if(lnf.turnSeat > s){
                                    --lnf.turnSeat;
                                }
                            }
                        }
                        
                        if(field.isNF()){
                            
                            s = lnf.turnSeat;
                            /*
                             uint64_t hash=knitHash_L3CI(field.hand[infoSeatPlayer3[s]].hash,
                             field.hand[infoSeatPlayer3[(s+1)%3]].hash,
                             field.hand[infoSeatPlayer3[(s+2)%3]].hash,
                             lnf.bd);
                             uint64_t bookrew=LnCI::book.read( hash );
                             if( bookrew && holdsBits( REWARD_MASK, bookrew ) ){//結果あり。
                             *reward=bookrew;
                             ret=0;
                             }else{
                             */
                            ret = turn<3, _YES>(reward, 0, mv_buf, lnf, field, conPlayers);
                            
                            //if( ret != -1 ){
                            //	LnCI::book.regist( (uint64_t)*reward, hash );
                            //}
                            //}
                        }else{
                            ret = turn<3, _NO>(reward, 0, mv_buf, lnf, field, conPlayers);
                        }
                    }break;
                    case 4:{
                        infoSeatPlayer4 = field.infoSeatPlayer;
                        // すでに上がったブレーヤーの席を詰める
                        /*uint32_t dead=((1<<N_PLAYERS)-1) & (~lnf.infoPlayerState);
                         
                         assert( countBits32(dead) == 1 );
                         
                         //pが上がったプレーヤーのプレーヤー番号
                         int p=bsr32(dead);
                         int s=field.infoSeat[p];
                         
                         assert( !field.isAlive(p) );
                         
                         assert( holdsBits( REWARD_MASK, 1023ULL<<(p*12)) );
                         
                         REWARD_MASK^=(1023ULL<<(p*12));
                         
                         infoSeatPlayer4.remove(s);
                         
                         if( lnf.turnSeat == 4 ){
                         lnf.turnSeat=3;
                         }
                         if( lnf.PMOwnerSeat == 4 ){
                         lnf.PMOwnerSeat=0;
                         }*/
                        int s;
                        for(s = N_PLAYERS - 1; s >= 0; --s){
                            int p = infoSeatPlayer4[s];
                            if(!lnf.isAlive(p)){
                                infoSeatPlayer4.remove(s);
                                
                                assert(holdsBits((uint64_t)REWARD_MASK, uint64_t(1023ULL << (p * 11))));
                                
                                REWARD_MASK ^= (1023ULL << (p * 11));
                                if(lnf.PMOwnerSeat > s){
                                    --lnf.PMOwnerSeat;
                                }
                                if(lnf.turnSeat > s){
                                    --lnf.turnSeat;
                                }
                                break;
                            }
                        }
                        
                        if(field.isNF()){
                            
                            //s=lnf.turnSeat;
                            
                            /*uint64_t hash=knitHash_L4CI(field.hand[infoSeatPlayer4[s]].hash,
                             field.hand[infoSeatPlayer4[(s+1)%4]].hash,
                             field.hand[infoSeatPlayer4[(s+2)%4]].hash,
                             field.hand[infoSeatPlayer4[(s+3)%4]].hash,
                             lnf.bd);
                             uint64_t bookrew=LnCI::book.read( hash );
                             if( bookrew && holdsBits( REWARD_MASK, bookrew ) ){//結果あり。
                             *reward=bookrew;
                             ret=0;
                             }else{*/
                            ret=turn<4, _YES>(reward, 0, mv_buf, lnf, field, conPlayers);
                            
                            //if( ret != -1 ){
                            //	LnCI::book.regist( (uint64_t)*reward, hash );
                            //}
                            //}
                        }else{
                            ret=turn<4, _NO>(reward, 0, mv_buf, lnf, field, conPlayers);
                        }
                    }break;
#if N_PLAYERS >= 5
                    case 5:{
                        infoSeatPlayer5 = field.infoSeatPlayer;
                        if(field.isNF()){
                            
                            //int s=lnf.turnSeat;
                            
                            /*uint64_t hash=knitHash_L5CI(field.hand[infoSeatPlayer5[s]].hash,
                             field.hand[infoSeatPlayer5[(s+1)%5]].hash,
                             field.hand[infoSeatPlayer5[(s+2)%5]].hash,
                             field.hand[infoSeatPlayer5[(s+3)%5]].hash,
                             field.hand[infoSeatPlayer5[(s+4)%5]].hash,
                             lnf.bd);
                             uint64_t bookrew=LnCI::book.read( hash );
                             if( bookrew && holdsBits( REWARD_MASK, bookrew ) ){//結果あり。
                             *reward=bookrew;
                             ret=0;
                             }else{*/
                            ret=turn<5, _YES>(reward, 0, mv_buf, lnf, field, conPlayers);
                            
                            //if( ret != -1 ){
                            //	LnCI::book.regist( (uint64_t)*reward, hash );
                            //}
                            //}
                        }else{
                            ret=turn<5, _NO>(reward, 0, mv_buf, lnf, field, conPlayers);
                        }
                    }break;
#endif // N_PLAYERS >= 5
                    default: UNREACHABLE; break;
                }
                
                if(ret == -1){
                    // 失敗
                    ana.addFailure();
                }else{
                    assert((uint64_t)(*reward) != 0ULL); // 成功のとき報酬が付いてなかったらおかしい
                    assert(holdsBits((uint64_t)REWARD_MASK, (uint64_t)(*reward))); // 変な所に報酬値がある
                }
                
                //DERR<<"BA64 : "<<sizeof(BitArray64<12,N_PLAYERS>)<<endl;
                
                ana.end();
                
                return ret;
            }
        };
        
        AtomicAnalyzer<1, 8, Analysis::TYPE_SEARCH> LnCIJudge::ana("LnCIJudge");
        
    }
}

#endif // UECDA_SEARCH_LNCIJUDGE_HPP_
