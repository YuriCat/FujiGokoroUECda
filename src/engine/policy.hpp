#pragma once

// 線形方策

#include "../core/daifugo.hpp"
#include "../base/softmaxClassifier.hpp"

namespace PlayPolicySpace {
    enum {
        // 後場パラメータ
        FEA_HAND_SNOWL, // snowl点

        FEA_HAND_S3, // ジョーカー-S3
        FEA_HAND_PQR_RANK, // 平均pqrランク項
        FEA_HAND_NF_PARTY, // 最小分割数
        FEA_HAND_P8_JOKER, // ジョーカーと飛ばし重合
        
        // 着手パラメータ
        FEA_MOVE_QTY, // 着手の枚数、空場のみ
        FEA_SUITLOCK_EFFECT, // スートロックの後自分がターンを取れそうか?
        FEA_SAME_QR, // 同じ枚数組の多さに応じた加点

        FEA_REV_CLASS, // 革命優先度
        
        FEA_PASS_PHASE, // 序中終盤でのパス優先度
        FEA_PASS_DOM, // パスをしても場が取れるか

        FEA_PASS_OWNER_DISTANCE, // 現在場役主が場を取ったとして、自分が何人目か

        FEA_PASS_NAWAKE_OWNER, // 自分がパスをした後残っている人数
        
        FEA_MOVE_S3, // JK->S3
        FEA_MOVE_JOKER_AGAINST_S3, // S3返しの危険性に応じてシングルジョーカーの使用を考える
        FEA_MOVE_SEQ, // 階段
        FEA_MOVE_RF_GROUP_BREAK, // 通常場での強カードでのグループ崩し
        
        FEA_MOVE_NFDOM_PASSDOM, // パス支配の場で空場支配役を出すのはどうなのか
        FEA_MOVE_NF_EIGHT_MANY_WEAKERS, // 8より弱いカードが複数ランク(階段除く)ある場合の8
        FEA_MOVE_EIGHT_QTY, // 8と残りカードの枚数
        FEA_MOVE_MIN_RANK,
        FEA_ALMOST_MATE, // 高確率で勝ちの時に勝負に出るか
        
        FEA_GR_CARDS, // MC関係(グループ)
        FEA_SEQ_CARDS, // MC関係(階段)
        FEA_GR_MO, // MO関係(グループ)
        FEA_SEQ_MO, // MO関係(グループ)
        FEA_ALL,
    };
    
    constexpr int feaNumTable[] = {
        //手札
        83 * 2 * 3,
        3, 1, 2, 1,
        4, 3, 3,
        5, // N_PLAYERS,
        3, 2,
        5 - 1, // N_PLAYERS - 1,
        (5 - 1) * 8, // (N_PLAYERS - 1) * 8,
        2, 3, 3, 3,            
        1, 1, 2, 2, 1,

        2 * (16 * 2) * (16) * N_PATTERNS_SUITS_SUITS, // オーダー x 着手ランク x スートロック x 手札ランク x (Suits, Suits)パターン
        2 * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS,  // オーダー x 着手ランク x 枚数 x 手札ランク x (Suit, Suits)パターン
        2 * (16 * 2) * (16) * N_PATTERNS_SUITS_SUITS, // オーダー x 着手ランク x スートロック x 手札ランク x (Suits, Suits)パターン
        2 * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS,  // オーダー x 着手ランク x 枚数 x 手札ランク x (Suit, Suits)パターンåå
    };
    static_assert(FEA_ALL == sizeof(feaNumTable) / sizeof(int), "");
    constexpr int FEA_NUM(unsigned int fea) {
        return feaNumTable[fea];
    }
    constexpr int FEA_IDX(unsigned int fea) {
        return (fea == 0) ? 0 : (FEA_IDX(fea - 1) + FEA_NUM(fea - 1));
    }
    constexpr int FEA_NUM_ALL = FEA_IDX(FEA_ALL);
}
namespace ChangePolicySpace {
    enum {
        FEA_CHANGE_HAND_1PQR_SEQ = 0, // JK以外
        
        FEA_CHANGE_HAND_D3,
        FEA_CHANGE_HAND_JOKER_S3,
        
        FEA_CHANGE_HAND_MAX1_RANK,
        FEA_CHANGE_HAND_MAX2_RANK,
        FEA_CHANGE_HAND_MIN1_RANK,
        FEA_CHANGE_HAND_MIN2_RANK,
        
        FEA_CHANGE_DOUBLE, // 2枚捨ての場合にダブルをあげるかどうか
        FEA_CHANGE_PART_SEQ, // 2枚捨ての場合に3階段の部分集合をあげるかどうか
        
        FEA_CHANGE_CC,
        
        FEA_ALL,
    };
    constexpr int feaNumTable[FEA_ALL] = {
        11 + 10 + 9 + 4 * 13,
        1, 3,
        1, 1, 1, 1,
        1, 2,
        16 * 16 * N_PATTERNS_SUITS_SUITS,
    };
    static_assert(FEA_ALL == sizeof(feaNumTable) / sizeof(int), "");
    constexpr int FEA_NUM(unsigned int fea) {
        return feaNumTable[fea];
    }
    constexpr int FEA_IDX(unsigned int fea) {
        return (fea == 0) ? 0 : (FEA_IDX(fea - 1) + FEA_NUM(fea - 1));
    }
    constexpr int FEA_NUM_ALL = FEA_IDX(FEA_ALL);
}

#define LINEOUT(feature, str) { out << str << endl; int base = FEA_IDX(feature);\
for (int i = 0; i < FEA_NUM(feature); i++) { os(base + i); } out << endl; }
    
#define LINEOUTX(feature, str, x) { out << str << endl; int base = FEA_IDX(feature); int num = FEA_NUM(feature);\
for (int i = 0;;) { os(base + i); i++; if (i >= num) break; if (i % (x) == 0) { out << endl; }} out << endl; }

namespace PlayPolicySpace {
    template <typename T>
    int commentToPolicyParam(std::ostream& out, const T param[FEA_NUM_ALL]) {
        auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };
        
        out << "****** MOVE POLICY ******" << endl;
        
        out << "HAND_SNOWL" << endl;
        int base = FEA_IDX(FEA_HAND_SNOWL);
        std::string situationString[3] = { "NF", "RF", "UR" };
        std::string orderString[2] = { "NML", "REV" };
        for (int s = 0; s < 3; s++) {
            for (int o = 0; o < 2; o++) {
                out << situationString[s] << " " << orderString[o] << endl;
                out << "  SEQ" << endl;
                for (int i = 0; i < 11; i++) os(base + i);
                out << endl;
                for (int i = 0; i < 10; i++) os(base + 11 + i);
                out << endl;
                for (int i = 0; i < 9; i++) os(base + 21 + i);
                out << endl;
                out << "  GROUP" << endl;
                for (int i = 0; i < 13; i++) {
                    for (int j = 0; j < 4; j++) os(base + 30 + 4 * i + j);
                    out << endl;
                }
                out << "  JOKER" << endl;
                os(base + 82); out << endl;
                base += 83;
            }
        }
        
        LINEOUT(FEA_HAND_S3, "JK_S3");
        LINEOUT(FEA_HAND_PQR_RANK, "AVG_PQR");
        LINEOUT(FEA_HAND_NF_PARTY, "NF_PARTY");
        LINEOUT(FEA_HAND_P8_JOKER, "P8_JOKER");
        LINEOUT(FEA_MOVE_QTY, "QTY");
        LINEOUT(FEA_SUITLOCK_EFFECT, "SUITLOCK_EFFECT");
        LINEOUT(FEA_SAME_QR, "SAME_QR");
        LINEOUT(FEA_REV_CLASS, "REV_CLASS");
        LINEOUT(FEA_MOVE_S3, "S3 ON JOKER");
        LINEOUT(FEA_MOVE_JOKER_AGAINST_S3, "JOKER AGAINST S3");
        LINEOUT(FEA_PASS_PHASE, "PASS_PHASE");
        LINEOUT(FEA_PASS_DOM, "PASS_DOM");
        LINEOUT(FEA_PASS_OWNER_DISTANCE, "PASS_OWNER_DISTANCE");
        LINEOUTX(FEA_PASS_NAWAKE_OWNER, "PASS_DOM_NAWAKE_OWNER", 8);
        LINEOUT(FEA_MOVE_SEQ, "SEQ");
        LINEOUT(FEA_MOVE_RF_GROUP_BREAK, "RF_GROUP_BREAK");
        LINEOUT(FEA_MOVE_NFDOM_PASSDOM , "NFDOM_PASSDOM");
        LINEOUT(FEA_MOVE_NF_EIGHT_MANY_WEAKERS, "NF_EIGHT_MANY_WEAKERS_NOREV");
        LINEOUT(FEA_MOVE_EIGHT_QTY, "EIGHT_QTY");
        LINEOUT(FEA_MOVE_MIN_RANK, "MIN_RANK");
        //LINEOUT(,);
        return 0;
    }
}
namespace ChangePolicySpace {
    template <typename T>
    int commentToPolicyParam(std::ostream& out, const T param[FEA_NUM_ALL]) {
        auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };
        
        out << "****** CHANGE POLICY ******" << endl;
        
        {
            out << "1PQR or SEQ" << endl;
            int base = FEA_IDX(FEA_CHANGE_HAND_1PQR_SEQ);
            out << "  SEQ" << endl;
            for (int i = 0; i < 11; i++) { os(base + i); } out << endl;
            for (int i = 0; i < 10; i++) { os(base + 11 + i); } out << endl;
            for (int i = 0; i < 9; i++) { os(base + 21 + i); } out << endl;
            out << "  GROUP" << endl;
            for (int i = 0; i < 13; i++) {
                for (int j = 0; j < 4; j++) { os(base + 30 + 4 * i + j); } out << endl;
            }
        }

        LINEOUT(FEA_CHANGE_HAND_D3, "D3 BONUS");
        LINEOUT(FEA_CHANGE_HAND_JOKER_S3, "S3-JOKER LINK BONUS");
        LINEOUT(FEA_CHANGE_HAND_MAX1_RANK, "MAX1 RANK");
        LINEOUT(FEA_CHANGE_HAND_MAX2_RANK, "MAX2 RANK");
        LINEOUT(FEA_CHANGE_HAND_MIN1_RANK, "MIN1 RANK");
        LINEOUT(FEA_CHANGE_HAND_MIN2_RANK, "MIN2 RANK");
        LINEOUT(FEA_CHANGE_DOUBLE, "DOUBLE PRESENT");
        LINEOUT(FEA_CHANGE_PART_SEQ, "PART SEQ PRESENT");
        //LINEOUT(,);
        return 0;
    }
}
#undef LINEOUTX
#undef LINEOUT

template <typename T> using PlayPolicy = SoftmaxClassifier<PlayPolicySpace::FEA_NUM_ALL, 1, 1, T>;
template <typename T> using PlayPolicyLearner = SoftmaxClassifyLearner<PlayPolicy<T>>;
template <typename T> using ChangePolicy = SoftmaxClassifier<ChangePolicySpace::FEA_NUM_ALL, 2, 1, T>;
template <typename T> using ChangePolicyLearner = SoftmaxClassifyLearner<ChangePolicy<T>>;

template <typename T>
int foutComment(const PlayPolicy<T>& pol, const std::string& fName) {
    std::ofstream ofs(fName, std::ios::out);
    return PlayPolicySpace::commentToPolicyParam(ofs, pol.param_);
}
template <class T>
int foutComment(const ChangePolicy<T>& pol, const std::string& fName) {
    std::ofstream ofs(fName, std::ios::out);
    return ChangePolicySpace::commentToPolicyParam(ofs, pol.param_);
}

extern int playPolicyScore(double *const dst, Move *const mbuf, const int numMoves,
                           const Field& field, const PlayPolicy<policy_value_t>& pol, int mode = 1);
extern int playPolicyScore(double *const dst, Move *const mbuf, const int numMoves,
                           const Field& field, PlayPolicyLearner<policy_value_t>& pol, int mode = 1);
extern int changePolicyScore(double *const dst, const Cards *const change, const int NChanges,
                             const Cards myCards, const int NChangeCards,
                             const ChangePolicy<policy_value_t>& pol, int mode = 1);
extern int changePolicyScore(double *const dst, const Cards *const change, const int NChanges,
                             const Cards myCards, const int NChangeCards,
                             ChangePolicyLearner<policy_value_t>& pol, int mode = 1);

template <int STOCK = 0, class policy_t, class dice_t>
int playWithPolicy(Move *const mbuf, const int NMoves, const policy_t& pol, dice_t& dice,
                   double *const pentropy = nullptr) {
    double score[N_MAX_MOVES];
    playPolicyScore(score, mbuf, NMoves, pol, STOCK ? 2 : 0);
    SoftmaxSelector<double> selector(score, NMoves, pol.temperature());
    if (pentropy != nullptr) *pentropy = selector.entropy();
    return selector.select(dice.random());
}

template <class policy_t, class dice_t>
int changeWithPolicy(Cards *const cbuf, const int NChanges,
                     const Cards myCards, const int NChangeCards,
                     const policy_t& pol, dice_t& dice) {
    double score[N_MAX_CHANGES];
    changePolicyScore(score, cbuf, NChanges, myCards, NChangeCards, pol, 0);
    SoftmaxSelector<double> selector(score, NChanges, pol.temperature());
    return selector.select(dice.random());
}
template <class policy_t, class dice_t>
int changeWithBestPolicy(const Cards *const cbuf, const int NChanges,
                         const Cards myCards, const int NChangeCards,
                         const policy_t& pol, dice_t& dice) {
    double score[N_MAX_CHANGES + 1];
    changePolicyScore(score, cbuf, NChanges, myCards, NChangeCards, pol, 0);
    int bestIndex[N_MAX_CHANGES];
    bestIndex[0] = -1;
    int NBestMoves = 0;
    double bestScore = -DBL_MAX;
    for (int m = 0; m < NChanges; m++) {
        double s = score[m];
        if (s > bestScore) {
            bestIndex[0] = m;
            bestScore = s;
            NBestMoves = 1;
        } else if (s == bestScore) {
            bestIndex[NBestMoves++] = m;
        }
    }
    if (NBestMoves <= 1) return bestIndex[0];
    else return bestIndex[dice() % NBestMoves];
}
template <int STOCK = 0, class policy_t, class dice_t>
int playWithBestPolicy(Move *const mbuf, const int NMoves, const Field& field, const policy_t& pol, dice_t& dice) {
    double score[N_MAX_MOVES + 1];
    playPolicyScore(score, mbuf, NMoves, field, pol, STOCK ? 2 : 0);
    int bestIndex[N_MAX_MOVES];
    bestIndex[0] = -1;
    int NBestMoves = 0;
    double bestScore = -DBL_MAX;
    for (int m = 0; m < NMoves; m++) {
        double s = score[m];
        if (s > bestScore) {
            bestIndex[0] = m;
            bestScore = s;
            NBestMoves = 1;
        } else if (s == bestScore) {
            bestIndex[NBestMoves++] = m;
        }
    }
    if (NBestMoves <= 1) return bestIndex[0];
    else return bestIndex[dice() % NBestMoves];
}