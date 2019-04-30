 
// シミュレーションの性能諸々チェック

#include "../include.h"
#include "../engine/engineSettings.h"

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/linearPolicy.hpp"
#include "../engine/playerModel.hpp"
#include "../engine/playerBias.hpp"

struct ThreadTools {
    MoveInfo buffer[8192];
    XorShift64 dice;
};

struct SubjectivePlayouterField : public PlayouterField {
    // シミュレーション用局面情報の主観化
    int myPlayerNum;
    
    int getMyPlayerNum() const noexcept{ return myPlayerNum; }
    Cards getMyCards() const { return getCards(getMyPlayerNum()); }
    const Hand& myHand() const { return getHand(getMyPlayerNum()); }
    Cards getOpsCards() const { return getOpsCards(getMyPlayerNum()); }
    const Hand& getOpsHand() const { return getOpsHand(getMyPlayerNum()); }
    Cards getSentCards() const { return getSentCards(getMyPlayerNum()); }
    Cards getRecvCards() const { return getRecvCards(getRecvCards()); }
    
    SubjectivePlayouterField(const PlayouterField& objField, consy int ap):
    PlayouterField(objField), myPlayerNum(ap) {}
};

using namespace UECda;

Clock cl;
ThreadTools threadTools;
PlayerModelSpace playerModelSpace;

int testSimulations(const logs_t& mLog) {
    // 棋譜を読んでシミュレーションを行う
    
    uint64_t dealTime[4] = {0};
    uint64_t dealCount[4] = {0};
    
    double simpleConcordanceSum[4] = {0};
    double rankConcordanceSum[4] = {0};
    
    iterateGameLogAfterChange<PlayouterField>
    (mLog,
     [&](const auto& field) {}, // first callback
     [&](const auto& field, const auto move, const uint64_t time)->int{ // play callback
         // このプレーヤーの視点で推定
         SubjectivePlayouterField sbjField(field, field.turn());
         
         Dealer<N_PLAYERS> dealer;
         dealer.set(field);
         field.
         
         for (int i = 0; i < 100; ++i) {
             Cards dst[N_PLAYERS];
             
             // 完全ランダム
             clock.start();
             dealer.deal<DealType::RANDOM>(dst, &threadTools.dice);
             deatTime[DeatType::RANDOM] += clock.stop();
             dealCount[DealType::RANDOM] += 1;
             simpleConcordanceSum += simpleConcordance(field.hand, dst, );
             
             // 主観的に確実な情報のみ用いる
             clock.start();
             dealer.deal<DealType::SBJINFO>(dst, &threadTools.dice);
             deatTime[DeatType::SBJINFO] += clock.stop();
             dealCount[DealType::SBJINFO] += 1;
             
             // 逆関数バイアス
             clock.start();
             dealer.deal<DealType::BIAS>(dst, &threadTools.dice);
             deatTime[DeatType::BIAS] += clock.stop();
             dealCount[DealType::BIAS] += 1;
             
             // 採択棄却
             //clock.start();
             //deatTime[DeatType::REJECTION] += clock.stop();
             //dealCount[DealType::REJECTION] += 1;
             
             
         }
         
         return 0;
     },
     [&](const auto& field) {} // last callback
     );
    
    /*cerr << "judge result (cards) = " << endl;
     for (int i = 0; i < 2; ++i) {
     for (int j = 0; j < 2; ++j) {
     cerr << judgeMatrix[0][i][j] << " ";
     }cerr << endl;
     }*/
    cerr << "check result (hand) = " << endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            cerr << checkMatrix[1][i][j] << " ";
        }cerr << endl;
    }
    
    //cerr << "judge time (cards) = " << judgeTime[0] / (double)judgeCount[0] << endl;
    cerr << "check time (hand)    = " << checkTime[1] / (double)checkCount[1] << endl;
    cerr << "check time (pw-slow) = " << checkTime[2] / (double)checkCount[1] << endl;
    
    return 0;
}

template <class logs_t>
int testEstimationWithModeling(const logs_t& mLog) {
    // 相手モデリングによる着手の変化について実験
    // 棋譜ファイルは1つのみ受ける(相手が変わる場合には最初からなのでまたこの関数を呼ぶ)
    
    constexpr int numCheckGames = 1000; // この試合数ごとにモデリングの結果を調べる
    
    playerModelSpace.init();
    
    for (int g = 0; g < mLog.games(); ++g) {
        const auto& gLog = mLog.game(g);
        
        iterateGameLogAfterChange<PlayouterField>
        (gLog,
         [](const auto& field)->void{}, // first callback
         [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
             // この局面からシミュレーションを行い、
             // 結果がどの程度近いか、ばらけるか見る
             for (int i = 0; i < 1000; ++i) {
                 PlayouterField tfield = field;
                 startS
             }
             return 0;
         },
         [](const auto& field)->void{ // last callback
             // 試合棋譜からの学習
             learnPlayBiasGame(gLog, &playerModelSpace, &threadTools);
         });
        if ((g + 1) % 100 == 0) {
            // 元の方策からの変化を解析
            
            // モデリングの結果を全試合に対して解析
            // 棋譜中のプレーヤーの方策も試合進行とともに変化している可能性はあるが
            // 現状は無視している
            
            double samePlayProb[N_PLAYERS] = {0};
            double Plays[N_PLAYERS]= {0};
            
            for (int gg = g + 1; gg < mLog.games(); ++gg) {
                const auto& ggLog = mLog.game(gg);
                
                iterateGameLogAfterChange<PlayouterField>
                (ggLog,
                 [](const auto& field)->void{}, // first callback
                 [&](const auto& field, Move pl, uint32_t tm)->int{ // play callback
                     
                     MoveInfo play[N_MAX_MOVES];
                     double score[N_MAX_MOVES + 1];
                     
                     const int p = field.turn();
                     const int moves = genMove(play, field.hand, field.board;
                     
                     // ベース方策計算
                     calcPlayPolicyScoreSlow(score, play, moves, field, playerModelSpace.playPolicy());
                     SoftmaxSelector selector(score, moves, SIMULATION_TEMPERATURE_PLAY);
                     
                     // バイアス付加
                     addPlayerPlayBias(score, play, moves, field, playerModelSpace.model(p));
                     
                     // 確率化
                     selector.to_prob();
                     
                     // 一致率解析
                     int recordIndex = searchMove(play, moves, MoveInfo(pl));
                     samePlayProb[p] += selector.prob(recordIndex);
                     plays[p] += 1;
                     
                     return 0;
                 },
                 [](const auto& field)->void{} // last callback
                 );
            }
            
            // 解析結果表示
            for (int p = 0; p < N_PLAYERS; ++p) {
                cerr << mLog.player(p);
                cerr << " " << samePlayProb[p] / (double)plays[p];
                cerr << endl;
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> logFileNames;
    threadTools.dice.srand((unsigned int)time(NULL));
    
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-i")) { // input directory
            DIRECTORY_PARAMS_IN = std::string(argv[c + 1]);
        }else if (!strcmp(argv[c], "-l")) { // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    
    for (const std::string& log : logFileNames) {
        MinMatchLog<MinGameLog<MinPlayLog<N_PLAYERS>>> mLog(log);
        testPlayPolicyModeling(mLog);
    }
    
    return 0;
}
