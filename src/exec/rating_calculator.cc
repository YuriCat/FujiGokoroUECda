
// 棋譜からオフラインでレーティングを推定

#include "../include.h"
#include "../core/record.hpp"
#include "../engine/engineSettings.h"
#include "../engine/engineStructure.hpp"
#include "../engine/rating.hpp"

MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog>>, 8192> mLogs;

Fuji::FujiSharedData shared;
Fuji::FujiThreadTools threadTools[N_THREADS];

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

using namespace UECda;
using namespace UECda::Fuji;

struct RatingSurvey {
    int games;
    std::map<std::string, double> playerRate;
    std::map<std::string, double> playerRateMean;
    std::map<std::string, double> playerRateExpMean;
    static constexpr double rateEpsilon = 0.99;
    
    std::vector<double> toMeanVector()const{
        std::vector<double> v;
        for (auto& p : playerRateMean) {
            v.push_back(p.second);
        }
        return v;
    }
    
    std::string toString()const{
        std::ostringstream oss;
        std::vector<std::pair<std::string, double>> v;
        for (auto& p : playerRateMean) {
            v.push_back(p);
        }
        std::sort(v.begin(), v.end(), [](const auto& a, const auto& b)->bool{
            return a.second < b.second;
        });
        double sum = 0;
        for (auto& p : v) {
            oss << p.first << " : " << p.second << std::endl;
            sum += p.second;
        }
        oss << "mean = " << sum / v.size() << endl;
        return oss.str();
    }
    
    RatingSurvey():games(0) {}
};

template<class gameLog_t, class matchLog_t>
int calcNextRate(const gameLog_t& gLog, const matchLog_t& mLog, const int simulations, const double coef, bool absolute, RatingSurvey *const pdst) {
    // シミュレーションを行ってレーティング更新
    std::array<double, N_PLAYERS> orgRate;
    for (int p = 0; p < N_PLAYERS; ++p) {
        orgRate[p] = pdst->playerRate[mLog.player(p)];
    }
    
    std::array<double, N_PLAYERS> diff;
    if (!absolute) {
        diff = UECda::Fuji::calcDiffRateByRelativeWpWithSimulation(orgRate, gLog, simulations, coef, &shared, &threadTools[0]);
    } else {
        diff = UECda::Fuji::calcDiffRateByAbsoluteWpWithSimulation(orgRate, gLog, simulations, coef, &shared, &threadTools[0]);
    }
    
    for (int p = 0; p < N_PLAYERS; ++p) {
        const std::string& name = mLog.player(p);
        pdst->playerRate[name] += diff[p];
        pdst->playerRateMean[name] = (pdst->playerRateMean[name] * pdst->games + pdst->playerRate[name]) / (pdst->games + 1);
        const double e = RatingSurvey::rateEpsilon;
        pdst->playerRateExpMean[name] = pdst->playerRateExpMean[name] * e + pdst->playerRate[name] * (1 - e);
    }
    pdst->games += 1;
    cerr << pdst->toString();
    return 0;
}
template<class result_t, class gameLog_t, class matchLog_t>
int calcNextRateWithRelativeResult(const gameLog_t& gLog, const matchLog_t& mLog, const result_t& simulationResult, const double coef, RatingSurvey *const pdst) {
    // 先に計算されたシミュレーション結果を用いてレーティング更新
    std::array<double, N_PLAYERS> orgRate;
    for (int p = 0; p < N_PLAYERS; ++p) {
        orgRate[p] = pdst->playerRate[mLog.player(p)];
    }

    std::array<double, N_PLAYERS> diff = UECda::Fuji::calcDiffRateByRelativeWp(gLog, UECda::Fuji::calcExpectedRelativeWp(orgRate, simulationResult), coef);
    
    for (int p = 0; p < N_PLAYERS; ++p) {
        const std::string& name = mLog.player(p);
        pdst->playerRate[name] += diff[p];
        pdst->playerRateMean[name] = (pdst->playerRateMean[name] * pdst->games + pdst->playerRate[name]) / (pdst->games + 1);
        const double e = RatingSurvey::rateEpsilon;
        pdst->playerRateExpMean[name] = pdst->playerRateExpMean[name] * e + pdst->playerRate[name] * (1 - e);
    }
    pdst->games += 1;
    cerr << pdst->toString();
    return 0;
}
template<class result_t, class gameLog_t, class matchLog_t>
int calcNextRateWithAbsoluteResult(const gameLog_t& gLog, const matchLog_t& mLog, const result_t& simulationResult, const double coef, RatingSurvey *const pdst) {
    // 先に計算されたシミュレーション結果を用いてレーティング更新
    std::array<double, N_PLAYERS> orgRate;
    for (int p = 0; p < N_PLAYERS; ++p) {
        orgRate[p] = pdst->playerRate[mLog.player(p)];
    }
    
    std::array<double, N_PLAYERS> diff = UECda::Fuji::calcDiffRateByAbsoluteWp(gLog, UECda::Fuji::calcExpectedAbsoluteWp(orgRate, simulationResult), coef);

    for (int p = 0; p < N_PLAYERS; ++p) {
        const std::string& name = mLog.player(p);
        pdst->playerRate[name] += diff[p];
        pdst->playerRateMean[name] = (pdst->playerRateMean[name] * pdst->games + pdst->playerRate[name]) / (pdst->games + 1);
        const double e = RatingSurvey::rateEpsilon;
        pdst->playerRateExpMean[name] = pdst->playerRateExpMean[name] * e + pdst->playerRate[name] * (1 - e);
    }
    pdst->games += 1;
    cerr << pdst->toString();
    return 0;
}

template<class array_t>
int invertRates(const array_t& rates) {
    // TODO: 実際の棋譜から分布を計算
    
    {
        cerr << "Be(0.5, 0.5)" << endl;
        std::vector<double> scores = calcExpectedTotalScore(rates, BetaDistribution(0.5, 0.5));
        double sum = 0;
        for (double s : scores) {
            cerr << s << endl;
            sum += s;
        }
        cerr << "avg = " << sum / scores.size() << endl;
    }
    {
        cerr << "Be(0.65, 0.65)" << endl;
        std::vector<double> scores = calcExpectedTotalScore(rates, BetaDistribution(0.65, 0.65));
        double sum = 0;
        for (double s : scores) {
            cerr << s << endl;
            sum += s;
        }
        cerr << "avg = " << sum / scores.size() << endl;
    }
    {
        cerr << "Be(0.7, 0.7)" << endl;
        std::vector<double> scores = calcExpectedTotalScore(rates, BetaDistribution(0.7, 0.7));
        double sum = 0;
        for (double s : scores) {
            cerr << s << endl;
            sum += s;
        }
        cerr << "avg = " << sum / scores.size() << endl;
    }
    {
        cerr << "Be(1, 1)" << endl;
        std::vector<double> scores = calcExpectedTotalScore(rates, BetaDistribution(1, 1));
        double sum = 0;
        for (double s : scores) {
            cerr << s << endl;
            sum += s;
        }
        cerr << "avg = " << sum / scores.size() << endl;
    }
    {
        cerr << "Be(2, 2)" << endl;
        std::vector<double> scores = calcExpectedTotalScore(rates, BetaDistribution(2, 2));
        double sum = 0;
        for (double s : scores) {
            cerr << s << endl;
            sum += s;
        }
        cerr << "avg = " << sum / scores.size() << endl;
    }
    return 0;
}

template<class logs_t>
int calcRating(logs_t& mLogs, const int games, const int simulations, const double coef, const bool absolute) {
    
    std::mt19937 mt(threadTools[0].dice.rand());
    mLogs.initRandomList();
    mLogs.shuffleRandomList(0, mLogs.games(), mt);
    
    RatingSurvey survey;
    
    if (games >= mLogs.games()) { // 同じ試合を複数回使う場合
        // 全試合の平均得点を記録
        std::map<std::string, double> scoreSum;
        std::map<std::string, int> gameSum;
        iterateGameRandomlyWithIndex(mLogs, 0, mLogs.games(), [&scoreSum, &gameSum](int index, const auto& gLog, const auto& mLog)->void{
            for (int p = 0; p < N_PLAYERS; ++p) {
                scoreSum[mLog.player(p)] += REWARD(gLog.getPlayerNewClass(p));
                gameSum[mLog.player(p)] += 1;
            }
        });
        for (auto& p : gameSum) {
            std::cerr << p.first << " : " << (scoreSum[p.first] / p.second) << std::endl;
        }
        
        // 全試合の予測相対勝率を先に計算
        std::vector<std::array<std::array<BetaDistribution, N_PLAYERS>, N_PLAYERS>> relativeResults;
        std::vector<std::array<BetaDistribution, N_PLAYERS>> absoluteResults;
        
        relativeResults.reserve(mLogs.games());
        absoluteResults.reserve(mLogs.games());
        
        iterateGameRandomlyWithIndex(mLogs, 0, mLogs.games(), [&relativeResults, &absoluteResults, simulations](int index, const auto& gLog, const auto& mLog)->void{
            cerr << index << endl;
            RateCalculationData result;
            doSimulationsToEvaluate(gLog, simulations, &result, &shared, threadTools);
            relativeResults[index] = result.relativeWins;
            absoluteResults[index] = result.absoluteWins;
        });
        int gameCount = 0;
        while(gameCount < games) {
            iterateGameRandomlyWithIndex(mLogs, 0, mLogs.games(),
                                         [&survey, &relativeResults, &absoluteResults, &gameCount, games, coef, absolute](int index, const auto& gLog, const auto& mLog)->void{
                                             if (gameCount < games) {
                                                 if (!absolute) {
                                                     calcNextRateWithRelativeResult(gLog, mLog, relativeResults[index], coef, &survey);
                                                 } else {
                                                     calcNextRateWithAbsoluteResult(gLog, mLog, absoluteResults[index], coef, &survey);
                                                 }
                                                 gameCount += 1;
                                             }
                                         });
        }
    } else { // 使用する試合数が全試合数に満たない場合
        int gameCount = 0;
        while(gameCount < games) {
            iterateGameRandomly(mLogs, 0, mLogs.games(),
                                [&survey, &gameCount, games, simulations, coef, absolute](const auto& gLog, const auto& mLog)->void{
                                    if (gameCount < games) {
                                        calcNextRate(gLog, mLog, simulations, coef, absolute, &survey);
                                        gameCount += 1;
                                    }
                                });
        }
    }
    // 逆計算して予測得点計算
    invertRates(survey.toMeanVector());
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
    int simulations = 3000; // 予測勝率計算のためのシミュレーション回数
    double coef = 16.0; // レート変動係数
    // 逆計算用
    std::vector<double> rates;
    bool absolute = false;
    int games = -1;
    
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-i")) { // input directory
            DIRECTORY_PARAMS_IN = std::string(argv[c + 1]);
        }else if (!strcmp(argv[c], "-l")) { // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }else if (!strcmp(argv[c], "-ld")) { // log directory
            logFileNames = std::move(getFilePathVectorRecursively(std::string(argv[c + 1]), ".dat"));
        }else if (!strcmp(argv[c], "-c")) { // coefficient
            coef = atof(argv[c + 1]);
        }else if (!strcmp(argv[c], "-s")) { // num of simulations
            simulations = atoi(argv[c + 1]);
        }else if (!strcmp(argv[c], "-inv")) { // rate to score
            for (int cc = c + 1; cc < argc; ++cc) {
                if (!strcmp(argv[cc], "-f")) { break; } // finish command
                rates.push_back(atof(argv[cc]));
            }
        }else if (!strcmp(argv[c], "-a")) { // absolute mode
            absolute = true;
        }else if (!strcmp(argv[c], "-g")) { // num of used games
            games = atoi(argv[c + 1]);
        }
    }
    
    // レート計算のためシミュレーションに使用するデータを準備
    shared.initMatch();
    shared.setMyPlayerNum(-1);
    shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");
    shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    
    shared.basePlayPolicy.setTemperature(Settings::simulationTemperaturePlay);
    shared.baseChangePolicy.setTemperature(Settings::simulationTemperatureChange);
    // スレッドごとのデータ初期化
    for (int th = 0; th < N_THREADS; ++th) {
        threadTools[th].init(th);
    }
    XorShift64 tdice;
    tdice.srand((unsigned int)time(NULL));
    for (int th = 0; th < N_THREADS; ++th) {
        threadTools[th].dice.srand(tdice.rand() * (th + 111));
    }
    
    mLogs.fin(logFileNames, true);
    
    if (rates.size() > 0) {
        invertRates(rates);
        return 0;
    }
    if (games < 0) {
        games = 100 * mLogs.games();
    }
    calcRating(mLogs, games, simulations, coef, absolute);
    
    return 0;
}
