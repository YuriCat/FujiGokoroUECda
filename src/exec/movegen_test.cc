
// 着手生成のテスト

#include "../settings.h"

// アナライザを強制使用にする
#ifndef USE_ANALYZER
#define USE_ANALYZER
#endif

#include "../include.h"
#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"

using namespace UECda;

MoveInfo buffer[8192];
Clock cl;
std::mt19937 mt;

#define GENERATION_CASE(c, label) {\
assert(c.countInCompileTime() == N_MAX_OWNED_CARDS_PLAY);\
int moves = genLead(buffer, c);\
cerr << moves << " moves were generated for " << c << endl;\
for (int m = 0; m < moves; ++m) { cerr << buffer[m].mv() << " "; }\
cerr << endl;}

int outputGenerationResult() {
    // 気になるケースやコーナーケース、代表的なケースでの着手生成の結果を出力する

    // コーナーケースでの生成着手数
    const Cards groups = CARDS_JOKER | CARDS_A | CARDS_K | IntCardToCards(INTCARD_CQ) | IntCardToCards(INTCARD_DQ);
    GENERATION_CASE(groups, "groups");
    
    const Cards groups2 = CARDS_JOKER | CARDS_A | CARDS_Q | IntCardToCards(INTCARD_CK) | IntCardToCards(INTCARD_DK);
    GENERATION_CASE(groups2, "groups2");
    
    const Cards sequences = CARDS_JOKER | (CARDS_S & RankRangeToCards(RANK_6, RANK_2));
    GENERATION_CASE(sequences, "sequences");
    
    const Cards sequences2 = CARDS_JOKER | (CARDS_S & (RankRangeToCards(RANK_5, RANK_9) | RankRangeToCards(RANK_J, RANK_2)));
    GENERATION_CASE(sequences2, "sequences2");
    
    const Cards grseq = CARDS_JOKER | (CARDS_HS & RankRangeToCards(RANK_5, RANK_9));
    GENERATION_CASE(grseq, "grseq");
    
    return 0;
}

template <class move_t>
int testMoveValidity(const move_t *const mv0, const int moves, const Field& field) {
    // moves 個生成された着手をチェック
    const Cards c = field.getCards(field.turn());
    
    // 客観的合法性
    
    // 主観的合法性
    for (const move_t *tmp = mv0; tmp != mv0 + moves; tmp++) {
        if (!isSubjectivelyValid(field.board, tmp->mv(), c,
                                 field.getNCards(field.turn()))) {
            // 主観的合法性違反
            cerr << "invalid move" << c << "(" << field.getNCards(field.turn()) << ")";
            cerr << " -> " << *tmp << " on " << field.board << endl;
            return -1;
        }
    }
    
    // 排他性
    for (const move_t *tmp = mv0; tmp != mv0 + moves; tmp++) {
        for (const move_t *tmp2 = mv0; tmp2 != tmp; tmp2++) {
            if (*tmp == *tmp2) {
                // 同じ役であった
                cerr << "same meld " << c << " -> " << *tmp << " <-> " << *tmp2 << " on " << field.board << endl;
                return -1;
            }
        }
    }
    
    return 0;
    
}

int testRecordMoves(const Record& record) {
    // 棋譜中の局面においてテスト
    uint64_t genTime[2] = {0};
    uint64_t genCount[2] = {0};
    uint64_t genHolded[2] = {0};
    Field field;
    
    // プリミティブ型での着手生成
    for (int i = 0; i < record.games(); i++) {
        if (iterateGameLogAfterChange
        (field, record.game(i),
            [&](const auto& field) {}, // first callback
            [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
                int turnPlayer = field.turn();
                Cards cards = field.getCards(turnPlayer);
                Board bd = field.board;
                
                // Cards 型で生成
                cl.start();
                int moves = genMove(buffer, cards, bd);
                genTime[0] += cl.stop();
                genCount[0] += 1;
                
                // 重大な問題がないかチェック
                if (testMoveValidity(buffer, moves, field)) {
                    return -4;
                }
                
                // 棋譜の着手が生成されているかチェック
                if (searchMove(buffer, moves, MoveInfo(move)) < 0) {
                    cerr << "ungenerated record move " << move;
                    cerr << " " << std::hex << move.toInt() << std::dec;
                    cerr << " by " << cards << " on " << bd << endl;
                } else {
                    genHolded[0] += 1;
                }
                
                return 0;
            },
            [&](const auto& field) {} // last callback
            )) {
            cerr << "failed move generation by Cards." << endl;
            return -1;
        }
    }
    cerr << "generation rate (cards) = " << genHolded[0] / (double)genCount[0] << endl;
    cerr << "generation time (cards) = " << genTime[0] / (double)genCount[0] << endl;
    return 0;
}

int main(int argc, char* argv[]) {
    
    std::vector<std::string> logFileNames;
    
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-l")) {
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    mt.seed(1);
    
    if (outputGenerationResult()) {
        cerr << "failed case test." << endl;
        return -1;
    }
    cerr << "passed case test." << endl;
    Record record(logFileNames);
    if (testRecordMoves(logFileNames)) {
        cerr << "failed record moves generation test." << endl;
        return -1;
    }
    cerr << "passed record moves generation test." << endl;
    
    return 0;
}
