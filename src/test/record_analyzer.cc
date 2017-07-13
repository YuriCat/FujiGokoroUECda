/*
 record_analyzer.cc
 Katsuki Ohto
 */

// 棋譜ファイルからの基本統計量の計算

#include "../include.h"
#include "../generator/moveGenerator.hpp"
#include "../structure/log/minLog.hpp"
#include "../fuji/montecarlo/playout.h"
//#include "../fuji/logic/dominance.hpp"

using namespace UECda;
using namespace Fuji;

MoveGenerator<MoveInfo, Cards> mgCards;
MoveGenerator<MoveInfo, Hand> mgHand;
Clock cl;

int analyzeRecords(const std::vector<std::string>& logs){
    
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog<N_PLAYERS>>>, 16384> mLogs(logs);
    
    int64_t fields = 0;
    std::array<int64_t, N_PLAYERS> fieldsPerNAlive;
    std::array<int64_t, N_PLAYERS> fieldsPerNAsleep;
    
    int64_t nullFields = 0;
    std::array<int64_t, N_PLAYERS> nullFieldsPerNAlive;
    std::array<int64_t, N_PLAYERS> nullFieldsPerNAsleep;
    
    int64_t nullPass = 0;
    std::array<int64_t, N_PLAYERS> nullPassPerNAlive;
    std::array<int64_t, N_PLAYERS> nullPassPerNAsleep;
    
    int64_t normalFields = 0;
    std::array<int64_t, N_PLAYERS> normalFieldsPerNAlive;
    std::array<int64_t, N_PLAYERS> normalFieldsPerNAsleep;
    
    int64_t normalPass = 0;
    std::array<int64_t, N_PLAYERS> normalPassPerNAlive;
    std::array<int64_t, N_PLAYERS> normalPassPerNAsleep;
    
    int64_t normalUnforcedFields = 0;
    std::array<int64_t, N_PLAYERS> normalUnforcedFieldsPerNAlive;
    std::array<int64_t, N_PLAYERS> normalUnforcedFieldsPerNAsleep;
    
    int64_t normalUnforcedPass = 0;
    std::array<int64_t, N_PLAYERS> normalUnforcedPassPerNAlive;
    std::array<int64_t, N_PLAYERS> normalUnforcedPassPerNAsleep;
    
    fieldsPerNAlive.fill(0);
    fieldsPerNAsleep.fill(0);
    nullFieldsPerNAlive.fill(0);
    nullFieldsPerNAsleep.fill(0);
    nullPassPerNAlive.fill(0);
    nullPassPerNAsleep.fill(0);
    normalFieldsPerNAlive.fill(0);
    normalFieldsPerNAsleep.fill(0);
    normalPassPerNAlive.fill(0);
    normalPassPerNAsleep.fill(0);
    normalUnforcedFieldsPerNAlive.fill(0);
    normalUnforcedFieldsPerNAsleep.fill(0);
    normalUnforcedPassPerNAlive.fill(0);
    normalUnforcedPassPerNAsleep.fill(0);
    
    int64_t pass = 0;
    
    std::array<std::array<int64_t, N_PLAYERS>, N_PLAYERS> nullPassResult;
    std::array<std::pair<int, int>, 256> nullPassBuffer;
    int nullPassTemp;
    
    for(int p = 0; p < N_PLAYERS; ++p){
        nullPassResult[p].fill(0);
    }
    
    int64_t turns = 0;
    double turns2 = 0;
    int maxTurns = 0;
    int minTurns = std::numeric_limits<int>::max();
    int64_t games = 0;
    double games2 = 0;
    int maxGames = 0;
    int minGames = std::numeric_limits<int>::max();
    int64_t matches = mLogs.matches();
    
    MoveInfo buffer[8192];
    
    // 棋譜を読んで解析
    for(int m = 0; m < mLogs.matches(); ++m){
        const auto& mLog = mLogs.match(m);
        
        games += mLog.games();
        games2 += mLog.games() * mLog.games();
        maxGames = max(maxGames, mLog.games());
        minGames = min(minGames, mLog.games());
        
        for(int g = 0; g < mLog.games(); ++g){
            const auto& gLog = mLog.game(g);
            
            turns += gLog.plays();
            turns2 += gLog.plays() * gLog.plays();
            maxTurns = max(maxTurns, gLog.plays());
            minTurns = min(minTurns, gLog.plays());
            
            iterateGameLogAfterChange<PlayouterField>
            (gLog,
             [&](const auto& field){ // first callback
                 nullPassTemp = 0;
             },
             [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
                 
                 const int turnPlayer = field.getTurnPlayer();
                 const int NAlive = field.getNAlivePlayers();
                 int NAsleep = field.getNAlivePlayers() - field.getNAwakePlayers();
                 
                 fields += 1;
                 fieldsPerNAlive[NAlive - 1] += 1;
                 fieldsPerNAsleep[NAsleep] += 1;
                 
                 if(field.isNF()){
                     nullFields += 1;
                     nullFieldsPerNAlive[NAlive - 1] += 1;
                     nullFieldsPerNAsleep[NAsleep] += 1;
                     if(move.isPASS()){
                         nullPass += 1;
                         nullPassPerNAlive[NAlive - 1] += 1;
                         nullPassPerNAsleep[NAsleep] += 1;
                         
                         nullPassBuffer[nullPassTemp].first = turnPlayer;
                         nullPassBuffer[nullPassTemp].second = NAlive;
                         nullPassTemp += 1;
                     }
                 }else{
                     normalFields += 1;
                     normalFieldsPerNAlive[NAlive - 1] += 1;
                     normalFieldsPerNAsleep[NAsleep] += 1;
                     if(move.isPASS()){
                         normalPass += 1;
                         normalPassPerNAlive[NAlive - 1] += 1;
                         normalPassPerNAsleep[NAsleep] += 1;
                     }
                     
                     int moves = genFollowExceptPASS(buffer, field.getCards(turnPlayer), field.getBoard());
                     if(moves > 0){
                         normalUnforcedFields += 1;
                         normalUnforcedFieldsPerNAlive[NAlive - 1] += 1;
                         normalUnforcedFieldsPerNAsleep[NAsleep] += 1;
                         if(move.isPASS()){
                             normalUnforcedPass += 1;
                             normalUnforcedPassPerNAlive[NAlive - 1] += 1;
                             normalUnforcedPassPerNAsleep[NAsleep] += 1;
                         }
                     }
                 }
                 
                 if(move.isPASS()){
                     pass += 1;
                 }
                 
                 return 0;
             },
             [&](const auto& field){ // last callback
                 for(int i = 0; i < nullPassTemp; ++i){
                     nullPassResult[nullPassBuffer[i].second - 1][field.getPlayerNewClass(nullPassBuffer[i].first)] += 1;
                 }
             });
        }
    }
    
    cerr << "matches = " << matches << endl;
    cerr << "games = " << games << endl;
    cerr << "min games per match = " << minGames << endl;
    cerr << "max games per match = " << maxGames << endl;
    cerr << "avg games per match = " << games / double(matches) << endl;
    cerr << "std games per match = " << sqrt(games2 / double(matches) - pow(games / double(matches), 2)) << endl;
    cerr << "turns = " << turns << endl;
    cerr << "min turns per game = " << minTurns << endl;
    cerr << "max turns per game = " << maxTurns << endl;
    cerr << "avg turns per game = " << turns / double(games) << endl;
    cerr << "std turns per game = " << sqrt(turns2 / double(games) - pow(turns / double(games), 2)) << endl;
    cerr << "fields = " << fields << endl;
    cerr << "(per num alive) = " << fieldsPerNAlive << endl;
    cerr << "(per num asleep) = " << fieldsPerNAsleep << endl;
    cerr << "null fields = " << nullFields << endl;
    cerr << "(per num alive) = " << nullFieldsPerNAlive << endl;
    cerr << "(per num asleep) = " << nullFieldsPerNAsleep << endl;
    cerr << "null pass = " << nullPass << endl;
    cerr << "(per num alive) = " << nullPassPerNAlive << endl;
    cerr << "(per num asleep) = " << nullPassPerNAsleep << endl;
    cerr << "null pass result = " << endl << toString(nullPassResult, "\n") << endl;
    cerr << "normal fields = " << normalFields << endl;
    cerr << "(per num alive) = " << normalFieldsPerNAlive << endl;
    cerr << "(per num asleep) = " << normalFieldsPerNAsleep << endl;
    cerr << "normal pass = " << normalPass << endl;
    cerr << "(per num alive) = " << normalPassPerNAlive << endl;
    cerr << "(per num asleep) = " << normalPassPerNAsleep << endl;
    cerr << "normal unforced fields = " << normalUnforcedFields << endl;
    cerr << "(per num alive) = " << normalUnforcedFieldsPerNAlive << endl;
    cerr << "(per num asleep) = " << normalUnforcedFieldsPerNAsleep << endl;
    cerr << "normal unforced pass = " << normalUnforcedPass << endl;
    cerr << "(per num alive) = " << normalUnforcedPassPerNAlive << endl;
    cerr << "(per num asleep) = " << normalUnforcedPassPerNAsleep << endl;
    
    
    return 0;
}

int main(int argc, char* argv[]){
    
    std::vector<std::string> logFileNames;
    std::string logIdentifier = ".dat";
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-l")){
            logFileNames.push_back(std::string(argv[c + 1]));
        }else if(!strcmp(argv[c], "-ld")){
            const std::string logDirectory = std::string(argv[c + 1]);
            const std::vector<std::string> added = getFilePathVectorRecursively(std::string(argv[c + 1]), logIdentifier);
            logFileNames.insert(logFileNames.end(), added.begin(), added.end());
        }else if(!strcmp(argv[c], "-li")){
            logIdentifier = std::string(argv[c + 1]);
        }
    }
    
    return analyzeRecords(logFileNames);
}
