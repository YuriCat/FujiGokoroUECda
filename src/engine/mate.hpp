#pragma once

// 詰み(Mate)判定

#include "../core/daifugo.hpp"
#include "../core/hand.hpp"
#include "../core/action.hpp"
#include "../core/dominance.hpp"

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

    assert(pqr == CardsToPQR(cards));
    const int ord = b.order();
#define PPW(s) { DERR << "PPW" << s << " " << cards << std::endl; return true; }

    const Cards ndpqr = pqr & nd[ord] & ~CARDS_8; // 支配出来ていないpqr

    if (!ndpqr) {
        // 革命を返される可能性のみ考慮
        Cards ndquad = pqr & PQR_4 & ~CARDS_8 & ~ndpqr & nd[flipOrder(ord)];
        // 支配できない革命が無い
        if (!ndquad) PPW("0(NO-NDQUAD)");
        // ジョーカーがあるとき革命にジョーカーを加えて大体大丈夫
        // （革命複数の場合にはアウト）
        if (jk) PPW("0(QUAD+JK)");
        return false;
    }

    if (!any2Cards(ndpqr)) {
        // ndpqrが1ビット
        // ジョーカーを加えて必勝になるか確かめる
        if (jk) {
            // 5枚グループにして勝ち
            if (ndpqr & PQR_4) PPW("1(QUIN)");
            // ndと交差しなければ勝ち
            if (!((ndpqr << 1) & nd[ord])) PPW("1(+JK)");
            // スペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
            if (ndpqr == (CARDS_3 & PQR_1) && containsS3(cards)) PPW("1(JK-S3)");
        }
    }

    // 革命によって勝てないか考える
    if (pqr & PQR_4) {
        // ジョーカーを使わない革命あり
        Cards quad = pqr & PQR_4;
        Cards ndpqr_new = ndpqr & ~quad & nd[flipOrder(ord)];

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
                    Cards ndpqr_jk = ndpqr_new << 1;
                    if (!(ndpqr_jk & nd[ord]) || !(ndpqr_jk & nd[flipOrder(ord)])) PPW("2(NJ_QUAD +JK)");
                }
            }
        }
    }

    if (jk && (pqr & PQR_3)) {
        // ジョーカーを使えば革命あり
        // ジョーカー無し革命とジョーカーあり革命が両方の場合は多分上のに含まれているので考えない
        Cards quad = (pqr & PQR_3) << 1;
        // トリプルが2つあるのは結構あるだろう。少なくとも1つで支配出来れば良い
        Cards tmp = quad & nd[flipOrder(ord)] & ~CARDS_8;
        if (quad != tmp) {
            // 革命でターンがとれる
            // このとき、まだターンが取れていなかったやつを逆オーダーで考える
            Cards ndpqr_new = ndpqr & ~((quad ^ tmp) >> 1) & nd[flipOrder(ord)]; // 革命の分も消しとく
            // 全部支配できた場合のみOK
            if (!ndpqr_new) PPW("2(J_QUAD)");
        }
    }

#undef PPW
    return false;
}

inline bool judgeHandPW_NF(const Hand& mh, const Hand& oh, const Board& b) {
    // 合法着手生成を用いずに必勝判定
    // ただし
    // クアドラを2つ以上
    // トリプルを3つ以上
    // クアドラ1つ以上 && トリプル2つ以上
    // といった稀な場合には高確率で誤判定する

    assert(mh.exam1stHalf() && oh.exam_nd());
    const int ord = b.order();
#define PW(s) { DERR << "PPW" << s << " " << mh.cards << ", " << oh.cards << std::endl; return true; }

    const Cards ndpqr = mh.pqr & oh.nd[ord] & ~CARDS_8; // 支配出来ていないpqr

    if (!ndpqr) {
        // このときほぼ必勝なのだが、一応4枚グループが複数ある場合にはそうでないことがある
        Cards quad = mh.pqr & PQR_4;
        // 4枚グループが1つ以下
        if (!any2Cards(quad)) PW("0(U1QUAD)"); 
        // 以下4枚グループ2つ以上 誤りを許容
        // 革命のうち1つにジョーカーを加えれば勝ち
        if (mh.jk) PW("0(2QUAD+JK)"); 
        if (!any2Cards(quad & ~CARDS_8 & oh.nd[ord])) PW("0(U1NDQUAD)");
        return false;
    }

    const Cards ndpqr_m = popLsb(ndpqr);

    if (!ndpqr_m) {
        // ndpqrが1ビットだけ
        // 革命を返される可能性のみ考慮
        Cards ndquad = mh.pqr & PQR_4 & ~CARDS_8 & ~ndpqr & oh.nd[flipOrder(ord)];
        if (!ndquad) PW("1(NO-NDQUAD)");
        // （革命複数の場合にはアウト）
        if (mh.jk) PW("1(QUAD+JK)");
        return false;
    }

    // 支配出来ない部分が2つ以上
    if (mh.seq) { // 階段である場合がある
        if (mh.qty == 3) PW("0(SEQ3)"); // 3階段1つ
        if (mh.qty == 4 && polymRanks(mh.seq, 2 + mh.jk)) PW("0(SEQ4)"); // 4階段1つ
    } else {
        // ここから長いので、ジョーカーも革命も階段も無い場合(大部分)はさっさと帰る
        if (!mh.jk && !(mh.pqr & PQR_4)) return false;
    }

    if (!any2Cards(ndpqr_m)) {
        // 2ビットのみ
        // いずれかにジョーカーを加えて必勝になるか確かめる
        if (mh.jk) {
            // 5枚出しで勝てる
            if (ndpqr & PQR_4) PW("1(QUAD+JK)");

            Cards l = ndpqr.divide().lowest();
            Cards h = ndpqr - l;
            // どちらかとndが交差しなければ勝ち ただし革命の場合は逆オーダー
            bool jh = (h << 1) & oh.nd[(h & PQR_3) ? (1 - ord) : ord];
            bool jl = (l << 1) & oh.nd[(l & PQR_3) ? (1 - ord) : ord];
            if (!jh || !jl) PW("1(+JK)");

            // 2ビットのうち片方がスペ3単体であった場合には、ジョーカーと組み合わせて出せるので勝ち
            // 3はランク最低なのでlだろう
            if (l == (CARDS_3 & PQR_1) && containsS3(mh.cards)) PW("1(JK-S3)");
        }
    }

    // 革命によって勝てないか考える
    if (mh.pqr & PQR_4) {
        // ジョーカーを使わない革命あり。
        Cards quad = mh.pqr & PQR_4;
        Cards ndpqr_new = ndpqr & ~quad & oh.nd[flipOrder(ord)];

        // 全部支配できたら勝ち
        if (!ndpqr_new) PW("2(NJ_QUAD)");

        Cards ndpqr_new_m = ndpqr_new & (ndpqr_new - 1ULL);
        if (mh.jk) {
            // 1つ残し。5枚出しすればよい。勝ち
            if (!ndpqr_new_m) PW("2(NJ_QUIN)");

            // 2つ以上残っている
            if (!any2Cards(ndpqr_new_m)) {
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
                    if (   !((h << 1) & oh.nd[ord]) || !((h << 1) & oh.nd[flipOrder(ord)])
                        || !((l << 1) & oh.nd[ord]) || !((l << 1) & oh.nd[flipOrder(ord)])) {
                        PW("2NJ_QUAD(+JK)");
                    }
                    // このときスペ3は通常か逆のいずれかで必ず支配的なはずなので、スペ3判定必要なし
                }
            }
        } else {
            if (!ndpqr_new_m) {
                // 1つ残し。革命でターンが取れるなら勝ち
                if (!(quad & oh.nd[flipOrder(ord)] & ~CARDS_8)) PW("2(NJ_QUAD 1)");
            }
        }
    }

    if (mh.jk && (mh.pqr & PQR_3)) {
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
inline int searchHandMate(const int, Move *const, const int,
                          const Hand&, const Hand&,  const Board&);

inline bool judgeHandMate(const int depth, Move *const mbuf,
                          const Hand& mh, const Hand& oh, const Board& b) {
    if (b.isNull()) {
        if (judgeMate_Easy_NF(mh)) return true;
        if (judgeHandPW_NF(mh, oh, b)) return true;
    }

    if (b.isNull() && depth == 0) {
        // 階段パターンのみ検討
        // 階段と革命が絡む場合は勝ちと判定出来ない場合が多い
        if (mh.seq) {
            const int NMoves = genAllSeq(mbuf, mh.cards);
            for (int i = 0; i < NMoves; i++) {
                const Move& m = mbuf[i];
                if (m.qty() >= mh.qty) return true; // 即上がり

                Hand nmh;
                makeMove1stHalf(mh, &nmh, m);

                // 5枚以上の階段は支配としておく
                if (!depth && m.qty() > 4) {
                    Board nb = b;
                    nb.procOrder(m);
                    // いずれかのオーダーで必勝ならOK
                    if (judgeHandPW_NF(nmh, oh, b) || judgeHandPW_NF(nmh, oh, nb)) return true;
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
    } else {
        // depth > 0 のとき 空場でない場合は合法手生成して詳しく判定
        const int NMoves = genMove(mbuf, mh, b);
        if (searchHandMate(depth, mbuf, NMoves, mh, oh, b) != -1) return true;
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
        // シングルジョーカーは少なくともBNPWではない
        if (m.isSingleJOKER()) return false;
        if (mh.jk && !containsS3(oh.cards)) { // まずジョーカーを検討
            // 相手に8切りで返される可能性があればだめ
            if (ops8 && isValidGroupRank(RANK_8, curOrder, m.rank())) return false;
            // 残り1枚がジョーカーなら勝ち
            if (mh.qty == m.qty() + 1) return true;

            // 簡単な勝ちの条件にはまらなかったので手札を更新して空場必勝を確認
            Hand nmh;
            makeMove1stHalf(mh, &nmh, m);
            Move sj; sj.setSingleJOKER();
            nmh.makeMove1stHalf(sj, CARDS_JOKER, 1);
            Board nb = b;
            nb.flush(true);
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
    }
    // TODO: 他の役で検討
    return false;
}

inline bool checkHandMate(const int depth, Move *const mbuf, Move& m,
                          const Hand& mh, const Hand& oh, const Board& b) {

    if (b.isUnrivaled()) { // 独断場のとき
        m.setDO(); // 支配フラグ付加
        if (m.isPASS()) {
            Board nb = b;
            nb.flush(true);
            return judgeHandMate(depth, mbuf, mh, oh, nb);
        } else {
            if (m.qty() >= mh.qty) return true; // あがり
            Board nb = b;
            Hand nmh;
            makeMove1stHalf(mh, &nmh, m);
            if (dominatesCards(m, nmh.cards, b)) { // 自己支配
                m.setDM(); // 支配フラグ付加
                nb.procAndFlush(m, true);
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
        nb.flush(true);
        return judgeHandMate(depth, mbuf, mh, oh, nb);
    }

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
        nb.flush(true);
        if (judgeHandMate(m.isRev() ? depth : 0, mbuf, nmh, oh, nb)) return true;
        // 自分の出したジョーカーをS3で返してからの必勝チェック
        if (m.isSingleJOKER() && containsS3(nmh.cards)) {
            Move s3; s3.setSingle(INTCARD_S3);
            nmh.makeMove1stHalf(s3, CARDS_S3, 1);
            return judgeHandMate(0, mbuf, nmh, oh, nb);
        }
    } else { // 支配しない
        if (depth <= 0) return false;
        // S3分岐必勝を検討
        // awakeな相手の最小枚数2以上、ジョーカー以外に返せる着手が存在しない場合、
        // ジョーカー -> S3の場合とそのまま流れた場合にともに必勝なら必勝(ぱおーん氏の作より)
        if (m.isSingle() // シングルのみ
            && b.getMinNCardsAwake() > 1 // 相手に即上がりされない
            && containsS3(mh.cards - m.cards())) { // 残り手札にS3がある
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
                        DERR << "BRPW(S3)!!!" << std::endl;
                        return true;
                    }
                }
            }
        }
        // BNPWを検討
        if (checkHandBNPW(depth - 1, mbuf, m, mh, oh, b)) {
            DERR << "BNPW - " << m << " ( " << b.getMinNCardsAwake() << " ) " << std::endl;
            return true;
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