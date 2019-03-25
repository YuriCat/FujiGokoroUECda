/*
 timeAnaysis.hpp
 Katsuki Ohto
 */

#pragma once

namespace UECda{
    namespace Fuji{
        
        // 計算量解析
        const uint32_t time_model_easy4[2][16] = {
            0, 0, 0, 0, 1744, 3695, 9160, 4661, 2967, 3602, 12473, 35549, 78575, 139675, 171586, 215824,
            105, 212, 532, 8392, 257443, 528169, 1232153, 517278, 252578, 5783, 231, 121, 308, 769, 1286, 1558,
        };
        
        constexpr uint64_t mypc_play_time[6] = {
            // うちのPCで実際に自分が掛けた平均着手決定時間(ノーチャンス等含む)
            0, // レベル0はどうでもいい
            14416ULL,
            19892ULL,
            26715ULL,
            31522ULL,
            33607ULL,
        };
        
        struct MyTimeAnalyzer{
            // 主観的な時間の解析
            uint64_t time_rate; // 実レートの256倍
            // 自分のプレー記録
            uint64_t my_play_time_lock;
            uint64_t my_play_time_sum;
            uint64_t my_plays;
            
            void modifyTimeRate() {
                //プレーヤーの平均計算時間(自分が計測。通信の影響を受けていないもの)に合わせ、枠を調節する
                if (!my_play_time_lock) {
                    if (my_play_time_sum & (1ULL << 63)) { // 最上位ビットが埋まってしまった
                        my_play_time_lock = true; // この記録は役に立たないかもしれないのでこれ以降固定とする
                    }
                    if (my_plays) {
                        uint64_t my_avg_time = my_play_time_sum / my_plays;
                        assert(my_avg_time > 0);
                        
                        if (THINKING_LEVEL >= 6) {
                            time_rate = 40000ULL * 256 / my_avg_time; // データが無いので適当に4万としている
                        } else {
                            time_rate = mypc_play_time[min(5, max(0, THINKING_LEVEL))] * 256 / my_avg_time;
                        }
                    }
                    CERR << "new time rate = " << time_rate / (double)256 << endl;
                }
            }
            
            MyTimeAnalyzer() {
                time_rate = 256;
                my_play_time_lock = false;
                my_play_time_sum = 0ULL;
                my_plays = 0U;
            }
        };
        
        struct TimeModel{
            
            uint32_t time_dist[2][16];
            
            uint32_t time_dist7[2][7];
            
            uint32_t dist_sum[2];
            
            uint32_t trials;
            uint64_t real_sum;
            
            void attenuateDist() {
                // 分布を減衰させる
                // 減衰率は63/64とする
                
                for (int ph = 0; ph < 2; ++ph) {
                    for (int s = 0; s < 16; ++s) {
                        time_dist[ph][s] *= 63;
                        time_dist[ph][s] = max(time_dist[ph][s] / 64, 16U);
                    }
                }
                
                for (int ph = 0; ph < 2; ++ph) {
                    time_dist7[ph][0] = time_dist[ph][0] + time_dist[ph][1] + time_dist[ph][2] + time_dist[ph][3];
                    time_dist7[ph][1] = time_dist[ph][4] + time_dist[ph][5];
                    time_dist7[ph][2] = time_dist[ph][6] + time_dist[ph][7];
                    time_dist7[ph][3] = time_dist[ph][8] + time_dist[ph][9];
                    time_dist7[ph][4] = time_dist[ph][10] + time_dist[ph][11];
                    time_dist7[ph][5] = time_dist[ph][12] + time_dist[ph][13];
                    time_dist7[ph][6] = time_dist[ph][14] + time_dist[ph][15];
                    
                    for (int s = 0; s < 7; ++s) {
                        CERR << time_dist7[ph][s] << ", ";
                    }
                    CERR << endl;
                }
                
                //合計を計算し直す
                for (int ph = 0; ph < 2; ++ph) {
                    
                    dist_sum[ph] = 0;
                    
                    for (int s = 0; s < 7; ++s) {
                        dist_sum[ph] += time_dist7[ph][s];
                    }
                }
            }
            void feed_real_time(int ph, uint64_t arg) {
                // 実時間を受け取り、mypc時間に変換、分布に加算する
# ifdef MODELING_TIME
                uint32_t s = min(15U, log2i((arg * time_rate / 256) + 1));
                
                assert(ph == 0 || ph == 1);
                assert(s <= 15U);
                
                constexpr uint32_t sz = 32U; // 基本ウェイト
                
                time_dist[ph][s] += sz * 5;
                
                if (s >= 1U) {
                    time_dist[ph][s - 1] += sz * 2;
                } else {
                    time_dist[ph][s] += sz * 2;
                }
                if (s + 1 <= 15U) {
                    time_dist[ph][s + 1] += sz * 2;
                } else {
                    time_dist[ph][s] += sz * 2;
                }
                
                if (s >= 2U) {
                    time_dist[ph][s - 2] += sz;
                }
                if (s + 2 <= 15U) {
                    time_dist[ph][s + 2] += sz;
                }
                
                // それとは別に実際の時間を記録
                ++trials;
                real_sum += arg;
                
#endif
            }
            
            void init(const uint32_t model[2][16]) {
                trials = 0U;
                real_sum = 0ULL;
                //分布に事前分布を入れる
                for (int ph = 0; ph < 2; ++ph) {
                    for (int s = 0; s < 16; ++s) {
                        time_dist[ph][s]= max(model[ph][s] / 128, 16U);
                    }
                }
                for (int ph = 0; ph < 2; ++ph) {
                    time_dist7[ph][0] = time_dist[ph][0] + time_dist[ph][1] + time_dist[ph][2] + time_dist[ph][3];
                    time_dist7[ph][1] = time_dist[ph][4] + time_dist[ph][5];
                    time_dist7[ph][2] = time_dist[ph][6] + time_dist[ph][7];
                    time_dist7[ph][3] = time_dist[ph][8] + time_dist[ph][9];
                    time_dist7[ph][4] = time_dist[ph][10] + time_dist[ph][11];
                    time_dist7[ph][5] = time_dist[ph][12] + time_dist[ph][13];
                    time_dist7[ph][6] = time_dist[ph][14] + time_dist[ph][15];
                }
                
                for (int ph = 0; ph < 2; ++ph) {
                    
                    dist_sum[ph] = 0;
                    
                    for (int s = 0; s < 7; ++s) {
                        dist_sum[ph] += time_dist7[ph][s];
                    }
                }
                
            }
        };
        
        template<class clientGameLog_t>
        bool judgeWatchingMode(const clientGameLog_t& gLog) {
            // 試合後に判定
            // 試合が観戦モードで動いていたか調べる
            // その試合が観戦モードで動いていた場合、
            // この試合の計算時間を学習しないようにする
            // &次の試合の局面推定に計算量推定をいれないようにする
            
            // javaサーバーでは機能していないようだ
            // 実はcサーバーで機能するかもよくわからない
            
            // 判定基準:
            // 自分のプレー決定実時間と、着手間隔時間の間に0.1秒以上のずれがあったことが2回以上あった
            
            const int delay_line = 2;
            int delays = 0;
            
            for (int t = 1; t < gLog.plays(); ++t) { // turn1から
                const auto& pLog = gLog.play(t);
                
                if (pLog.subjectiveTime != 0) { // 主観的計算時間は自分以外は0のはず
                    if (pLog.time() - pLog.subjectiveTime >= 1000 * 1000 / 10) { // 0.1秒以上のディレイがあった
                        if (delays >= delay_line - 1) {
                            return true;
                        }
                        ++delays;
                    }
                }
            }
            return false;
        }
    }
}
