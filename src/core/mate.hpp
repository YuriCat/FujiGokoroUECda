#pragma once

// 詰み(Mate)判定

#include "daifugo.hpp"
#include "hand.hpp"
#include "action.hpp"
#include "dominance.hpp"

namespace UECda {
    
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
    
    namespace Mate {
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
    bool checkHandBNPW(const int, MoveInfo *const, const MoveInfo,
                       const Hand&, const Hand&, const Board&, const FieldAddInfo&);
    
    // checkmate
    template<int IS_NF = _BOTH, int IS_UNRIVALED = _BOTH>
    bool judgeHandMate(const int, MoveInfo *const buf,
                       const Hand&, const Hand&, const Board&, const FieldAddInfo&);
    
    template<int IS_NF = _BOTH, int IS_UNRIVALED = _BOTH>
    bool checkHandMate(const int, MoveInfo *const, MoveInfo&,
                       const Hand&, const Hand&, const Board&, const FieldAddInfo&);
    
    template<int IS_NF = _BOTH, int IS_UNRIVALED = _BOTH>
    int searchHandMate(const int, MoveInfo *const, const int,
                       const Hand&, const Hand&, const Board&, const FieldAddInfo&);
    
    
    // 汎用関数
    bool judgeMate_1M_NF(const Hand& myHand) {
        // 空場、1手詰み（つまり出して上がり,確実にPW）
        // 8切り関係の3手詰み等も含めた高速化は別関数にて
        if (myHand.qty <= 1) return true;
        Cards pqr = myHand.pqr;
        if (myHand.containsJOKER()) { // ジョーカーあり
            if (!any2Cards(pqr)) return true;
            if (myHand.seq) {
                if (myHand.qty == 3) return true;
                if (canMakeJokerSeq(myHand.cards.plain(), myHand.jk, myHand.qty)) return true;
            }
        } else {
            if (!any2Cards(pqr)) return true;
            if (myHand.seq) {
                if (myHand.qty == 3) return true;
                if (canMakePlainSeq(myHand.cards, myHand.qty)) return true;
            }
        }
        return false;
    }
    
    bool judgeMate_Easy_NF(const Hand& myHand) {
        // とりあえず高速でNF_Mateかどうか判断したい場合
        if (myHand.qty <= 1) return true;
        Cards pqr = myHand.pqr;
        if (!any2Cards(maskCards(pqr, CARDS_8 | CARDS_JOKER))) return true;
        if (myHand.containsJOKER()) { // ジョーカーあり
            if (myHand.seq) {
                if (myHand.qty == 3) return true;
                if (canMakeJokerSeq(myHand.cards.plain(), myHand.jk, myHand.qty)) return true;
            }
        } else {
            if (myHand.seq) {
                if (myHand.qty == 3) return true;
                if (canMakePlainSeq(myHand.cards, myHand.qty)) return true;
            }
        }
        return false;
    }
    
    bool judgeHandPPW_NF(const Cards cards, const Cards pqr, const int jk,
                         const Cards *const nd, const Board& b) {
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
        
        const int ord = b.tmpOrder();
        
        const Cards ndpqr = pqr & nd[ord] & ~CARDS_8; // 支配出来ていないpqr
        if (!ndpqr) {
            // 革命を返される可能性のみ考慮しておく。
            if (pqr & PQR_4 & ~CARDS_8) {
                Cards quad = pqr & PQR_4 & ~CARDS_8 & ~ndpqr;
                if (quad) {
                    // このとき8以外の革命でターンをとる予定
                    if (quad & nd[flipOrder(ord)]) {
                        // 革命でターンが取れない。
                        if (!jk) {
                            return false;
                        } else {
                            // 革命にジョーカーを加えれば良い。
                            // 革命2つの場合には間違い
                        }
                    }
                }
            }
            DERR << "PPW " << cards << endl;
            return true;
        } else {
            Cards ndpqr_m = ndpqr & (ndpqr - 1ULL);
            if (!ndpqr_m) {
                // ndpqrが1ビット
                // ジョーカーを加えて必勝になるか確かめる
                if (jk) {
                    if (ndpqr & PQR_4) {
                        // 5枚出しで勝てる
                        DERR << "PPW (QUIN) " << cards << endl;
                        return true;
                    } else {
                        // ndと交差しなければ勝ち
                        if (!((ndpqr << 1) & nd[ord])) {
                            DERR << "PPW (+JK) " << cards << endl;
                            return true;
                        }
                        // スペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                        if (ndpqr == CARDS_MIN && containsS3(cards)) {
                            DERR << "PPW (JK-S3) " << cards << endl;
                            return true;
                        }
                    }
                }
            }
            // 革命によって勝てないか考える
            if (pqr & PQR_4) {
                // ジョーカーを使わない革命あり。
                Cards quad = pqr & PQR_4;
                Cards ndpqr_new = ndpqr & ~quad & nd[flipOrder(ord)];
                
                if (jk) {
                    if (!ndpqr_new) {
                        // 革命がだめでも5枚出しがある。勝ち
                        DERR << "PPW (QUIN) " << cards << endl;
                        return true;
                    } else {
                        if (!(ndpqr_new & (ndpqr_new - 1ULL))) {
                            // 1つ残し
                            // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                            // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                            if (!(quad & nd[flipOrder(ord)] & ~CARDS_8)) {
                                // 革命でターンがとれる
                                // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                                // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければよい
                                
                                // いずれかのオーダーのndと交差しなければ勝ち
                                if ((!((ndpqr_new << 1) & nd[ord]))
                                   || (!((ndpqr_new << 1) & nd[flipOrder(ord)]))) {
                                    DERR << "NJ_QUAD PPW (+JK) " << cards << endl;
                                    return true;
                                }
                                // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし。
                            }
                        }
                    }
                } else {
                    //ジョーカーが無いので、革命が支配的かつ他のも全て支配的が必須
                    if (!ndpqr_new && !(quad & nd[ flipOrder(ord)] & ~CARDS_8)) {
                        DERR << "NJ_QUAD PPW " << cards << endl;
                        return true;
                    }
                }
            } else if (jk && (pqr & PQR_3)) {
                //ジョーカーを使えば革命あり。
                //ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
                //まず、この革命がターンを取れるか
                Cards quad = (pqr & PQR_3) << 1;
                //トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
                Cards tmp = quad & nd[flipOrder(ord)] & ~CARDS_8;
                if (quad != tmp) {
                    //革命でターンがとれる
                    //このとき、まだターンが取れていなかったやつを逆オーダーで考える
                    Cards ndpqr_new = ndpqr & ~((quad ^ tmp) >> 1) & nd[flipOrder(ord)];//革命の分も消しとく
                    if (!ndpqr_new) {
                        //全部支配できた場合のみOK
                        DERR << "J_QUAD PPW " << cards << endl;
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    bool judgeHandPW_NF(const Hand& myHand, const Hand& opsHand, const Board& b) {
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
        
        const int ord = b.tmpOrder();
        
        const Cards ndpqr = myHand.pqr & opsHand.nd[ord] & ~CARDS_8; // 支配出来ていないpqr
        if (!ndpqr) {
            // このときほぼ必勝なのだが、一応4枚グループが2つある場合にはそうでないことがある
            DERR << "PW0 " << myHand.cards << " , " << opsHand.cards << endl;
            Cards pqr4 = myHand.pqr & PQR_4;
            if (!any2Cards(pqr4)) return true; // ４枚グループが1つ以下
            //以下4枚グループ2つ。ほぼないので適当で良い。
            if (myHand.jk) return true; //革命のうち1つにジョーカーを加えれば勝ち
            if (!any2Cards(pqr4 & opsHand.nd[ord])) return true;
            return false;
        } else {
            Cards ndpqr_m = ndpqr & (ndpqr - 1ULL);
            
            if (!ndpqr_m) {
                // ndpqrが1ビットだけ
                // このときも基本的に必勝。
                // 革命を返される可能性のみ考慮しておく。
                if (myHand.pqr & PQR_4 & ~CARDS_8) {
                    Cards quad = myHand.pqr & PQR_4 & ~CARDS_8 & ~ndpqr;
                    if (quad) {
                        // このとき8以外の革命でターンをとる予定
                        if (quad & opsHand.nd[flipOrder(ord)]) {
                            // 革命でターンが取れない。
                            if (!myHand.jk) {
                                return false;
                            } else {
                                // 革命にジョーカーを加えれば良い。
                                // 革命2つの場合には間違い
                            }
                        }
                    }
                }
                // 一応OKとする
                DERR << "PW1 " << myHand.cards << " , " << opsHand.cards << endl;
                return true;
            } else {
                // 支配出来ない部分が2つ以上
                if (myHand.seq) { // 階段である場合がある。
                    if (myHand.qty == 3) { // 3階段1つ
                        DERR << "SEQ PW0 " << myHand.cards << " , " << opsHand.cards << endl;
                        return true;
                    }
                    
                    // 階段が支配的か判断
                    /*IntCard ic = CardsToLowestIntCard(myHand.seq);
                    int seqRank = IntCardToRank(ic);
                    uint32_t seqSuit = CardsRankToSuits(myHand.seq, seqRank);
                    Hand nextHand;
                    
                    if (isEightSeqRank(tmpOrder, seqRank, 3)) { // 支配的
                        // 出した後の場でもう一度必勝判定
                        if (hand.p4 & hand.p8) { // プレーン階段
                            makeMove1stHalf(hand, &nextHand, SequenceMove(3, r, seqSuit));
                        } else {
                            makeMove1stHalf(hand, &nextHand)
                        }
                    } else { // 支配的でない
                        // これを最後に出すとしてPPW判定
                        
                    }*/
                    // シングルやグループで出しても支配出来るランクの階段を使う場合は難しい。
                    // 3階段1つ以外の場合、合法着手生成によって確かめる(暫定的処置)
                } else {
                    // ここから長いので、ジョーカーも革命も階段も無い場合(大部分)はさっさと帰る
                    if (!myHand.jk) {
                        if (!(myHand.pqr & PQR_4)) return false;
                    }
                }
                
                if (!(ndpqr_m & (ndpqr_m - 1ULL))) {
                    // 2ビットのみ。
                    // いずれかにジョーカーを加えて必勝になるか確かめる
                    if (myHand.jk) {
                        if (ndpqr & PQR_4) {
                            // 5枚出しで勝てる
                            DERR << "PW1 (QUAD+JK) " << myHand.cards << " , " << opsHand.cards << endl;
                            return true;
                        } else {
                            Cards h = ndpqr;
                            Cards l = popLow(&h);
                            
                            // どちらかとndが交差しなければ勝ち ただし革命の場合は逆オーダー
                            bool jh = (h << 1) & opsHand.nd[(h & PQR_3) ? (1 - ord) : ord];
                            bool jl = (l << 1) & opsHand.nd[(l & PQR_3) ? (1 - ord) : ord];
                            if (!jh || !jl) {
                                DERR << "PW1 (+JK) " << myHand.cards << " , " << opsHand.cards << endl;
                                return true;
                            }
                            
                            // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                            if (l == (CARDS_3 & PQR_1) && containsS3(myHand.cards)) {
                                // 3はランク最低なのでlだろう
                                DERR << "PW1 (JK-S3) " << myHand.cards << " , " << opsHand.cards << endl;
                                return true;
                            }
                        }
                    }
                }
                
                // 革命によって勝てないか考える
                if (myHand.pqr & PQR_4) {
                    // ジョーカーを使わない革命あり。
                    Cards quad = myHand.pqr & PQR_4;
                    Cards nd_quad = ndpqr & quad & opsHand.nd[flipOrder(ord)];
                    Cards ndpqr_new = ndpqr & ~quad & opsHand.nd[flipOrder(ord)];
                    
                    if (nd_quad) {
                        // 全部支配できた。勝ち
                        DERR << "NJ_QUAD PW0~1 " << myHand.cards << " , " << opsHand.cards << endl;
                        return true;
                    }
                    Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
                    if (myHand.jk) {
                        if (!ndpqr_new_m) {
                            // 1つ残し。5枚出しすればよい。勝ち
                            DERR << "NJ_QUIN PW1 " << myHand.cards << " , " << opsHand.cards << endl;
                            return true;
                        } else {
                            // 2つ以上残っている。
                            if (!(ndpqr_new_m & (ndpqr_new_m - 1ULL))) {
                                // 2つだけ
                                // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                                // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                                if (!(quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8)) {
                                    // 革命でターンがとれる
                                    // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                                    
                                    // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければよい
                                    Cards h = ndpqr_new;
                                    Cards l = popLow(&h);
                                    
                                    // どちらかが、いずれかのオーダーのndと交差しなければ勝ち
                                    if (!((h << 1) & opsHand.nd[ord]) || !((h << 1) & opsHand.nd[flipOrder(ord)])
                                       || !((l << 1) & opsHand.nd[ord]) || !((l << 1) & opsHand.nd[flipOrder(ord)])) {
                                        DERR << "NJ_QUAD PW1 (+JK) " << myHand.cards << " , " << opsHand.cards << endl;
                                        return true;
                                    }
                                    
                                    // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし。
                                    // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                                    /*if ( l == CARDS_C3 && containsS3( myHand.cards )) {
                                     //3はランク最低なのでlだろう
                                     DERR<<"NJ_QUAD PW1 (JK-S3)"<<endl;
                                     return true;
                                     }*/
                                }
                            }
                        }
                    } else {
                        if (!ndpqr_new_m) {
                            // 1つ残し。革命でターンが取れるなら勝ち。
                            if (!(quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8)) {
                                DERR << "NJ_QUAD PW1 " << myHand.cards << " , " << opsHand.cards << endl;
                                return true;
                            }
                        }
                    }
                } else if (myHand.jk && (myHand.pqr & PQR_3)) {
                    // ジョーカーを使えば革命あり。
                    // ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
                    // まず、この革命がターンを取れるか
                    Cards quad = (myHand.pqr & PQR_3) << 1;
                    // トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
                    Cards tmp = quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8;
                    if (quad != tmp) {
                        // 革命でターンがとれる
                        // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                        Cards ndpqr_new = ndpqr & ~((quad ^ tmp) >> 1) & opsHand.nd[flipOrder(ord)]; // 革命の分も消しとく
                        
                        if (!ndpqr_new) {
                            //全部支配できた。勝ち
                            DERR << "J_QUAD PW0 " << myHand.cards << " , " << opsHand.cards << endl;
                            return true;
                        } else {
                            Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
                            if (!ndpqr_new_m) {
                                // 1つ残し。勝ち
                                DERR << "J_QUAD PW1 " << myHand.cards << " , " << opsHand.cards << endl;
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
    
    bool checkHandBNPW(const int depth, MoveInfo *const buf, const MoveInfo mv,
                             const Hand& myHand, const Hand& opsHand,
                             const Board& b, const FieldAddInfo& fieldInfo) {
        // 間に他プレーヤーの着手を含んだ必勝を検討
        // 支配的でないことは前提とする
        if (!b.isNF()) return false; // 未実装
        if (mv.isPASS()) return false; // パスからのBNPWはない
        int curOrder = b.prmOrder();
        Cards ops8 = opsHand.cards & CARDS_8;
        uint32_t aw = fieldInfo.getMinNCardsAwake();
        
        // 相手に間で上がられる可能性がある
        if (aw <= mv.qty()) return false;
        
        FieldAddInfo nextFieldInfo;
        
        if (mv.isSingle()) {
            if (mv.isSingleJOKER()) {
                // シングルジョーカーは少なくともBNPWではない
                return false;
            }
            if (myHand.containsJOKER() && !opsHand.containsS3()) { // まずジョーカーを検討
                if (ops8 && isValidGroupRank(RANK_8, curOrder, mv.rank())) {
                    // 相手に8切りで返される可能性があればだめ
                    return false;
                }
                if (myHand.qty == mv.qty() + 1) {
                    // 残り1枚がジョーカーなら勝ち
                    return true;
                }
                // 簡単な勝ちの条件にはまらなかったので手札を更新して空場必勝を確認
                Hand nextHand;
                makeMove1stHalf(myHand, &nextHand, mv.mv());
                nextHand.makeMove1stHalf(MOVE_SINGLEJOKER, CARDS_JOKER, 1);
                flushFieldAddInfo(fieldInfo, &nextFieldInfo);
                nextFieldInfo.setMinNCards(aw - mv.qty());
                nextFieldInfo.setMinNCardsAwake(aw - mv.qty());
                if (judgeHandMate<_YES, _NO>(depth, buf, nextHand, opsHand, OrderToNullBoard(curOrder), nextFieldInfo)) {
                    return true;
                }
                // ジョーカー -> S3 からの空場必勝を確認
                if (nextHand.containsS3()) {
                    if (nextHand.qty == 1) return true;
                    nextHand.makeMove1stHalf(MOVE_S3FLUSH, CARDS_S3, 1);
                    if (judgeHandMate<_YES, _NO>(depth, buf, nextHand, opsHand, OrderToNullBoard(curOrder), nextFieldInfo)) {
                        return true;
                    }
                }
            }
            // 他のシングルを検討
            // 現時点のオーダーで最強の
        }
        
        //ここからもう計算量度外視(暫定的処置)
        /*if ( !mv.isSeq()
         && !mv.flipsTmpOrder()
         ) {//階段、オーダー変更はめんどいしあまりないのでやらない
         
         //8切りで返される可能性があればダメ
         if ( !dominatesCards(mv,opsHand.cards & (CARDS_8|CARDS_JOKER)) ) {
         return false;
         }
         
         //上に出せる着手を全て列挙
         int NMoves=genMove(buf,myHand.cards,b);
         for (int m=NMoves-1;m>=0;m--) {
         const MoveInfo& tmp=buf[m];
         
         */
        
        
        return false;
    }
    
    // 必勝判定
    
    template <int IS_NF, int IS_UNRIVALED>
    bool judgeHandMate(const int depth, MoveInfo *const buf,
                       const Hand& myHand, const Hand& opsHand,
                       const Board& b, const FieldAddInfo& fieldInfo) {
        // 簡単詰み
        if (TRI_BOOL_YES(IS_NF, b.isNF()) && judgeMate_Easy_NF(myHand)) {
            return true;
        }
        // 中程度詰み
        if (TRI_BOOL_YES(IS_NF, b.isNF()) && judgeHandPW_NF(myHand, opsHand, b)) {
            return true;
        }
        if (TRI_BOOL_NO(IS_NF, !b.isNF()) || depth > 0) {
            // depth > 0 のとき 空場でない場合は合法手生成して詳しく判定
            const int NMoves = genMove(buf, myHand, b);
            if (searchHandMate<IS_NF, IS_UNRIVALED>(depth, buf, NMoves,
                                                   myHand, opsHand, b, fieldInfo) != -1) {
                return true;
            }
        } else {
            // 階段パターンのみ検討
            // 階段と革命が絡む場合は勝ちと判定出来ない場合が多い
            if (myHand.seq) {
                const int NMoves = genAllSeq(buf, myHand.cards);
                for (int m = 0; m < NMoves; ++m) {
                    const MoveInfo& mv = buf[m];
                    if (mv.qty() >= myHand.qty) return true; // 即上がり
                    
                    Board nb = b;
                    Hand nextHand;
                    
                    makeMove1stHalf(myHand, &nextHand, mv.mv());
                    
                    nb.procOrder(mv.mv());
                    if (!depth && mv.qty() > 4) {
                        // 5枚以上の階段は支配としとく
                        if (judgeHandPW_NF(nextHand, opsHand, b)
                           || judgeHandPW_NF(nextHand, opsHand, (uint32_t)b ^ (MOVE_FLAG_TMPORD | MOVE_FLAG_PRMORD))
                           ) {
                            // いずれかのオーダーで必勝ならOK
                            return true;
                        }
                    } else {
                        if (dominatesHand(mv.mv(), opsHand, b)) {
                            if (judgeHandPW_NF(nextHand, opsHand, b)) return true;
                        } else {
                            // 支配的でない場合、この役を最後に出すことを検討
                            if (judgeHandPPW_NF(nextHand.cards, nextHand.pqr, nextHand.jk, opsHand.nd, b))return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    template <int IS_NF, int IS_UNRIVALED>
    bool checkHandMate(const int depth, MoveInfo *const buf, MoveInfo& mv,
                       const Hand& myHand, const Hand& opsHand,
                       const Board& b, const FieldAddInfo& fieldInfo) {
        
        if (TRI_BOOL_YES(IS_UNRIVALED, fieldInfo.isUnrivaled())) { // 独断場のとき
            mv.setDO(); // 支配フラグ付加
            if (mv.isPASS()) {
                Board nb = b;
                nb.flush();
                FieldAddInfo nextFieldInfo;
                flushFieldAddInfo(fieldInfo, &nextFieldInfo);
                return judgeHandMate<_YES, _NO>(depth, buf, myHand, opsHand, nb, nextFieldInfo);
            } else {
                if (mv.qty() >= myHand.qty) return true; // あがり
                Hand nextHand;
                Board bd = b;
                FieldAddInfo nextFieldInfo;
                makeMove1stHalf(myHand, &nextHand, mv.mv());
                if (dominatesCards(mv.mv(), nextHand.cards, b)) { // 自己支配
                    mv.setDM(); // 支配フラグ付加
                    bd.procAndFlush(mv.mv());
                    flushFieldAddInfo(fieldInfo, &nextFieldInfo);
                    return judgeHandMate<_YES, _NO>(depth, buf, nextHand, opsHand, bd, nextFieldInfo);
                } else { // セルフフォロー必勝を判定
                    bd.proc(mv.mv());
                    procUnrivaled(fieldInfo, &nextFieldInfo);
                    return judgeHandMate<_NO, _YES>(depth, buf, nextHand, opsHand, bd, nextFieldInfo);
                }
            }
            return false;
        }
        
        if (mv.isPASS()) {
            if (!fieldInfo.isPassDom()) {
                // パス支配でない場合は判定終了
                return false;
            } else {
                // Unrivaledでないがパス支配の場合がある?
                // パス支配の場合, 流れてからの必勝を判定
                mv.setDO(); // 支配フラグ付加
                FieldAddInfo nextFieldInfo;
                nextFieldInfo.init();
                flushFieldAddInfo(fieldInfo, &nextFieldInfo);
                return judgeHandMate<_YES, _NO>(depth, buf, myHand, opsHand,
                                                OrderToNullBoard(b.prmOrder()), nextFieldInfo);
            }
        }
        // DERR << "CHECK - " << argMove << " " << IS_NF << ", " << IS_UNRIVALED << endl;
        // DERR << depth << endl;
        // 通常
        if (mv.qty() >= myHand.qty) return true; // 即上がり
        if (dominatesHand(mv.mv(), opsHand, b)
           || mv.qty() > fieldInfo.getMaxNCardsAwake()) { // 支配
            
            mv.setDO(); // 支配フラグ付加
            
            Board bd = b;
            bd.proc(mv.mv());
            Hand nextHand;

            makeMove1stHalf(myHand, &nextHand, mv.mv());
            
            // セルフフォロー
            
            // 自分の出した役を流してからの必勝チェック
            // 永続的パラメータ変更を起こす場合はBNPW判定を続け、起こさない場合はPWのみ検討
            bd.flush();
            FieldAddInfo nextFieldInfo;
            flushFieldAddInfo(fieldInfo, &nextFieldInfo);
            if (judgeHandMate<_YES, _NO>(mv.changesPrmState() ? depth : 0,
                                        buf, nextHand, opsHand, bd, nextFieldInfo)) {
                return true;
            }
            // 自分の出したジョーカーをS3で返してからの必勝チェック
            if (mv.isSingleJOKER() && nextHand.containsS3()) {
                nextHand.makeMove1stHalf(MOVE_S3FLUSH);
                return judgeHandMate<_YES, _NO>(0, buf, nextHand, opsHand, bd, nextFieldInfo);
            }
        } else { // 支配しない
            
            if (depth) {
                // S3分岐必勝を検討
                // awakeな相手の最小枚数2以上、ジョーカー以外に返せる着手が存在しない場合、
                // ジョーカー -> S3の場合とそのまま流れた場合にともに必勝なら必勝(ぱおーん氏の作より)
                if (mv.isSingle() // シングルのみ
                   && fieldInfo.getMinNCardsAwake() > 1 // 相手に即上がりされない
                   && containsS3(myHand.cards) // 残り手札にS3がある
                   && !mv.isEqualRankSuits(RANK_3, SUITS_S)) { // 今出す役がS3でない
                    Cards zone = ORToGValidZone(b.tmpOrder(), mv.rank());
                    if (b.locksSuits(mv.mv())) {
                        zone &= SuitsToCards(mv.suits());
                    }
                    if (!(zone & opsHand.cards)) { // ジョーカー以外は出せない
                        Hand nextHand;
                        makeMove1stHalf(myHand, &nextHand, mv.mv());
                        if (judgeHandPW_NF(nextHand, opsHand, b)) { // 流れた場合
                            nextHand.makeMove1stHalf(MOVE_S3FLUSH, CARDS_S3, 1); // S3 で進める
                            if (judgeHandPW_NF(nextHand, opsHand, b)) { // S3で返した場合
                                // 両方で勝ったので必勝
                                DERR << "BRPW(S3)!!!" << endl;
                                return true;
                            }
                        }
                    }
                }
                // BNPWを検討
                if (checkHandBNPW(depth - 1, buf, mv, myHand, opsHand, b, fieldInfo)) {
                    DERR << "BNPW - " << mv << " ( " << fieldInfo.getMinNCardsAwake() << " ) " << endl;
                    return true;
                }
            }
        }
        return false;
    }
    
    
    template <int IS_NF, int IS_UNRIVALED>
    int searchHandMate(const int depth, MoveInfo *const buf, const int NMoves,
                       const Hand& myHand, const Hand& opsHand,
                       const Board& b, const FieldAddInfo& fieldInfo) {
        // 必勝手探し
        for (int m = NMoves - 1; m >= 0; m--) {
            if (buf[m].qty() == myHand.qty) return m;
        }
        for (int m = NMoves - 1; m >= 0; m--) {
            if (checkHandMate<IS_NF, IS_UNRIVALED>(depth, buf + NMoves, buf[m],
                                                   myHand, opsHand, b, fieldInfo)) return m;
        }
        return -1;
    }
}