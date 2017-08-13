/*
 fuji.h
 Katsuki Ohto
 */
//#pragma once
#ifndef FUJI_H_
#define FUJI_H_

// 思考部本体でだけでなくテストコード等でも読み込むコードとデータのヘッダ

#include "../include.h"

namespace UECda{
    namespace Fuji{
        namespace Settings{
            // パラメータ等の可変設定
            // 本番用(match)ビルドでは全て定数として埋め込む
            // こちらに値を直書きしてもいいが途中でデフォルトに戻す操作を可能にするため分けている
            
            // 行動評価点による事前分布設定
            MATCH_CONST double rootPlayPriorCoef = 4;
            MATCH_CONST double rootPlayPriorExponent = 0.6;
            
            MATCH_CONST double rootChangePriorCoef = 4;
            MATCH_CONST double rootChangePriorExponent = 0.6;
            
            // 手札推定設定
            MATCH_CONST double estimationTemperatureChange = SIMULATION_TEMPERATURE_CHANGE;
            MATCH_CONST double estimationTemperaturePlay = SIMULATION_TEMPERATURE_CHANGE;
            
            // プレー設定
            MATCH_CONST bool L2SearchOnRoot = true;
            MATCH_CONST bool MateSearchOnRoot = true;
            
            // シミュレーション設定
            MATCH_CONST bool L2SearchInSimulation = true;
            MATCH_CONST bool LnCISearchInSimulation = false;
            MATCH_CONST bool MateSearchInSimulation = true;
            
            MATCH_CONST double simulationTemperatureChange = SIMULATION_TEMPERATURE_CHANGE;
            MATCH_CONST double simulationTemperaturePlay = SIMULATION_TEMPERATURE_PLAY;
            
            MATCH_CONST double simulationAmplifyCoef = SIMULATION_AMPLIFY_COEF;
            MATCH_CONST double simulationAmplifyExponent = SIMULATION_AMPLIFY_EXPONENT;
            
            MATCH_CONST double temperatureChange = TEMPERATURE_CHANGE;
            MATCH_CONST double temperaturePlay = TEMPERATURE_PLAY;
            
            MATCH_CONST int NPlayThreads = N_PLAY_THREADS;
            MATCH_CONST int NChangeThreads = N_CHANGE_THREADS;
            
            MATCH_CONST Selector simulationSelector = SIMULATION_SELECTOR;
            
            MATCH_CONST DealType monteCarloDealType = MONTECARLO_DEAL_TYPE;
            
            // シミュレーション中の相手モデル利用設定
#ifdef MODELING_PLAY
            MATCH_CONST bool simulationPlayModel = true;
#else
            MATCH_CONST bool simulationPlayModel = false;
#endif
            // 相手モデル化のバイアス設定
            MATCH_CONST double playerBiasAttenuateRate = 0.999;//0.9997;
            MATCH_CONST double playerBiasPriorSize = 100;//1000;
            MATCH_CONST double playerBiasCoef = 1;//0.65;
        }
    }
}

#endif
