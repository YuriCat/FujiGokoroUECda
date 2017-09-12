/*
 playPolicy.hpp
 Katsuki Ohto
 */

#pragma once

// 役提出方策
#include "../core/prim.hpp"
#include "../core/prim2.hpp"

#include "../core/moveGenerator.hpp"

#include "../core/dominance.hpp"
#include "../core/appliedLogic.hpp"

namespace UECda{
    
    /**************************着手方策点**************************/
    
    namespace PlayPolicySpace{
        enum{
            
            // 後場パラメータ
            POL_HAND_SNOWL, // snowl点
            POL_HAND_S3, // ジョーカー-S3
            POL_HAND_PQR_RANK, // 平均pqrランク項
            POL_HAND_NF_PARTY, // 最小分割数
            
            POL_HAND_P8_JOKER, // ジョーカーと飛ばし重合
            
            // 着手パラメータ
            POL_MOVE_QTY, // 着手の枚数、空場のみ
            //POL_DOM, // 予測支配率
            POL_SUITLOCK_EFFECT, // スートロックの後自分がターンを取れそうか?
            POL_SAME_QR, // 同じ枚数組の多さに応じた加点
            POL_REV_CLASS, // 革命優先度
            
            POL_PASS_PHASE, // 序中終盤でのパス優先度
            POL_PASS_DOM, // パスをしても場が取れるか
            POL_PASS_OWNER_DISTANCE, // 現在場役主が場を取ったとして、自分が何人目か
            POL_PASS_NAWAKE_OWNER, // 自分がパスをした後残っている人数
            
            POL_MOVE_S3, // JK->S3
            POL_MOVE_JOKER_AGAINST_S3, // S3返しの危険性に応じてシングルジョーカーの使用を考える
            POL_MOVE_SEQ, // 階段
            POL_MOVE_RF_GROUP_BREAK, // 通常場での強カードでのグループ崩し
            
            POL_MOVE_NFDOM_PASSDOM, // パス支配の場で空場支配役を出すのはどうなのか
            POL_MOVE_NF_EIGHT_MANY_WEAKERS, // 8より弱いカードが複数ランク(階段除く)ある場合の8
            POL_MOVE_EIGHT_QTY, // 8と残りカードの枚数
            
            //POL_STOP_OWNER_MATE, // 場役主が詰めろのときに止めに行くか
            
            //POL_MOVE_STRONG_BEFORE_REV, // 革命が有効そうな時に先に強い札を出すか?
            
            POL_MOVE_MIN_RANK,
            
            POL_ALMOST_MATE, // 高確率で勝ちの時に勝負に出るか
            
            // 以下、シミュレーション中には差分計算しておくパラメータ
            
            //POL_HAND_2PQR, // 2分割相関
            POL_GR_CARDS, // MC関係(グループ)
            POL_SEQ_CARDS, // MC関係(階段)
            
            POL_GR_MO, // MO関係(グループ)
            POL_SEQ_MO, // MO関係(グループ)
            
            //POL_GR_MCC, // MCC関係
            //POL_GR_MCO, // MCO関係
            
            //POL_PLAY_CC, // 自分の使った札と残り札
            
            //POL_UC_OC, // 自分が今回使う札と自分以外の残り札
            
#ifdef MODELING_PLAY
            POL_MODEL_NF,
            POL_MODEL_RF,
#endif
            
            POL_ALL,
        };
        
        constexpr int numTable[] = {
            //手札
            83 * 2 * 3,
            3,
            1,
            2,//16,
            
            1,
            
            4,
            //2 * (4 + 4),
            3,
            3,
            5,//N_PLAYERS,
            
            3,
            2,
            5 - 1, //N_PLAYERS - 1,
            (5 - 1) * 8, //(N_PLAYERS - 1) * 8,
            
            2,
            3,
            3,
            3,
            
            1,
            1,
            2,
            
            //0,//3,
            
            //0,
            
            2,
            
            1,
            
            //2 * 16 * 16 * 16 * 16,
            //2 * (16 * 16 * 2) * (16 * 16),
            //2 * (16 * 4 * 3) * (16 * 16),
            
            2 * (16 * 2) * (16) * N_PATTERNS_SUITS_SUITS, // オーダー x 着手ランク x スートロック x 手札ランク x (Suits, Suits)パターン
            2 * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS, // オーダー x 着手ランク x 枚数 x 手札ランク x (Suit, Suits)パターン
            
            2 * (16 * 2) * (16) * N_PATTERNS_SUITS_SUITS, // オーダー x 着手ランク x スートロック x 手札ランク x (Suits, Suits)パターン
            2 * (16 * 3) * (16) * N_PATTERNS_SUIT_SUITS, // オーダー x 着手ランク x 枚数 x 手札ランク x (Suit, Suits)パターン
            
            //2 * (16 * 2) * (16) * (16) * N_PATTERNS_SUITS_SUITS_SUITS, // オーダー x 着手R x スートロック x 手札1R x 手札2R x (Suits, Suits, Suits)パターン
            
            //2 * 16 * 16 * 35, // オーダー x 変化カードランク x 手札カードランク x (Suits, Suits)パターン
            
#ifdef MODELING_PLAY
            5,
            12,
#endif
        };
        constexpr int FEA_NUM(unsigned int fea){
            return numTable[fea];
        }
        constexpr int FEA_IDX(unsigned int fea){
            return (fea == 0) ? 0 : (FEA_IDX(fea - 1) + FEA_NUM(fea - 1));
        }
        constexpr int FEA_NUM_ALL = FEA_IDX(POL_ALL);
        
#define LINEOUT(feature, str) { out << str << endl; int base = FEA_IDX(feature);\
for (int i = 0; i < FEA_NUM(feature); ++i){os(base + i); } out << endl; }
        
#define LINEOUTX(feature, str, x) { out << str << endl; int base = FEA_IDX(feature); int num = FEA_NUM(feature);\
for (int i = 0;;){os(base + i); ++i; if(i >= num)break; if(i % (x) == 0){ out << endl; }} out << endl; }
        
        template<typename T>
        int commentToPolicyParam(std::ostream& out, const T param[FEA_NUM_ALL]){
            auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };
            
            out << "****** MOVE POLICY ******" << endl;
            
            {
                out << "HAND_SNOWL" << endl;
                int base = FEA_IDX(POL_HAND_SNOWL);
                std::string situationString[3] = { "NF", "RF", "UR" };
                std::string orderString[2] = { "NML", "REV" };
                for (int s = 0; s < 3; ++s){
                    for (int o = 0; o < 2; ++o){
                        out << situationString[s] << " " << orderString[o] << endl;
                        out << "  SEQ" << endl;
                        for (int i = 0; i < 11; ++i){ os(base + i); }out << endl;
                        for (int i = 0; i < 10; ++i){ os(base + 11 + i); }out << endl;
                        for (int i = 0; i < 9; ++i){ os(base + 21 + i); }out << endl;
                        out << "  GROUP" << endl;
                        for (int i = 0; i < 13; ++i){
                            for (int j = 0; j < 4; ++j){ os(base + 30 + 4 * i + j); }out << endl;
                        }
                        out << "  JOKER" << endl;
                        os(base + 82); out << endl;
                        base += 83;
                    }
                }
            }
            
            LINEOUT(POL_HAND_S3, "JK_S3");
            LINEOUT(POL_HAND_PQR_RANK, "AVG_PQR");
            LINEOUT(POL_HAND_NF_PARTY, "NF_PARTY");
            LINEOUT(POL_HAND_P8_JOKER, "P8_JOKER");
            LINEOUT(POL_MOVE_QTY, "QTY");
            LINEOUT(POL_SUITLOCK_EFFECT, "SUITLOCK_EFFECT");
            LINEOUT(POL_SAME_QR, "SAME_QR");
            LINEOUT(POL_REV_CLASS, "REV_CLASS");
            LINEOUT(POL_MOVE_S3, "S3 ON JOKER");
            LINEOUT(POL_MOVE_JOKER_AGAINST_S3, "JOKER AGAINST S3");
            LINEOUT(POL_PASS_PHASE, "PASS_PHASE");
            LINEOUT(POL_PASS_DOM, "PASS_DOM");
            LINEOUT(POL_PASS_OWNER_DISTANCE, "PASS_OWNER_DISTANCE");
            LINEOUTX(POL_PASS_NAWAKE_OWNER, "PASS_DOM_NAWAKE_OWNER", 8);
            LINEOUT(POL_MOVE_SEQ, "SEQ");
            LINEOUT(POL_MOVE_RF_GROUP_BREAK, "RF_GROUP_BREAK");
            LINEOUT(POL_MOVE_NFDOM_PASSDOM , "NFDOM_PASSDOM");
            LINEOUT(POL_MOVE_NF_EIGHT_MANY_WEAKERS, "NF_EIGHT_MANY_WEAKERS_NOREV");
            LINEOUT(POL_MOVE_EIGHT_QTY, "EIGHT_QTY");
            LINEOUT(POL_MOVE_MIN_RANK, "MIN_RANK");
            //LINEOUT(,);
            return 0;
        }
#undef LINEOUT
    }
    
    //using PlayPolicy = SoftmaxPolicy<PlayPolicySpace::FEA_NUM_ALL, 1>;
    //using PlayPolicyLearner = SoftmaxPolicyLearner<PlayPolicy>;
    template<typename T> using PlayPolicy = SoftmaxClassifier<PlayPolicySpace::FEA_NUM_ALL, 1, 1, T>;
    template<typename T> using PlayPolicyLearner = SoftmaxClassifyLearner<PlayPolicy<T>>;
    
    template<typename T>
    int foutComment(const PlayPolicy<T>& pol, const std::string& fName){
        std::ofstream ofs(fName, std::ios::out);
        return PlayPolicySpace::commentToPolicyParam(ofs, pol.param_);
    }
    
    double calcRankScore(Cards pqr, int jk, int ord){
        // 階級平均点を計算
        int r = 0;
        int cnt = 0;
        if(ord == ORDER_NORMAL){
            while(pqr){
                IntCard ic = popIntCardLow(&pqr);
                r += IntCardToRank(ic);
                ++cnt;
            }
        }else{
            while(pqr){
                IntCard ic = popIntCardLow(&pqr);
                r += RANK_3 + RANK_2 - IntCardToRank(ic);
                ++cnt;
            }
        }
        if(jk){
            r += RANK_2 + 1;
            ++cnt;
        }
        return r / (double)cnt;
    }
    
#define Foo(i) s += pol.param(i);\
    if(!MODELING && M){ pol.feedFeatureScore(m, (i), 1.0); }
    
#define FooX(i, x) s += pol.param(i) * (x);FASSERT(x,);\
    if(!MODELING && M){ pol.feedFeatureScore(m, (i), (x)); }
    
#define FooM(i) s += pol.param(i);\
    if(M){ pol.feedFeatureScore(m, (i), 1.0); }
    
    struct PlayerPolicySubValue{
        // 方策計算を行うために予め計算しておく値(プレーヤーごと)
        double twoMeldsValue[2]; // 2役スコア(オーダー通常, 反転)
    };
    struct PolicySubValue{
        // 方策計算を行うために予め計算しておく値(全体)
        
    };
    
    /*double calcTwoMeldsValueDiff(int afterOrder){
     // 元の手札の2役点のスコアから、検討する着手における2役点の差分値を
     // 後場のオーダーに対して計算する
     
     }*/
    
    template<
    int M = 1, // 0 通常系計算, 1 学習のため特徴ベクトル記録, 2 強化学習のためデータ保存
    int MODELING = 0, // 相手モデル化項追加
    int PRECALC = 0, // 計算高速化のための事前計算を行っている
    class move_t, class field_t, class policy_t>
    int calcPlayPolicyScoreSlow(double *const dst,
                                move_t *const buf,
                                const int NMoves,
                                const field_t& field,
                                policy_t& pol){ // learnerとして呼ばれうるため const なし
        
        using namespace PlayPolicySpace;
        
        // 速度不問で計算
        
        // 恒常パラメータ
        const Board bd = field.bd;
        const int tp = field.getTurnPlayer();
        const int turnSeat = field.getPlayerSeat(tp);
        const int owner = field.getPMOwner();
        const int ownerSeat = field.getPlayerSeat(owner);
        const Hand& myHand = field.getHand(tp);
        const Hand& opsHand = field.getOpsHand(tp);
        const Cards myCards = myHand.getCards();
        const int NMyCards = myHand.getQty();
        const uint32_t oq = myHand.qty;
        const Cards curPqr = myHand.pqr;
        const FieldAddInfo& fieldInfo = field.fieldInfo;
        const int NParty = calcMinNMelds(buf + NMoves, myCards);
        
        // 元々の手札の最低、最高ランク
        const int myLR = IntCardToRank(pickIntCardLow(myCards));
        const int myHR = IntCardToRank(pickIntCardHigh(myCards));
        
        const int order = bd.tmpOrder();
        
        const double nowRS = calcRankScore(curPqr, containsJOKER(myCards) ? 1 : 0, bd.tmpOrder());
        
        // 場役主から自分が何人目か数える
        int distanceToOwner = 0;
        if (owner != tp){
            distanceToOwner = 1;
            for (int s = getPreviousSeat<N_PLAYERS>(turnSeat); s != ownerSeat; s = getPreviousSeat<N_PLAYERS>(s)){
                if (field.isAlive(field.getSeatPlayer(s))){
                    ++distanceToOwner;
                }
            }
            if(field.isAlive(owner)){ ++distanceToOwner; }
        }
        
        // 自分以外のプレーヤーの手札の和
        const Cards opsCards = opsHand.getCards();
        const Cards opsPlainCards = maskJOKER(opsCards);
        
        // 各階級の予測支配率（tagashira's score）
        /*int domProb[2][16][4]; // オーダー x ランク x 枚数
         //tick();
         for(int o = 0; o < 2; ++o){
         for(int r = myLR; r <= myHR; ++r){
         const Cards strongerMsk = strongerMask(r, o);
         for(int q = 1; q <= 4; ++q){
         domProb[o][r][q - 1] = 100;
         }
         if(r != RANK_8){
         // シングル
         domProb[o][r][0] = max(0, int(domProb[o][r][0] - 30 * countCards(opsHand.nd[o] & genSinglePQR(1) & strongerMsk)));
         // 複数枚
         for(int q = 2; q <= 4; ++q){
         if(opsHand.nd[o] & strongerMsk){ // 支配できない可能性がある
         if(o == ORDER_NORMAL){
         for(int rr = r + 1; rr <= RANK_2; ++rr){
         int oq = opsHand.qr[rr] + opsHand.jk;
         int k = oq - q - field.ps.getNAlive() + N_PLAYERS;
         if(k <= 0){
         domProb[o][r][q - 1] -= 4;
         }else if(k == 1){
         domProb[o][r][q - 1] -= 9;
         }else if(k == 2){
         domProb[o][r][q - 1] -= 15;
         }else{
         domProb[o][r][q - 1] -= 24;
         }
         }
         }else{
         for(int rr = r - 1; rr >= RANK_3; --rr){
         int oq = opsHand.qr[rr] + opsHand.jk;
         int k = oq - q - field.ps.getNAlive() + N_PLAYERS;
         if(k <= 0){
         domProb[o][r][q - 1] -= 4;
         }else if(k == 1){
         domProb[o][r][q - 1] -= 9;
         }else if(k == 2){
         domProb[o][r][q - 1] -= 15;
         }else{
         domProb[o][r][q - 1] -= 24;
         }
         }
         }
         domProb[o][r][q - 1] = max(0, domProb[o][r][q - 1]);
         }
         }
         }
         }
         }
         
         for(int o = 0; o < 2; ++o){
         for(int q = 1; q <= 4; ++q){
         for(int r = 0; r < 16; ++r){
         cerr << std::setw(3) << domProb[o][r][q - 1] << " ";
         }cerr << endl;
         }cerr <<endl;
         }
         getchar();
         //tock();*/
        
        pol.template initCalculatingScore(NMoves);
        
        for (int m = 0; m < NMoves; ++m){
            
            pol.template initCalculatingCandidateScore();
            
            MoveInfo& mv = buf[m];
            typename policy_t::real_t s = 0;
            
            if(mv.isMate() || !anyCards(subtrCards(myCards, mv.cards()))){
                s -= 1000; // Mateのときは低い点にする
            }else{
                // 着手パラメータ
                const int aftOrd = bd.afterTmpOrder(mv.mv());
                const int q4 = min(mv.qty(), 4U);
                
                int i;
                const Cards afterCards = subtrCards(myCards, mv.cards());
                const Cards afterPqr = CardsToPQR(afterCards);
                const Cards myAfterSeqCards = polymRanks<3>(afterCards);
                
                const int afterOrder = aftOrd;
                
                // snowl score
                {
                    int base = FEA_IDX(POL_HAND_SNOWL);
                    
                    if(!bd.isNF()){
                        if (fieldInfo.isUnrivaled()){
                            base += 166 * 2;
                        }else{
                            base += 166;
                        }
                    }
                    if (afterOrder != ORDER_NORMAL){ base += 83; }
                    
                    if(containsJOKER(afterCards)){
                        i = base + 82;
                        Foo(i);
                    }
                    
                    Cards tmp = maskJOKER(afterCards);
                    
                    Cards seq3 = polymRanks<3>(tmp);
                    if(seq3){
                        Cards seq4 = polymRanks<2>(seq3);
                        if(seq4){
                            Cards seq5 = polymRanks<2>(seq4);
                            if(seq5){
                                // 6枚以上の階段も5枚階段と同じパラメータで扱う(ダブルカウントしないように注意)
                                maskCards(&seq4, extractRanks<2>(seq5));
                                maskCards(&seq3, extractRanks<3>(seq5));
                                maskCards(&tmp, extractRanks<5>(seq5));
                                while(1){
                                    IntCard ic = pickIntCardLow(seq5);
                                    
                                    i = base + IntCardToRank(ic) - RANK_3;
                                    Foo(i);
                                    
                                    maskCards(&seq5, extractRanks<5>(IntCardToCards(ic)));
                                    if(!seq5){ break; }
                                }
                            }
                            if(seq4){
                                maskCards(&tmp, extractRanks<4>(seq4));
                                maskCards(&seq3, extractRanks<2>(seq4));
                                while(1){
                                    IntCard ic = popIntCard(&seq4);
                                    
                                    i = base + 9 + IntCardToRank(ic) - RANK_3;
                                    Foo(i);
                                    
                                    if(!seq4){ break; }
                                }
                            }
                        }
                        if(seq3){
                            maskCards(&tmp, extractRanks<3>(seq3));
                            while(1){
                                IntCard ic = popIntCard(&seq3);
                                
                                i = base + 19 + IntCardToRank(ic) - RANK_3;
                                Foo(i);
                                
                                if(!seq3){ break; }
                            }
                        }
                    }
                    
                    if(tmp){
                        base += 30 - 4;
                        //CERR<<OutCards(tmp);
                        tmp = CardsToPQR(tmp); // 枚数位置型に変換
                        //CERR<<OutCards(tmp)<<endl;getchar();
                        while(1){
                            IntCard ic = popIntCard(&tmp);
                            
                            i = base + ic; // 枚数位置型なのでそのままインデックスになる
                            Foo(i);
                            
                            if(!tmp){break;}
                        }
                    }
                }
                FASSERT(s,);
                
                // joker, s3 bonus
                {
                    constexpr int base = FEA_IDX(POL_HAND_S3);
                    if(containsS3(afterCards) && containsJOKER(afterCards)){
                        i = base + 0;
                        Foo(i);
                    }else if(containsS3(afterCards) && containsJOKER(opsCards)){
                        i = base + 1;
                        Foo(i);
                    }else if (containsS3(opsCards) && containsJOKER(afterCards)){
                        i = base + 2;
                        Foo(i);
                    }
                }
                FASSERT(s,);
                
                // avg pqr rank
                {
                    constexpr int base = FEA_IDX(POL_HAND_PQR_RANK);
                    double rs = calcRankScore(afterPqr, containsJOKER(afterCards) ? 1 : 0, aftOrd);
                    i = base;
                    FooX(i, (((double)oq) * log (max(rs / nowRS, 0.000001))));
                    //FooX(i, rs);
                    //cerr<<pol.param(i)<<" "<<(((double)oq) * log (max(rs/nowRS,0.000001)))<<endl;
                    //cerr << "Cards = "<<OutCards(myCards)<<" Move = "<<mv<<" nowRS = " << nowRS << " rs = " << rs << endl;
                }
                FASSERT(s,);
                
                // nf min party
                {
                    /*constexpr int base = FEA_IDX(POL_HAND_NF_PARTY);
                     if(bd.isNF()){
                     i = base + calcMinNMelds(buf + NMoves, c);
                     Foo(i);
                     }*/
                    const int base = FEA_IDX(POL_HAND_NF_PARTY);
                    if(bd.isNF()){
                        i = base;
                        FooX(i, calcMinNMelds(buf + NMoves, afterCards) - NParty);
                    }else{
                        i = base + 1;
                        FooX(i, calcMinNMelds(buf + NMoves, afterCards) - NParty);
                    }
                }
                FASSERT(s,);
                
                // after hand joker - p8
                if(polymJump(maskJOKER(afterCards)) && containsJOKER(afterCards)){
                    i = FEA_IDX(POL_HAND_P8_JOKER);
                    Foo(i);
                }
                FASSERT(s,);
                
                // qty
                {
                    constexpr int base = FEA_IDX(POL_MOVE_QTY);
                    if(bd.isNF()){
                        i = base + q4 - 1;
                        Foo(i);
                    }
                }
                FASSERT(s,);
                
                //same qr
                {
                    constexpr int base = FEA_IDX(POL_SAME_QR);
                    if(bd.isNF() && !mv.isSeq()){
                        i = base;
                        int sameQ = countCards(curPqr & (PQR_1 << (q4 - 1)));
                        if (!mv.containsJOKER()){
                            FooX(i, sameQ);
                        }
                        else{
                            FooX(i + 1, sameQ);
                            if(q4 > 0){ // シングルジョーカーで無い
                                FooX(i + 2, countCards(curPqr & (PQR_1 << (q4 - 2))));
                            }
                        }
                    }
                }
                FASSERT(s,);
                
                // suit lock
                {
                    constexpr int base = FEA_IDX(POL_SUITLOCK_EFFECT);
                    if(!mv.isPASS() && !bd.isNF()){
                        if((!fieldInfo.isLastAwake()) && bd.locksSuits(mv.mv())){ // LAでなく、スートロックがかかる
                            if(mv.qty() > 1){
                                // 2枚以上のロックは強力
                                i = base + 2;
                            }
                            else{
                                // 最強のを自分が持っているか??
                                Cards lockedSuitCards = SuitsToCards(bd.suits());
                                Cards mine = maskJOKER(afterCards) & lockedSuitCards;
                                // ジョーカーをここで使っていい場合にはロックしたいので、
                                // それは考慮に入れる
                                if(!anyCards(maskJOKER(afterCards))){
                                    i = base + 0;
                                }else{
                                    Cards ops = maskJOKER(opsCards) & lockedSuitCards;
                                    bool hasStrong = (afterOrder == ORDER_NORMAL) ? (mine > ops) : (pickLow(mine) < pickLow(ops));
                                    if (hasStrong){
                                        i = base + 0;
                                    }else{
                                        if(!containsS3(opsCards)
                                           &&
                                           !any2Cards(CardsToPQR(maskJOKER(afterCards)))){
                                            // ジョーカー出して流してあがり
                                            i = base + 0;
                                        }else{
                                            i = base + 1;
                                        }
                                    }
                                }
                            }
                            Foo(i);
                        }
                    }
                }
                FASSERT(s,);
                
                // rev(only normal order)
                {
                    constexpr int base = FEA_IDX(POL_REV_CLASS);
                    if ((!bd.isPrmOrderRev()) && mv.flipsPrmOrder()){
                        int relativeClass = field.getPlayerClass(tp);
                        for (int r = 0; r < (int)field.getPlayerClass(tp); ++r){
                            if (!field.isAlive(field.getClassPlayer(r))){ --relativeClass; }
                        }
                        i = base + relativeClass;
                        Foo(i);
                    }
                }
                FASSERT(s,);
                
                // pass(game phase)
                {
                    constexpr int base = FEA_IDX(POL_PASS_PHASE);
                    if (mv.isPASS()){
                        i = base + ((field.getNRemCards() > 30) ? 0 : ((field.getNRemCards() > 15) ? 1 : 2));
                        Foo(i);
                    }
                }
                FASSERT(s,);
                
                // pass(unrivaled)
                {
                    constexpr int base = FEA_IDX(POL_PASS_DOM);
                    if (mv.isPASS()){
                        if (fieldInfo.isPassDom()){
                            i = base;
                            Foo(i);
                            if(!fieldInfo.isUnrivaled()){
                                i = base + 1;
                                Foo(i);
                            }
                        }
                    }
                }
                FASSERT(s,);
                
                // pass(owner distance)
                {
                    constexpr int base = FEA_IDX(POL_PASS_OWNER_DISTANCE);
                    if(mv.isPASS() && !fieldInfo.isUnrivaled()){
                        i = base + (distanceToOwner - 1);
                        Foo(i);
                    }
                    /*if(field.isAlive(owner)){
                     i = base + 3 + (N_PLAYERS - 2) + (field.getNAwakePlayers() - 2);
                     FooX(i, sqrt((double)field.getNCards(owner)));
                     }*/
                    /*if(field.isAlive(owner)){
                     i = base + field.getNAwakePlayers() - 2;
                     //FooX(i, sqrt((double)field.getNCards(owner)));
                     Foo(i);
                     }*/
                }
                FASSERT(s,);
                
                // pass nawake and qty
                {
                    constexpr int base = FEA_IDX(POL_PASS_NAWAKE_OWNER);
                    if(mv.isPASS() && owner != tp){
                        if(field.isAlive(owner)){
                            i = base + (field.getNAwakePlayers() - 2) * 8 + min(field.getNCards(owner), 8U) - 1;
                            Foo(i);
                        }
                    }
                }
                FASSERT(s,);
                
                // s3(move)
                {
                    constexpr int base = FEA_IDX(POL_MOVE_S3);
                    if (bd.isSingleJOKER() && mv.isS3Flush()){
                        i = base + (fieldInfo.isPassDom() ? 1 : 0);
                        Foo(i);
                    }
                }
                FASSERT(s,);
                
                // joker against S3
                {
                    constexpr int base = FEA_IDX(POL_MOVE_JOKER_AGAINST_S3);
                    if (mv.isSingleJOKER()){
                        if(containsS3(afterCards)){ // 自分でS3を被せられる
                            i = base + 0;
                        }else if (fieldInfo.isLastAwake() || (!containsCard(opsCards, CARDS_S3))){ // safe
                            i = base + 1;
                        }
                        else{ // dangerous
                            i = base + 2;
                        }
                        Foo(i);
                    }
                }
                FASSERT(s,);
                
                // sequence
                {
                    constexpr int base = FEA_IDX(POL_MOVE_SEQ);
                    if(bd.isNF()){
                        if (mv.isSeq()){
                            i = base;
                            Foo(i);
                            i = base + 2;
                            FooX(i, NMyCards);
                        }
                    }else{
                        if(bd.isSeq()){
                            i = base + 1;
                            Foo(i);
                        }
                    }
                }
                FASSERT(s,);
                
                // rf group break
                {
                    constexpr int base = FEA_IDX(POL_MOVE_RF_GROUP_BREAK);
                    if(!bd.isNF() && !fieldInfo.isUnrivaled() && mv.isSingleOrGroup() && !mv.containsJOKER()){
                        if(myHand.qr[mv.rank()] != mv.qty()){ // 崩して出した
                            if(mv.domInevitably()){ // 8切り
                                i = base;
                                Foo(i);
                            }else{
                                if(aftOrd == ORDER_NORMAL){
                                    if(mv.rank() >= IntCardToRank(pickIntCardHigh(opsPlainCards))){
                                        if(containsJOKER(opsCards)){
                                            i = base + 2;
                                            Foo(i);
                                        }else{
                                            i = base + 1;
                                            Foo(i);
                                        }
                                    }
                                }else{
                                    if(mv.rank() <= IntCardToRank(pickIntCardLow(opsPlainCards))){
                                        if(containsJOKER(opsCards)){
                                            i = base + 2;
                                            Foo(i);
                                        }else{
                                            i = base + 1;
                                            Foo(i);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                FASSERT(s,);
                
                // NF_Dominance Move On PassDom
                {
                    if(fieldInfo.isPassDom()){
                        if(mv.domInevitably() || dominatesHand(mv.mv(), opsHand, OrderToNullBoard(order))){
                            i = FEA_IDX(POL_MOVE_NFDOM_PASSDOM);
                            Foo(i);
                        }
                    }
                }
                FASSERT(s,);
                
                // NF_EIGHT
                if(bd.isNF()){
                    if(mv.domInevitably() && !mv.isSeq()){
                        if(isNoRev(afterCards)){
                            Cards seq = myAfterSeqCards;
                            Cards weakers = (order == ORDER_NORMAL) ?
                            RankRangeToCards(RANK_3, RANK_7) : RankRangeToCards(RANK_9, RANK_2);
                            if(any2Cards(maskCards(afterCards & weakers, seq)) && any2Cards(maskCards(curPqr & weakers, seq))){
                                i = FEA_IDX(POL_MOVE_NF_EIGHT_MANY_WEAKERS);
                                Foo(i);
                            }
                        }
                    }
                }
                FASSERT(s,);
                
                // eight
                {
                    if(mv.domInevitably()){
                        i = FEA_IDX(POL_MOVE_EIGHT_QTY);
                        FooX(i, (oq - mv.qty()) * (oq - mv.qty()));
                        i = FEA_IDX(POL_MOVE_EIGHT_QTY) + 1;
                        FooX(i, oq - mv.qty());
                    }
                }
                FASSERT(s,);
                
                // min rank
                {
                    constexpr int base = FEA_IDX(POL_MOVE_MIN_RANK);
                    if(bd.tmpOrder() == ORDER_NORMAL){
                        if(mv.rank() == myLR){
                            i = base;
                            Foo(i);
                        }
                    }else{
                        if(mv.rank() == myHR){
                            i = base + 1;
                            Foo(i);
                        }
                    }
                }
                FASSERT(s,);
                
                // dom prob
                
                // mate prob by tagashira's score
                // 残りカードが必勝である確率を求める
                /*int minDR = 100, min2DR = 100;
                 iterateIntCard(afterPqr & (~CARDS_8), [&](IntCard ic)->void{
                 int r = getIntCard_Rank(ic);
                 int q = ic % 4;
                 
                 int tmpDR = domProb[afterOrder][r][q - 1];
                 if(tmpDR < min2DR){
                 if(tmpDR < minDR){
                 min2DR = minDR;
                 minDR = tmpDR;
                 }else{
                 min2DR = tmpDR;
                 }
                 }
                 });
                 // min2DRが大きいことを評価
                 FooX(POL_ALMOST_MATE, min2DR);*/
                
                // 2pqr
                /*if(!PRECALC){
                 // その場計算
                 const int base = FEA_IDX(POL_HAND_2PQR);
                 for(int f = 0; f < 16; ++f){
                 for(int j = f + 1; j < 16; ++j){
                 if(((c >> (f * 4)) & 15) && ((c >> (j * 4)) & 15)){
                 i = base
                 + 16 * 16 * 16 * 16 * afterOrder
                 + f * 16 * 16 * 16
                 + j * 16 * 16
                 + ((c >> (f * 4)) & 15) * 16
                 + ((c >> (j * 4)) & 15);
                 Foo(i);
                 }
                 }
                 }
                 }else{
                 // 差分計算
                 
                 }*/
                
                /*if(mv.isSeq()){
                 // 階段
                 constexpr int base = FEA_IDX(POL_SEQ_CARDS);
                 for(int f = 0; f < 16; ++f){
                 if((c >> (f * 4)) & 15){
                 i = base
                 + order * (16 * 4 * 3) * (16 * 16)
                 + mv.rank() * (4 * 3) * (16 * 16)
                 + SuitToSuitNum(mv.suits()) * (3) * (16 * 16)
                 + min(int(mv.qty()) - 3, 2) * (16 * 16)
                 + f * (16)
                 + ((c >> (f * 4)) & 15);
                 
                 Foo(i);
                 }
                 }
                 }else if(!mv.isPASS()){
                 // グループ
                 constexpr int base = FEA_IDX(POL_GR_CARDS);
                 for(int f = 0; f < 16; ++f){
                 if((c >> (f * 4)) & 15){
                 i = base
                 + order * (16 * 16 * 2) * (16 * 16)
                 + mv.rank() * (16 * 2) * (16 * 16)
                 + mv.suits() * (2) * (16 * 16)
                 + (bd.locksSuits(mv.mv()) ? 1 : 0) * (16 * 16)
                 + f * (16)
                 + ((c >> (f * 4)) & 15);
                 Foo(i);
                 }
                 }
                 }*/
                
                // MCパターン
                if(!mv.isPASS()){
                    if(!mv.isSeq()){
                        // グループ
                        constexpr int base = FEA_IDX(POL_GR_CARDS);
                        using Index = TensorIndexType<2, 16, 2, 16, N_PATTERNS_SUITS_SUITS>;
                        if(mv.isSingleJOKER()){
                            for(int f = RANK_MIN; f <= RANK_MAX; ++f){
                                uint32_t qb = (afterPqr >> (f * 4)) & SUITS_ALL;
                                if(qb){
                                    i = base + Index::get(order, RANK_MAX + 2, 0, f, qb);
                                    // suits - suits のパターン数より少ないのでOK
                                    Foo(i);
                                }
                            }
                        }else{
                            for(int f = RANK_MIN; f <= RANK_MAX; ++f){
                                uint32_t as = (afterCards >> (f * 4)) & SUITS_ALL;
                                if(as){
                                    i = base + Index::get(order, mv.rank(), bd.locksSuits(mv.mv()) ? 1 : 0,
                                                          f, getSuitsSuitsIndex(mv.suits(), as));
                                    Foo(i);
                                }
                            }
                            if(containsJOKER(afterCards)){
                                i = base + Index::get(order, mv.rank(), bd.locksSuits(mv.mv()) ? 1 : 0,
                                                      RANK_MAX + 2, mv.qty());
                                // suits - suits のパターン数より少ないのでOK
                                Foo(i);
                            }
                        }
                    }else{
                        // 階段
                        constexpr int base = FEA_IDX(POL_SEQ_CARDS);
                        using Index = TensorIndexType<2, 16, 3, 16, N_PATTERNS_SUIT_SUITS>;
                        for(int f = RANK_MIN; f <= RANK_MAX; ++f){
                            uint32_t as = (afterCards >> (f * 4)) & SUITS_ALL;
                            if(as){
                                i = base + Index::get(order, mv.rank(), min(int(mv.qty()) - 3, 2),
                                                      f, getSuitSuitsIndex(mv.suits(), as));
                                Foo(i);
                            }
                        }
                        if(containsJOKER(afterCards)){
                            i = base + Index::get(order, mv.rank(), min(int(mv.qty()) - 3, 2),
                                                  RANK_MAX + 2, 0);
                            Foo(i);
                        }
                    }
                    FASSERT(s,);
                    
                    // MOパターン
                    if(!mv.isSeq()){
                        // グループ
                        constexpr int base = FEA_IDX(POL_GR_MO);
                        using Index = TensorIndexType<2, 16, 2, 16, N_PATTERNS_SUITS_SUITS>;
                        if(mv.isSingleJOKER()){
                            for(int f = RANK_MIN; f <= RANK_MAX; ++f){
                                uint32_t qb = (opsHand.pqr >> (f * 4)) & SUITS_ALL;
                                if(qb){
                                    i = base + Index::get(order, RANK_MAX + 2, 0, f, qb);
                                    // suits - suits のパターン数より少ないのでOK
                                    Foo(i);
                                }
                            }
                        }else{
                            for(int f = RANK_MIN; f <= RANK_MAX; ++f){
                                uint32_t os = (opsCards >> (f * 4)) & SUITS_ALL;
                                if(os){
                                    i = base + Index::get(order, mv.rank(), bd.locksSuits(mv.mv()) ? 1 : 0,
                                                          f, getSuitsSuitsIndex(mv.suits(), os));
                                    Foo(i);
                                }
                            }
                            if(containsJOKER(opsCards)){
                                i = base + Index::get(order, mv.rank(), bd.locksSuits(mv.mv()) ? 1 : 0,
                                                      RANK_MAX + 2, mv.qty());
                                // suits - suits のパターン数より少ないのでOK
                                Foo(i);
                            }
                        }
                    }else{
                        // 階段
                        constexpr int base = FEA_IDX(POL_SEQ_MO);
                        using Index = TensorIndexType<2, 16, 3, 16, N_PATTERNS_SUIT_SUITS>;
                        for(int f = RANK_MIN; f <= RANK_MAX; ++f){
                            uint32_t os = (opsCards >> (f * 4)) & SUITS_ALL;
                            if(os){
                                i = base + Index::get(order, mv.rank(), min(int(mv.qty()) - 3, 2),
                                                      f, getSuitSuitsIndex(mv.suits(), os));
                                Foo(i);
                            }
                        }
                        if(containsJOKER(opsCards)){
                            i = base + Index::get(order, mv.rank(), min(int(mv.qty()) - 3, 2),
                                                  RANK_MAX + 2, 0);
                            Foo(i);
                        }
                    }
                    FASSERT(s,);
                }
                
                // CCパターン
                /*{
                 constexpr int base = POL_IDX(POL_PLAY_CC);
                 const Cards diffRanks = gatherAll4Bits(myCards ^ afterCards);
                 Cards tmp = diffRanks;
                 while(tmp){
                 IntCard ic = popIntCard(&tmp);
                 unsigned int rx4 = getIntCard_Rankx4(ic);
                 unsigned int rank = rx4 / 4;
                 uint32_t suits = (myCards >> rx4) & SUITS_ALL;
                 for(int r = RANK_MIN; r <= RANK_MAX + 2; ++r){
                 FooX(base
                 + ord * 16 * 16 * 35
                 + rank * 16 * 35
                 + r * 35
                 + getSuitsSuitsIndex((myCards >> rx4) & SUITS_ALL,
                 (myCards >> (r * 4)) & SUITS_ALL),
                 -1);
                 Foo(base
                 + afterOrd * 16 * 16 * 35
                 + rank * 16 * 35 + r * 35 + getSuitsSuitsIndex((afterCards >> rx4) & SUITS_ALL,
                 (afterCards >> (r * 4)) & SUITS_ALL));
                 }
                 }
                 }*/
                FASSERT(s,);
                
                
#ifdef MODELING_PLAY
            MODEL:
                {
                    // 相手行動傾向をモデル化する項
                    if(field.isNF()){
                        constexpr int base = FEA_IDX(POL_MODEL_NF);
                        if(mv.containsJOKER()){
                            FooM(base + 2);
                        }else if(mv.domInevitably()){
                            FooM(base + 3);
                        }
                        if(mv.isGroup()){
                            FooM(base + 0);
                        }else if(mv.isSeq()){
                            FooM(base + 1);
                        }
                        if(mv.flipsPrmOrder()){
                            FooM(base + 4);
                        }
                    }else{
                        const int base = FEA_IDX(POL_MODEL_RF) + 4 * (bd.isSeq() ? 2 : (bd.isGroup() ? 1 : 0));
                        if(!mv.isPASS()){
                            if(mv.containsJOKER()){
                                if(mv.isSingleJOKER() && NMoves == 2){
                                    FooM(base + 1);
                                }else{
                                    FooM(base + 2);
                                }
                            }else if(mv.domInevitably()){
                                FooM(base + 3);
                            }
                        }else{
                            FooM(base + 0);
                        }
                    }
                }
#endif
            }
            pol.template feedCandidateScore(m, exp(s / pol.temperature()));
            
            if(!M || dst != nullptr){
                dst[m] = s;
            }
        }
        pol.template finishCalculatingScore();
        
        return 0;
    }
    
#undef FooX
#undef Foo
    template<int M = 1, class field_t, class policy_t>
    int calcPlayPolicyScoreSlow(
                                double *const dst,
                                const field_t& field,
                                const policy_t& pol
                                ){
        return calcPlayPolicyScoreSlow<M>(dst, field.mv, field.NActiveMoves, field, pol);
    }
    template<int M = 1, class move_t, class field_t, class policy_t>
    double calcPlayPolicyExpScoreSlow(double *const dst,
                                      move_t *const buf,
                                      const int NMoves,
                                      const field_t& field,
                                      const policy_t& pol){
        // 指数をかけるところまで計算したsoftmax policy score
        // 指数を掛けた後の合計値を返す
        calcPlayPolicyScoreSlow<M>(dst, buf, NMoves, field, pol);
        double sum = 0;
        for(int m = 0; m < NMoves; ++m){
            double es = exp(dst[m] / pol.temperature());
            dst[m] = es;
            sum += es;
        }
        return sum;
    }
    
    template<int STOCK = 0, class move_t, class field_t, class policy_t, class dice_t>
    int playWithPolicy(move_t *const buf, const int NMoves, const field_t& field, const policy_t& pol, dice_t *const pdice,
                       double *const pentropy = nullptr){
        double score[N_MAX_MOVES + 1];
        double sum = calcPlayPolicyExpScoreSlow<STOCK ? 2 : 0>(score, buf, NMoves, field, pol);
        //cerr << sum << endl;
        double r = pdice->drand() * sum;
        double entropy = 0;
        if(sum > 0){
            for(int m = 0; m < NMoves; ++m){
                double prob = score[m] / sum;
                if(prob > 0){
                    entropy -= prob * log(prob) / log(2.0);
                }
            }
        }
        int m = 0;
        for(; m < NMoves - 1; ++m){
            r -= score[m];
            if(r <= 0){
                break;
            }
        }
        if(pentropy != nullptr){
            *pentropy = entropy;
        }
        return m;
        // 和表現の場合
        //double drand = pdice->drand() * score[NMoves];
        //return sortedDAsearch(score, 0, NMoves, drand);
    }
    
    template<int STOCK = 0, class move_t, class field_t, class policy_t, class dice_t>
    int playWithBestPolicy(move_t *const buf, const int NMoves, const field_t& field, const policy_t& pol, dice_t *const pdice){
        double score[N_MAX_MOVES + 1];
        calcPlayPolicyScoreSlow<STOCK ? 2 : 0>(score, buf, NMoves, field, pol);
        int bestIndex[N_MAX_MOVES];
        bestIndex[0] = -1;
        int NBestMoves = 0;
        double bestScore = -DBL_MAX;
        for(int m = 0; m < NMoves; ++m){
            double s = score[m];
            if(s > bestScore){
                bestIndex[0] = m;
                bestScore = s;
                NBestMoves = 1;
            }else if(s == bestScore){
                bestIndex[NBestMoves] = m;
                ++NBestMoves;
            }
        }
        if(NBestMoves <= 1){
            return bestIndex[0];
        }else{
            return bestIndex[pdice->rand() % NBestMoves];
        }
    }
}
