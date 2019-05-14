#pragma once

#include "daifugo.hpp"
#include "field.hpp"

extern int genChange(Cards *const pc0, const Cards myCards, const int changeQty);

extern int genAllSeqWithJoker(Move *const mv0, const Cards x);
extern int genAllPlainSeq(Move *const mv0, const Cards x);

inline int genAllSeq(Move *const mv, const Cards c) {
    if (!containsJOKER(c)) return genAllPlainSeq(mv, c);
    else return genAllSeqWithJoker(mv, c);
}

extern int genFollowPlainSeq(Move *const mv0, Cards c, Board b);
extern int genFollowSeqWithJoker(Move *const mv0, const Cards plain, const Board b);

inline int genFollowSeq(Move *const mv, const Cards c, const Board b) {
    if (!containsJOKER(c)) return genFollowPlainSeq(mv, c, b);
    else return genFollowSeqWithJoker(mv, c, b);
}

extern int genFollowSingle(Move *const mv0, const Cards myCards, const Board b);
extern int genFollowDouble(Move *const mv0, const Cards c, const Board b);
extern int genFollowTriple(Move *const mv0, const Cards c, const Board b);
extern int genFollowQuadruple(Move *const mv0, const Cards c, const Board b);
extern int genAllSingle(Move *const mv0, Cards c);
extern int genLead(Move *const mv0, const Cards c);
extern int genLegalLead(Move *const mv0, const Cards c);

extern int genJokerGroup(Move *const mv0, Cards c, const Cards ops, const Board b);

extern int genGroupDebug(Move *const mv0, Cards c, Board b);

inline int genFollowExceptPASS(Move *const mv, const Cards c, const Board b) {
    // 返り値は、生成した手の数
    if (!b.isSeq()) {
        int cnt;
        switch (b.qty()) {
            case 0: cnt = 0; break;
            case 1: cnt = genFollowSingle(mv, c, b); break;
            case 2: cnt = genFollowDouble(mv, c, b); break;
            case 3: cnt = genFollowTriple(mv, c, b); break;
            case 4: cnt = genFollowQuadruple(mv, c, b); break;
            default: cnt = 0; break;
        }
        return cnt;
    } else return genFollowSeq(mv, c, b);
}

inline int genFollow(Move *const mv, const Cards c, const Board b) {
    // 返り値は、生成した手の数
    mv->setPASS();
    return 1 + genFollowExceptPASS(mv + 1, c, b);
}
inline int genMove(Move *mv, const Cards c, const Board b) {
    if (b.isNull()) return genLead(mv, c);
    else return genFollow(mv, c, b);
}
inline int genLegal(Move *mv, const Cards c, const Board b) {
    if (b.isNull()) return genLegalLead(mv, c);
    else return genFollow(mv, c, b);
}
inline std::vector<Move> genMove(const Field& field) {
    std::vector<Move> moves(N_MAX_MOVES);
    int numMoves = genMove(moves.data(), field.hand[field.turn()], field.board);
    moves.resize(numMoves);
    return moves;
}

inline int divisionCount(Move *const mbuf, const Cards c) {
    // paoon氏のbeersongのアイデアを利用
    int ret = countCards(CardsToER(c)); // 階段なしの場合の最小分割数
    const int cnt = genAllSeq(mbuf, c);
    // 階段を使った場合の方が分割数を減らせる場合を考慮
    for (int i = 0; i < cnt; i++) {
        int nret = divisionCount(mbuf + cnt, c - mbuf[i].cards()) + 1;
        ret = min(ret, nret);
    }
    return ret;
}