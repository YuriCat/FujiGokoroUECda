#pragma once

#include "settings.h"
#include "core/daifugo.hpp"

// UECdaルールで戦うためのヘッダ
// 様々なルールを設定
constexpr bool RULE_FIRSTTURNPLAYER_D3 = true;

namespace UECda {
    
#if _PLAYERS == 5 // 5人時
    // 階級
    enum Class {
        DAIFUGO = 0,
        FUGO,
        HEIMIN,
        HINMIN,
        DAIHINMIN,
        MIDDLE = HEIMIN, // 中央
    };
    
    // UECdaの固定ルール
    constexpr int N_PLAYERS = 5;
    constexpr int N_CLASSES = 5;
    
    constexpr int N_CHANGE_FALL_CARDS(int cl) { return MIDDLE - cl; }
    constexpr int N_CHANGE_RISE_CARDS(int cl) { return cl - MIDDLE; }
    
    inline int N_CHANGE_CARDS(int cl) { return abs(cl - MIDDLE); }
    
    constexpr int REWARD(int cl) { return N_PLAYERS - cl; }
    
    constexpr int N_MAX_OWNED_CARDS_CHANGE = 13;
    constexpr int N_MAX_OWNED_CARDS_PLAY = 11;
    
    constexpr int N_MAX_CHANGES = 13 * 12 / 2;
    constexpr int N_MAX_MOVES = 256;
    
#elif _PLAYERS == 4 // 4人時
    
    // 階級
    enum Class {
        DAIFUGO = 0,
        FUGO,
        HINMIN,
        DAIHINMIN,
        HEIMIN = -1,
        MIDDLE = HINMIN,
    };
    
    // UECdaの固定ルール
    constexpr int N_PLAYERS = 4;
    constexpr int N_CLASSES = 4;
    
    constexpr int NChangeFallCards[N_PLAYERS] = {2, 1, 0, 0};
    constexpr int NChangeRiseCards[N_PLAYERS] = {0, 0, 1, 2};
    constexpr int NChangeCards[N_PLAYERS] = {2, 1, 1, 2};
    
    constexpr int N_CHANGE_FALL_CARDS(int cl) { return NChangeFallCards[cl]; }
    constexpr int N_CHANGE_RISE_CARDS(int cl) { return NChangeRiseCards[cl]; }
    
    constexpr int N_CHANGE_CARDS(int cl) { return NChangeCards[cl]; }
    
    constexpr int REWARD(int cl) { return N_PLAYERS - cl; }
    
    constexpr int N_MAX_OWNED_CARDS_CHANGE = 16;
    constexpr int N_MAX_OWNED_CARDS_PLAY = 14;
    
    constexpr int N_MAX_CHANGE = 16 * 15 / 2;
    constexpr int N_MAX_MOVES = 1024;
    
#endif
    constexpr int N_SEATS = N_PLAYERS;
    
    constexpr int CLASS_INIT_CYCLE = 100; // 階級リセット試合数(変更の可能性あり)
    constexpr int SEAT_INIT_CYCLE = 3; // 席順リセット試合数(変更の可能性あり)
    
    //const int N_GAMES; // 総試合数
    // ここにルールを書いていく

    
    // UECdaが提供しているデータ構造。
    // プレーヤー番号などの基本型は、特に断りがなければ
    // ここに定義されているものと考える
    
    // プレーヤー識別
    // 0からN_PLAYERS - 1という標準型なので特に定義しない
    
    // 階級識別
    // 0からN_PLAYERS - 1という標準型なので特に定義しない
    
    // 座席識別
    // 0からN_PLAYERS - 1として問題ないので定義しない
    
    // 基本通信型。手札兼着手兼エントリー時情報送信
    inline void clearAll(int table[8][15]) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0 ; j < 15; j++) {
                table[i][j] = 0;
            }
        }
    }
    inline void clearCards(int table[8][15]) {
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 15; j++) {
                table[i][j] = 0;
            }
        }
    }
    
    inline bool isInitGame(const int table[8][15]) { return (table[6][5] == table[6][6]) ? true : false; } // プレーヤー0と1の階級が同じかどうかで判別
    inline int getTmpOrder(const int table[8][15]) { return (table[5][5] != table[5][6]) ? 1 : 0; } // 革命とイレバの片方のみ起こっている
    inline int getPrmOrder(const int table[8][15]) { return table[5][6] ? 1 : 0; } // 革命が起こっている
    inline int suitsLocked(const int table[8][15]) { return table[5][7]; }
    inline int isMyTurn(const int table[8][15]) { return table[5][2]; }
    inline int turnOf(const int table[8][15]) { return table[5][3]; }
    inline int isNull(const int table[8][15]) { return table[5][4]; } // 空場
    inline int getNCards(const int table[8][15], int p) { return table[6][0 + p]; }
    inline int classOf(const int table[8][15], int p) { return table[6][5 + p]; }
    inline int seatPlayer(const int table[8][15], int s) { return table[6][10 + s]; } // seat:0-4
    inline int isInChange(const int table[8][15]) { return table[5][0]; }
    inline int getChangeQty(const int table[8][15]) { return table[5][1]; }
    
    // コンバート
    // ジョーカー関連は対応していないので注意
    constexpr inline int WToRank(int w) { return w; }
    constexpr inline int HToSuitNum(int h) { return 3 - h; }
    constexpr inline uint32_t HToSuit(int h) { return SuitNumToSuits(HToSuitNum(h)); }
    
    constexpr inline int RankToW(int r) { return r; }
    constexpr inline int SuitNumToH(int sn) { return 3 - sn; }
    inline int SuitToH(uint32_t s) { return SuitNumToH(SuitToSuitNum(s)); }
    
    constexpr inline int IntCardToH(IntCard ic) { return SuitNumToH(IntCardToSuitNum(ic)); }
    constexpr inline int IntCardToW(IntCard ic) { return RankToW(IntCardToRank(ic)); }
    constexpr inline IntCard HWtoIC(int h, int w) { return RankSuitNumToIntCard(WToRank(w), HToSuitNum(h)); }
    
    // カード集合変換(chara == true の場合は性質カードに変換)
    inline Cards TableToCards(const int table[8][15], bool chara = false) {
        Cards c = CARDS_NULL;
        bool jk = false;
        for (int h = 0; h < 4; h++) {
            for (int w = 0; w < 15; w++) {
                if (table[h][w]) {
                    if (table[h][w] == 1) {
                        c.insert(HWtoIC(h, w));
                    } else {
                        if (chara) {
                            jk = true;
                            c.insert(HWtoIC(h, w));
                        } else {
                            c.insertJOKER();
                        }
                    }
                }
            }
        }
        // 通常のカード位置以外にあればジョーカー
        for (int w = 0; w < 15; w++) {
            if (table[4][w]) c.insertJOKER();
        }
        // ジョーカー1枚使用の場合には chara もジョーカーに変更
        if (jk && c.count() == 1) c = CARDS_JOKER;
        return c;
    }
    
    inline void CardsToTable(Cards c, int table[8][15]) {
        clearAll(table);
        if (c.joker()) table[4][1] = 2;
        for (IntCard ic : c.plain()) table[IntCardToH(ic)][IntCardToW(ic)] = 1;
    }
    
    inline void replaceCards(Cards c, int table[8][15]) {
        clearCards(table);
        if (c.joker()) table[4][1] = 2;
        for (IntCard ic : c.plain()) table[IntCardToH(ic)][IntCardToW(ic)] = 1;
    }
    
    // 着手変換
    static void MoveToTable(const Move m, const Board bd, int table[8][15]) {
        clearAll(table);
        if (m.isPASS()) return;
        if (m.isSingleJOKER()) { // シングルジョーカー
            int ord = bd.order();
            if (bd.isNull()) {
                table[0][(ord == 0) ? 14 : 0] = 2;
            } else {
                // ジョーカーでスートロックを掛けないように、別スートになるようにする
                // java版サーバーのリジェクトバグにひっかからないための処理
                uint32_t bs = bd.suits();
                int h = SuitToH(bs);
                h = (h + 1) % 4; // スートを変更
                table[h][(ord == 0) ? 14 : 0] = 2;
            }
            return;
        }
        if (m.isGroup() && m.qty() == 5) { // クインタプル
            int w = RankToW(m.rank());
            for (int h = 0; h < 4; h++) table[h][w] = 1;
            table[4][w] = 2;
            return;
        }
        // それ以外の役
        Cards charaCards = m.charaCards();
        Cards usedCards = m.cards();
        for (IntCard ic : charaCards) {
            int h = IntCardToH(ic);
            int w = IntCardToW(ic);
            if (usedCards.contains(ic)) table[h][w] = 1;
            else table[h][w] = 2; // JOKERが代役
        }
    }
    
    static Move TableToMove(const int table[8][15]) {
        Cards chara = TableToCards(table, true);
        Cards used = TableToCards(table, false);
        DERR << "chara " << chara << " used " << used << endl;
        return CardsToMove(chara, used);
    }
    
    static std::string toString(const int table[8][15]) { // 出力
        std::ostringstream oss;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 15; j++) {
                oss << std::setw(2) << table[i][j];
            }
            oss << std::endl;
        }
        return oss.str();
    }
}

using namespace UECda;

    // ルールの性質
    
    // オーダー逆転のパターン
    // スートロックの条件
    // 初手プレーヤーの決め方
    // 場の流れ方
    //

constexpr int SEQ_MIN_QTY = 3;

inline bool Move::domInevitably() const {
    if (isSeq()) return rank() <= RANK_8 && RANK_8 < rank() + qty();
    else return rank() == RANK_8;
}
inline bool Move::isRev() const { return isSeq() ? qty() >= 5 : qty() >= 4; }
inline bool Move::isBack() const { return false; }

inline bool Board::domConditionally(Move m) const { return isSingleJOKER() && m.isS3(); }
inline bool Board::locksSuits(Move m) const { return !isNull() && suits() == m.suits(); }
inline bool Board::locksRank(Move m) const { return false; }

inline bool Move::exam() const {
    // 変な値でないかチェック
    if (isPASS()) {
        if (qty() != 0) return false;
    } else if (isSingleJOKER()) {
        if (qty() != 1) return false;
        if (!isGroup()) return false;
    } else {
        int suitCount = countSuits(extendedSuits());
        if (isSeq()) {
            if (qty() < SEQ_MIN_QTY) return false;
            if (suitCount != 1) return false;
        } else {
            if (suitCount != qty()) return false;
        }
    }
    return true;
}

inline bool isNoRev(const Cards mine) {
    // 無革命性の証明
    return !groupCards(mine, 4) && !canMakeSeq(mine, 5);
}

constexpr int getChangePartnerClass(int acl) {
    // 交換相手の階級
    return N_CLASSES - 1 - acl;
}

constexpr int numGamesBeforeClassInit(int gn) {
    // 試合番号から、階級初期化ゲームへの残り試合数に変換
    return CLASS_INIT_CYCLE - 1 - (gn % CLASS_INIT_CYCLE);
}
constexpr int getNGamesForSeatInitGame(int gn) {
    // 試合番号から、席順初期化ゲームへの残り試合数に変換
    return SEAT_INIT_CYCLE - 1 - (gn % SEAT_INIT_CYCLE);
}


template <int N = N_PLAYERS>
constexpr unsigned int getNextSeat(unsigned int s) {
    return (s + 1) % (unsigned int)cmax(N, 1);
}
template <int N>
constexpr unsigned int getRemovedNextSeat(unsigned int s) {
    return s % (unsigned int)cmax(N - 1, 1);
}
template <int N = N_PLAYERS>
constexpr unsigned int getPreviousSeat(unsigned int s) {
    return (s + N - 1) % (unsigned int)cmax(N, 1);
}
