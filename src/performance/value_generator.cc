
// 試合の棋譜から末端報酬を計算
// 考え方: 階級の遷移確率行列から後の試合も考えた順位報酬を算出

#include "../UECda.h"
#include "../core/record.hpp"

using namespace UECda;

static void calcAveragedValue(const double transProbMatrix[N_PLAYERS][N_PLAYERS],
                              double value[N_REWARD_CALCULATED_GAMES][N_PLAYERS]) {
    // 階級のみ考慮するシンプルなモデル
    const int N = N_PLAYERS;
    double svalue[N_REWARD_CALCULATED_GAMES + 1][N_PLAYERS]; // 状態価値 (残り試合 x 階級)
    for (int i = 0; i < N; ++i)svalue[0][i] = 0;
    for (int g = 0; g < N_REWARD_CALCULATED_GAMES; ++g) {
        // 順位価値計算
        for (int i = 0; i < N; ++i) { // 順位
            value[g][i] = REWARD(i) + svalue[g][i];
        }
        // 状態価値計算
        for (int i = 0; i < N; ++i) { // 次の試合の階級
            svalue[g + 1][i] = 0;
            for (int j = 0; j < N; ++j) { // 次の試合の順位
                // 以降の試合の相対報酬を加算
                svalue[g + 1][i] += transProbMatrix[i][j] * value[g][j];
            }
        }
    }
    for (int g = 0; g < N_REWARD_CALCULATED_GAMES; ++g) {
        for (int i = 0; i < N; ++i)value[g][i] -= value[g][N - 1];
        for (int i = 0; i < N - 1; ++i)printf("%.6f, ", value[g][i]);
        printf("0.0,\n"); // 0を細かく表示しても意味ないので
    }
    printf("\n");
}

void calcDaifugoSeatValue(const double transProbMatrix[N_PLAYERS][N_PLAYERS][N_PLAYERS * N_PLAYERS],
                          double value[N_REWARD_CALCULATED_GAMES][SEAT_INIT_CYCLE][N_PLAYERS][N_PLAYERS]) {
    // 大富豪の座席と席替えを考慮したモデル
    const int N = N_PLAYERS, S = SEAT_INIT_CYCLE;
    // 状態勝ち (残り試合数 x 席替えまで試合数 x 大富豪からの相対座席位置 x 階級)
    double svalue[N_REWARD_CALCULATED_GAMES + 1][SEAT_INIT_CYCLE][N_PLAYERS][N_PLAYERS];
    
    for (int k = 0; k < S; ++k)for (int s = 0; s < N; ++s)for (int i = 0; i < N; ++i)
        svalue[0][k][s][i] = 0;
    
    for (int g = 0; g < N_REWARD_CALCULATED_GAMES; ++g) {
        // 順位価値計算
        for (int k = 0; k < S; ++k) { // 席替えまでの試合数
            for (int s = 0; s < N; ++s) { // この試合の1位からの相対席順
                for (int i = 0; i < N; ++i) { // 順位
                    if ((i == 0) != (s == 0)) {
                        value[g][k][s][i] = -100; // 起こり得ないので適当な値を入れておく
                        continue;
                    }
                    value[g][k][s][i] = REWARD(i);
                    if (s == 0 || k > 0) { // 席順シャッフル
                        value[g][k][s][i] += svalue[g][(k + S - 1) % S][s][i];
                    } else { // 相対席順ランダムに変化
                        for (int ss = 1; ss < N_PLAYERS; ++ss)
                            value[g][k][s][i] += svalue[g][(k + S - 1) % S][ss][i] / (N_PLAYERS - 1);
                    }
                }
            }
        }
        for (int k = 0; k < S; ++k)cerr << svalue[g][k][0][0] << " ";
        cerr << endl;
        // 状態価値計算
        for (int k = 0; k < S; ++k) { // 席替えまでの試合数
            for (int s = 0; s < N; ++s) { // 次の試合の大富豪からの相対席順
                for (int i = 0; i < N; ++i) { // 次の試合の階級
                    svalue[g + 1][k][s][i] = 0;
                    double probSum = 0;
                    for (int ss = 0; ss < N; ++ss) { // 次の試合の1位の相対席順
                        for (int j = 0; j < N; ++j) { // 次の試合の順位
                            int nrs = (s + N_PLAYERS - ss) % N_PLAYERS;
                            // 以降の試合の相対報酬を加算
                            svalue[g + 1][k][s][i]
                            += transProbMatrix[s][i][ss * N_PLAYERS + j] * value[g][k][nrs][j];
                            //cerr << g << " " << k << " " << s << " " << i << " " << ss << " " << j << " ";
                            //cerr << transProbMatrix[s][i][ss * N_PLAYERS + j] << " " << value[g][k][nrs][j] << endl;
                            assert(transProbMatrix[s][i][ss * N_PLAYERS + j] == 0
                                   || value[g][k][nrs][j] > 0);
                            probSum += transProbMatrix[s][i][ss * N_PLAYERS + j];
                        }
                    }
                    //cerr << probSum << " ";
                }
            }
        }
    }
    for (int g = 0; g < N_REWARD_CALCULATED_GAMES; ++g) {
        for (int k = 0; k < S; ++k) {
            double minValue = 100000;
            for (int s = 1; s < N; ++s) {
                minValue = min(minValue, value[g][k][s][N - 1]);
            }
            for (int s = 0; s < N; ++s)
                for (int i = 0; i < N; ++i)
                    value[g][k][s][i] -= minValue;
        }
        for (int k = 0; k < S; ++k) {
            for (int s = 0; s < N; ++s) {
                for (int i = 0; i < N; ++i)
                    if (value[g][k][s][i] > 0)printf("%.4f,", value[g][k][s][i]);
                    else if (value[g][k][s][i] < 0)printf("-1.0,");
                    else printf("0.0,"); // 0を細かく表示しても意味ないので
                printf(" ");
            }
            printf("\n");
        }
        printf("\n");
    }
}

template <int M = N_PLAYERS, int N = N_PLAYERS>
std::string toMatString(const double mat[M][N], const std::string& name) {
    std::ostringstream oss;
    oss << " " << name << " = {" << endl;
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            oss << mat[i][j] << ", ";
        }oss << endl;
    }oss << "}" << endl;
    return oss.str();
}


int main(int argc, char* argv[]) {
    
    std::vector<std::string> logFileNames;
    
    for (int c = 1; c < argc; ++c) {
        if (!strcmp(argv[c], "-l")) { // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }else if (!strcmp(argv[c], "-ld")) {
            const std::string logDirectory = std::string(argv[c + 1]);
            const std::vector<std::string> added = getFilePathVectorRecursively(std::string(argv[c + 1]), ".dat");
            logFileNames.insert(logFileNames.end(), added.begin(), added.end());
        }
    }
    
    // 試合棋譜を読み込んで遷移行列を計算
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog>>, 8192> mLogs(logFileNames);
    
    // 全体 (階級 x 順位)
    double transProbMatrixAll[N_PLAYERS][N_PLAYERS] = {0};
    
    // 大富豪の相対的な座席ごとに計算(自分が大富豪のときは除外)
    // (相対的座席位置 x 階級 x 順位)
    double transProbMatrixDaifugoSeatMe[N_PLAYERS][N_PLAYERS][N_PLAYERS] = {0};
    // (相対的座席位置 x 階級 x (1位あがりの相対的座席位置 x 順位))
    double transProbMatrixDaifugoSeat[N_PLAYERS][N_PLAYERS][N_PLAYERS * N_PLAYERS] = {0};
    
    // 遷移回数カウント
    for (int m = 0; m < mLogs.matches(); ++m) {
        const auto& mLog = mLogs.match(m);
        for (int g = 0; g < mLog.games(); ++g) {
            const auto gLog = mLog.game(g);
            if (!gLog.isInitGame()) { // 階級初期化ゲームは除外
                int daifugoSeat = gLog.seatOf(gLog.classPlayer(DAIFUGO)); // 大富豪の座席
                int winnerSeat = gLog.seatOf(gLog.newClassPlayer(DAIFUGO)); // 1位あがりの座席
                for (int p = 0; p < N_PLAYERS; ++p) {
                    int cl = gLog.classOf(p); // 元々の階級
                    int ncl = gLog.newClassOf(p); // この試合の順位
                    
                    transProbMatrixAll[cl][ncl] += 1;
                    
                    // 大富豪基準で見た自分の座席位置
                    int relativeSeat = (gLog.seatOf(p) + N_PLAYERS - daifugoSeat) % N_PLAYERS;
                    ASSERT(0 <= relativeSeat && relativeSeat < N_PLAYERS,
                           cerr << relativeSeat << endl;);
                    
                    int winnerRelativeSeat = (winnerSeat + N_PLAYERS - daifugoSeat) % N_PLAYERS;
                    
                    transProbMatrixDaifugoSeatMe[relativeSeat][cl][ncl] += 1;
                    transProbMatrixDaifugoSeat[relativeSeat][cl][winnerRelativeSeat * N_PLAYERS + ncl] += 1;
                }
            }
        }
    }
    
    // 遷移回数表示
    cerr << toMatString(transProbMatrixAll, "all count");
    for (int s = 0; s < N_PLAYERS; ++s)
        cerr << toMatString(transProbMatrixDaifugoSeatMe[s],
                            "daifugo-based-seat(" + std::to_string(s) + ") count");
    for (int s = 0; s < N_PLAYERS; ++s)
        cerr << toMatString<N_PLAYERS, N_PLAYERS * N_PLAYERS>(transProbMatrixDaifugoSeat[s],
                                                              "daifugo-based-seat-all(" + std::to_string(s) + ") count");
    
    // 遷移確率行列への正規化
    normalize2<N_PLAYERS, N_PLAYERS>(transProbMatrixAll, 16);
    cerr << toMatString(transProbMatrixAll, "all trans prob");
    for (int s = 0; s < N_PLAYERS; ++s) {
        for (int cl = 0; cl < N_PLAYERS; ++cl) {
            normalize<N_PLAYERS>(transProbMatrixDaifugoSeatMe[s][cl]);
            normalize<N_PLAYERS * N_PLAYERS>(transProbMatrixDaifugoSeat[s][cl]);
        }
    }
    for (int s = 0; s < N_PLAYERS; ++s)
        cerr << toMatString<N_PLAYERS>(transProbMatrixDaifugoSeatMe[s],
                                       "daifugo-based-seat(" + std::to_string(s) + ") trans prob");
    for (int s = 0; s < N_PLAYERS; ++s)
        cerr << toMatString<N_PLAYERS, N_PLAYERS * N_PLAYERS>(transProbMatrixDaifugoSeat[s],
                                       "daifugo-based-seat-all(" + std::to_string(s) + ") trans prob");
    
    // 順位価値の計算
    double standardValue[N_REWARD_CALCULATED_GAMES][N_PLAYERS];
    double daifugoSeatValue[N_REWARD_CALCULATED_GAMES][SEAT_INIT_CYCLE][N_PLAYERS][N_PLAYERS];
    
    calcAveragedValue(transProbMatrixAll, standardValue);
    calcDaifugoSeatValue(transProbMatrixDaifugoSeat, daifugoSeatValue);
    
    return 0;
}
