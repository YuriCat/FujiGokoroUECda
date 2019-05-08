#pragma once

// 相手手札を推定して配布

#include "../core/field.hpp"
#include "../core/record.hpp"
#include "../core/action.hpp"
#include "data.hpp"
#include "mate.hpp"
#include "linearPolicy.hpp"

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
    World create(DealType type, const gameRecord_t& record,
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
        return World(turnCount, c);
    }
    
    void dealAllRand(Cards *const dst, Dice& dice) const;
    void dealWithSubjectiveInfo(Cards *const dst, Dice& dice) const;
    void dealWithBias(Cards *const dst, Dice& dice) const;
    template <class gameRecord_t>
    void dealWithRejection(Cards *const dst,
                           const gameRecord_t& record,
                           const SharedData& shared,
                           ThreadTools *const ptools) {
        int bestDeal = 0;
        // カード交換の効果を考慮して手札を分配
        //for (int i = 0; i < 100; i++) {
        while (worldPool.size() < 1024) {
            // まず主観情報から矛盾の無いように配る
            Cards c[N_PLAYERS];
            dealWithSubjectiveInfo(c, ptools->dice);
            World world(turnCount, c);
            // 既にプールにある場合は飛ばす
            if (worldPool.count(world)) continue;
            // 交換尤度を計算する
            double changellh = changeLikelihood(world.cards, shared, ptools);
            double playllh = playLikelihood(world.cards, record, shared, ptools);
            double temperature = 1;
            world.likelihood = std::exp((changellh + playllh) / temperature);
            //cerr << world.key << " " << changellh << " " << playllh << endl;
            // 世界プールに入れる
            worldPool.insert(world);
            wholeLikelihood += world.likelihood;
        }
        // 世界を選ぶ
        cerr << worldPool.size() << endl;
        double r = ptools->dice.random() * wholeLikelihood;
        World chosen;
        for (auto& w : worldPool) {
            r -= w.likelihood;
            if (r <= 0) {
                chosen = w;
                break;
            }
        }
        // 選ばれた世界を抜く
        wholeLikelihood -= chosen.likelihood;
        worldPool.erase(chosen);
        for (int p = 0; p < N; p++) dst[p] = chosen.cards[p];
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
    Cards remCards;  // まだ使用されていないカード
    Cards dealCards; // まだ特定されていないカード
    std::array<Cards, N> usedCards; // 使用済みカード
    std::array<Cards, N> detCards; // 現時点で所持が特定されている、またはすでに使用したカード

    std::unordered_set<World, World::Hash> worldPool;
    double wholeLikelihood = 0;

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
    double dealWithChangeRejection(Cards *const dst,
                                   const SharedData& shared, ThreadTools *const ptools);
    double changeLikelihood(const Cards *const dst,
                            const SharedData& shared, ThreadTools *const ptools);
    double onePlayLikelihood(const Field& field, Move move, unsigned time,
                             const SharedData& shared, ThreadTools *const ptools) const;

    // 採択棄却法のためのカード交換モデル
    Cards change(const int p, const Cards cards, const int qty,
                 const SharedData& shared, ThreadTools *const ptools) const;
    Cards selectInWA(double urand) const;

    template <class gameRecord_t>
    double playLikelihood(const Cards *const c, const gameRecord_t& gLog,
                          const SharedData& shared, ThreadTools *const ptools) const {
        // 想定した手札配置から、試合進行がどの程度それっぽいか考える
        double playllh = 0; // 対数尤度
        std::array<Cards, N> orgCards;
        for (int p = 0; p < N; p++) orgCards[p] = c[p] | detCards[infoClass[p]];
        Field field;
        iterateGameLogInGame
        (field, gLog, gLog.plays(), orgCards,
        // after change callback
        [](const Field& field)->void{},
        // play callback
        [this, &playllh, &orgCards, &shared, ptools]
        (const Field& field, Move chosen, unsigned time)->int{
            // 決めたところまで読み終えた場合は終了(オフラインでの判定用)
            if (field.turnCount() >= turnCount) return -1;
            // カードが全確定しているプレーヤーについては考慮しない
            if (!playFlag.test(field.turn())) return 0;
            // 一手分の尤度を追加
            double prob = onePlayLikelihood(field, chosen, time, shared, ptools);
            playllh += std::log(prob);
            return 0;
        });
        return playllh;
    }
};