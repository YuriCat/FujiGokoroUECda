/*
 value_generator.cc
 Katsuki Ohto
 */

// 試合の棋譜から末端報酬を計算

#include "../include.h"
#include "../structure/log/minLog.hpp"

using namespace UECda;

template<int N = N_PLAYERS>
static void calcAveragedValue(const double transProbMatrix[N][N]){
    // 階級の遷移確率行列から後の試合も考えた順位報酬を算出
    double value[N_REWARD_CALCULATED_GAMES + 1][N];
    for(int i = 0; i < N; ++i){
        value[0][i] = 0;
    }
    for(int g = 1; g <= N_REWARD_CALCULATED_GAMES; ++g){
        for(int i = 0; i < N; ++i){
            value[g][i] = REWARD(i); // 実報酬
            for(int j = 0; j < N; ++j){
                value[g][i] += transProbMatrix[i][j] * value[g - 1][j];
            }
        }
        for(int i = 0; i < N; ++i){
            value[g][i] -= value[g][N - 1];
        }
        for(int i = 0; i < N; ++i){
            printf("%d, ", (int)(value[g][i] * 100));
        }
        printf("\n");
    }
}

template<int N = N_PLAYERS>
void calcDaifugoSeatValue(){
    // 大富豪の座席を考慮
    /*double value[N_REWARD_CALCULATED_GAMES][3][N];
    for(int s = 0; s < 3; ++s){
        for(int i = 0; i < N; ++i){
            value[0][s][i] = 0;
        }
    }
    for(int g = 1; g < N_REWARD_CALCULATED_GAMES; ++g){
        for(int s = 0; s < 3; ++s){
            for(int i = 0; i < N; ++i){
                value[g][s][i] = REWARD(i); // 実報酬
                if(i == DAIFUGO){
                    for(int j = 0; j < N; ++j){
                    value[g][s][i] += tmpAll[i][]
                    
                for(int j = 1; j < N; ++j){
                    value[g][i] += transProbMatrix[i][j] * value[g - 1][j];
                }
            }
        }
        for(int i = 0; i < N; ++i){
            value[g][i] -= value[g][N- 1];
        }
        for(int i = 0; i < N; ++i){
            printf("%d, ", (int)(value[g][i] * 100));
        }
        printf("\n");
    }*/
}

template<int M = N_PLAYERS, int N = N_PLAYERS>
std::string toMatString(const double mat[M][N], const std::string& name){
    std::ostringstream oss;
    oss << " " << name << " = {" << endl;
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < N; ++j){
            oss << mat[i][j] << ", ";
        }oss << endl;
    }oss << "}" << endl;
    return oss.str();
}


int main(int argc, char* argv[]){
    
    std::vector<std::string> logFileNames;
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-l")){ // log path
            logFileNames.push_back(std::string(argv[c + 1]));
        }
    }
    
    // 試合棋譜を読み込んで遷移行列を計算
    MinMatchLogAccessor<MinMatchLog<MinGameLog<MinPlayLog<N_PLAYERS>>>, 256> mLogs(logFileNames);
    
    // 全体
    double transProbMatrixAll[N_PLAYERS][N_PLAYERS] = {0};
    
    // 大富豪の相対的な座席ごとに計算(自分が大富豪のときは除外)
    double transProbMatrixDaifugoSeat[N_PLAYERS][N_PLAYERS - 1][N_PLAYERS] = {0};
    
    // 遷移回数カウント
    for(int m = 0; m < mLogs.matches(); ++m){
        const auto& mLog = mLogs.match(m);
        for(int g = 0; g < mLog.games(); ++g){
            const auto gLog = mLog.game(g);
            if(!gLog.isInitGame()){ // 階級初期化ゲームは除外
                int daifugoSeat = gLog.infoSeat().at(invert(gLog.infoClass()).at(DAIFUGO)); // 大富豪の座席
                for(int p = 0; p < N_PLAYERS; ++p){
                    int cl = gLog.infoClass().at(p); // 元々の階級
                    int ncl = gLog.infoNewClass().at(p); // この試合の順位
                    
                    transProbMatrixAll[cl][ncl] += 1;
                    
                    if(cl != DAIFUGO){
                        int relativeSeat = (daifugoSeat + N_PLAYERS - gLog.infoSeat().at(p)) % N_PLAYERS;
                        ASSERT(0 <= relativeSeat && relativeSeat < N_PLAYERS,
                               cerr << relativeSeat << endl;);
                        transProbMatrixDaifugoSeat[relativeSeat][cl - 1][ncl] += 1;
                    }else{
                        
                    }
                }
            }
        }
    }
    
    // 遷移回数表示
    cerr << toMatString(transProbMatrixAll, "all count");
    for(int s = 0; s < N_PLAYERS; ++s){
        cerr << toMatString(transProbMatrixDaifugoSeat[s],
                            "daifugo-seat(" + std::to_string(s) + ") count");
    }
    
    // 遷移確率行列への正規化
    normalize2<N_PLAYERS, N_PLAYERS>(transProbMatrixAll, 16);
    cerr << toMatString(transProbMatrixAll, "all trans prob");
    for(int s = 0; s < N_PLAYERS; ++s){
        for(int cl = 1; cl < N_PLAYERS; ++cl){
            normalize<N_PLAYERS>(transProbMatrixDaifugoSeat[s][cl - 1]);
        }
        cerr << toMatString<N_PLAYERS - 1>(transProbMatrixDaifugoSeat[s],
                            "daifugo-seat(" + std::to_string(s) + ") trans prob");
    }
    
    calcAveragedValue(transProbMatrixAll);
    
    return 0;
}
