
// 着手生成のテスト

#include "../core/action.hpp"
#include "../core/record.hpp"
#include "../core/field.hpp"
#include "test.h"

using namespace std;

static MoveInfo buffer[8192];
static Clock cl;
static std::mt19937 mt;

#define GENERATION_CASE(c, label) {\
assert(c.countInCompileTime() == N_MAX_OWNED_CARDS_PLAY);\
int numMoves = genLead(buffer, c);\
cerr << numMoves << " moves were generated for " << c << endl;\
for (int m = 0; m < numMoves; m++) { cerr << buffer[m] << " "; }\
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
int testMoveValidity(const move_t *const mv0, const int numMoves, const Field& field) {
    // numMoves 個生成された着手をチェック
    const Cards c = field.getCards(field.turn());

    // 客観的合法性

    // 主観的合法性
    for (const move_t *tmp = mv0; tmp != mv0 + numMoves; tmp++) {
        if (!isSubjectivelyValid(field.board, *tmp, c,
                                 field.numCardsOf(field.turn()))) {
            // 主観的合法性違反
            cerr << "invalid move" << c << "(" << field.numCardsOf(field.turn()) << ")";
            cerr << " -> " << *tmp << " on " << field.board << endl;
            return -1;
        }
    }

    // 排他性
    for (const move_t *tmp = mv0; tmp != mv0 + numMoves; tmp++) {
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
    // 棋譜中の局面においてテスト
    uint64_t genTime[2] = {0};
    uint64_t genCount[2] = {0};
    uint64_t genHolded[2] = {0};

    // プリミティブ型での着手生成
    for (int i = 0; i < record.games(); i++) {
        Field field;
        for (Move move : PlayRoller(field, record.game(i))) {
            int turnPlayer = field.turn();
            Cards cards = field.getCards(turnPlayer);
            Board b = field.board;

            // Cards 型で生成
            cl.start();
            int numMoves = genMove(buffer, cards, b);
            genTime[0] += cl.stop();
            genCount[0] += 1;

            // 重大な問題がないかチェック
            if (testMoveValidity(buffer, numMoves, field)) {
                cerr << "failed move generation by Cards." << endl;
                return -1;
            }

            // 棋譜の着手が生成されているかチェック
            if (searchMove(buffer, numMoves, move) < 0) {
                cerr << "ungenerated record move " << move;
                cerr << " " << std::hex << move.toInt() << std::dec;
                cerr << " by " << cards << " on " << b << endl;
            } else {
                genHolded[0] += 1;
            }
        }
    }

    cerr << "generation rate = " << genHolded[0] / (double)genCount[0] << endl;
    cerr << "generation time = " << genTime[0] / (double)genCount[0] << endl;

    return 0;
}

bool MovegenTest(const vector<string>& recordFiles) {
    mt.seed(1);

    if (outputGenerationResult()) {
        cerr << "failed case test." << endl;
        return false;
    }
    cerr << "passed case test." << endl;
    Record record(recordFiles);
    if (testRecordMoves(recordFiles)) {
        cerr << "failed record moves generation test." << endl;
        return false;
    }
    cerr << "passed record moves generation test." << endl;

    return true;
}
