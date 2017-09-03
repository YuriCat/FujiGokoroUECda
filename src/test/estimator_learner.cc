/*
 esimator_learner.cc
 Katsuki Ohto
 */

// 手札推定関連の統計情報をまとめて学習をかける

#include "../include.h"
#include "../structure/log/minLog.hpp"
#include "../generator/changeGenerator.hpp"
#include "../generator/moveGenerator.hpp"
#include "../fuji/montecarlo/playout.h"
#include "../fuji/policy/changePolicy.hpp"
#include "../fuji/policy/playPolicy.hpp"

#include "../fuji/model/playerModel.hpp"
#include "../fuji/model/playerBias.hpp"

struct ThreadTools{
    MoveInfo buffer[8192];
    XorShift64 dice;
};

/*struct SubjectivePlayouterField : public PlayouterField{
    // シミュレーション用局面情報の主観化
    int myPlayerNum;
    
    int getMyPlayerNum()const noexcept{ return myPlayerNum; }
    Cards getMyCards()const{ return getCards(getMyPlayerNum()); }
    const Hand& myHand()const{ return getHand(getMyPlayerNum()); }
    Cards getOpsCards()const{ return getOpsCards(getMyPlayerNum()); }
    const Hand& getOpsHand()const{ return getOpsHand(getMyPlayerNum()); }
    Cards getSentCards()const{ return getSentCards(getMyPlayerNum()); }
    Cards getRecvCards()const{ return getRecvCards(getRecvCards()); }
    
    SubjectivePlayouterField(const PlayouterField& objField, consy int ap):
    PlayouterField(objField), myPlayerNum(ap){}
};*/

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

using namespace UECda;
using namespace UECda::Fuji;

Clock cl;
ThreadTools threadTools;

template<class logs_t>
int makeCardPriorTable(const logs_t& mLogs, const uint64_t N){
    // Card 単体での事前分布テーブルを作る
    double table[INTCARD_MAX + 1][N_PLAYERS] = {0};

    Field field;
    iterateGameLogAfterChange
    (field, mLogs,
     [&](const auto& field){ // first callback
         // 交換の無い試合はとばす
         if(!field.isInitGame()){
             // 各階級の初期手札を記録
             for(int cl = DAIFUGO; cl <= DAIHINMIN; ++cl){
                 iterateIntCard(field.getCards(field.getClassPlayer(cl)),
                 [&](IntCard ic)->void{
                     table[ic][cl] += 1;
                 });
             }
         }
     },
     [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
         return -1;
     },
     [&](const auto& field){} // last callback
     );
    // 両方向で正規化
    double cardSum[INTCARD_MAX + 1] = {0};
    for(IntCard ic = INTCARD_PLAIN_MIN; ic <= INTCARD_PLAIN_MAX; ++ic){
        cardSum[ic] = N_PLAYERS;
    }
    cardSum[INTCARD_JOKER] = N_PLAYERS;
    double playerSum[N_PLAYERS] = {0};
    for(int p = 0; p < N_PLAYERS; ++p){
        playerSum[p] = N_CARDS;
    }
    normalize2<INTCARD_MAX + 1, N_PLAYERS>(table, 16, playerSum, cardSum);
    
    // 1枚をNとして整数で表示
    uint64_t intTable[INTCARD_MAX + 1][N_PLAYERS] = {0};
    for(int p = 0; p < N_PLAYERS; ++p){
        for(IntCard ic = INTCARD_MIN; ic <= INTCARD_MAX; ++ic){
            intTable[ic][p] = table[ic][p] * N;
        }
    }
    
    // テーブル表示
    cerr << "{" << endl;
    for(IntCard ic = 0; ic <= INTCARD_MAX; ++ic){
        cerr << toString(intTable[ic], N_PLAYERS) << "," << endl;
    }cerr << "};" << endl;
    return 0;
}

template<class logs_t>
int makeRankSuitsPriorTable(const logs_t& mLogs, const uint64_t N){
    // Rank x Suits の事前分布テーブルを作る
    double table[N_PLAYERS][16][16] = {0};
    
    Field field;
    iterateGameLogAfterChange
    (field, mLogs,
     [&](const auto& field){ // first callback
         // 交換の無い試合はとばす
         if(!field.isInitGame()){
             // 各階級の初期手札を記録
             for(int cl = DAIFUGO; cl <= DAIHINMIN; ++cl){
                 for(int r = 0; r < 16; ++r){
                     table[cl][r][getSuits(field.getCards(field.getClassPlayer(cl)), r)] += 1;
                 }
             }
         }
     },
     [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
         return -1;
     },
     [&](const auto& field){} // last callback
     );
    // 全方向で正規化
    // 特にスート方向の正規化は単純加算ではなく枚数加算になるので注意
    /*double cardSum[INTCARD_MAX + 1] = {0};
    for(IntCard ic = INTCARD_PLAIN_MIN; ic <= INTCARD_PLAIN_MAX; ++ic){
        cardSum[ic] = N_PLAYERS;
    }
    cardSum[INTCARD_JOKER] = N_PLAYERS;
    double playerSum[N_PLAYERS] = {0};
    for(int p = 0; p < N_PLAYERS; ++p){
        playerSum[p] = N_CARDS;
    }
    normalize2<INTCARD_MAX + 1, N_PLAYERS>(table, 16, playerSum, cardSum);
    
    // 1枚をNとして整数で表示
    uint64_t intTable[INTCARD_MAX + 1][N_PLAYERS] = {0};
    for(int p = 0; p < N_PLAYERS; ++p){
        for(IntCard ic = INTCARD_MIN; ic <= INTCARD_MAX; ++ic){
            intTable[ic][p] = table[ic][p] * N;
        }
    }
    
    // テーブル表示
    cerr << "{" << endl;
    for(IntCard ic = 0; ic <= INTCARD_MAX; ++ic){
        cerr << toString(intTable[ic], N_PLAYERS) << "," << endl;
    }cerr << "};" << endl;*/
    return 0;
}

int main(int argc, char* argv[]){
    
    {
        std::ifstream ifs("blauweregen_config.txt");
        if(ifs){ ifs >> DIRECTORY_PARAMS_IN; }
        if(ifs){ ifs >> DIRECTORY_PARAMS_OUT; }
        if(ifs){ ifs >> DIRECTORY_LOGS; }
    }
    std::vector<std::string> logFileNames;
    
    threadTools.dice.srand((unsigned int)time(NULL));
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-i")){ // input directory
            DIRECTORY_PARAMS_IN = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-l")){ // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog>>, 256> mLogs(logFileNames);
    makeCardPriorTable(mLogs, 1ULL << 16);
    
    return 0;
}
