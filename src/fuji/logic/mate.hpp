/*
 mate.hpp
 Katsuki Ohto
 */

// 詰み(Mate)判定

#ifndef UECDA_LOGIC_MATE_HPP_
#define UECDA_LOGIC_MATE_HPP_

namespace UECda{
    
    // 他のプレーヤー達がいかなる行動信念を持っていたとしても
    // 自分のあがりを阻止出来ない事の判定
    
    // judge...用件の判定さえ出来れば良い
    // check...ある着手について用件の判定を行う
    // search...用件を満たす着手を探す
    
    // 詰みにも色々な場合があるだろうが、
    // 多くは以下のうちのどれかである
    
    // 完全勝利(PW)...他のプレーヤーに一度も選択機会を与えずに上がる
    // N手完全勝利(BNPW)...相手にN区間の着手決定機会があるが最終的にPWになることがわかっている
    
    // 独壇場の場合は他支配判定の必要がないのでSF関数で
    
    // 空場勝利...空場状態での必勝確定。空場は多いので重要
    
    // また、一つ必勝を見つけたら終わるのか、全部探すのかも要指定
    
    // ジョーカーや階段、グループが存在する事を考慮に入れるかによって分けると
    // さらに高速化が期待出来る
    
    namespace Mate{
        /*#ifdef USE_MATEBOOK
         const std::string MateBook_name="MateBook";
         const int MATEBOOK_SIZE=(1<<14)-3;
         TwoValueBook<MATEBOOK_SIZE> book(MateBook_name);
         #endif
         */
    }
    
    // 関数群
    // check, judgeは基本的にブール関数
    
    // BNPW(unrivaledな場合はBNPWではないので指定無し)
    template<int IS_NF = _BOTH>
    bool checkHandBNPW(const int depth, MoveInfo *const buf, const MoveInfo,
                       const Hand& myHand, const Hand& opsHand,
                       const Board argBd, const uint32_t al, const uint32_t aw);
    
    // checkmate
    template<int IS_NF = _BOTH, int IS_UNRIVALED = _NO>
    bool judgeHandMate(const int depth, MoveInfo *const buf,
                       const Hand& myHand, const Hand& opsHand,
                       const Board argBd, const uint32_t al, const uint32_t aw);
    
    template<int IS_NF = _BOTH, int IS_UNRIVALED = _NO>
    bool checkHandMate(const int depth, MoveInfo *const buf, MoveInfo&,
                       const Hand& myHand, const Hand& opsHand, const Board argBd, const uint32_t al, const uint32_t aw);
    
    template<int IS_NF = _BOTH,int IS_UNRIVALED = _NO>
    int searchHandMate(const int depth, MoveInfo *const buf, const int NMoves,
                       const Hand& myHand, const Hand& opsHand,
                       const Board argBd, const uint32_t al, const uint32_t aw);
    
    
    // 汎用関数
    bool judgeMate_1M_NF(const Hand& myHand){
        // 空場、1手詰み（つまり出して上がり,確実にPW）
        // 8切り関係の3手詰み等も含めた高速化は別関数にて
        if(myHand.qty <= 1){ return true; }
        Cards pqr = myHand.pqr;
        if(myHand.containsJOKER()){//ジョーカーあり
            if(!any2Cards(pqr))return true;
            if(myHand.seq){
                if(myHand.qty == 3){ return true; }
                if(canMakeJokerSeq<_YES>(myHand.cards, myHand.qty)){ return 1; }
            }
        }else{
            if(!any2Cards(pqr))return true;
            if(myHand.seq){
                if(myHand.qty == 3){ return true; }
                if(canMakePlainSeq<_NO>(myHand.cards, myHand.qty)){ return 1; }
            }
        }
        return 0;
    }
    
    bool judgeMate_Easy_NF(const Hand& myHand){
        // とりあえず高速でNF_Mateかどうか判断したい場合
        if(myHand.qty <= 1){return true;}
        Cards pqr = myHand.pqr;
        if(!any2Cards(maskCards(pqr, CARDS_8 | CARDS_JOKER)))return true;
        if(myHand.containsJOKER()){ // ジョーカーあり
            if(myHand.seq){
                if(myHand.qty == 3){return true;}
                if(canMakeJokerSeq<1>(myHand.cards, myHand.qty)){return 1;}
            }
            //if(myHand.containsS3()){ // S3あり
        }else{
            if(myHand.seq){
                if(myHand.qty == 3){return true;}
                if(canMakePlainSeq<0>(myHand.cards, myHand.qty)){return 1;}
            }
        }
        return 0;
    }
    
    bool judgeHandPPW_NF(const Cards cards, const Cards pqr, const uint32_t jk, const Cards *const nd,const Board argBd){
        // すでに支配的でない着手があることがわかっており、
        // それ以外が全て支配的かを確認するメソッド
        // 階段未対応
        
        // 使用...mine - jk, pqr, ops - nd
        
        // ただし、
        // クアドラを2つ以上
        // トリプルを3つ以上
        // クアドラ1つ以上 && トリプル2つ以上
        // といった稀な場合には高確率で誤判定する
        
        assert(pqr == CardsToPQR(cards));
        
        const int ord = argBd.tmpOrder();
        
        const Cards ndpqr = pqr & nd[ord] & ~CARDS_8; // 支配出来ていないpqr
        if(!ndpqr){
            // 革命を返される可能性のみ考慮しておく。
            if(pqr & PQR_4 & ~CARDS_8){
                Cards quad = pqr & PQR_4 & ~CARDS_8 & ~ndpqr;
                if(quad){
                    // このとき8以外の革命でターンをとる予定
                    if(quad & nd[flipOrder(ord)]){
                        // 革命でターンが取れない。
                        if(!jk){
                            return false;
                        }else{
                            // 革命にジョーカーを加えれば良い。
                            // 革命2つの場合には間違い
                        }
                    }
                }
            }
            DERR << "PPW " << OutCards(cards) << endl;
            return true;
        }else{
            Cards ndpqr_m = ndpqr & (ndpqr - 1ULL);
            if(!ndpqr_m){
                // ndpqrが1ビット
                // ジョーカーを加えて必勝になるか確かめる
                if(jk){
                    if(ndpqr & PQR_4){
                        // 5枚出しで勝てる
                        DERR << "PPW (QUIN) " << OutCards(cards) << endl;
                        return true;
                    }else{
                        // ndと交差しなければ勝ち
                        if(!((ndpqr << 1) & nd[ord])){
                            DERR << "PPW (+JK) " << OutCards(cards) << endl;
                            return true;
                        }
                        // スペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                        if(ndpqr == CARDS_MIN && containsS3(cards)){
                            DERR << "PPW (JK-S3) " << OutCards(cards) << endl;
                            return true;
                        }
                    }
                }
            }
            // 革命によって勝てないか考える
            if(pqr & PQR_4){
                // ジョーカーを使わない革命あり。
                Cards quad = pqr & PQR_4;
                Cards ndpqr_new = ndpqr & ~quad & nd[flipOrder(ord)];
                
                if(jk){
                    if(!ndpqr_new){
                        // 革命がだめでも5枚出しがある。勝ち
                        DERR << "PPW (QUIN) " << OutCards(cards) << endl;
                        return true;
                    }else{
                        if(!(ndpqr_new & (ndpqr_new - 1ULL))){
                            // 1つ残し
                            // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                            // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                            if(!(quad & nd[flipOrder(ord)] & ~CARDS_8)){
                                // 革命でターンがとれる
                                // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                                // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければよい
                                
                                // いずれかのオーダーのndと交差しなければ勝ち
                                if((!((ndpqr_new << 1) & nd[ord]))
                                   || (!((ndpqr_new << 1) & nd[flipOrder(ord)]))){
                                    DERR << "NJ_QUAD PPW (+JK) " << OutCards(cards) << endl;
                                    return true;
                                }
                                // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし。
                            }
                        }
                    }
                }else{
                    //ジョーカーが無いので、革命が支配的かつ他のも全て支配的が必須
                    if(!ndpqr_new && !(quad & nd[ flipOrder(ord)] & ~CARDS_8)){
                        DERR << "NJ_QUAD PPW " << OutCards(cards) << endl;
                        return true;
                    }
                }
            }else if(jk && (pqr & PQR_3)){
                //ジョーカーを使えば革命あり。
                //ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
                //まず、この革命がターンを取れるか
                Cards quad = (pqr & PQR_3) << 1;
                //トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
                Cards tmp = quad & nd[flipOrder(ord)] & ~CARDS_8;
                if(quad != tmp){
                    //革命でターンがとれる
                    //このとき、まだターンが取れていなかったやつを逆オーダーで考える
                    Cards ndpqr_new = ndpqr & ~((quad ^ tmp) >> 1) & nd[flipOrder(ord)];//革命の分も消しとく
                    if(!ndpqr_new){
                        //全部支配できた場合のみOK
                        DERR << "J_QUAD PPW " << OutCards(cards) << endl;
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    bool judgeHandPW_NF(const Hand& myHand, const Hand& opsHand, const Board argBd){
        // 合法着手生成を用いずに判定出来る場合
        
        // ただし、
        // クアドラを2つ以上
        // トリプルを3つ以上
        // クアドラ1つ以上 && トリプル2つ以上
        // といった稀な場合には高確率で誤判定する
        
        assert(myHand.exam_jk());
        assert(myHand.exam_seq());
        assert(myHand.exam_pqr());
        assert(opsHand.exam_jk());
        assert(opsHand.exam_seq());
        assert(opsHand.exam_nd());
        
        const int ord = argBd.tmpOrder();
        
        const Cards ndpqr = myHand.pqr & opsHand.nd[ord] & ~CARDS_8; // 支配出来ていないpqr
        if(!ndpqr){
            // このときほぼ必勝なのだが、一応4枚グループが2つある場合にはそうでないことがある
            // 面倒なので勝ちとしておく
            DERR << "PW0 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
            return true;
            /*Cards pqr4=myHand.pqr & PQR_4;
             if( (!pqr4) || (!any2Cards(pqr4)) ){
             //４枚グループが1つ以下
             return true;
             }
             //以下4枚グループ2つ。ほぼないので適当で良い。
             if( myHand.jk ){
             //革命のうち1つにジョーカーを加えれば勝ち
             return true;
             }else{
             if( !any2Cards( pqr4 & opsHand.nd[ord] ) ){
             return true;
             }
             }
             return false;
             */
        }else{
            Cards ndpqr_m = ndpqr & (ndpqr - 1ULL);
            
            if(!ndpqr_m){
                // ndpqrが1ビットだけ
                // このときも基本的に必勝。
                // 革命を返される可能性のみ考慮しておく。
                if(myHand.pqr & PQR_4 & ~CARDS_8){
                    Cards quad = myHand.pqr & PQR_4 & ~CARDS_8 & ~ndpqr;
                    if(quad){
                        // このとき8以外の革命でターンをとる予定
                        if(quad & opsHand.nd[flipOrder(ord)]){
                            // 革命でターンが取れない。
                            if(!myHand.jk){
                                return false;
                            }else{
                                // 革命にジョーカーを加えれば良い。
                                // 革命2つの場合には間違い
                            }
                        }
                    }
                }
                // 一応OKとする
                DERR << "PW1 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                return true;
            }else{
                // 支配出来ない部分が2つ以上
                if(myHand.seq){ // 階段である場合がある。
                    if(myHand.qty == 3){ // 3階段1つ
                        DERR << "SEQ PW0 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                        return true;
                    }
                    
                    // シングルやグループで出しても支配出来るランクの階段を使う場合は難しい。
                    // 3階段1つ以外の場合、合法着手生成によって確かめる(暫定的処置)
                }else{
                    // ここから長いので、ジョーカーも革命も階段も無い場合(大部分)はさっさと帰る
                    if(!myHand.jk){
                        if(!(myHand.pqr & PQR_4)){
                            return false;
                        }
                    }
                }
                
                if(!(ndpqr_m & (ndpqr_m - 1ULL))){
                    // 2ビットのみ。
                    // いずれかにジョーカーを加えて必勝になるか確かめる
                    if(myHand.jk){
                        if(ndpqr & PQR_4){
                            // 5枚出しで勝てる
                            DERR << "PW1 (QUAD+JK) " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                            return true;
                        }else{
                            Cards h = ndpqr;
                            Cards l = popLow(&h);
                            
                            // どちらかとndが交差しなければ勝ち
                            if(!((h << 1) & opsHand.nd[ord]) || !((l << 1) & opsHand.nd[ord])){
                                DERR << "PW1 (+JK) " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                                return true;
                            }
                            
                            // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                            if(l == (CARDS_3 & PQR_1) && containsS3(myHand.cards)){
                                // 3はランク最低なのでlだろう
                                DERR << "PW1 (JK-S3) " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                                return true;
                            }
                        }
                    }
                }
                
                // 革命によって勝てないか考える
                if(myHand.pqr & PQR_4){
                    // ジョーカーを使わない革命あり。
                    Cards quad = myHand.pqr & PQR_4;
                    Cards ndpqr_new = ndpqr & ~quad & opsHand.nd[flipOrder(ord)];
                    
                    if(!ndpqr_new){
                        // 全部支配できた。勝ち
                        DERR << "NJ_QUAD PW0~1 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                        return true;
                    }
                    Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
                    if(myHand.jk){
                        if(!ndpqr_new_m){
                            // 1つ残し。5枚出しすればよい。勝ち
                            DERR << "NJ_QUIN PW1 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                            return true;
                        }else{
                            // 2つ以上残っている。
                            if(!(ndpqr_new_m & (ndpqr_new_m - 1ULL))){
                                // 2つだけ
                                // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                                // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                                if(!(quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8)){
                                    // 革命でターンがとれる
                                    // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                                    
                                    // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければよい
                                    Cards h = ndpqr_new;
                                    Cards l = popLow(&h);
                                    
                                    // どちらかが、いずれかのオーダーのndと交差しなければ勝ち
                                    if(!((h << 1) & opsHand.nd[ord]) || !((h << 1) & opsHand.nd[flipOrder(ord)])
                                       || !((l << 1) & opsHand.nd[ord]) || !((l << 1) & opsHand.nd[flipOrder(ord)])){
                                        DERR << "NJ_QUAD PW1 (+JK) " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                                        return true;
                                    }
                                    
                                    // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし。
                                    // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                                    /*if( l == CARDS_C3 && containsS3( myHand.cards )){
                                     //3はランク最低なのでlだろう
                                     DERR<<"NJ_QUAD PW1 (JK-S3)"<<endl;
                                     return true;
                                     }*/
                                }
                            }
                        }
                    }else{
                        if(!ndpqr_new_m){
                            // 1つ残し。革命でターンが取れるなら勝ち。
                            if(!(quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8)){
                                DERR << "NJ_QUAD PW1 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                                return true;
                            }
                        }
                    }
                }else if(myHand.jk && (myHand.pqr & PQR_3)){
                    // ジョーカーを使えば革命あり。
                    // ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
                    // まず、この革命がターンを取れるか
                    Cards quad = (myHand.pqr & PQR_3) << 1;
                    // トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
                    Cards tmp = quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8;
                    if(quad != tmp){
                        // 革命でターンがとれる
                        // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                        Cards ndpqr_new = ndpqr & ~((quad ^ tmp) >> 1) & opsHand.nd[flipOrder(ord)]; // 革命の分も消しとく
                        
                        if(!ndpqr_new){
                            //全部支配できた。勝ち
                            DERR << "J_QUAD PW0 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                            return true;
                        }else{
                            Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
                            if(!ndpqr_new_m){
                                // 1つ残し。勝ち
                                DERR << "J_QUAD PW1 " << OutCards(myHand.cards) << " , " << OutCards(opsHand.cards) << endl;
                                return true;
                            }
                            // 2つ以上残っている。ジョーカーを既に使ってしまったので勝てない。
                        }
                    }
                }
            }
        }
        return false;
    }
    
    
    template<int IS_NF>
    bool checkHandBNPW(const int depth, MoveInfo *const buf, const MoveInfo mv,
                       const Hand& myHand, const Hand& opsHand,
                       const Board argBd, const uint32_t al, const uint32_t aw){
        bool res;
        if(argBd.isNF()){
            res = checkHandBNPW<_YES>(depth, buf, mv, myHand, opsHand, argBd, al, aw);
        }else{
            res = checkHandBNPW<_NO>(depth, buf, mv, myHand, opsHand, argBd, al, aw);
        }
        return res;
    }
    
    template<>
    bool checkHandBNPW<_YES>(const int depth, MoveInfo *const buf, const MoveInfo mv,
                             const Hand& myHand, const Hand& opsHand,
                             const Board argBd, const uint32_t al, const uint32_t aw){
        // 支配的でないことは前提とする
        if(mv.isPASS()){ return false; }
        int rank = mv.rank();
        int curOrder = argBd.prmOrder();
        Cards ops8 = opsHand.cards & CARDS_8;
        
        if(aw <= mv.qty()){
            return false;
        }
        
        uint32_t nfmin = min(al, aw - mv.qty());
        
        if(mv.isSingle()){
            if(aw <= 1){ return false; }
            if(mv.isSingleJOKER()){ return false; }
            if(myHand.containsJOKER() && (!opsHand.containsS3())){ // まずジョーカーを検討
                if(ops8){ if(isValidGroupRank(RANK_8, curOrder, rank)){ return false; }}
                if(myHand.qty == mv.qty() + 1){ return true; }
                Hand nextHand;
                makeMove1stHalf(myHand, &nextHand, mv.mv());
                nextHand.makeMove1stHalf(MOVE_SINGLEJOKER, CARDS_JOKER, 1);
                if(judgeHandMate<_YES, _NO>(depth, buf, nextHand, opsHand, OrderToNullBoard(curOrder), nfmin, nfmin)){
                    return true;
                }
                if(nextHand.containsS3()){
                    if(nextHand.qty == 1){ return true; }
                    nextHand.makeMove1stHalf(MOVE_S3FLUSH, CARDS_S3, 1);
                    if(judgeHandMate<_YES, _NO>(depth, buf, nextHand, opsHand, OrderToNullBoard(curOrder), nfmin, nfmin)){
                        return true;
                    }
                }
            }
            // 他のシングルを検討
        }
        
        //ここからもう計算量度外視(暫定的処置)
        /*if( !mv.isSeq()
         && !mv.flipsTmpOrder()
         ){//階段、オーダー変更はめんどいしあまりないのでやらない
         
         //8切りで返される可能性があればダメ
         if( !dominatesCards(mv,opsHand.cards & (CARDS_8|CARDS_JOKER)) ){
         return false;
         }
         
         //上に出せる着手を全て列挙
         int NMoves=genMove(buf,myHand.cards,argBd);
         for(int m=NMoves-1;m>=0;--m){
         const MoveInfo& tmp=buf[m];
         
         */
        
        
        return false;
    }
    
    
    template<>
    bool checkHandBNPW<_NO>(const int depth,MoveInfo *const buf,const MoveInfo mv,const Hand& myHand,const Hand& opsHand,const Board argBd,const uint32_t al,const uint32_t aw){
        return false;
    }
    
    
    
    // 必勝判定
    
    template<int IS_NF, int IS_UNRIVALED>
    bool judgeHandMate(const int depth, MoveInfo *const buf,
                       const Hand& myHand, const Hand& opsHand,
                       const Board argBd, const uint32_t al, const uint32_t aw){
        //DERR << "judge" << endl;
        if(IS_NF == _YES && judgeMate_Easy_NF(myHand)){
            return true; // 簡単詰み
        }else{
            // 中程度詰み
            if(IS_NF){
                if(judgeHandPW_NF(myHand, opsHand, argBd)){
                    return true;
                }
            }
            if(IS_NF != _YES || depth > 0){
                int NMoves = genMove<IS_NF>(buf, myHand, argBd);
                if(searchHandMate<IS_NF, IS_UNRIVALED>(depth, buf, NMoves, myHand, opsHand, argBd, al, aw) != -1){
                    return true;
                }
                
            }else{
                // 階段パターンのみ検討
                // 階段と革命が絡む場合は勝ちと判定出来ない場合が多い
                if(myHand.seq){
                    int NMoves = genAllSeq(buf, myHand.cards);
                    for(int m = 0; m < NMoves; ++m){
                        const MoveInfo& mv = buf[m];
                        if(mv.qty() >= myHand.qty){
                            return true;
                        }else{
                            Board nbd = argBd;
                            Hand nextHand;
                            
                            makeMove1stHalf(myHand, &nextHand, mv.mv());
                            
                            nbd.procOrder(mv.mv());
                            if(!depth && mv.qty() > 4){
                                // 5枚以上の階段は支配としとく
                                if(judgeHandPW_NF(nextHand, opsHand, argBd)
                                   ||
                                   judgeHandPW_NF(nextHand, opsHand, (uint32_t)argBd ^ (MOVE_FLAG_TMPORD | MOVE_FLAG_PRMORD))
                                   ){
                                    // いずれかのオーダーで必勝ならOK
                                    return true;
                                }
                            }else{
                                if(dominatesHand(mv.mv(), opsHand, argBd)){
                                    if(judgeHandPW_NF(nextHand, opsHand, argBd)){
                                        return true;
                                    }
                                }else{
                                    if(judgeHandPPW_NF(nextHand.cards, nextHand.pqr, nextHand.jk, opsHand.nd, argBd)){
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
    
    template<int IS_NF, int IS_UNRIVALED>
    bool checkHandMate(const int depth, MoveInfo *const buf, MoveInfo& mv,
                       const Hand& myHand, const Hand& opsHand,
                       const Board argBd, const uint32_t al, const uint32_t aw){
        if(mv.isPASS()){ return false; }
        //DERR << "CHECK - " << argMove << " " << IS_NF << ", " << IS_UNRIVALED << endl;
        //DERR << depth << endl;
        // 通常
        if(mv.qty() >= myHand.qty){
            return true;
        }
        if(dominatesHand(mv.mv(), opsHand, argBd)){ // 支配
            
            mv.setDO(); // 支配フラグ付加
            
            Board bd = argBd;
            bd.proc(mv.mv());
            Hand nextHand;

            makeMove1stHalf(myHand, &nextHand, mv.mv());
            
            // セルフフォロー
            
            
            // 永続的パラメータ変更を起こす場合はBNPW判定を続け、起こさない場合はPWのみ検討
            bd.flush();
            uint32_t mn = min(al, aw);
            if(judgeHandMate<_YES, _NO>(mv.changesPrmState() ? depth : 0, buf, nextHand, opsHand, bd, mn, mn)){
                return true;
            }
            if(mv.isSingleJOKER() && nextHand.containsS3()){
                nextHand.makeMove1stHalf(MOVE_S3FLUSH);
                return judgeHandMate<_YES, _NO>(0, buf, nextHand, opsHand, bd, mn, mn);
            }
        }else{ // 支配しない
            
            if(depth){
                // S3分岐必勝を検討
                // awakeな相手の最小枚数2以上、ジョーカー以外に返せる着手が存在しない場合、
                // ジョーカー->S3の場合とそのまま流れた場合にともに必勝なら必勝(ぱおーん氏の作より)
                if(mv.qty() == 1
                   && aw > 1
                   && containsS3(myHand.cards)
                   && !containsS3(mv.cards())
                   ){
                    Cards zone = ORToGValidZone(argBd.tmpOrder(), mv.rank());
                    if(argBd.locksSuits(mv.mv())){
                        zone &= SuitsToCards(mv.suits());
                    }
                    if(!(zone & opsHand.cards)){ // ジョーカー以外は出せない
                        
                        Hand nextHand;
                        makeMove1stHalf(myHand, &nextHand, mv.mv());
                        
                        if(judgeHandPW_NF(nextHand, opsHand, argBd)){ // 流れた場合
                            
                            nextHand.makeMove1stHalf(MOVE_S3FLUSH, CARDS_S3, 1);
                            
                            if(judgeHandPW_NF(nextHand, opsHand, argBd)){ // S3で返した場合
                                // 両方で勝ったので必勝
                                DERR << "BRPW(S3)!!!" << endl;
                                //getchar();
                                return true;
                            }
                        }
                    }
                }
                
                
                
                // BNPWを検討
                if(checkHandBNPW<IS_NF>(depth - 1, buf, mv, myHand, opsHand, argBd, al, aw)){
                    DERR << "BNPW - " << mv << " ( " << aw << " ) " << endl;
                    //getchar();
                    return true;
                }
            }
        }
        /*if(ret){
         DERR << "CM" << endl << myHand << endl << opsHand << endl;
         }*/
        return false;
    }
    
    template<>
    bool checkHandMate<_NO, _YES>(const int depth, MoveInfo *const buf, MoveInfo& mv,
                                  const Hand& myHand, const Hand& opsHand,
                                  const Board argBd, const uint32_t al, const uint32_t aw){
        //DERR << "CHECK - " << argMove << " " << _NO << ", " << _YES << endl;
        // セルフフォロー時
        // 支配チェックなしでOK
        
        mv.setDO(); // 支配フラグ付加
        
        bool ret = false;
        if(mv.isPASS()){
            Board bd = argBd;
            bd.flush();
            uint32_t mn = min(al, aw);
            ret = judgeHandMate<_YES, _NO>(depth, buf, myHand, opsHand, bd, mn, mn);
        }else{
            if(mv.qty() >= myHand.qty){ return true; }
            Hand nextHand;
            Board bd = argBd;
            
            makeMove1stHalf(myHand, &nextHand, mv.mv());
            
            if(dominatesCards(mv.mv(), nextHand.cards, argBd)){ // 自己支配
                
                mv.setDM(); // 支配フラグ付加
                
                bd.procAndFlush(mv.mv());
                ret = judgeHandMate<_YES,_NO>(depth, buf, nextHand, opsHand, bd, al, aw);
            }else{
                bd.proc(mv.mv());
                ret = judgeHandMate<_NO,_YES>(depth, buf, nextHand, opsHand, bd, al, aw);
            }
        }
        
        return ret;
    }
    
    bool checkHandMate(const int depth, MoveInfo *const buf, MoveInfo& mv,
                       const Hand& myHand, const Hand& opsHand,
                       const Board bd, const FieldAddInfo& fInfo,
                       const uint32_t al, const uint32_t aw){
        bool mate;
        if(bd.isNF()){
            mate = checkHandMate<_YES, _NO>(depth, buf, mv, myHand, opsHand, bd, al, aw);
        }else{
            if(fInfo.isUnrivaled()){
                mate = checkHandMate<_NO, _YES>(depth, buf, mv, myHand, opsHand, bd, al, aw);
            }else{
                mate = checkHandMate<_NO, _NO>(depth, buf, mv, myHand, opsHand, bd, al, aw);
            }
        }
        return mate;
    }
    
    
    template<int IS_NF,int IS_UNRIVALED>
    int searchHandMate(const int depth, MoveInfo *const buf, const int NMoves,
                       const Hand& myHand, const Hand& opsHand, const Board argBd, const uint32_t al, const uint32_t aw){
        // 必勝手探し
        for(int m = NMoves - 1; m >= 0; --m){
            if(buf[m].qty() == myHand.qty){ return m; }
        }
        for(int m = NMoves - 1; m >= 0; --m){
            if(checkHandMate<IS_NF, IS_UNRIVALED>(depth, buf + NMoves, buf[m], myHand, opsHand, argBd, al, aw)){ return m; }
        }
        return -1;
    }
    
    
    // ルートノード用の必勝判定関数
    // 速度度外視で丁寧に読む
    // 作成中...
    /*
     bool judgeRootMate( MoveInfo *const buf,
     const Hand& myHand, 
     const Hand& posHand[N_PLAYERS],
     const Board bd,
     const FieldAddInfo& fInfo){
     
     //着手生成
     int NMoves=genMove( buf, myHand.cards, bd );
     
     for( int m=NMoves-1; m>=0; --m ){
     MoveInfo& tmp=buf[m];
     
     if( checkHandMate( buf, tmp, myHand, posHand, bd, fInfo )){
     return true;
     }
     }
     return false;
     }
     
     bool checkRootMate( MoveInfo *const buf, 
     MoveInfo& mv,
     const Hand& myHand, 
     const Hand& posHand[N_PLAYERS],
     const Board bd,
     const FieldAddInfo& fInfo){
     PW:
     {
     //完全勝利の判定
     int doth,dm;
     if( fInfo.is)
     
     for( int p=0, p<N_PLAYERS; ++p ){
     if( p != player_num ){
     if( dominatesHand( mv.mv() ), posHand )){
     }
     
     
     if( mv.isDO() ){
     if( mv.isDM() ){
     //確実に流れる
     
     
     }else{
     //フォロー
     
     }
     }
     }
     BN:
     if( !mv.isDO() && !mv.isDM() && !mv.isPASS() && !mv.isSeq() && !mv.flipsTmpOrder() ){
     //BN必勝確認
     //革命や階段はあまり無いので無視
     Board nbd=bd;
     bd.proc(mv);
     
     int NMoves=genMove( buf, myHand.cards, bd );
     
     for( int m=NMoves-1; m>=0; --m){
     MoveInfo& mv1=buf[m];
     for( int p=0; p<N_PLAYERS; ++p ){
     if( dominatesHand() ){//今の手か、次の手のどちらかで支配を確認
     }
     BR:
     
     }
     }
     }
     }
     
     
     */
}

#endif // UECDA_LOGIC_MATE_HPP_
