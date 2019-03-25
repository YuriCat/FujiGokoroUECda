
// オンラインでの相手方策のモデリングのテスト

#include "../include.h"
#include "../engine/engineSettings.h"
#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/linearPolicy.hpp"
#include "../engine/playerModel.hpp"
#include "../engine/playerBias.hpp"

struct ThreadTools{
    MoveInfo buf[8192];
    XorShift64 dice;
};

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

using namespace UECda;
using namespace UECda::Fuji;

XorShift64 dice((unsigned int)time(NULL));

ThreadTools threadTools;
ChangePolicy<policy_value_t> baseChangePolicy;
PlayPolicy<policy_value_t> basePlayPolicy;

template<class logs_t>
int testPlayPolicyModeling(const logs_t& mLog) {
    // 相手モデリングによる着手の変化について実験
    // 棋譜ファイルは1つのみ受ける(相手が変わる場合には最初からなのでまたこの関数を呼ぶ)
    
    constexpr int numCheckGames = 100; // この試合数ごとにモデリングの結果を調べる
    
    PlayerModelSpace playerModelSpace;
    playerModelSpace.init(0);
    Field field;
    
    for (int p = 0; p < N_PLAYERS; ++p) {
        cerr << mLog.player(p) << " ";
    }cerr << endl;
    
    for (int g = 0; g < mLog.games(); ++g) {
        
        if (g % numCheckGames == 0) {
            // 元の方策からの変化を解析
            
            // モデリングの結果を全試合に対して解析
            // 棋譜中のプレーヤーの方策も試合進行とともに変化している可能性はあるが
            // 現状は無視している
            
            double samePlayProb[N_PLAYERS] = {0};
            double wMAE[N_PLAYERS] = {0};
            double KLDivergence[N_PLAYERS] = {0};
            double plays[N_PLAYERS]= {0};
            
            for (int gg = g + 1; gg < mLog.games(); ++gg) {
                const auto& ggLog = mLog.game(gg);
                
                iterateGameLogAfterChange
                (field, ggLog,
                 [](const auto& field)->void{}, // first callback
                 [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
                     
                     MoveInfo play[N_MAX_MOVES];
                     double score[N_MAX_MOVES + 1], score2[N_MAX_MOVES + 1];
                     
                     const int p = field.getTurnPlayer();
                     const int moves = genMove(play, field.getHand(p), field.getBoard());
                     
                     if (moves <= 1) { return 0; }
                     
                     // ベース方策計算
                     calcPlayPolicyScoreSlow<0>(score, play, moves, field, basePlayPolicy);
                     
                     memcpy(score2, score, sizeof(double) * (moves + 1));
                     
                     SoftmaxSelector selector(score, moves, SIMULATION_TEMPERATURE_PLAY);
                     SoftmaxSelector biasedSelector(score2, moves, SIMULATION_TEMPERATURE_PLAY);
                     
                     // バイアス付加
                     addPlayerPlayBias(score2, play, moves, field, playerModelSpace.model(p), Settings::playerBiasCoef);
                     
                     // 確率化
                     selector.to_prob();
                     biasedSelector.to_prob();
                     
                     selector.to_real_prob();
                     biasedSelector.to_real_prob();
                     
                     // 一致率解析
                     int recordIndex = searchMove(play, moves, MoveInfo(pl));
                     if (recordIndex >= 0) {
                         samePlayProb[p] += score2[recordIndex];
                         wMAE[p] += weightedMeanAbsoluteError(score, score2, moves);
                         KLDivergence[p] += kullbackLeiblerDivergence(score, score2, moves);
                         plays[p] += 1;
                     }
                     
                     return 0;
                 },
                 [](const auto& field)->void{} // last callback
                 );
            }
            
            // 解析結果表示
            cerr << " at " << g << " games." << endl;
            /*for (int p = 0; p < N_PLAYERS; ++p) {
             cerr << samePlayProb[p] << " " << plays[p] << endl;
             }*/
            for (int p = 0; p < N_PLAYERS; ++p) {
                cerr << (samePlayProb[p] / plays[p]);
                cerr << " (" << (wMAE[p] / plays[p]) << ") ";
            }cerr << endl;
        }
        
        const auto& gLog = mLog.game(g);
        iterateGameLogAfterChange
        (field, gLog,
         [](const auto& field)->void{}, // first callback
         [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
             return 0;
         },
         [&](const auto& field)->void{ // last callback
             // 試合棋譜からの学習
             // 全員から学習するようにプレーヤー番号 -1 とする
             learnPlayBiasGame(-1, gLog, basePlayPolicy, &playerModelSpace, &threadTools);
         });
    }
    return 0;
}

int main(int argc, char* argv[]) {
    
    {
        std::ifstream ifs("blauweregen_config.txt");
        if (ifs) { ifs >> DIRECTORY_PARAMS_IN; }
        if (ifs) { ifs >> DIRECTORY_PARAMS_OUT; }
        if (ifs) { ifs >> DIRECTORY_LOGS; }
    }
    std::vector<std::string> logFileNames;
    
    threadTools.dice.srand((unsigned int)time(NULL));
    
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-i")) { // input directory
            DIRECTORY_PARAMS_IN = std::string(argv[c + 1]);
        }else if (!strcmp(argv[c], "-l")) { // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    
    baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");
    
    for (const std::string& log : logFileNames) {
        MinMatchLog<MinGameLog<MinPlayLog>> mLog(log);
        testPlayPolicyModeling(mLog);
    }
    
    return 0;
}
