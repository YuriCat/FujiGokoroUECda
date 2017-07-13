/*
 worldCreator.hpp
 Katsuki Ohto
 */

#ifndef UECDA_FUJI_WORLDCREATOR_HPP_
#define UECDA_FUJI_WORLDCREATOR_HPP_

#include "../../settings.h"
#include "dealer.hpp"

// 仮想世界作成

namespace UECda{
    namespace Fuji{

        /**************************仮想世界作成(連続分配)**************************/

        template<class dice64_t>
        class WorldCreator{
            // 世界創世者
            // 実装はWorldCreator.hpp内にて
        private:
            RandomDealer<N_PLAYERS> dealer; // カードディーラー
            bool preparedFlag;

        public:
            template<class sbjField_t>
            void prepare(const sbjField_t& field){
                // 連続世界生成のための準備をする
                
                // ディーラー準備
                dealer.set(field);
                preparedFlag = 1;
                
                DERR << Space(field.getDepth() * 2) << "WORLD CREATOR PREPARED." << endl;
            }

            template<int LEVEL, class sbjField_t, class world_t, class sharedData_t, class threadTools_t>
            int create(world_t *const dst, const sbjField_t& field, const sharedData_t& shared, threadTools_t *const ptools){
                // 世界を1つ生成
                // 生成失敗の場合は-1を返す
                
                DERR << Space(field.getDepth() * 2) << "START CREATION" << endl;
                
                // 共通
                Cards c[N_PLAYERS]; // 自分以外
                
                // レベル指定からカード分配
                switch(LEVEL){
                    case DealType::RANDOM: // レベル0は残りカードを完全ランダム分配
                        dealer.dealAllRand(c, &ptools->dice); break;
                    case DealType::SBJINFO: // 交換等は考慮するが残りは完全ランダム
                        dealer.dealWithAbsSbjInfo(c, &ptools->dice); break;
                    case DealType::BIAS: // 逆関数法でバイアスを掛けて配る
                        dealer.dealWithBias(c, &ptools->dice); break;
                    case DealType::REJECTION: // 採択棄却法で良さそうな配置のみ返す
                        dealer.dealWithRejection(c, shared, ptools); break;
                    default: UNREACHABLE; break;
                }
                
                //カード分配から世界生成
                dst->set(field,c);
                
                DERR << Space(field.getDepth() * 2) << "END CREATION" << endl;
                
                return 0;
            }
            
            void init(){
                preparedFlag = false;
            }

            WorldCreator()
            {
                init();
            }

            ~WorldCreator(){}
        };
    }
}

#endif // UECDA_FUJI_WORLDCREATOR_HPP_

