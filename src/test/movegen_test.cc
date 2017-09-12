/*
 movegen_test.cc
 Katsuki Ohto
 */

// 着手生成のテスト

#include "../settings.h"

// アナライザを強制使用にする
#ifndef USE_ANALYZER
#define USE_ANALYZER
#endif

#include "../include.h"
#include "../core/moveGenerator.hpp"
#include "../core/minLog.hpp"
#include "../core/field.hpp"

using namespace UECda;

MoveInfo buffer[8192];
MoveGenerator<MoveInfo, Cards> mgCards;
MoveGenerator<MoveInfo, Hand> mgHand;
Clock cl;
std::mt19937 mt;

#define GENERATION_CASE(c, label) {\
static_assert(countFewCards(c) == N_MAX_OWNED_CARDS_PLAY, "sample is not correct.");\
int moves = mgCards.genLead(buffer, c);\
cerr << moves << " moves were generated for " << OutCards(c) << endl;\
for(int m = 0; m < moves; ++m){ cerr << Move(buffer[m]) << " "; }\
cerr << endl;}

int outputGenerationResult(){
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

template<class move_t, class field_t>
int testMoveValidity(const move_t *const mv0, const int moves, const field_t& field){
    // moves 個生成された着手をチェック
    const Cards c = field.getCards(field.getTurnPlayer());
    
    // 客観的合法性
    
    // 主観的合法性
    for(const move_t *tmp = mv0; tmp != mv0 + moves; ++tmp){
        if(!isSubjectivelyValid(field.getBoard(), tmp->mv(), c,
                                field.getNCards(field.getTurnPlayer()))){
            // 主観的合法性違反
            cerr << "invalid move" << OutCards(c) << "(" << field.getNCards(field.getTurnPlayer()) << ")";
            cerr << " -> " << *tmp << " on " << field.getBoard() << endl;
            return -1;
        }
    }
    
    // 排他性
    for(const move_t *tmp = mv0; tmp != mv0 + moves; ++tmp){
        for(const move_t *tmp2 = mv0; tmp2 != tmp; ++tmp2){
            if(tmp->meldPart() == tmp2->meldPart()){
                // 同じ役であった
                cerr << "same meld " << OutCards(c) << " -> " << *tmp << " <-> " << *tmp2 << " on " << field.getBoard() << endl;
                return -1;
            }
        }
    }
    
    return 0;
    
}

int testRecordMoves(const std::vector<std::string>& logs){
    // 棋譜中の局面においてテスト
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog>>, 256> mLogs(logs);
    
    uint64_t genTime[2] = {0};
    uint64_t genCount[2] = {0};
    uint64_t genHolded[2] = {0};
    Field field;
    
    // プリミティブ型での着手生成
    if(iterateGameLogAfterChange
       (field, mLogs,
        [&](const auto& field){}, // first callback
        [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.getTurnPlayer();
            Cards cards = field.getCards(turnPlayer);
            Board bd = field.getBoard();
            
            // Cards 型で生成
            cl.start();
            int moves = mgCards.genMove(buffer, cards, bd);
            genTime[0] += cl.stop();
            genCount[0] += 1;
            
            // 重大な問題がないかチェック
            if(testMoveValidity(buffer, moves, field)){
                return -4;
            }
            
            // 棋譜の着手が生成されているかチェック
            if(searchMove(buffer, moves, MoveInfo(move)) < 0){
                cerr << "ungenerated record move " << move;
                cerr << " " << std::hex << move.meldPart() << std::dec;
                cerr << " by " << OutCards(cards) << " on " << bd << endl;
            }else{
                genHolded[0] += 1;
            }
            
            return 0;
        },
        [&](const auto& field){} // last callback
        )){
           cerr << "failed move generation by Cards." << endl;
           return -1;
       }
    
    // より複雑なデータ構造を用いた着手生成
    if(iterateGameLogAfterChange
       (field, mLogs,
        [&](const auto& field){}, // first callback
        [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.getTurnPlayer();
            Board bd = field.getBoard();
            const Hand& hand = field.getHand(turnPlayer);
            
            // Hand 型で生成
            cl.start();
            int moves = mgHand.genMove(buffer, hand, bd);
            genTime[1] += cl.stop();
            genCount[1] += 1;

            // 重大な問題がないかチェック
            if(testMoveValidity(buffer, moves, field)){
                return -4;
            }
            
            // 棋譜の着手が生成されているかチェック
            if(searchMove(buffer, moves, MoveInfo(move)) < 0){
                cerr << "ungenerated record move " << move;
                cerr << " " << std::hex << move.meldPart() << std::dec;
                cerr << " by " << OutCards(hand.getCards()) << " on " << bd << endl;
            }else{
                genHolded[1] += 1;
            }
            
            return 0;
        },
        [&](const auto& field){} // last callback
        )){
           cerr << "failed move generation by Hand." << endl;
           return -1;
       }
    
    cerr << "generation rate (cards) = " << genHolded[0] / (double)genCount[0] << endl;
    cerr << "generation rate (hand)  = " << genHolded[1] / (double)genCount[1] << endl;
    
    cerr << "generation time (cards) = " << genTime[0] / (double)genCount[0] << endl;
    cerr << "generation time (hand)  = " << genTime[1] / (double)genCount[1] << endl;
    
    // 着手生成の一貫性
    if(iterateGameLogAfterChange
       (field, mLogs,
        [&](const auto& field){}, // first callback
        [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
            int turnPlayer = field.getTurnPlayer();
            Board bd = field.getBoard();
            Cards cards = field.getCards(turnPlayer);
            const Hand& hand = field.getHand(turnPlayer);
            
            // Cards 型で生成
            int movesCards = genMove(buffer, cards, bd);
            // Hand 型で生成
            int movesHand = genMove(buffer + movesCards, hand, bd);
            
            if(movesCards != movesHand){
                cerr << "different numbers of moves ";
                cerr << movesCards << " (by Cards) <-> " << movesHand << " (by Hand)" << endl;
                return -4;
            }
            
            for(int i = 0; i < movesCards; ++i){
                int cnt = 0;
                for(int j = 0; j < movesHand; ++j){
                    if(buffer[i].mv() == buffer[movesCards + j].mv()){
                        cnt += 1;
                    }
                }
                if(cnt <= 0){
                    cerr << buffer[i] << " was generated by Cards but not by Hand." << endl;
                    return -4;
                }
                if(cnt >= 2){
                    cerr << buffer[i] << " generated by Cards were generated ";
                    cerr << cnt << " times by Hand." << endl;
                    return -4;
                }
            }
            
            return 0;
        },
        [&](const auto& field){} // last callback
        )){
           cerr << "failed Cards <-> Hand generation consistency test." << endl;
           return -1;
       }
    cerr << "passed Cards <-> Hand generation consistency test." << endl;
    
    return 0;
}

int main(int argc, char* argv[]){
    
    std::vector<std::string> logFileNames;
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-l")){
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    mt.seed(1);
    
    if(outputGenerationResult()){
        cerr << "failed case test." << endl;
        return -1;
    }
    cerr << "passed case test." << endl;
    if(testRecordMoves(logFileNames)){
        cerr << "failed record moves generation test." << endl;
        return -1;
    }
    cerr << "passed record moves generation test." << endl;
    
    return 0;
}
