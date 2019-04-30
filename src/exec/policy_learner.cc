// 方策関数の学習

#include <thread>
#include "../UECda.h"
#include "../core/record.hpp"
#include "../engine/engineSettings.h"
#include "../engine/linearPolicy.hpp"
#include "../extra/policyGradient.hpp"

struct ThreadTools {
    Move buf[8192];
    XorShift64 dice;
};

using namespace std;

string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");


enum {
    MODE_FLAG_TEST = 1,
    MODE_FLAG_CHANGE = 2,
    MODE_FLAG_PLAY = 4,
    MODE_FLAG_SHUFFLE = 8,
};

namespace LearningSettings {
    // 学習用パラメータの設定
    constexpr int N_MAX_THREADS = 64;
    int threads = 1;
    constexpr double temperature = 1;
    double learningRate = 0.000005;
    double attenuationRate = 0.00000000005; // 局面数あたり
    double L1Rate = 0;
    double L2Rate = 0.000000001;
    int batchSize = 1;
    int iterations = 200;
    double testRate = 0.77;
}

class LearningSpace{
private:
    ChangePolicyLearner<double> changeLearner_;
    PlayPolicyLearner<double> playLearner_;
    
public:
    // 相手プレーヤー別に呼び出しを分ける想定もあるので引数を取れるようにはしておく
    auto& changeLearner(int p = 0)noexcept{ return changeLearner_; }
    auto& playLearner(int p = 0)noexcept{ return playLearner_; }
    
    LearningSpace(ChangePolicy<double> *const pcp,
                  PlayPolicy<double> *const ppp) {
        changeLearner_.setClassifier(pcp);
        playLearner_.setClassifier(ppp);
    }
};

// 重いのでグローバルに置く
ChangePolicy<double> changePolicy;
PlayPolicy<double> playPolicy;
vector<ThreadTools> threadTools;
vector<LearningSpace> ls;

// 解析, 学習用スレッド
void learnThread(int threadIndex, int st, int ed, bitset<32> flag,
                 const Record *const precord, bool change) {
    auto& thls = ls.at(threadIndex);
    ThreadTools *const ptools = &threadTools.at(threadIndex);
    if (change) thls.changeLearner().initObjValue();
    else thls.playLearner().initObjValue();
    for (int i = st; i < ed; i++) {
        PolicyGradient::learnParamsGame(precord->rgame(i), flag, &thls, ptools, change); // 学習
    }
}

void analyzeThread(int threadIndex, int st, int ed, bitset<32> flag,
                   const Record *const precord, bool change) {
    auto& thls = ls.at(threadIndex);
    ThreadTools *const ptools = &threadTools.at(threadIndex);
    if (change) thls.changeLearner().initFeatureValue();
    else thls.playLearner().initFeatureValue();
    for (int i = st; i < ed; i++) {
        PolicyGradient::learnParamsGame(precord->rgame(i), flag, &thls, ptools, change); // 特徴解析
    }
    if (change) thls.changeLearner().closeFeatureValue();
    else thls.playLearner().closeFeatureValue();
}

vector<int64_t> divide(int64_t st, int64_t ed, int n) {
    vector<int64_t> v = {st};
    for (int i = 0; i < n; i++) v.push_back(v.back() + (ed - v.back()) / (n - i));
    return v;
}
int learn(vector<string> logFileNames, string outDirName, int mode) {
    
    XorShift64 dice((uint32_t)time(NULL));
    mt19937 mt((uint32_t)time(NULL));
    
    if (outDirName == "") outDirName = DIRECTORY_PARAMS_OUT;
    
    // 学習クラスとスレッドツールをスレッド数分確保
    const int threads = LearningSettings::threads;
    ls.clear();
    for (int th = 0; th < threads; th++) {
        ls.emplace_back(LearningSpace(&changePolicy, &playPolicy));
        threadTools.emplace_back(ThreadTools());
    }
    
    // 0番スレッドの学習クラスをマスターとする
    auto& masterChangeLearner = ls.at(0).changeLearner();
    auto& masterPlayLearner = ls.at(0).playLearner();
    
    // ログを読み込み
    Record record(logFileNames);
    
    // preparing
    for (int th = 0; th < threads; th++) {
        ls.at(th).changeLearner().setLearnParam(LearningSettings::temperature, 0, 0, 0, 0);
        ls.at(th).playLearner().setLearnParam(LearningSettings::temperature, 0, 0, 0, 0);
    }
    const int64_t games = record.games();
    
    // トレーニングとテストの試合数を決める
    const double learnGameRate = games / (games + pow((double)games, LearningSettings::testRate));
    const int64_t learnGames = (mode & MODE_FLAG_TEST) ? min((int64_t)(games * learnGameRate), games) : games;
    const int64_t testGames = games - learnGames;
    if (mode & MODE_FLAG_SHUFFLE) record.shuffle(mt);
    
    cerr << learnGames << " games for learning, " << testGames << " games for test." << endl;
    
    cerr << "change policy : " << masterChangeLearner.toOverviewString() << endl;
    cerr << "play   policy : " << masterPlayLearner.toOverviewString() << endl;
    
    // 特徴要素の解析(学習時のステップ幅決め)
    cerr << "started analyzing feature." << endl;
    
    // 解析スレッドごとの試合割り振り
    vector<int64_t> eachLearnGames = divide(0, learnGames, threads);
    vector<int64_t> eachTestGames = divide(learnGames, games, threads);
    cerr << "learn games dist = " << eachLearnGames << endl;
    cerr << "test  games dist = " << eachTestGames << endl;
    
    bitset<32> flag(0);
    flag.set(1);
    
    vector<thread> threadPool;
    
    if (mode & MODE_FLAG_CHANGE) {
        // 交換学習データ
        threadPool.clear();
        for (int th = 0; th < threads; th++) {
            threadPool.emplace_back(thread(analyzeThread, th,
                                                eachLearnGames[th], eachLearnGames[th + 1],
                                                flag, &record, true));
        }
        for (auto& t : threadPool) t.join();
        for (int th = 1; th < threads; th++) masterChangeLearner.mergeFeatureValue(ls.at(th).changeLearner());
        masterChangeLearner.foutFeatureSurvey(outDirName + "change_policy_feature_survey.dat"); // 学習時に見るので出力
        for (int ph = 0; ph < 2; ph++) {
            cerr << "change - training record : " << masterChangeLearner.toRecordString(ph) << endl;
        }
        
        // 交換テストデータ
        threadPool.clear();
        for (int th = 0; th < threads; th++) {
            threadPool.emplace_back(thread(analyzeThread, th,
                                                eachTestGames[th], eachTestGames[th + 1],
                                                flag, &record, true));
        }
        for (auto& t : threadPool) t.join();
        for (int th = 1; th < threads; th++) masterChangeLearner.mergeFeatureValue(ls.at(th).changeLearner());
        for (int ph = 0; ph < 2; ph++) {
            cerr << "change - test     record : " << masterChangeLearner.toRecordString(ph) << endl;
        }
    }
    
    if (mode & MODE_FLAG_PLAY) {
        // 着手学習データ
        threadPool.clear();
        for (int th = 0; th < threads; th++) {
            threadPool.emplace_back(thread(analyzeThread, th,
                                                eachLearnGames[th], eachLearnGames[th + 1],
                                                flag, &record, false));
        }
        for (auto& t : threadPool) t.join();
        for (int th = 1; th < threads; th++) masterPlayLearner.mergeFeatureValue(ls.at(th).playLearner());
        masterPlayLearner.foutFeatureSurvey(outDirName + "play_policy_feature_survey.dat");
        cerr << "play   - training record : " << masterPlayLearner.toRecordString() << endl;
        
        // 着手テストデータ
        threadPool.clear();
        for (int th = 0; th < threads; th++) {
            threadPool.emplace_back(thread(analyzeThread, th,
                                                eachTestGames[th], eachTestGames[th + 1],
                                                flag, &record, false));
        }
        for (auto& t : threadPool) t.join();
        for (int th = 1; th < threads; th++) masterPlayLearner.mergeFeatureValue(ls.at(th).playLearner());
        cerr << "play   - test     record : " << masterPlayLearner.toRecordString() << endl;
    }
    
    cerr << "finished analyzing feature." << endl;
    
    // learning
    cerr << "started learning." << endl;
    
    // 学習の設定を表示
    cerr << "learning rate = " << LearningSettings::learningRate << endl;
    cerr << "attenuation rate = " << LearningSettings::attenuationRate << endl;
    cerr << "L1 reguralization rate = " << LearningSettings::L1Rate << endl;
    cerr << "L2 reguralization rate = " << LearningSettings::L2Rate << endl;
    cerr << "batch size = " << LearningSettings::batchSize << endl;
    
    if (mode & MODE_FLAG_CHANGE) {
        for (int th = 0; th < threads; th++) {
            ls.at(th).changeLearner().finFeatureSurvey(outDirName + "change_policy_feature_survey.dat");
        }
    }
    if (mode & MODE_FLAG_PLAY) {
        for (int th = 0; th < threads; th++) {
            ls.at(th).playLearner().finFeatureSurvey(outDirName + "play_policy_feature_survey.dat");
        }
    }
    
    int64_t changeTrials = 0;
    int64_t playTrials = 0;
    uint64_t startTime = (uint64_t)time(NULL);
    for (int j = 0; j < LearningSettings::iterations; ++j) {
        
        cerr << ((uint64_t)time(NULL) - startTime) << "sec. iteration " << j << " change trials " << changeTrials << " play trials " << playTrials << endl;
        
        for (int th = 0; th < threads; th++) {
            double catr = exp(-changeTrials * LearningSettings::attenuationRate);
            ls.at(th).changeLearner().setLearnParam(LearningSettings::temperature,
                                                    LearningSettings::learningRate * catr,
                                                    LearningSettings::L1Rate * catr,
                                                    LearningSettings::L2Rate * catr,
                                                    LearningSettings::batchSize);
            double patr = exp(-playTrials * LearningSettings::attenuationRate);
            ls.at(th).playLearner().setLearnParam(LearningSettings::temperature,
                                                  LearningSettings::learningRate * patr,
                                                  LearningSettings::L1Rate * patr,
                                                  LearningSettings::L2Rate * patr,
                                                  LearningSettings::batchSize);
        }
        
        if (mode & MODE_FLAG_SHUFFLE) record.shuffle(0, learnGames, mt);
        
        bitset<32> flag(0);
        flag.set(0);
        flag.set(2);
        
        // 学習フェーズ
        if (mode & MODE_FLAG_CHANGE) {
            threadPool.clear();
            for (int th = 0; th < threads; th++) {
                threadPool.emplace_back(thread(learnThread, th,
                                                    eachLearnGames[th], eachLearnGames[th + 1],
                                                    flag, &record, true));
            }
            for (auto& t : threadPool) t.join();
            for (int th = 1; th < threads; th++)masterChangeLearner.mergeObjValue(ls.at(th).changeLearner());
            changeTrials += masterChangeLearner.trials(0);
            changeTrials += masterChangeLearner.trials(1);
            changePolicy.fout(outDirName + "change_policy_param.dat");
            foutComment(changePolicy, outDirName + "change_policy_comment.txt");
            for (int ph = 0; ph < 2; ph++) {
                cerr << "Change Training : " << masterChangeLearner.toObjValueString(ph) << endl;
            }
        }
        if ((mode & MODE_FLAG_PLAY) && !((mode & MODE_FLAG_CHANGE) && (j % 5 != 0))) {
            // 交換も学習する場合は複数イテレーションに1回のみ
            threadPool.clear();
            for (int th = 0; th < threads; th++) {
                threadPool.emplace_back(thread(learnThread, th,
                                                    eachLearnGames[th], eachLearnGames[th + 1],
                                                    flag, &record, false));
            }
            for (auto& t : threadPool) t.join();
            for (int th = 1; th < threads; th++)masterPlayLearner.mergeObjValue(ls.at(th).playLearner());
            playTrials += masterPlayLearner.trials();
            playPolicy.fout(outDirName + "play_policy_param.dat");
            foutComment(playPolicy, outDirName + "play_policy_comment.txt");
            cerr << "Play   Training : " << masterPlayLearner.toObjValueString() << endl;
        }
        
        if (testGames > 0) {
            // テストフェーズ
            flag.reset(0);
            
            if (mode & MODE_FLAG_CHANGE) {
                threadPool.clear();
                for (int th = 0; th < threads; th++) {
                    threadPool.emplace_back(thread(learnThread, th,
                                                        eachTestGames[th], eachTestGames[th + 1],
                                                        flag, &record, true));
                }
                for (auto& t : threadPool) t.join();
                for (int th = 1; th < threads; th++)masterChangeLearner.mergeObjValue(ls.at(th).changeLearner());
                for (int ph = 0; ph < 2; ph++) {
                    cerr << "Change Test     : " << masterChangeLearner.toObjValueString(ph) << endl;
                }
            }
            if ((mode & MODE_FLAG_PLAY) && !((mode & MODE_FLAG_CHANGE) && (j % 5 != 0))) {
                // 交換も学習する場合は複数イテレーションに1回のみ
                threadPool.clear();
                for (int th = 0; th < threads; th++) {
                    threadPool.emplace_back(thread(learnThread, th,
                                                        eachTestGames[th], eachTestGames[th + 1],
                                                        flag, &record, false));
                }
                for (auto& t : threadPool) t.join();
                for (int th = 1; th < threads; th++) masterPlayLearner.mergeObjValue(ls.at(th).playLearner());
                cerr << "Play   Test     : " << masterPlayLearner.toObjValueString() << endl;
            }
        }
    }
    cerr << "finished learning." << endl;
    
    return 0;
}

int main(int argc, char* argv[]) { // For UECda
    
    // 基本的な初期化
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    // ファイルパスの取得
    {
        ifstream ifs("blauweregen_config.txt");
        if (ifs) { ifs >> DIRECTORY_PARAMS_IN; }
        if (ifs) { ifs >> DIRECTORY_PARAMS_OUT; }
        if (ifs) { ifs >> DIRECTORY_LOGS; }
    }
    
    vector<string> logFileNames;
    string outDirName = "";
    
    int mode = MODE_FLAG_CHANGE | MODE_FLAG_PLAY | MODE_FLAG_SHUFFLE;
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-t")) {
            mode |= MODE_FLAG_TEST;
        } else if (!strcmp(argv[c], "-c")) {
            mode &= (~MODE_FLAG_PLAY);
        } else if (!strcmp(argv[c], "-p")) {
            mode &= (~MODE_FLAG_CHANGE);
        } else if (!strcmp(argv[c], "-i")) {
            LearningSettings::iterations = atoi(argv[c + 1]);
        } else if (!strcmp(argv[c], "-l")) {
            logFileNames.push_back(string(argv[c + 1]));
        } else if (!strcmp(argv[c], "-ld")) {
            vector<string> tmpLogFileNames = getFilePathVectorRecursively(string(argv[c + 1]), ".dat");
            logFileNames.insert(logFileNames.end(), tmpLogFileNames.begin(), tmpLogFileNames.end());
        } else if (!strcmp(argv[c], "-o")) {
            outDirName = string(argv[c + 1]);
        } else if (!strcmp(argv[c], "-f")) {
            mode &= (~MODE_FLAG_SHUFFLE);
        } else if (!strcmp(argv[c], "-ar")) {
            LearningSettings::attenuationRate = atof(argv[c + 1]);
        } else if (!strcmp(argv[c], "-lr")) {
            LearningSettings::learningRate = atof(argv[c + 1]);
        } else if (!strcmp(argv[c], "-bs")) {
            LearningSettings::batchSize = atoi(argv[c + 1]);
        } else if (!strcmp(argv[c], "-th")) {
            LearningSettings::threads = atoi(argv[c + 1]);
        }
    }
    
    int ret = learn(logFileNames, outDirName, mode);
    
    return ret;
}
