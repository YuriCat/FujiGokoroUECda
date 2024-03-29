#pragma once

// 詰み(Mate)判定

#include "../core/daifugo.hpp"
#include "../core/hand.hpp"
#include "../core/action.hpp"
#include "../core/dominance.hpp"

inline bool judgeMate_Easy_NF(const Hand& myHand) {
    // とりあえず高速でNF_Mateかどうか判断したい場合
    if (myHand.qty <= 1) return true;
    BitCards pqr = myHand.pqr;
    if (!any2Cards(maskCards(pqr, CARDS_8 | CARDS_JOKER))) return true;
    if (myHand.jk) { // ジョーカーあり
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

inline bool judgeHandPPW_NF(const Cards cards, const Cards pqr, const int jk,
                            const Cards *const nd, const Board& b) {
    // すでに支配的でない着手があることがわかっており、
    // それ以外が全て支配的かを確認するメソッド
    // 階段未対応

    assert(pqr == CardsToPQR(cards));
    const int ord = b.order();
#define PPW(s) { DERR << "PPW" << s << " " << cards << std::endl; return true; }

    const BitCards ndpqr = pqr & nd[ord] & ~CARDS_8; // 支配出来ていないpqr

    if (!ndpqr) {
        // 革命を返される可能性のみ考慮
        BitCards ndquad = pqr & PQR_4 & ~CARDS_8 & ~ndpqr & nd[flipOrder(ord)];
        // 支配できない革命が無い
        if (!ndquad) PPW("0(NO-NDQUAD)");
        // ジョーカーがあるとき革命が2つ以下ならジョーカーを加えて勝ち
        if (jk) PPW("0(QUAD+JK)");
        // 革命を崩して支配できたら勝ち 比較的低頻度
        //BitCards nddouble = (ndquad >> 2) & nd[ord];
        //if (!nddouble) PPW("0(QUADDIV)");
        return false;
    }

    if (!any2Cards(ndpqr)) {
        // ndpqrが1ビット
        // ジョーカーを加えて必勝になるか確かめる
        if (jk) {
            // 5枚グループにして勝ち
            if (ndpqr & PQR_4) PPW("1(QUIN)");
            // ndと交差しなければ勝ち
            if (!((ndpqr << 1) & nd[(ndpqr & PQR_3) ? flipOrder(ord) : ord])) PPW("1(+JK)");
            // スペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
            if (ndpqr == (CARDS_3 & PQR_1) && containsS3(cards)) PPW("1(JK-S3)");
        }
    }

    // 革命によって勝てないか考える
    if (pqr & PQR_4) {
        // ジョーカーを使わない革命あり
        BitCards quad = pqr & PQR_4;
        BitCards ndpqr_new = ndpqr & ~quad & nd[flipOrder(ord)];

        // 革命が支配的かつ他の役も全て支配的なら勝ち
        bool qdom = !(quad & nd[flipOrder(ord)] & ~CARDS_8);
        if (!ndpqr_new && qdom) PPW("2(NJ_QUAD)");
        if (jk) {
            // 革命がだめでも5枚出しがあるなら勝ち
            if (!ndpqr_new) PPW("2(QUIN)");
            if (!any2Cards(ndpqr_new)) {
                // 1つ残し
                // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                if (qdom) {
                    // 革命でターンがとれる
                    // このとき、まだターンが取れていなかったやつを逆オーダーで考える
                    // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければ勝ち
                    // いずれかのオーダーのndと交差しなければ勝ち
                    // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし
                    BitCards ndpqr_jk = ndpqr_new << 1;
                    if (!(ndpqr_jk & nd[ord]) || !(ndpqr_jk & nd[flipOrder(ord)])) PPW("2(NJ_QUAD +JK)");
                }
            }
        }
    }

    if (jk && (pqr & PQR_3)) {
        // ジョーカーを使えば革命あり
        // ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
        BitCards quad = (pqr & PQR_3) << 1;
        // トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
        BitCards ndquad = quad & nd[flipOrder(ord)] & ~CARDS_8;

        if (quad != ndquad) {
            // 革命でターンがとれる
            // 複数ターンが取れる場合は1つを使う
            BitCards dquad = (quad - ndquad) >> 1;
            dquad = dquad & -dquad;
            // このとき、まだターンが取れていなかったやつを逆オーダーで考える
            BitCards ndpqr_new = ndpqr & ~dquad & nd[flipOrder(ord)]; // 革命の分も消しとく
            // 全部支配できた場合のみOK
            if (!ndpqr_new) PPW("2(J_QUAD)");
        }
    }

#undef PPW
    return false;
}

inline bool judgeHandPW_NF(const Hand& myHand, const Hand& opsHand, const Board& b) {
    // 合法着手生成を用いずに必勝判定
    // ただし
    // クアドラを2つ以上
    // トリプルを3つ以上
    // クアドラ1つ以上 && トリプル2つ以上
    // といった稀な場合には高確率で誤判定する

    assert(myHand.exam1stHalf() && opsHand.exam_nd());
    const int ord = b.order();
#define PW(s) { DERR << "PW" << s << " " << myHand.cards << ", " << opsHand.cards << std::endl; return true; }

    const BitCards ndpqr = myHand.pqr & opsHand.nd[ord] & ~CARDS_8; // 支配出来ていないpqr

    if (!ndpqr) {
        // このときほぼ必勝なのだが、一応4枚グループが複数ある場合にはそうでないことがある
        BitCards quad = myHand.pqr & PQR_4;
        // 4枚グループが1つ以下なら最後に出せば勝ち
        if (!any2Cards(quad)) PW("0(U1QUAD)");
        // 以下4枚グループ2つ以上
        // 革命のうち1つにジョーカーを加えれば勝ち
        if (myHand.jk) PW("0(2QUAD+JK)");
        // 支配できない革命が1つだけであれば勝ち
        BitCards ndquad = quad & ~CARDS_8 & opsHand.nd[flipOrder(ord)];
        if (!any2Cards(ndquad)) PW("0(U1NDQUAD)");
        // 革命を崩して支配できたら勝ち 低頻度
        //BitCards nddouble = (ndquad >> 2) & opsHand.nd[ord];
        //if (!any2Cards(nddouble)) PW("0(QUADDIV)");
        return false;
    }

    const BitCards ndpqr_m = popLsb(ndpqr);

    if (!ndpqr_m) {
        // ndpqrが1ビットだけ
        // 革命を返される可能性のみ考慮
        BitCards ndquad = myHand.pqr & PQR_4 & ~CARDS_8 & ~ndpqr & opsHand.nd[flipOrder(ord)];
        if (!ndquad) PW("1(NO-NDQUAD)");
        // ジョーカーを5枚出しに使えば勝ち
        if (myHand.jk) PW("1(QUAD+JK)");
        // 革命を崩して支配できたら勝ち
        BitCards nddouble = (ndquad >> 2) & opsHand.nd[ord];
        if (!nddouble) PW("1(QUADDIV)");
        return false;
    }

    // 支配出来ない部分が2つ以上
    if (myHand.seq) { // 階段である場合がある
        if (myHand.qty == 3) PW("0(SEQ3)"); // 3階段1つ
        if (myHand.qty == 4 && polymRanks(myHand.seq, 2 + myHand.jk)) PW("0(SEQ4)"); // 4階段1つ
    } else {
        // ここから長いので、ジョーカーも革命も階段も無い場合(大部分)はさっさと帰る
        if (!myHand.jk && !(myHand.pqr & PQR_4)) return false;
    }

    if (!any2Cards(ndpqr_m)) {
        // ndpqrが2ビットのみ
        // いずれかにジョーカーを加えて必勝になるか確かめる
        if (myHand.jk) {
            // 5枚出しで勝てる
            if (ndpqr & PQR_4) PW("1(QUAD+JK)");
            // ndpqr以外に革命がない場合
            if (!(myHand.pqr & PQR_4)) {
                BitCards h = ndpqr_m;
                BitCards l = ndpqr - ndpqr_m;
                // どちらかとndが交差しなければ勝ち ただし革命の場合は逆オーダー
                bool jh = (h << 1) & opsHand.nd[(h & PQR_3) ? flipOrder(ord) : ord];
                bool jl = (l << 1) & opsHand.nd[(l & PQR_3) ? flipOrder(ord) : ord];
                if (!jh || !jl) PW("2(+JK)");

                // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
                // 3はランク最低なのでlだろう
                if (l == (CARDS_3 & PQR_1) && containsS3(myHand.cards)) PW("2(JK-S3)");
            }
        }
    }

    // 革命によって勝てないか考える
    if (myHand.pqr & PQR_4) {
        // ジョーカーを使わない革命あり
        BitCards quad = myHand.pqr & PQR_4;
        BitCards ndpqr_new = ndpqr & ~quad & opsHand.nd[flipOrder(ord)];

        // 全部支配できたら勝ち TODO: 間違いだが何か意図があったのか?
        //if (!ndpqr_new) PW("2(NJ_QUAD)");

        BitCards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
        if (myHand.jk) {
            // 1つ残し。5枚出しすればよい。勝ち
            if (!ndpqr_new_m) PW("2(NJ_QUIN)");

            // 2つ以上残っている
            if (!any2Cards(ndpqr_new_m)) {
                // 2つだけ
                // 革命でターンが取れるならジョーカーは残っているのでジョーカーを使うことを考える
                // 革命でターンが取れない場合は5枚出しをする必要があったのでもうジョーカーは無い
                if (!(quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8)) {
                    // 革命でターンがとれる
                    // このとき、まだターンが取れていなかったやつを逆オーダーで考える

                    // ジョーカーを加えて、どちらかをいずれかのオーダーでndと交差しなければよい
                    BitCards h = ndpqr_new_m;
                    BitCards l = ndpqr_new - ndpqr_new_m;
                    // どちらかが、いずれかのオーダーのndと交差しなければ勝ち
                    if (   !((h << 1) & opsHand.nd[ord]) || !((h << 1) & opsHand.nd[flipOrder(ord)])
                        || !((l << 1) & opsHand.nd[ord]) || !((l << 1) & opsHand.nd[flipOrder(ord)])) {
                        PW("2NJ_QUAD(+JK)");
                    }
                    // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし
                }
            }
        } else {
            if (!ndpqr_new_m) {
                // 1つ残し。革命でターンが取れるなら勝ち
                if (!(quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8)) PW("2(NJ_QUAD 1)");
            }
        }
    }

    if (myHand.jk && (myHand.pqr & PQR_3)) {
        // ジョーカーを使えば革命あり。
        // ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
        // まず、この革命がターンを取れるか
        BitCards quad = (myHand.pqr & PQR_3) << 1;
        // トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
        BitCards ndquad = quad & opsHand.nd[flipOrder(ord)] & ~CARDS_8;

        if (quad != ndquad) {
            // 革命でターンがとれる
            // 複数ターンが取れる場合は1つを使う
            BitCards dquad = (quad - ndquad) >> 1;
            BitCards dquad_used = dquad & -dquad;

            // TODO: 複数トリプルの場合どれを革命に使うか
            // 理想は現オーダーでターンが取れないもののうち最も強いのを使いたい

            // このとき、まだターンが取れていなかったやつを逆オーダーで考える
            BitCards ndpqr_new = ndpqr & ~dquad_used & opsHand.nd[flipOrder(ord)]; // 革命の分も消しとく
            // 全部支配できたら勝ち
            if (!ndpqr_new) PW("2(J_QUAD 0)");
            // 1つ残しで勝ち
            if (!any2Cards(ndpqr_new)) PW("2(J_QUAD 1)");
            // 2つ以上残っている。ジョーカーを既に使ってしまったので勝てない
        }
    }

#undef PW
    return false;
}

// 必勝判定
inline int searchHandMate(const int, MoveInfo *const, const int,
                          const Hand&, const Hand&,  const Board&, const FieldAddInfo&);

inline bool judgeHandMate(const int depth, MoveInfo *const mbuf,
                          const Hand& myHand, const Hand& opsHand,
                          const Board& b, const FieldAddInfo& fieldInfo) {
    if (b.isNull()) {
        if (judgeHandPW_NF(myHand, opsHand, b)) return true;
    }

    if (b.isNull() && depth == 0) {
        // 階段パターンのみ検討
        // 階段と革命が絡む場合は勝ちと判定出来ない場合が多い
        if (myHand.seq) {
            int numMoves = genAllSeq(mbuf, myHand.cards);
            for (int i = 0; i < numMoves; i++) {
                const MoveInfo& m = mbuf[i];
                if (m.qty() >= myHand.qty) return true; // 即上がり

                Hand nextHand;
                makeMove1stHalf(myHand, &nextHand, m);

                // 5枚以上の階段は支配としておく
                if (m.qty() > 4) {
                    Board nb = b;
                    nb.procOrder(m);
                    // いずれかのオーダーで必勝ならOK
                    if (judgeHandPW_NF(nextHand, opsHand, b)
                        || judgeHandPW_NF(nextHand, opsHand, nb)) return true;
                } else {
                    if (dominatesHand(m, opsHand, b)) {
                        if (judgeHandPW_NF(nextHand, opsHand, b)) return true;
                    } else {
                        // 支配的でない場合、この役を最後に出すことを検討
                        if (judgeHandPPW_NF(nextHand.cards, nextHand.pqr, nextHand.jk, opsHand.nd, b)) return true;
                    }
                }
            }
        }
    } else {
        // depth > 0 のとき 空場でない場合は合法手生成して詳しく判定
        int numMoves = genMove(mbuf, myHand, b);
        if (searchHandMate(depth, mbuf, numMoves, myHand, opsHand, b, fieldInfo) != -1) return true;
    }
    return false;
}

inline bool bnpwSingle(const int depth, MoveInfo *const mbuf, const MoveInfo m, IntCard ic,
                       const Hand& myHand, const Hand& opsHand,
                       const Board& b, const FieldAddInfo& fieldInfo) {
    if (myHand.qty == m.qty() + 1) return true;
    Hand nextHand;
    makeMove1stHalf(myHand, &nextHand, m);
    Move fm; fm.setSingle(ic);
    nextHand.makeMove1stHalf(fm, IntCardToCards(ic), 1);
    FieldAddInfo nextFieldInfo;
    flushFieldAddInfo(fieldInfo, &nextFieldInfo);
    int n = std::min(fieldInfo.minNumCards(), fieldInfo.minNumCardsAwake() - m.qty());
    nextFieldInfo.setMinNumCards(n);
    nextFieldInfo.setMinNumCardsAwake(n);
    if (judgeHandMate(depth, mbuf, nextHand, opsHand, OrderToNullBoard(b.prmOrder()), nextFieldInfo)) return true;
    return false;
}

inline bool checkHandBNPW(const int depth, MoveInfo *const mbuf, const MoveInfo m,
                          const Hand& myHand, const Hand& opsHand,
                          const Board& b, const FieldAddInfo& fieldInfo) {
    // 間に他プレーヤーの着手を含んだ必勝を検討
    // 支配的でないことは前提とする
    if (m.isPASS()) return false; // パスからのBNPWはない

    // 相手に間で上がられる可能性がある場合
    // TODO: 実際は相手の枚数がぴたり m.qty() と同じでなければ上がられることはない
    if (fieldInfo.minNumCardsAwake() <= m.qty() && m.qty() <= fieldInfo.maxNumCardsAwake()) return false;

    // S3分岐必勝を検討
    // awakeな相手の最小枚数2以上、ジョーカー以外に返せる着手が存在しない場合、
    // ジョーカー -> S3の場合とそのまま流れた場合にともに必勝なら必勝(ぱおーん氏の作より)
    if (m.isSingle() // シングルのみ
        && containsS3(myHand.cards - m.cards())) { // 残り手札にS3がある
        Cards zone = ORToGValidZone(b.order(), m.rank());
        if (b.locksSuits(m)) zone &= SuitsToCards(m.suits());
        if (!(zone & opsHand.cards)) { // ジョーカー以外は出せない
            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, m);
            if (judgeHandPW_NF(nextHand, opsHand, b)) { // 流れた場合
                Move s3; s3.setSingle(INTCARD_S3);
                nextHand.makeMove1stHalf(s3, CARDS_S3, 1); // S3 で進める
                if (judgeHandPW_NF(nextHand, opsHand, b)) { // S3で返した場合
                    // 両方で勝ったので必勝
                    DERR << "BRPW(S3)!!!" << std::endl;
                    return true;
                }
            }
        }
    }

    int curOrder = b.prmOrder();
    Cards ops8 = opsHand.cards & CARDS_8;

    // BNPWを検討
    if (m.isSingle()) {
        // シングルジョーカーは少なくともBNPWではない
        if (m.isSingleJOKER()) return false;
        // 相手に8切りで返される可能性があればだめ
        if (b.locksSuits(m)) ops8 &= SuitsToCards(m.suits());
        if (ops8 && isValidGroupRank(RANK_8, curOrder, m.rank())) return false;
        if (myHand.jk && !containsS3(opsHand.cards)) { // まずジョーカーを検討
            // 残り1枚がジョーカーなら勝ち
            if (myHand.qty == m.qty() + 1) return true;

            // 簡単な勝ちの条件にはまらなかったので手札を更新して空場必勝を確認
            Hand nextHand;
            makeMove1stHalf(myHand, &nextHand, m);
            Move sj; sj.setSingleJOKER();
            nextHand.makeMove1stHalf(sj, CARDS_JOKER, 1);
            FieldAddInfo nextFieldInfo;
            flushFieldAddInfo(fieldInfo, &nextFieldInfo);
            int n = std::min(fieldInfo.minNumCards(), fieldInfo.minNumCardsAwake() - m.qty());
            nextFieldInfo.setMinNumCards(n);
            nextFieldInfo.setMinNumCardsAwake(n);
            if (judgeHandMate(depth, mbuf, nextHand, opsHand, OrderToNullBoard(curOrder), nextFieldInfo)) {
                return true;
            }
            // ジョーカー -> S3 からの空場必勝を確認
            if (containsS3(nextHand.cards)) {
                if (nextHand.qty == 1) return true;
                Move s3; s3.setSingle(INTCARD_S3);
                nextHand.makeMove1stHalf(s3, CARDS_S3, 1);
                if (judgeHandMate(depth, mbuf, nextHand, opsHand, OrderToNullBoard(curOrder), nextFieldInfo)) {
                    return true;
                }
            }
        }
        // 他のシングル役
        if (opsHand.jk == 0) {
            BitCards myUpper = myHand.cards - m.cards();
            BitCards opsUpper = opsHand.cards;
            BitCards moveSuitCards = SuitsToCards(m.suits());

            if (b.locksSuits(m)) {
                // 先にスート縛りがある場合
                myUpper &= moveSuitCards;
                opsUpper &= moveSuitCards;
            }

            if (curOrder == 0) {
                int opsHighRank = IntCardToRank(Cards(opsUpper).highest());
                myUpper &= RankRangeToCards(opsHighRank + 1, RANK_2);
                opsUpper &= RankRangeToCards(m.rank() + 1, RANK_2);
            } else {
                int opsHighRank = IntCardToRank(Cards(opsUpper).lowest());
                myUpper &= RankRangeToCards(RANK_3, opsHighRank - 1);
                opsUpper &= RankRangeToCards(RANK_3, m.rank() - 1);
            }

            if (myUpper) {
                if (b.locksSuits(m)) {
                    for (IntCard ic : Cards(myUpper)) {
                        assert(myHand.qty > m.qty() + 1); // 完全勝利でないので
                        if (bnpwSingle(depth, mbuf, m, ic, myHand, opsHand, b, fieldInfo)) {
                            //std::cerr << "already lock " << b << myHand.cards << opsHand.cards << m << "->" << ic; getchar();
                            return true;
                        }
                    }
                } else {
                    // 出した役と同じスートでのスートしばり
                    BitCards m1 = moveSuitCards & opsUpper;
                    std::array<BitCards, 4> all2 = {m1 & CARDS_C, m1 & CARDS_D, m1 & CARDS_H, m1 & CARDS_S};

                    // 出した役と違う役でのスートしばり TODO: 相手が2人以上
                    BitCards c2 = popLsb(opsUpper & CARDS_C);
                    BitCards d2 = popLsb(opsUpper & CARDS_D);
                    BitCards h2 = popLsb(opsUpper & CARDS_H);
                    BitCards s2 = popLsb(opsUpper & CARDS_S);
                    all2[0] |= c2; all2[1] |= d2; all2[2] |= h2; all2[3] |= s2;

                    // スートしばりがかからない場合、一つ必勝を見つけられたら勝ち
                    // かけられる可能性がある場合、どのスートでロックをかけられても必勝でないとダメ
                    // 実際にかけられなかった場合、どれか1つで勝てばいいのでその専用チェック不要
                    bool anyLock = all2[0] | all2[1] | all2[2] | all2[3];
                    if (!anyLock) {
                        for (IntCard ic : Cards(myUpper)) {
                            assert(myHand.qty > m.qty() + 1); // 完全勝利でないので
                            if (bnpwSingle(depth, mbuf, m, ic, myHand, opsHand, b, fieldInfo)) {
                                //std::cerr << "no lock " << b << myHand.cards << opsHand.cards << m << "->" << fm; getchar();
                                return true;
                            }
                        }
                    } else {
                        bool ok = true;
                        for (int sn = 0; sn < 4; sn++) {
                            if (all2[sn]) {
                                bool found = false;
                                BitCards tmpUpper = myUpper & SuitsToCards(1U << sn);
                                for (IntCard ic : Cards(tmpUpper)) {
                                    assert(myHand.qty > m.qty() + 1); // 完全勝利でないので
                                    if (bnpwSingle(depth, mbuf, m, ic, myHand, opsHand, b, fieldInfo)) {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) { ok = false; break; }
                            }
                        }
                        if (ok) {
                            // ここまで来たならOK
                            //std::cerr << "lock " << b << myHand.cards << opsHand.cards << m << "->>?" << all2 << Cards(myUpper) << IntCardToRank(opsHand.cards.highest()); getchar();
                            return true;
                        }
                    }
                }
            }
        }
    }
    // TODO: ダブル以上

    return false;
}

inline bool checkHandMate(const int depth, MoveInfo *const mbuf, MoveInfo& m,
                          const Hand& myHand, const Hand& opsHand,
                          const Board& b, const FieldAddInfo& fieldInfo) {

    if (fieldInfo.isUnrivaled()) { // 独断場のとき
        m.setDO(); // 支配フラグ付加
        if (m.isPASS()) {
            Board nb = b;
            nb.flush();
            FieldAddInfo nextFieldInfo;
            flushFieldAddInfo(fieldInfo, &nextFieldInfo);
            return judgeHandMate(depth, mbuf, myHand, opsHand, nb, nextFieldInfo);
        } else {
            if (m.qty() >= myHand.qty) return true; // あがり
            Hand nextHand;
            Board nb = b;
            FieldAddInfo nextFieldInfo;
            makeMove1stHalf(myHand, &nextHand, m);
            if (dominatesCards(m, nextHand.cards, b)) { // 自己支配
                m.setDM(); // 支配フラグ付加
                nb.procAndFlush(m);
                flushFieldAddInfo(fieldInfo, &nextFieldInfo);
                return judgeHandMate(depth, mbuf, nextHand, opsHand, nb, nextFieldInfo);
            } else { // セルフフォロー必勝を判定
                nb.proc(m);
                procUnrivaled(fieldInfo, &nextFieldInfo);
                return judgeHandMate(depth, mbuf, nextHand, opsHand, nb, nextFieldInfo);
            }
        }
        return false;
    }

    if (m.isPASS()) {
        // パス支配でない場合は判定終了
        if (!fieldInfo.isPassDom()) return false;

        // Unrivaledでないがパス支配の場合がある?
        // パス支配の場合, 流れてからの必勝を判定
        m.setDO(); // 支配フラグ付加
        FieldAddInfo nextFieldInfo;
        flushFieldAddInfo(fieldInfo, &nextFieldInfo);
        return judgeHandMate(depth, mbuf, myHand, opsHand,
                             OrderToNullBoard(b.prmOrder()), nextFieldInfo);
    }

    // 通常
    if (m.qty() >= myHand.qty) return true; // 即上がり
    if (dominatesHand(m, opsHand, b)
        || m.qty() > fieldInfo.maxNumCardsAwake()) { // 支配
        m.setDO(); // 支配フラグ付加

        Board nb = b;
        nb.proc(m);
        Hand nextHand;
        makeMove1stHalf(myHand, &nextHand, m);

        // セルフフォロー

        // 自分の出した役を流してからの必勝チェック
        // 永続的パラメータ変更を起こす場合はBNPW判定を続け、起こさない場合はPWのみ検討
        nb.flush();
        FieldAddInfo nextFieldInfo;
        flushFieldAddInfo(fieldInfo, &nextFieldInfo);
        if (judgeHandMate(m.isRev() ? depth : 0,
                          mbuf, nextHand, opsHand, nb, nextFieldInfo)) {
            return true;
        }
        // 自分の出したジョーカーをS3で返してからの必勝チェック
        if (m.isSingleJOKER() && containsS3(nextHand.cards)) {
            Move s3; s3.setSingle(INTCARD_S3);
            nextHand.makeMove1stHalf(s3, CARDS_S3, 1);
            return judgeHandMate(0, mbuf, nextHand, opsHand, nb, nextFieldInfo);
        }
    } else { // 支配しない
        if (depth <= 0) return false;

        // BNPWを検討
        assert(!fieldInfo.isLastAwake());
        if (checkHandBNPW(depth - 1, mbuf, m, myHand, opsHand, b, fieldInfo)) {
            DERR << "BNPW - " << m << " ( " << fieldInfo.minNumCardsAwake() << " ) " << std::endl;
            return true;
        }
    }
    return false;
}

inline int searchHandMate(const int depth, MoveInfo *const mbuf, int numMoves,
                          const Hand& myHand, const Hand& opsHand,
                          const Board& b, const FieldAddInfo& fieldInfo) {
    // 必勝手探し
    for (int i = numMoves - 1; i >= 0; i--) {
        if (mbuf[i].qty() == myHand.qty) return i;
    }
    for (int i = numMoves - 1; i >= 0; i--) {
        if (checkHandMate(depth, mbuf + numMoves, mbuf[i], myHand, opsHand, b, fieldInfo)) return i;
    }
    return -1;
}