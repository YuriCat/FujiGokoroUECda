/*
 UECda.h
 Katsuki Ohto
 */

#pragma once
#include "settings.h"
#include "structure/primitive/prim.hpp"

// UECdaルールで戦うためのヘッダ
// 様々なルールを設定
constexpr bool RULE_FIRSTTURNPLAYER_D3 = true;

namespace UECda{
    
#if _PLAYERS == 5 // 5人時
    // 階級
    enum Class{
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
    
    constexpr int N_CHANGE_FALL_CARDS(int cl)noexcept{ return MIDDLE - cl; }
    constexpr int N_CHANGE_RISE_CARDS(int cl)noexcept{ return cl - MIDDLE; }
    
    int N_CHANGE_CARDS(int cl)noexcept{ return abs(cl - MIDDLE); }
    
    constexpr int REWARD(int cl)noexcept{ return N_PLAYERS - cl; }
    
    constexpr int N_MAX_OWNED_CARDS_CHANGE = 13;
    constexpr int N_MAX_OWNED_CARDS_PLAY = 11;
    
    constexpr int N_MAX_CHANGES = 13 * 12 / 2;
    constexpr int N_MAX_MOVES = 256;
    
#elif _PLAYERS == 4 // 4人時
    
    // 階級
    enum Class{
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
    
    constexpr int N_CHANGE_FALL_CARDS(int cl)noexcept{ return NChangeFallCards[cl]; }
    constexpr int N_CHANGE_RISE_CARDS(int cl)noexcept{ return NChangeRiseCards[cl]; }
    
    constexpr int N_CHANGE_CARDS(int cl)noexcept{ return NChangeCards[cl]; }
    
    constexpr int REWARD(int cl)noexcept{ return N_PLAYERS - cl; }
    
    constexpr int N_MAX_OWNED_CARDS_CHANGE = 16;
    constexpr int N_MAX_OWNED_CARDS_PLAY = 14;
    
    constexpr int N_MAX_CHANGE = 16 * 15 / 2;
    constexpr int N_MAX_MOVES = 1024;
    
#endif
    constexpr int N_SEATS = N_PLAYERS;
    
    constexpr int CLASS_INIT_CYCLE = 100; // 階級リセット試合数(変更の可能性あり)
    constexpr int SEAT_INIT_CYCLE = 3; // 席順リセット試合数(変更の可能性あり)
    
    //const int N_GAMES; // 総試合数
    
    // ルールの性質
    
    // オーダー逆転のパターン
    // スートロックの条件
    // 初手プレーヤーの決め方
    // 場の流れ方
    //
    
    constexpr int getChangePartnerClass(int acl)noexcept{
        // 交換相手の階級
        return N_CLASSES - 1 - acl;
    }
    
    constexpr int getNGamesForClassInitGame(int gn)noexcept{
        // 試合番号から、階級初期化ゲームへの残り試合数に変換
        return CLASS_INIT_CYCLE - 1 - (gn % CLASS_INIT_CYCLE);
    }
    constexpr int getNGamesForSeatInitGame(int gn)noexcept{
        // 試合番号から、席順初期化ゲームへの残り試合数に変換
        return SEAT_INIT_CYCLE - 1 - (gn % SEAT_INIT_CYCLE);
    }
    
    
    template<int N = N_PLAYERS>
    constexpr unsigned int getNextSeat(unsigned int s)noexcept{
        return (s + 1) % (unsigned int)cmax(N, 1);
    }
    template<int N>
    constexpr unsigned int getRemovedNextSeat(unsigned int s)noexcept{
        return s % (unsigned int)cmax(N - 1, 1);
    }
    template<int N = N_PLAYERS>
    constexpr unsigned int getPreviousSeat(unsigned int s)noexcept{
        return (s + N - 1) % (unsigned int)cmax(N, 1);
    }
    
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
    void clearAll(int table[8][15]){
        for(int i = 0; i < 8; ++i){
            for(int j = 0 ; j < 15; ++j){
                table[i][j] = 0;
            }
        }
    }
    void clearCards(int table[8][15]){
        for(int i = 0; i < 5; ++i){
            for(int j = 0; j < 15; ++j){
                table[i][j] = 0;
            }
        }
    }
    
    bool isInitGame(const int table[8][15]){ return (table[6][5] == table[6][6]) ? true : false; } // プレーヤー0と1の階級が同じかどうかで判別
    int getTmpOrder(const int table[8][15]){ return (table[5][5] != table[5][6]) ? 1 : 0; } // 革命とイレバの片方のみ起こっている
    int getPrmOrder(const int table[8][15]){ return table[5][6] ? 1 : 0; } // 革命が起こっている
    int suitsLocked(const int table[8][15]){ return table[5][7]; }
    int isMyTurn(const int table[8][15]){ return table[5][2]; }
    int getTurnPlayer(const int table[8][15]){ return table[5][3]; }
    int isNF(const int table[8][15]){ return table[5][4]; } // 空場
    int getNCards(const int table[8][15], int p){ return table[6][0 + p]; }
    int getPlayerClass(const int table[8][15], int p){ return table[6][5 + p]; }
    int getSeatPlayer(const int table[8][15], int s){ return table[6][10 + s]; } // seat:0-4
    int isInChange(const int table[8][15]){ return table[5][0]; }
    int getChangeQty(const int table[8][15]){ return table[5][1]; }
    
    // コンバート
    // ジョーカー関連は対応していないので注意
    constexpr inline int WToRank(int w){ return w; }
    constexpr inline int HToSuitNum(int h){ return 3 - h; }
    constexpr inline uint32_t HToSuit(int h){ return SuitNumToSuits(HToSuitNum(h)); }
    
    constexpr inline int RankToW(int r){ return r; }
    constexpr inline int SuitNumToH(int sn){ return 3 - sn; }
    inline int SuitToH(uint32_t s){ return SuitNumToH(SuitToSuitNum(s)); }
    
    constexpr inline int IntCardToH(IntCard ic){ return SuitNumToH(IntCardToSuitNum(ic)); }
    constexpr inline int IntCardToW(IntCard ic){ return RankToW(IntCardToRank(ic)); }
    constexpr inline IntCard HWtoIC(int h, int w)noexcept{ return RankSuitNumToIntCard(WToRank(w), HToSuitNum(h)); }
    
    // カード集合変換(chara == true の場合は性質カードに変換)
    Cards TableToCards(const int table[8][15], bool chara = false){
        Cards r = CARDS_NULL;
        bool jk = false;
        for(int h = 0; h < 4; ++h){
            for(int w = 0; w < 15; ++w){
                if(table[h][w]){
                    if(table[h][w] == 1){
                        addIntCard(&r, HWtoIC(h, w));
                    }else if(chara){
                        jk = true;
                        addIntCard(&r, HWtoIC(h, w));
                    }else{
                        addJOKER(&r);
                    }
                }
            }
        }
        // 通常のカード位置以外にあればジョーカー
        for(int w = 0; w < 15; ++w){
            if(table[4][w])addJOKER(&r);
        }
        // ジョーカー1枚使用の場合には chara もジョーカーに変更
        if(jk && countCards(r) == 1)r = CARDS_JOKER;
        return r;
    }
    
    void CardsToTable(const Cards arg, int table[8][15]){
        clearAll(table);
        Cards c = arg;
        if(containsJOKER(c)){
            table[4][1] = 2;
            subtrJOKER(&c);
        }
        while(anyCards(c)){
            IntCard ic = popIntCardLow(&c);
            table[IntCardToH(ic)][IntCardToW(ic)] = 1;
        }
    }
    
    void replaceCards(const Cards arg, int table[8][15]){
        clearCards(table);
        Cards c = arg;
        if(containsJOKER(c)){
            table[4][1] = 2;
            subtrJOKER(&c);
        }
        while(anyCards(c)){
            IntCard ic = popIntCardLow(&c);
            table[IntCardToH(ic)][IntCardToW(ic)] = 1;
        }
    }
    
    // 着手変換
    void MoveToTable(const Move m, const Board bd, int table[8][15]){
        clearAll(table);
        if(m.isPASS()){ return; }
        if(m.isSingleJOKER()){ // シングルジョーカー
            int ord = bd.tmpOrder();
            if(bd.isNF()){
                table[0][(ord == ORDER_NORMAL) ? 14 : 0] = 2;
            }else{
                // ジョーカーでスートロックを掛けないように、別スートになるようにする
                // java版サーバーのリジェクトバグにひっかからないための処理
                uint32_t bs = bd.suits();
                int h = SuitToH(bs);
                h = (h + 1) % 4; // スートを変更
                table[h][(ord == ORDER_NORMAL) ? 14 : 0] = 2;
            }
            return;
        }
        if(m.isQuintuple()){ // クインタプル
            int w = RankToW(m.rank());
            for(int h = 0; h < 4; ++h){
                table[h][w] = 1;
            }
            table[4][w] = 2;
            return;
        }
        // それ以外の役
        Cards charaCards = m.charaCards();
        Cards usedCards = m.cards();
        while(anyCards(charaCards)){
            IntCard ic = popIntCardLow(&charaCards);
            int h = IntCardToH(ic);
            int w = IntCardToW(ic);
            if(containsIntCard(usedCards, ic)){
                table[h][w] = 1;
            }else{ // JOKERが代役
                table[h][w] = 2;
            }
        }
    }
    
    Move TableToMove(const int table[8][15]){
        Cards chara = TableToCards(table, true);
        Cards used = TableToCards(table, false);
        DERR << "chara " << OutCards(chara) << " used " << OutCards(used) << endl;
        return CardsToMove(chara, used);
    }
    
    Board TableToBoard(const int table[8][15]){
        Move mv = TableToMove(table);
        Board bd = MoveToBoard(mv);
        if(suitsLocked(table)){ bd.lockSuits(); }
        bd.setPrmOrder(getPrmOrder(table));
        bd.setTmpOrder(getTmpOrder(table));
        return bd;
    }
    
    std::string toString(const int table[8][15]){ // 出力
        std::ostringstream oss;
        for(int i = 0; i < 8; ++i){
            for(int j = 0; j < 15; ++j){
                oss << std::setw(2) << table[i][j];
            }
            oss << std::endl;
        }
        return oss.str();
    }
}
