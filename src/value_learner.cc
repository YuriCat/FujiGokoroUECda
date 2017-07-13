/*
 pglearner.cc
 Katsuki Ohto
 */

// 方策関数の学習

#include "include.h"
#include "structure/log/minLog.hpp"

#include "fuji/fuji.h"
#include "fuji/policy/changePolicy.hpp"
#include "fuji/policy/playPolicy.hpp"

#include "fuji/learning/policyGradient.hpp"

struct ThreadTools{
    MoveInfo buf[8192];
    XorShift64 dice;
};

ThreadTools threadTools;

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

using namespace UECda;
using namespace UECda::Fuji;

enum{
    MODE_FLAG_TEST = 1,
    MODE_FLAG_CHANGE = 2,
    MODE_FLAG_PLAY = 4,
    MODE_FLAG_SHUFFLE = 8,
};

namespace LearningSettings{
    // 学習用パラメータの設定
    constexpr double temperature = 1;
    double learningRate = 0.00005;
    double attenuatingRate = 0.05;
    double L1Rate = 0;
    double L2Rate = 0.0000001;
    int batchSize = 8192;
    int iterations = 200;
}

class LearningSpace{
private:
    PlayPolicy playPolicy_;
    ChangePolicy changePolicy_;
    
    PlayPolicyLearner playLearner_;
    ChangePolicyLearner changeLearner_;
    
public:
    auto& playPolicy(int p = 0)noexcept{ return playPolicy_; }
    auto& changePolicy(int p = 0)noexcept{ return changePolicy_; }
    auto& playLearner(int p = 0)noexcept{ return playLearner_; }
    auto& changeLearner(int p = 0)noexcept{ return changeLearner_; }
    
    LearningSpace(){
        playPolicy_.setLearner(&playLearner_);
        changePolicy_.setLearner(&changeLearner_);
        
        playLearner_.setPolicy(&playPolicy_);
        changeLearner_.setPolicy(&changePolicy_);
    }
};

LearningSpace ls; // 重いのでグローバルに置く

int learn(std::vector<std::string> logFileNames, std::string outDirName, int mode){
    
    using namespace UECda::Fuji;
    
    double testRate = (mode & MODE_FLAG_TEST) ? 0.77 : 0;
    
    XorShift64 dice((uint32_t)time(NULL));
    std::mt19937 mt((uint32_t)time(NULL));
    
    ThreadTools *const ptools = &threadTools;
    
    if(outDirName == ""){
        outDirName = DIRECTORY_PARAMS_OUT;
    }
    
    auto& playPolicy = ls.playPolicy();
    auto& changePolicy = ls.changePolicy();
    auto& playLearner = ls.playLearner();
    auto& changeLearner = ls.changeLearner();
    
    // ログを読み込み
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog<N_PLAYERS>>>, 2048> mLogs(logFileNames);
 
    // preparing
    changeLearner.setLearnParam(LearningSettings::temperature, 0, 0, 0, 0);
    playLearner.setLearnParam(LearningSettings::temperature, 0, 0, 0, 0);

    mLogs.initRandomList();
    const int games = mLogs.games();
    
    // トレーニングとテストの試合数を決める
    const double learnGameRate = games / (games + pow((double)games, testRate));
    
    const int learnGames = min((int)(games * learnGameRate), games);
    const int testGames = games - learnGames;
    if(mode & MODE_FLAG_SHUFFLE){
        mLogs.shuffleRandomList(0, games, mt);
    }
    
    cerr << learnGames << " games for learning, " << testGames << " games for test." << endl;
    
    cerr << "change policy : " << changeLearner.toOverviewString() << endl;
    cerr << "play   policy : " << playLearner.toOverviewString() << endl;
    
    // 特徴要素の解析(学習時のステップ幅決め)
    cerr << "started analyzing feature." << endl;
    
    BitSet32 flag(0);
    flag.set(1);
    
    if(mode & MODE_FLAG_CHANGE){
        // 交換学習データ
        changeLearner.initFeatureValue();
        iterateGameRandomly(mLogs, 0, learnGames, [flag, ptools](const auto& gLog, const auto& mLog)->void{
            PolicyGradient::learnChangeParamsGame(gLog, flag, &ls, ptools); // 特徴解析
        });
        changeLearner.closeFeatureValue();
        changeLearner.foutFeatureSurvey(outDirName + "change_policy_feature_survey.dat");
        for(int ph = 0; ph < 2; ++ph){
            cerr << "change - training record : " << changeLearner.toRecordString(ph) << endl;
        }
        
        // 交換テストデータ
        changeLearner.initFeatureValue();
        iterateGameRandomly(mLogs, learnGames, games, [flag, ptools](const auto& gLog, const auto& mLog)->void{
            PolicyGradient::learnChangeParamsGame(gLog, flag, &ls, ptools); // 特徴解析
        });
        changeLearner.closeFeatureValue();
        for(int ph = 0; ph < 2; ++ph){
            cerr << "change - test     record : " << changeLearner.toRecordString(ph) << endl;
        }
    }
    
    if(mode & MODE_FLAG_PLAY){
        // 着手学習データ
        playLearner.initFeatureValue();
        iterateGameRandomly(mLogs, 0, learnGames, [flag, ptools](const auto& gLog, const auto& mLog)->void{
            PolicyGradient::learnPlayParamsGame(gLog, flag, &ls, ptools); // 特徴解析
        });
        playLearner.closeFeatureValue();
        playLearner.foutFeatureSurvey(outDirName + "play_policy_feature_survey.dat");
        
        cerr << "play   - training record : " << playLearner.toRecordString() << endl;
        
        // 着手テストデータ
        playLearner.initFeatureValue();
        iterateGameRandomly(mLogs, learnGames, games, [flag, ptools](const auto& gLog, const auto& mLog)->void{
            PolicyGradient::learnPlayParamsGame(gLog, flag, &ls, ptools); // 特徴解析
        });
        playLearner.closeFeatureValue();
        cerr << "play   - test     record : " << playLearner.toRecordString() << endl;
    }
    
    cerr << "finished analyzing feature." << endl;
    
    //learning
    cerr << "started learning." << endl;
    
    if(mode & MODE_FLAG_CHANGE){
        changeLearner.finFeatureSurvey(outDirName + "change_policy_feature_survey.dat");
    }
    if(mode & MODE_FLAG_PLAY){
        playLearner.finFeatureSurvey(outDirName + "play_policy_feature_survey.dat");
    }
    
    for (int j = 0; j < LearningSettings::iterations; ++j){
        
        cerr << "iteration " << j << endl;
        
        changeLearner.setLearnParam(LearningSettings::temperature,
                                    LearningSettings::learningRate * exp(-j * LearningSettings::attenuatingRate),
                                    LearningSettings::L1Rate * exp(-j * LearningSettings::attenuatingRate),
                                    LearningSettings::L2Rate * exp(-j * LearningSettings::attenuatingRate),
                                    LearningSettings::batchSize);
        playLearner.setLearnParam(LearningSettings::temperature,
                                  LearningSettings::learningRate * exp(-j * LearningSettings::attenuatingRate),
                                  LearningSettings::L1Rate * exp(-j * LearningSettings::attenuatingRate),
                                  LearningSettings::L2Rate * exp(-j * LearningSettings::attenuatingRate),
                                  LearningSettings::batchSize);
        
        if(mode & MODE_FLAG_SHUFFLE){
            mLogs.shuffleRandomList(0, learnGames, mt);
        }
        
        BitSet32 flag(0);
        flag.set(0);
        flag.set(2);
        
        // 学習フェーズ
        if(mode & MODE_FLAG_CHANGE){
            changeLearner.initObjValue();
            iterateGameRandomly(mLogs, 0, learnGames, [flag, ptools](const auto& gLog, const auto& mLog)->void{
                PolicyGradient::learnChangeParamsGame(gLog, flag, &ls, ptools); // 学習
            });
            changePolicy.fout(outDirName + "change_policy_param.dat");
            foutComment(changePolicy, outDirName + "change_policy_comment.txt");
            for(int ph = 0; ph < 2; ++ph){
                cerr << "Change Training : " << changeLearner.toObjValueString(ph) << endl;
            }
        }
        if(mode & MODE_FLAG_PLAY){
            playLearner.initObjValue();
            iterateGameRandomly(mLogs, 0, learnGames, [flag, ptools](const auto& gLog, const auto& mLog)->void{
                PolicyGradient::learnPlayParamsGame(gLog, flag, &ls, ptools); // 学習
            });
            playPolicy.fout(outDirName + "play_policy_param.dat");
            foutComment(playPolicy, outDirName + "play_policy_comment.txt");
            cerr << "Play   Training : " << playLearner.toObjValueString() << endl;
        }
        
        if(testGames > 0){
            // テストフェーズ
            flag.reset(0);
            
            if(mode & MODE_FLAG_CHANGE){
                changeLearner.initObjValue();
                iterateGameRandomly(mLogs, learnGames, games, [flag, ptools](const auto& gLog, const auto& mLog)->void{
                    PolicyGradient::learnChangeParamsGame(gLog, flag, &ls, ptools); // テスト
                });
                for(int ph = 0; ph < 2; ++ph){
                    cerr << "Change Test     : " << changeLearner.toObjValueString(ph) << endl;
                }
            }
            if(mode & MODE_FLAG_PLAY){
                playLearner.initObjValue();
                iterateGameRandomly(mLogs, learnGames, games, [flag, ptools](const auto& gLog, const auto& mLog)->void{
                    PolicyGradient::learnPlayParamsGame(gLog, flag, &ls, ptools); // テスト
                });
                cerr << "Play   Test     : " << playLearner.toObjValueString() << endl;
            }
        }
    }
    cerr << "finished learning." << endl;
    
    return 0;
}

int main(int argc, char* argv[]){ // For UECda
    
    // 基本的な初期化
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    // ファイルパスの取得
    {
        std::ifstream ifs("blauweregen_config.txt");
        if(ifs){ ifs >> DIRECTORY_PARAMS_IN; }
        if(ifs){ ifs >> DIRECTORY_PARAMS_OUT; }
        if(ifs){ ifs >> DIRECTORY_LOGS; }
    }
    
    std::vector<std::string> logFileNames;
    std::string outDirName = "";
    
    int mode = MODE_FLAG_CHANGE | MODE_FLAG_PLAY | MODE_FLAG_SHUFFLE;
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-t")){
            mode |= MODE_FLAG_TEST;
        }else if(!strcmp(argv[c], "-c")){
            mode &= (~MODE_FLAG_PLAY);
        }else if(!strcmp(argv[c], "-p")){
            mode &= (~MODE_FLAG_CHANGE);
        }else if(!strcmp(argv[c], "-i")){
            LearningSettings::iterations = atoi(argv[c + 1]);
        }else if(!strcmp(argv[c], "-l")){
            logFileNames.push_back(std::string(argv[c + 1]));
        }else if(!strcmp(argv[c], "-ld")){
            std::vector<std::string> tmpLogFileNames = getFilePathVectorRecursively(std::string(argv[c + 1]), ".dat");
            logFileNames.insert(logFileNames.end(), tmpLogFileNames.begin(), tmpLogFileNames.end());
        }else if(!strcmp(argv[c], "-o")){
            outDirName = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-f")){
            mode &= (~MODE_FLAG_SHUFFLE);
        }else if(!strcmp(argv[c], "-ar")){
            LearningSettings::attenuatingRate = atof(argv[c + 1]);
        }else if(!strcmp(argv[c], "-lr")){
            LearningSettings::learningRate = atof(argv[c + 1]);
        }
    }
    
    int ret = learn(logFileNames, outDirName, mode);
    
    return ret;
}
