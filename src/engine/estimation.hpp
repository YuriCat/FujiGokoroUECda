#pragma once

#include "../core/field.hpp"
#include "../core/record.hpp"
#include "data.hpp"

enum DealType {
    RANDOM, SBJINFO, BIAS, REJECTION,
};

class RandomDealer {
    // ランダムに手札配置を作る
    // 基本的に連続分配を想定しているので、異なるプレーヤーが互いに分配するような状況ではインスタンスを複数作るべき
    static constexpr int N = N_PLAYERS;
public:
    RandomDealer(const Field& f, int turn) {
        set(f, turn);
    }

    World create(DealType type, const GameRecord& game,
                 const SharedData& shared, ThreadTools *const ptools) {
        Cards c[N_PLAYERS];
        // レベル指定からカード分配
        switch (type) {
            case DealType::RANDOM: // 残りカードを完全ランダム分配
                dealAllRand(c, ptools->dice); break;
            case DealType::SBJINFO: // 交換等は考慮するが残りは完全ランダム
                dealWithSubjectiveInfo(c, ptools->dice); break;
            case DealType::BIAS: // 逆関数法でバイアスを掛けて配る*/
                dealWithBias(c, ptools->dice); break;
            case DealType::REJECTION: // 採択棄却法で良さそうな配置のみ返す
                dealWithRejection(c, game, shared, ptools); break;
            default: UNREACHABLE; break;
        }
        World world;
        world.set(turnCount, c);
        return world;
    }
    
    void dealAllRand(Cards *const dst, Dice& dice) const;
    void dealWithSubjectiveInfo(Cards *const dst, Dice& dice) const;
    void dealWithBias(Cards *const dst, Dice& dice) const;
    void dealWithRejection(Cards *const dst, const GameRecord& game,
                           const SharedData& shared, ThreadTools *const ptools);

private:
    // 棋譜
    int turnCount;
    
    // 引数は交換ありの場合は階級、交換無しの場合はプレーヤー番号と同じ
    std::array<int8_t, N> NOwn;  // 持っているカード数
    std::array<int8_t, N> NDet;  // 特定＋使用カード数
    std::array<int8_t, N> NDeal; // 配布カード数
    std::array<int8_t, N> NOrg;  // 初期配布カード数
    std::array<int8_t, N> infoClassPlayer;
    std::array<int8_t, N> infoClass;
    unsigned NdealCards;
    Cards dealCards; // まだ特定されていないカード
    Cards remCards; // まだ使用されていないカード
    std::array<Cards, N> usedCards; // 使用済みカード
    std::array<Cards, N> detCards; // 現時点で所持が特定されている、またはすでに使用したカード

    int myNum, myClass;
    int myChangePartner, firstTurnClass;

    Cards myCards;
    Cards myDealtCards; // 配布時
    Cards recvCards, sentCards;
    
    bool inChange;
    bool initGame;
    bool failed;
    unsigned failures;
    
    // 自分が上位のときに献上された最小の札の確率
    int candidatesInWA;
    double thresholdInWA[16];
    Cards dealCardsUnderInWA[16];
    Cards dealCardsOverInWA[16];

    int buckets;
    
    // 着手について検討の必要があるプレーヤーフラグ
    std::bitset<N_PLAYERS> playFlag;
    
    // inner function
    void set(const Field& field, int playerNum);
    void prepareSubjectiveInfo();
    void setWeightInWA();

    void addDetedtedCards(int dstClass, Cards c) {
        detCards[dstClass] += c;
        dealCards -= c;
        int n = c.count();
        NDeal[dstClass] -= n;
        NDet[dstClass] += n;
    }
    void checkDeal(const Cards *dst, bool sbj = true) const;
    bool okForRejection() const;
    bool dealWithChangeRejection(Cards *const dst,
                                 const SharedData& shared,
                                 ThreadTools *const ptools) const;

    // 採択棄却法のためのカード交換モデル
    Cards change(const int p, const Cards cards, const int qty,
                 const SharedData& shared, ThreadTools *const ptools) const;
    Cards selectInWA(double urand) const;

    double onePlayLikelihood(const Field& field, Move move,
                             const SharedData& shared, ThreadTools *const ptools) const;
    double playLikelihood(const Cards *c, const GameRecord& game,
                          const SharedData& shared, ThreadTools *const ptools) const;
};