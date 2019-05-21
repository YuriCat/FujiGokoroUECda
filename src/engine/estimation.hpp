#pragma once

// 相手手札を推定して配布

#include "../core/field.hpp"
#include "../core/record.hpp"
#include "../core/action.hpp"
#include "data.hpp"
#include "mate.hpp"
#include "policy.hpp"

namespace Settings {
    const double estimationTemperaturePlay = 1.1;
}

class RandomDealer {
    // ランダムに手札配置を作る
    // 基本的に連続分配を想定しているので、異なるプレーヤーが互いに分配するような状況ではインスタンスを複数作るべき
    static constexpr int N = N_PLAYERS;
public:
    RandomDealer(const Field& f, int turn) {
        set(f, turn);
    }

    template <class gameRecord_t>
    int create(ImaginaryWorld *const dst, DealType type, const gameRecord_t& record,
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
                dealWithRejection(c, record, shared, ptools); break;
            default: UNREACHABLE; break;
        }
        dst->set(turnCount, c);
        return 0;
    }
    
    void dealAllRand(Cards *const dst, Dice& dice) const;
    void dealWithSubjectiveInfo(Cards *const dst, Dice& dice) const;
    void dealWithBias(Cards *const dst, Dice& dice) const;
    template <class gameRecord_t>
    void dealWithRejection(Cards *const dst,
                           const gameRecord_t& record,
                           const SharedData& shared,
                           ThreadTools *const ptools) {
        // 採択棄却法メイン
        Cards deal[BUCKET_MAX][N]; // カード配置候補
        int bestDeal = 0;
        for (int i = 0; i < buckets; i++) {
            if (failed) {
                dealWithBias(deal[i], ptools->dice);
            } else {
                bool ok = dealWithChangeRejection(deal[i], shared, ptools);
                // 失敗の回数が一定値を超えた以降は逆関数法に移行
                if (!ok && ++failures > 1) failed = true;
            }
        }
        // 役提出の尤度を計算してし使用する手札配置を決定
        if (buckets > 1) {
            double lhs[BUCKET_MAX];
            for (int i = 0; i < buckets; i++) {
                lhs[i] = playLikelihood(deal[i], record, shared, ptools);
            }
            SoftmaxSelector<double> selector(lhs, buckets, 0.3);
            bestDeal = selector.select(ptools->dice.random());
        }
        for (int p = 0; p < N; p++) dst[p] = deal[bestDeal][p];
        checkDeal(dst);
    }

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
    
    // 採択棄却法時の棄却回数限度
    static constexpr int MAX_REJECTION = 800;
    
    // 自分が上位のときに献上された最小の札の確率
    int candidatesInWA;
    double thresholdInWA[16];
    Cards dealCardsUnderInWA[16];
    Cards dealCardsOverInWA[16];
    
    // 進行得点関連
    static constexpr int BUCKET_MAX = 32;
    int buckets;
    
    // 着手について検討の必要があるプレーヤーフラグ
    std::bitset<N_PLAYERS> playFlag;
    
    // inner function
    void set(const Field& field, int playerNum);
    void setWeightInWA();
    void prepareSubjectiveInfo();

    void checkDeal(Cards *const dst, bool sbj = true) const;
    bool okForRejection() const;
    bool dealWithChangeRejection(Cards *const dst,
                                 const SharedData& shared,
                                 ThreadTools *const ptools) const;
    double onePlayLikelihood(const Field& field, Move move,
                             const SharedData& shared, ThreadTools *const ptools) const;

    // 採択棄却法のためのカード交換モデル
    Cards change(const int p, const Cards cards, const int qty,
                 const SharedData& shared, ThreadTools *const ptools) const;
    Cards selectInWA(double urand) const;

    template <class gameRecord_t>
    double playLikelihood(Cards *const c, const gameRecord_t& gLog,
                          const SharedData& shared, ThreadTools *const ptools) const {
        // 想定した手札配置から、試合進行がどの程度それっぽいか考える
        if (inChange) return 0;
        double playllh = 0; // 対数尤度
        std::array<Cards, N> orgCards;
        for (int p = 0; p < N; p++) {
            orgCards[p] = c[p] + usedCards[infoClass[p]];
        }
        Field field;
        iterateGameLogInGame
        (field, gLog, gLog.plays(), orgCards,
        // after change callback
        [](const Field& field)->void{},
        // play callback
        [this, &shared, ptools, &playllh]
        (const Field& field, Move move, unsigned time)->int{
            int turn = field.turn();
            if (field.turnCount() >= turnCount) return -1; // 決めたところまで読み終えた(オフラインでの判定用)
            if (!holdsCards(field.getCards(turn), move.cards())) return -1; // 終了(エラー)
            // カードが全確定しているプレーヤー(主に自分と、既に上がったプレーヤー)については考慮しない
            if (!playFlag.test(turn)) return 0;
            double lh = onePlayLikelihood(field, move, shared, ptools);
            if (lh < 1) playllh += log(lh);
            return 0;
        });
        return playllh;
    }
};