#pragma once

// 詰み(Mate)判定

#include "../core/daifugo.hpp"
#include "../core/hand.hpp"
#include "../core/action.hpp"
#include "../core/dominance.hpp"

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
    
    // 関数群
    // check, judgeは基本的にブール関数
    // BNPW(unrivaledな場合はBNPWではないので指定無し)
    
    // 汎用関数
    inline bool judgeMate_1M_NF(const Hand& mh) {
        // 空場、1手詰み（つまり出して上がり,確実にPW）
        // 8切り関係の3手詰み等も含めた高速化は別関数にて
        if (mh.qty <= 1) return true;
        Cards pqr = mh.pqr;
        if (mh.jk) { // ジョーカーあり
            if (!any2Cards(pqr)) return true;
            if (mh.seq) {
                if (mh.qty == 3) return true;
                if (canMakeJokerSeq(mh.cards.plain(), mh.jk, mh.qty)) return true;
            }
        } else {
            if (!any2Cards(pqr)) return true;
            if (mh.seq) {
                if (mh.qty == 3) return true;
                if (canMakePlainSeq(mh.cards, mh.qty)) return true;
            }
        }
        return false;
    }
    
    inline bool judgeMate_Easy_NF(const Hand& mh) {
        // とりあえず高速でNF_Mateかどうか判断したい場合
        if (mh.qty <= 1) return true;
        Cards pqr = mh.pqr;
        if (!any2Cards(maskCards(pqr, CARDS_8 | CARDS_JOKER))) return true;
        if (mh.jk) { // ジョーカーあり
            if (mh.seq) {
                if (mh.qty == 3) return true;
                if (canMakeJokerSeq(mh.cards.plain(), mh.jk, mh.qty)) return true;
            }
        } else {
            if (mh.seq) {
                if (mh.qty == 3) return true;
                if (canMakePlainSeq(mh.cards, mh.qty)) return true;
            }
        }
        return false;
    }
    
    inline bool judgeHandPPW_NF(const Cards cards, const Cards pqr, const int jk,
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
        const int ord = b.order();
        
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
                    if (!ndpqr_new && !(quad & nd[flipOrder(ord)] & ~CARDS_8)) {
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
    
    inline bool judgeHandPW_NF(const Hand& mh, const Hand& oh, const Board& b) {
        // 合法着手生成を用いずに判定出来る場合
        
        // ただし、
        // クアドラを2つ以上
        // トリプルを3つ以上
        // クアドラ1つ以上 && トリプル2つ以上
        // といった稀な場合には高確率で誤判定する
        
        assert(mh.exam_jk());
        assert(mh.exam_seq());
        assert(mh.exam_pqr());
        assert(oh.exam_jk());
        assert(oh.exam_seq());
        assert(oh.exam_nd());
        
        const int ord = b.order();
        
        const Cards ndpqr = mh.pqr & oh.nd[ord] & ~CARDS_8; // 支配出来ていないpqr
        if (!ndpqr) {
            // このときほぼ必勝なのだが、一応4枚グループが2つある場合にはそうでないことがある
            DERR << "PW0 " << mh.cards << " , " << oh.cards << endl;
            Cards pqr4 = mh.pqr & PQR_4;
            if (!any2Cards(pqr4)) return true; // ４枚グループが1つ以下
            //以下4枚グループ2つ。ほぼないので適当で良い。
            if (mh.jk) return true; //革命のうち1つにジョーカーを加えれば勝ち
            if (!any2Cards(pqr4 & oh.nd[ord])) return true;
            return false;
        } else {
            Cards ndpqr_m = ndpqr & (ndpqr - 1ULL);
            
            if (!ndpqr_m) {
                // ndpqrが1ビットだけ
                // このときも基本的に必勝。
                // 革命を返される可能性のみ考慮しておく。
                if (mh.pqr & PQR_4 & ~CARDS_8) {
                    Cards quad = mh.pqr & PQR_4 & ~CARDS_8 & ~ndpqr;
                    if (quad) {
                        // このとき8以外の革命でターンをとる予定
                        if (quad & oh.nd[flipOrder(ord)]) {
                            // 革命でターンが取れない。
                            if (!mh.jk) {
                                return false;
                            } else {
                                // 革命にジョーカーを加えれば良い。
                                // 革命2つの場合には間違い
                            }
                        }
                    }
                }
                // 一応OKとする
                DERR << "PW1 " << mh.cards << " , " << oh.cards << endl;
                return true;
            } else {
                // 支配出来ない部分が2つ以上
                if (mh.seq) { // 階段である場合がある。
                    if (mh.qty == 3) { // 3階段1つ
                        DERR << "SEQ PW0 " << mh.cards << " , " << oh.cards << endl;
                        return true;
                    }
                    
                    // 階段が支配的か判断
                    /*IntCard ic = CardsToLowestIntCard(mh.seq);
                    int seqRank = IntCardToRank(ic);
                    uint32_t seqSuit = CardsRankToSuits(mh.seq, seqRank);
                    Hand nmh;
                    
                    if (isEightSeqRank(order, seqRank, 3)) { // 支配的
                        // 出した後の場でもう一度必勝判定
                        if (hand.p4 & hand.p8) { // プレーン階段
                            makeMove1stHalf(hand, &nmh, SequenceMove(3, r, seqSuit));
                        } else {
                            makeMove1stHalf(hand, &nmh)
                        }
                    } else { // 支配的でない
                        // これを最後に出すとしてPPW判定
                        
                    }*/
                    // シングルやグループで出しても支配出来るランクの階段を使う場合は難しい。
                    // 3階段1つ以外の場合、合法着手生成によって確かめる(暫定的処置)
                } else {
                    // ここから長いので、ジョーカーも革命も階段も無い場合(大部分)はさっさと帰る
                    if (!mh.jk) {
                        if (!(mh.pqr & PQR_4)) return false;
                    }
                }
                
                if (!(ndpqr_m & (ndpqr_m - 1ULL))) {
                    // 2ビットのみ。
                    // いずれかにジョーカーを加えて必勝になるか確かめる
                    if (mh.jk) {
                        if (ndpqr & PQR_4) {
                            // 5枚出しで勝てる
                            DERR << "PW1 (QUAD+JK) " << mh.cards << " , " << oh.cards << endl;
                            return true;
                        } else {
                            Cards l = ndpqr.divide().lowest();
                            Cards h = ndpqr - l;
                            // どちらかとndが交差しなければ勝ち ただし革命の場合は逆オーダー
                            bool jh = (h << 1) & oh.nd[(h & PQR_3) ? (1 - ord) : ord];
                            bool jl = (l << 1) & oh.nd[(l & PQR_3) ? (1 - ord) : ord];
                            if (!jh || !jl) {
                                DERR << "PW1 (+JK) " << mh.cards << " , " << oh.cards << endl;
                                return true;
                            }
                            
                            // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                            if (l == (CARDS_3 & PQR_1) && containsS3(mh.cards)) {
                                // 3はランク最低なのでlだろう
                                DERR << "PW1 (JK-S3) " << mh.cards << " , " << oh.cards << endl;
                                return true;
                            }
                        }
                    }
                }
                
                // 革命によって勝てないか考える
                if (mh.pqr & PQR_4) {
                    // ジョーカーを使わない革命あり。
                    Cards quad = mh.pqr & PQR_4;
                    //Cards nd_quad = ndpqr & quad & oh.nd[flipOrder(ord)];
                    Cards ndpqr_new = ndpqr & ~quad & oh.nd[flipOrder(ord)];
                    
                    if (!ndpqr_new) {
                        // 全部支配できた。勝ち
                        DERR << "NJ_QUAD PW0~1 " << mh.cards << " , " << oh.cards << endl;
                        return true;
                    }
                    Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
                    if (mh.jk) {
                        if (!ndpqr_new_m) {
                            // 1つ残し。5枚出しすればよい。勝ち
                            DERR << "NJ_QUIN PW1 " << mh.cards << " , " << oh.cards << endl;
                            return true;
                        } else {
                            // 2つ以上残っている。
                            if (!(ndpqr_new_m & (ndpqr_new_m - 1ULL))) {
                                // 2つだけ
                                // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                                // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                                if (!(quad & oh.nd[flipOrder(ord)] & ~CARDS_8)) {
                                    // 革命でターンがとれる
                                    // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                                    
                                    // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければよい
                                    Cards l = ndpqr_new.divide().lowest();
                                    Cards h = ndpqr_new - l;
                                    // どちらかが、いずれかのオーダーのndと交差しなければ勝ち
                                    if (!((h << 1) & oh.nd[ord]) || !((h << 1) & oh.nd[flipOrder(ord)])
                                       || !((l << 1) & oh.nd[ord]) || !((l << 1) & oh.nd[flipOrder(ord)])) {
                                        DERR << "NJ_QUAD PW1 (+JK) " << mh.cards << " , " << oh.cards << endl;
                                        return true;
                                    }
                                    
                                    // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし。
                                    // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                                    /*if ( l == CARDS_C3 && containsS3( mh.cards )) {
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
                            if (!(quad & oh.nd[flipOrder(ord)] & ~CARDS_8)) {
                                DERR << "NJ_QUAD PW1 " << mh.cards << " , " << oh.cards << endl;
                                return true;
                            }
                        }
                    }
                } else if (mh.jk && (mh.pqr & PQR_3)) {
                    // ジョーカーを使えば革命あり。
                    // ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
                    // まず、この革命がターンを取れるか
                    Cards quad = (mh.pqr & PQR_3) << 1;
                    // トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
                    Cards tmp = quad & oh.nd[flipOrder(ord)] & ~CARDS_8;
                    if (quad != tmp) {
                        // 革命でターンがとれる
                        // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                        Cards ndpqr_new = ndpqr & ~((quad ^ tmp) >> 1) & oh.nd[flipOrder(ord)]; // 革命の分も消しとく
                        
                        if (!ndpqr_new) {
                            //全部支配できた。勝ち
                            DERR << "J_QUAD PW0 " << mh.cards << " , " << oh.cards << endl;
                            return true;
                        } else {
                            Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
                            if (!ndpqr_new_m) {
                                // 1つ残し。勝ち
                                DERR << "J_QUAD PW1 " << mh.cards << " , " << oh.cards << endl;
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
    
    // 必勝判定
    inline int searchHandMate(const int, Move *const, const int,
                              const Hand&, const Hand&,  const Board&);

    inline bool judgeHandMate(const int depth, Move *const mbuf,
                              const Hand& mh, const Hand& oh, const Board& b) {
        // 簡単詰み
        if (b.isNull() && judgeMate_Easy_NF(mh)) return true;
        // 中程度詰み
        if (b.isNull() && judgeHandPW_NF(mh, oh, b)) return true;

        if (!b.isNull() || depth > 0) {
            // depth > 0 のとき 空場でない場合は合法手生成して詳しく判定
            const int NMoves = genMove(mbuf, mh, b);
            if (searchHandMate(depth, mbuf, NMoves, mh, oh, b) != -1) return true;
        } else {
            // 階段パターンのみ検討
            // 階段と革命が絡む場合は勝ちと判定出来ない場合が多い
            if (mh.seq) {
                const int NMoves = genAllSeq(mbuf, mh.cards);
                for (int i = 0; i < NMoves; i++) {
                    const Move& m = mbuf[i];
                    if (m.qty() >= mh.qty) return true; // 即上がり
                    
                    Hand nmh;
                    makeMove1stHalf(mh, &nmh, m);
                    if (!depth && m.qty() > 4) {
                        Board nb = b;
                        nb.procOrder(m);
                        // 5枚以上の階段は支配としとく
                        if (judgeHandPW_NF(nmh, oh, b) || judgeHandPW_NF(nmh, oh, nb)) {
                            // いずれかのオーダーで必勝ならOK
                            return true;
                        }
                    } else {
                        if (dominatesHand(m, oh, b)) {
                            if (judgeHandPW_NF(nmh, oh, b)) return true;
                        } else {
                            // 支配的でない場合、この役を最後に出すことを検討
                            if (judgeHandPPW_NF(nmh.cards, nmh.pqr, nmh.jk, oh.nd, b)) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    inline bool checkHandBNPW(const int depth, Move *const mbuf, const Move m,
                              const Hand& mh, const Hand& oh, const Board& b) {
        // 間に他プレーヤーの着手を含んだ必勝を検討
        // 支配的でないことは前提とする
        if (!b.isNull()) return false; // 未実装
        if (m.isPASS()) return false; // パスからのBNPWはない
        int curOrder = b.prmOrder();
        Cards ops8 = oh.cards & CARDS_8;
        uint32_t aw = b.getMinNCardsAwake();
        
        // 相手に間で上がられる可能性がある
        if (aw <= m.qty()) return false;
        
        if (m.isSingle()) {
            if (m.isSingleJOKER()) {
                // シングルジョーカーは少なくともBNPWではない
                return false;
            }
            if (mh.jk && !containsS3(oh.cards)) { // まずジョーカーを検討
                if (ops8 && isValidGroupRank(RANK_8, curOrder, m.rank())) {
                    // 相手に8切りで返される可能性があればだめ
                    return false;
                }
                if (mh.qty == m.qty() + 1) {
                    // 残り1枚がジョーカーなら勝ち
                    return true;
                }
                // 簡単な勝ちの条件にはまらなかったので手札を更新して空場必勝を確認
                Hand nmh;
                makeMove1stHalf(mh, &nmh, m);
                Move sj; sj.setSingleJOKER();
                nmh.makeMove1stHalf(sj, CARDS_JOKER, 1);
                Board nb = b;
                nb.flush(); nb.flushInfo();
                nb.setMinNCards(aw - m.qty());
                nb.setMinNCardsAwake(aw - m.qty());
                if (judgeHandMate(depth, mbuf, nmh, oh, nb)) return true;
                // ジョーカー -> S3 からの空場必勝を確認
                if (containsS3(nmh.cards)) {
                    if (nmh.qty == 1) return true;
                    Move s3; s3.setSingle(INTCARD_S3);
                    nmh.makeMove1stHalf(s3, CARDS_S3, 1);
                    if (judgeHandMate(depth, mbuf, nmh, oh, nb)) return true;
                }
            }
            // 他のシングルを検討
            // 現時点のオーダーで最強の
        }
        
        //ここからもう計算量度外視(暫定的処置)
        /*if ( !m.isSeq()
         && !m.flipsTmpOrder()
         ) {//階段、オーダー変更はめんどいしあまりないのでやらない
         
         //8切りで返される可能性があればダメ
         if ( !dominatesCards(m,oh.cards & (CARDS_8|CARDS_JOKER)) ) {
         return false;
         }
         
         //上に出せる着手を全て列挙
         int NMoves=genMove(buf,mh.cards,b);
         for (int m=NMoves-1;m>=0;m--) {
         const Move& tmp=buf[m];
         
         */
        return false;
    }

    inline bool checkHandMate(const int depth, Move *const mbuf, Move& m,
                              const Hand& mh, const Hand& oh, const Board& b) {
        
        if (b.isUnrivaled()) { // 独断場のとき
            m.setDO(); // 支配フラグ付加
            if (m.isPASS()) {
                Board nb = b;
                nb.flush(); nb.flushInfo();
                return judgeHandMate(depth, mbuf, mh, oh, nb);
            } else {
                if (m.qty() >= mh.qty) return true; // あがり
                Hand nmh;
                Board nb = b;
                makeMove1stHalf(mh, &nmh, m);
                if (dominatesCards(m, nmh.cards, b)) { // 自己支配
                    m.setDM(); // 支配フラグ付加
                    nb.procAndFlush(m); nb.flushInfo();
                    return judgeHandMate(depth, mbuf, nmh, oh, nb);
                } else { // セルフフォロー必勝を判定
                    nb.proc(m);
                    nb.initTmpInfo();
                    nb.setUnrivaled();
                    return judgeHandMate(depth, mbuf, nmh, oh, nb);
                }
            }
            return false;
        }
        
        if (m.isPASS()) {
            if (!b.isPassDom()) return false; // パス支配でない場合は判定終了
            // Unrivaledでないがパス支配の場合がある?
            // パス支配の場合, 流れてからの必勝を判定
            m.setDO(); // 支配フラグ付加
            Board nb = b;
            nb.flush(); nb.flushInfo();
            return judgeHandMate(depth, mbuf, mh, oh, nb);
        }
        // DERR << "CHECK - " << argMove << " " << IS_NF << ", " << IS_UNRIVALED << endl;
        // DERR << depth << endl;
        // 通常
        if (m.qty() >= mh.qty) return true; // 即上がり
        if (dominatesHand(m, oh, b) || m.qty() > b.getMaxNCardsAwake()) { // 支配
            m.setDO(); // 支配フラグ付加
            Board nb = b;
            nb.proc(m);
            Hand nmh;
            makeMove1stHalf(mh, &nmh, m);
            
            // セルフフォロー
            // 自分の出した役を流してからの必勝チェック
            // 永続的パラメータ変更を起こす場合はBNPW判定を続け、起こさない場合はPWのみ検討
            nb.flush(); nb.flushInfo();
            if (judgeHandMate(m.isRev() ? depth : 0, mbuf, nmh, oh, nb)) return true;
            // 自分の出したジョーカーをS3で返してからの必勝チェック
            if (m.isSingleJOKER() && containsS3(nmh.cards)) {
                Move s3; s3.setSingle(INTCARD_S3);
                nmh.makeMove1stHalf(s3, CARDS_S3, 1);
                return judgeHandMate(0, mbuf, nmh, oh, nb);
            }
        } else { // 支配しない
            if (depth) {
                // S3分岐必勝を検討
                // awakeな相手の最小枚数2以上、ジョーカー以外に返せる着手が存在しない場合、
                // ジョーカー -> S3の場合とそのまま流れた場合にともに必勝なら必勝(ぱおーん氏の作より)
                if (m.isSingle() // シングルのみ
                    && b.getMinNCardsAwake() > 1 // 相手に即上がりされない
                    && containsS3(mh.cards) // 残り手札にS3がある
                    && !m.isEqualRankSuits(RANK_3, SUITS_S)) { // 今出す役がS3でない
                    Cards zone = ORToGValidZone(b.order(), m.rank());
                    if (b.locksSuits(m)) zone &= SuitsToCards(m.suits());
                    if (!(zone & oh.cards)) { // ジョーカー以外は出せない
                        Hand nmh;
                        makeMove1stHalf(mh, &nmh, m);
                        if (judgeHandPW_NF(nmh, oh, b)) { // 流れた場合
                            Move s3; s3.setSingle(INTCARD_S3);
                            nmh.makeMove1stHalf(s3, CARDS_S3, 1); // S3 で進める
                            if (judgeHandPW_NF(nmh, oh, b)) { // S3で返した場合
                                // 両方で勝ったので必勝
                                DERR << "BRPW(S3)!!!" << endl;
                                return true;
                            }
                        }
                    }
                }
                // BNPWを検討
                if (checkHandBNPW(depth - 1, mbuf, m, mh, oh, b)) {
                    DERR << "BNPW - " << m << " ( " << b.getMinNCardsAwake() << " ) " << endl;
                    return true;
                }
            }
        }
        return false;
    }

    inline int searchHandMate(const int depth, Move *const mbuf, const int NMoves,
                              const Hand& mh, const Hand& oh, const Board& b) {
        // 必勝手探し
        for (int i = NMoves - 1; i >= 0; i--) {
            if (mbuf[i].qty() == mh.qty) return i;
        }
        for (int i = NMoves - 1; i >= 0; i--) {
            if (checkHandMate(depth, mbuf + NMoves, mbuf[i], mh, oh, b)) return i;
        }
        return -1;
    }
}