#pragma once

#include "daifugo.hpp"
#include "field.hpp"

extern int genChange(Cards *const pc0, const Cards myCards, const int changeQty);

extern int genAllSeq(Move *const mv, const Cards c);

extern int genLead(Move *const mv0, const Cards c);
extern int genLegalLead(Move *const mv0, const Cards c);

extern int genJokerGroup(Move *const mv0, Cards c, const Cards ops, const Board b);

extern int genGroupDebug(Move *const mv0, Cards c, Board b);
extern int genFollowExceptPASS(Move *const mv, const Cards c, const Board b);

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
inline std::vector<MoveInfo> genMove(const Field& field) {
    std::vector<MoveInfo> moves(N_MAX_MOVES);
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
        ret = std::min(ret, nret);
    }
    return ret;
}