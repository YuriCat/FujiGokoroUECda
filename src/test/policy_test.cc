/*
 policy_test.cc
 Katsuki Ohto
 */

// 方策関数のテスト

#include "../include.h"
#include "../core/minLog.hpp"
#include "../core/changeGenerator.hpp"
#include "../core/moveGenerator.hpp"
#include "../core/field.hpp"
#include "../fuji/changePolicy.hpp"
#include "../fuji/playPolicy.hpp"

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

using namespace UECda;

XorShift64 dice((unsigned int)time(NULL));

ChangePolicy<policy_value_t> changePolicy;
PlayPolicy<policy_value_t> playPolicy;

using namespace UECda::PlayPolicySpace;
using namespace UECda::ChangePolicySpace;

int outputParams(){
    // 方策関数中で気になるパラメータを出力
    cerr << playPolicy.param(FEA_IDX(POL_GR_CARDS)
                             + ORDER_NORMAL * (16 * 2) * (16) * N_PATTERNS_SUITS_SUITS
                             + RANK_6 * (2) * (16) * N_PATTERNS_SUITS_SUITS
                             + 1 * (16) * N_PATTERNS_SUITS_SUITS
                             + RANK_A * N_PATTERNS_SUITS_SUITS
                             + getSuitsSuitsIndex(SUITS_S, SUITS_S)
                             ) << endl;
    
    cerr << playPolicy.param(FEA_IDX(POL_SEQ_CARDS)
                             + ORDER_NORMAL * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_4 * (3) * (16) * N_PATTERNS_SUIT_SUITS
                             + min(int(3) - 3, 2) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_7 * N_PATTERNS_SUIT_SUITS
                             + getSuitSuitsIndex(SUITS_H, SUITS_CH)
                             ) << endl;
    
    cerr << playPolicy.param(FEA_IDX(POL_SEQ_CARDS)
                             + ORDER_NORMAL * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_4 * (3) * (16) * N_PATTERNS_SUIT_SUITS
                             + min(int(4) - 3, 2) * (16) * N_PATTERNS_SUIT_SUITS
                             + RANK_7 * N_PATTERNS_SUIT_SUITS
                             + getSuitSuitsIndex(SUITS_H, SUITS_C)
                             ) << endl;
    
    return 0;
}

template<class logs_t>
int testChangePolicyWithRecord(const logs_t& mLog){
    // 棋譜中の交換との一致率計算
    // 棋譜ファイルは1つのみ受ける(相手が変わる場合には最初からなのでまたこの関数を呼ぶ)

    int sameCount[N_PLAYERS][2] = {0};
    int trials[N_PLAYERS][2] = {0};
    uint64_t time[N_PLAYERS][2] = {0};
    
    cerr << "change policy : " << endl;
    
    Field field;
    iterateGameLogBeforePlay
    (field, mLog,
     [](const auto& field)->void{}, // first callback
     [](const auto& field)->void{}, // dealt callback
     [&](const auto& field, int from, int to, Cards ch)->int{ // change callback
         Cards change[N_MAX_CHANGES + 1];
         const int cl = field.getPlayerClass(from);
         if(cl < MIDDLE){
             const Cards myCards = field.getCards(from);
             const int changeQty = N_CHANGE_CARDS(cl);
             const int NChanges = genChange(change, myCards, changeQty);
             
             Clock clock;
             clock.start();
             int index = changeWithBestPolicy(change, NChanges, myCards, changeQty, field, changePolicy, &dice);
             time[from][cl] += clock.stop();
             
             Cards p = change[index];
             
             if(ch == p){
                 sameCount[from][cl] += 1;
             }
             trials[from][cl] += 1;
         }
         return 0;
     },
     [](const auto& field)->void{}); // last callback

    for(int p = 0; p < N_PLAYERS; ++p){
        cerr << mLog.player(p);
        double sum = 0;
        for(int i = 0; i < 2; ++i){
            double prob = sameCount[p][i] / (double)trials[p][i];
            cerr << " " << prob << " (" << sameCount[p][i] << " / " << trials[p][i] << ")";
            cerr << " in " << time[p][i] / (double)trials[p][i] << " clock";
            sum += prob;
        }
        sum /= 2;
        cerr << " (" << sum << ")" << endl;
    }
    
    return 0;
}

template<class logs_t>
int testPlayPolicyWithRecord(const logs_t& mLog){
    // 棋譜中の役提出との一致率計算
    int sameCount[N_PLAYERS] = {0};
    int trials[N_PLAYERS] = {0};
    uint64_t time[N_PLAYERS] = {0};
    
    cerr << "play policy : " << endl;
    
    Field field;
    iterateGameLogAfterChange
    (field, mLog,
     [](const auto& field)->void{}, // first callback
     [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
         MoveInfo play[N_MAX_MOVES];
         
         const int turnPlayer = field.getTurnPlayer();
         const int moves = genMove(play, field.getCards(turnPlayer), field.getBoard());
         
         Clock clock;
         clock.start();
         int index = playWithBestPolicy(play, moves, field, playPolicy, &dice);
         time[turnPlayer] += clock.stop();
         
         Move p = play[index].mv();
         
         if(pl == p){
             sameCount[turnPlayer] += 1;
         }
         trials[turnPlayer] += 1;
         return 0;
     },
     [](const auto& field)->void{}); // last callback
    
    for(int p = 0; p < N_PLAYERS; ++p){
        cerr << mLog.player(p);
        double prob = sameCount[p] / (double)trials[p];
        cerr << " " << prob << " (" << sameCount[p] << " / " << trials[p] << ")";
        cerr << " in " << time[p] / (double)trials[p] << " clock" << endl;
    }
    return 0;
}

template<class logs_t>
int testSelector(const logs_t& mLog){
    // 方策の最終段階の実験
    double sameProb[4][7][5] = {0}; // 確率ベースでの一致率
    double entropy[4][7][5] = {0}; // 方策エントロピー
    int trials = 0;
    
    cerr << "play policy with selector : " << endl;
    
    Field field;
    iterateGameLogAfterChange
    (field, mLog,
     [](const auto& field)->void{}, // first callback
     [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
         MoveInfo play[N_MAX_MOVES];
         double score[N_MAX_MOVES];
         
         const int turnPlayer = field.getTurnPlayer();
         const int moves = genMove(play, field.getCards(turnPlayer), field.getBoard());
         
         calcPlayPolicyScoreSlow<0>(score, play, moves, field, playPolicy);
         
         int recordIndex = searchMove(play, moves, MoveInfo(pl));
         
         // ここから条件を少しずつ変更
         for(int i = 0; i < 7; ++i){
             {
                 // softmax
                 double tscore[N_MAX_MOVES];
                 memcpy(tscore, score, sizeof(double) * (moves + 1));
                 
                 double temp = 0.7 + 0.1 * i;
                 
                 SoftmaxSelector selector(tscore, moves, temp);
                 selector.to_prob();
                 if(recordIndex >= 0){
                     sameProb[0][i][0] += selector.prob(recordIndex);
                 }
                 entropy[0][i][0] += selector.entropy();
             }
             for(int j = 0; j < 5; ++j){
                 {
                     // truncated
                     double tscore[N_MAX_MOVES];
                     memcpy(tscore, score, sizeof(double) * (moves + 1));
                     
                     double temp = 0.7 + 0.1 * i;
                     double threshold = (4 - j) * 0.01;
                     
                     ThresholdSoftmaxSelector selector(tscore, moves, temp, threshold);
                     selector.cut();
                     selector.to_prob();
                     if(recordIndex >= 0){
                         sameProb[1][i][j] += selector.prob(recordIndex);
                     }
                     entropy[1][i][j] += selector.entropy();
                 }
                 {
                     // biased
                     double tscore[N_MAX_MOVES];
                     memcpy(tscore, score, sizeof(double) * (moves + 1));
                     
                     double temp = 0.8 + 0.1 * i;
                     double coef = 0.4 - 0.1 * j;
                     double rate = SIMULATION_AMPLIFY_EXPONENT;
                     
                     BiasedSoftmaxSelector selector(tscore, moves, temp, coef, rate);
                     selector.amplify();
                     selector.to_prob();
                     
                     if(recordIndex >= 0){
                         sameProb[2][i][j] += selector.prob(recordIndex);
                     }
                     entropy[2][i][j] += selector.entropy();
                 }
                 {
                     // exp-biased
                     double tscore[N_MAX_MOVES];
                     memcpy(tscore, score, sizeof(double) * (moves + 1));
                     
                     double temp = 0.8 + 0.1 * i;
                     double coef = 0.4 - 0.1 * j;
                     double etemp = 1 / log(2);
                     //double etemp = 0.1 * pow(4, j);
                     
                     ExpBiasedSoftmaxSelector selector(tscore, moves, temp, coef, etemp);
                     selector.amplify();
                     selector.to_prob();
                     
                     if(recordIndex >= 0){
                         sameProb[3][i][j] += selector.prob(recordIndex);
                     }
                     entropy[3][i][j] += selector.entropy();
                 }
             }
         }
         trials += 1;
         return 0;
     },
     [](const auto& field)->void{}); // last callback
    
    cerr << "Softmax Selector" << endl;
    for(int i = 0; i < 7; ++i){
        cerr << "(" << sameProb[0][i][0] / trials << ", " << entropy[0][i][0] / trials << ") " << endl;
    }
    cerr << "Truncated Softmax Selector" << endl;
    for(int i = 0; i < 7; ++i){
        for(int j = 0; j < 5; ++j){
            cerr << "(" << sameProb[1][i][j] / trials << ", " << entropy[1][i][j] / trials << ") ";
        }cerr << endl;
    }
    cerr << "Polynomially Biased Softmax Selector" << endl;
    for(int i = 0; i < 7; ++i){
        for(int j = 0; j < 5; ++j){
            cerr << "(" << sameProb[2][i][j] / trials << ", " << entropy[2][i][j] / trials << ") ";
        }cerr << endl;
    }
    cerr << "Exponentially Biased Softmax Selector" << endl;
    for(int i = 0; i < 7; ++i){
        for(int j = 0; j < 5; ++j){
            cerr << "(" << sameProb[3][i][j] / trials << ", " << entropy[3][i][j] / trials << ") ";
        }cerr << endl;
    }
    return 0;
}

/*int testPlayPolicyDiff(){
    // policy の計算において差分計算部分が計算出来ているか確認
    for(int m = 0; m < mLogs.matches(); ++m){
        const auto& mLog = mLogs.match(m);
        for(int g = 0; g < mLog.games(); ++g){
            const auto& gLog = mLog.game(g);
            iterateGameLogAfterChange<PlayouterField>
            (gLog,
             [](const auto& field)->void{}, // first callback
             [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
                     Move move[N_MAX_MOVES];
                     Cards myCards = field.getCards(from);
                     
                     int NChanges = genChange(change, myCards, changeQty[cl]);
                     int index = changeWithBestPolicy(change, NChanges, myCards, changeQty[cl], field, policy, &dice);
                     
                     Cards p = change[index];
                     
                     if(ch == p){
                         result[cl][0] += 1;
                     }else{
                         result[cl][1] += 1;
                         
                         cerr << OutCards(myCards) << " -> ";
                         cerr << "l : " << OutCards(ch) << " p : " << OutCards(p) << endl;
                         getchar();
                     }
                 }
                 return 0;
             },
             [](const auto& field)->void{}); // last callback
        }
    }
    
}*/

int main(int argc, char* argv[]){
    
    {
        std::ifstream ifs("blauweregen_config.txt");
        if(ifs){ ifs >> DIRECTORY_PARAMS_IN; }
        if(ifs){ ifs >> DIRECTORY_PARAMS_OUT; }
        if(ifs){ ifs >> DIRECTORY_LOGS; }
    }
    std::vector<std::string> logFileNames;
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-i")){ // input directory
            DIRECTORY_PARAMS_IN = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-l")){ // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }else if(!strcmp(argv[c], "-ld")){ // log directory path
            std::vector<std::string> tmpLogFileNames = getFilePathVectorRecursively(std::string(argv[c + 1]), ".dat");
            logFileNames.insert(logFileNames.end(), tmpLogFileNames.begin(), tmpLogFileNames.end());
        }
    }
    
    changePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    playPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");
    
    outputParams();
    
    for(const std::string& log : logFileNames){
        MinMatchLog<MinGameLog<MinPlayLog>> mLog(log);
        
        testChangePolicyWithRecord(mLog);
        testPlayPolicyWithRecord(mLog);
        testSelector(mLog);
    }
    
    return 0;
}
