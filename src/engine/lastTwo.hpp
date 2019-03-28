#pragma once

// ラスト2人探索
#include "../core/daifugo.hpp"
#include "../core/prim2.hpp"
#include "../core/hand.hpp"
#include "../core/mate.hpp"
#include "../core/logic.hpp"

namespace UECda {
    namespace Fuji {
        
        // L2局面情報(テンプレート引数)
        // IS_NF 0, 1 空場
        // IS_SFOL 2, 3 セルフフォロー
        
        constexpr int L2JUDGE_LEVEL_MAX = 1024;
        
        constexpr int genTA_L2FINFO(int IS_NF = _BOTH, int IS_SFOL = _BOTH) {
            return (IS_NF << 0) | (IS_SFOL << 2);
        }
        
        // L2着手情報(テンプレート引数)
        // IS_PASS 0, 1
        // IS_DO 2, 3
        // IS_DM 4, 5
        
        constexpr int genTA_L2MINFO(int IS_PASS = _BOTH) {
            return IS_PASS << 0;
        }
        
        namespace L2{
            // L2関連
            
            //#ifdef USE_L2BOOK
            constexpr int BOOK_SIZE = (1 << 18);
            TwoValueBook<BOOK_SIZE> book("L2Book");
            //#endif
        }
        
        enum{
            L2_NONE = -1,
            L2_WIN = 0,
            L2_DRAW = 1,
            L2_LOSE = 2
        };
        
        // L2局面表現
        struct L2Field{
            
            Board bd;
            FieldAddInfo fInfo;
#ifdef DEBUG
            int p; // プレーヤー確認用
#endif
            bool isNF()const noexcept{ return bd.isNF();}
            Board getBoard()const noexcept{ return bd;}
            
            uint32_t tmpOrder()const noexcept{ return bd.tmpOrder(); }
            
            uint64_t isLastAwake()const noexcept{ return fInfo.isLastAwake(); }
            uint64_t isFlushLead()const noexcept{ return fInfo.isFlushLead(); }
            uint64_t isUnrivaled()const noexcept{ return fInfo.isUnrivaled(); }
            
            uint64_t isTmpOrderSettled()const noexcept{ return fInfo.isTmpOrderSettled(); }
            uint64_t isDConst()const noexcept{ return fInfo.isDConst(); }
            
            void setSelfFollow()noexcept{ fInfo.setSelfFollow(); }
            void setLastAwake()noexcept{ fInfo.setLastAwake(); }
            void setFlushLead()noexcept{ fInfo.setFlushLead(); }
            
            void setDConst()noexcept{ fInfo.setDConst();}
            
            int getTurnPlayer()const noexcept{
#ifdef DEBUG
                return p;
#else
                return 0;
#endif
            }
            
            void flipTurnPlayer()noexcept{
#ifdef DEBUG
                p = 1 - p;
#endif
            }
            
            constexpr L2Field():
            bd(),fInfo()
#ifdef DEBUG
            ,p(0)
#endif
            {}
            
        };
        // L2局面表現へのチェンジ
        template<class argField_t>
        void convL2Field(const argField_t& argField, L2Field *const dstPtr) {
            dstPtr->bd = argField.getBoard();
            dstPtr->fInfo.init();
        }
        // フィールド追加情報から
        void convL2Field(const Board bd, const FieldAddInfo fInfo, L2Field *const dstPtr) {
            dstPtr->fInfo = fInfo;
            dstPtr->bd = bd;
        }
        
        template<int L2_FINFO, int L2_MINFO>
        void procAndFlushL2Field(const L2Field& cur, L2Field *const pnext, const Move mv) {
            // 場を流して自分から始めることは確実とされている場合
            constexpr int IS_NF = (L2_FINFO >> 0) & 3;
            constexpr int IS_PASS = (L2_MINFO >> 0) & 3;
            
            pnext->bd = cur.bd;
            pnext->fInfo = cur.fInfo;
            pnext->fInfo.initTmpInfo();
            
            pnext->bd.procAndFlush(mv);
        }
        
        template<int L2_FINFO, int L2_MINFO>
        int procL2Field(const L2Field& cur, L2Field *const pnext, const MoveInfo mi) {
            // テンプレート引数を使うバージョン
            // もしMAI部分に有益な情報が載らないなら、MAIの処理を消した方が速い
            
            constexpr int IS_NF = (L2_FINFO >> 0) & 3;
            constexpr int IS_SFOL = (L2_FINFO >> 2) & 3;
            
            constexpr int IS_PASS = (L2_MINFO >> 0) & 3;
            //constexpr int IS_DO = (L2_MINFO >> 2) & 3;
            //constexpr int IS_DM = (L2_MINFO >> 4) & 3;
            
            pnext->bd = cur.bd;
            pnext->fInfo = cur.fInfo;
            pnext->fInfo.initTmpInfo();
#ifdef DEBUG
            pnext->p = cur.p;
#endif
            
            const Move mv = mi.mv();
            
            if ((IS_SFOL == _YES) || cur.isUnrivaled()) { // 独壇場
                if (IS_PASS == _YES) {
                    pnext->bd.flush();
                    DERR << " -FLUSHED" << endl;
                } else {
                    if (mi.dominatesMe()) {
                        // 自己支配がかかるので、流れて自分から
                        pnext->bd.procAndFlush(mv);
                        DERR << " -FLUSHED" << endl;
                    } else {
                        // 流れなければSFが続く
                        pnext->bd.proc(mv);
                        if (pnext->bd.isNF()) { // renewed
                            DERR << " -FLUSHED" << endl;
                        } else {
                            pnext->setSelfFollow();
                            DERR << endl;
                        }
                    }
                }
                return 0;
            } else { // 独壇場でない
                if ((IS_NF == _YES) || ((IS_NF != _NO) && cur.isNF())) {
                    if (mi.dominatesAll()) {
                        pnext->bd.procAndFlush(mv);
                        DERR << " -FLUSHED" << endl; return 0;
                    } else {
                        if (mi.dominatesOthers()) {
                            pnext->bd.proc(mv);
                            if (pnext->bd.isNF()) { // renewed
                                DERR << " -FLUSHED" << endl; return 0;
                            } else {
                                pnext->setSelfFollow();
                                DERR << " -DO" << endl; return 0;
                            }
                        } else {
                            pnext->bd.proc(mv);
                            if (pnext->bd.isNF()) { // renewed
                                DERR << " -FLUSHED" << endl; return 0;
                            } else {
                                pnext->flipTurnPlayer();
                                DERR << endl; return 1;
                            }
                        }
                    }
                } else { // 通常場
                    if (IS_PASS == _YES || ((IS_PASS != _NO) && mv.isPASS())) {//pass
                        if (cur.isLastAwake()) {
                            pnext->bd.flush();
                            DERR << " -FLUSHED";
                            if (cur.isFlushLead()) {
                                DERR << "(LA & FL)" << endl;
                                return 0; // 探索中にここには来ないはずだが、入り口で来るかも
                            } else {
                                pnext->flipTurnPlayer();
                                DERR << endl;
                                return 1;
                            }
                        } else {
                            pnext->setLastAwake();
                            if (!cur.isFlushLead()) {
                                pnext->setFlushLead();
                            }
                            pnext->flipTurnPlayer();
                            DERR << " FIRST PASS" << endl;
                            return 1;
                        }
                    } else { // not pass
                        if (cur.isLastAwake()) {
                            if (mi.dominatesMe()) {
                                // 自己支配がかかるので、流れて自分から
                                pnext->bd.procAndFlush(mv);
                                DERR << " -FLUSHED" << endl;
                            } else {
                                pnext->bd.proc(mv);
                                if (pnext->bd.isNF()) { // renewed
                                    DERR << " -FLUSHED" << endl;
                                } else {
                                    pnext->setSelfFollow();
                                    DERR << endl;
                                }
                            }
                            return 0;
                        } else {
                            if (mi.dominatesAll()) {
                                pnext->bd.procAndFlush(mv);
                                DERR << " -FLUSHED" << endl; return 0;
                            } else {
                                if (mi.dominatesOthers()) {
                                    pnext->bd.proc(mv);
                                    if (pnext->bd.isNF()) { // renewed
                                        DERR << " -FLUSHED" << endl; return 0;
                                    } else {
                                        pnext->setSelfFollow();
                                        DERR << " -DO" << endl; return 0;
                                    }
                                } else {
                                    pnext->bd.proc(mv);
                                    if (pnext->bd.isNF()) { // renewed
                                        DERR << " -FLUSHED" << endl; return 0;
                                    } else {
                                        pnext->flipTurnPlayer();
                                        DERR << endl; return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            UNREACHABLE;
        }
        
        class L2Judge{
            
            // L2判定人
            // 単純な構造でデータを受け取り、
            
            // judge...局面L2判定を返す
            // check...着手L2判定を返す
            // search...与えられた着手集合に勝利着手があればインデックス、なければ-1を返す
            // play...局面から勝利着手を探し、勝利着手があれば着手、なければMOVE_ILLEGALを返す
            
            // 判定は、先手勝ち...L2_WIN 後手勝ち...L2_LOSE 判定出来ず...L2_DRAWと定義されているが、
            // L2_DRAWを返すことが無い方が処理が速いかも
            
        private:
            static AtomicAnalyzer<2, 8, Analysis::TYPE_SEARCH> ana;
            int mode; // 呼び出しモード
            
            const int NODE_LIMIT;
            
            MoveInfo *const buf;
            //TwoValueBook<L2::BOOK_SIZE>& book;
            
            int nodes;
            int childs;
            int failed;
            
        public:
            void init() {
                nodes = 0;
                childs = 0;
                failed = 0;
            }
            
            
            //L2Judge(int nl, MoveInfo *const argMI, TwoValueBook<L2::BOOK_SIZE>& argBook)
            //:
            //NODE_LIMIT(nl), // ほぼ無制限
            //buf(argMI),
            //book(argBook)
            //{
            //	init();
            //}
            
            L2Judge(int nl,MoveInfo *const argMI)
            :
            NODE_LIMIT(nl),
            buf(argMI)
            //book(L2::book)
            {
                init();
            }
            
            ~L2Judge() {}
            
            // IS_NF 空場
            // DOM_PROC 支配進行
            
            // 局面更新を終えないまま、情報を部分的に与えて高速判定する関数
            template<int L2_FINFO>
            int judge_level1(const int qty);
            
            template<int L2_FINFO>
            int judge_level2(const Hand& myHand);
            
            // 再帰版
            template<int S_LEVEL, int E_LEVEL, int L2_FINFO>
            int judge(const int depth, MoveInfo *const mv_buf, const Hand& myHand, const Hand& opsHand, const L2Field& field);
            
            template<int S_LEVEL, int E_LEVEL, int L2_FINFO, int L2_MINFO>
            int check(const int depth, MoveInfo *const mv_buf, MoveInfo& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field);
            
            template<int S_LEVEL, int E_LEVEL, int L2_FINFO>
            int search(const int depth, MoveInfo *const mv_buf, const int NMoves, const Hand& myHand, const Hand& opsHand, const L2Field& field);
            
            template<int S_LEVEL, int E_LEVEL, int L2_FINFO>
            int play(const int depth, MoveInfo *const mv_buf, const Hand& myHand, const Hand& opsHand, const L2Field& field);
            
            // 非再帰版
            
            //template<class field_t>int start_judge(const field_t& argField);
            //template<class field_t>int start_check(const Move move, const field_t& argField);
            template<class field_t>int start_search(const Move *mv, const int NMoves, const field_t& argField);
            template<class field_t>int start_play(const field_t& argField);
            
            int start_judge(const Hand& myHand, const Hand& opsHand, const Board bd, const FieldAddInfo fInfo);
            //int start_check(const Move move, MoveAddInfo *const mvInfo, const Hand& myHand, const Hand& opsHand, const Board bd, const FieldAddInfo fInfo);
            
            int start_check(const MoveInfo mi, const Hand& myHand, const Hand& opsHand, const Board bd, const FieldAddInfo fInfo);
            
        };
        
        AtomicAnalyzer<2, 8, Analysis::TYPE_SEARCH> L2Judge::ana("L2Judge");
        
        
        template<int L2_FINFO>
        int L2Judge::judge_level1(const int qty) {
            const int IS_NF = (L2_FINFO >> 0) & 3;
            // レベル1 自分のカード枚数のみ
            
            ++nodes;
            
            if (IS_NF == _YES) {
                if (qty <= 1) { return L2_WIN; }
            }
            return L2_DRAW;
        }
        
        template<int L2_FINFO>
        int L2Judge::judge_level2(const Hand& myHand) {
            const int IS_NF = (L2_FINFO >> 0) & 3;
            
            // レベル2 自分の手札情報のみ
            if (IS_NF == _YES) {
                if (judgeMate_Easy_NF(myHand)) {
                    //for (int d=0;d<depth;++d) {DERR<<" ";}
                    //DERR<<"-EMATEWIN"<<endl;
                    //ana.restart(mode,2);
                    return L2_WIN;
                }
            }
            return L2_DRAW;
        }
        
        template<int S_LEVEL, int E_LEVEL, int L2_FINFO>
        int L2Judge::judge(const int depth, MoveInfo *const mv_buf, const Hand& myHand, const Hand& opsHand, const L2Field& field) {
            
            constexpr int IS_NF = (L2_FINFO >> 0) & 3;
            
            DERR << "IS_NF = " << IS_NF << endl;
            
            // 判定を返す
            int res;
            
            assert(depth < 100);
            
            switch(S_LEVEL) {
                case 0: res = L2_NONE; if (E_LEVEL <= 0)break; // fallthrough
                case 1:
                    res = judge_level1<L2_FINFO>(myHand.qty);
                    if (res != L2_DRAW) { return res; }
                    if (E_LEVEL <= 1)break; // fallthrough
                case 2:
                    res = judge_level2<L2_FINFO>(myHand);
                    if (res != L2_DRAW) { return res; }
                    if (E_LEVEL <= 2)break; // fallthrough
                case 3:
                    // 局面や相手の手札も考えた必勝判定
                    ASSERT(myHand.exam1stHalf(), cerr << myHand.toDebugString(););
                    ASSERT(opsHand.exam1stHalf(), cerr << opsHand.toDebugString(););
                    
                    ana.restart(mode, 1);
                    if (IS_NF == _YES) {
                        if (judgeHandPW_NF(myHand, opsHand, field.getBoard())) {
                            ana.restart(mode, 5);
                            return L2_WIN;
                        }
                    }
                    ana.restart(mode, 5);
                    if (E_LEVEL <= 3)break; // fallthrough
                case 4:
                    // 局面登録を検索
#ifdef USE_L2BOOK
                    // NFのみ
                    uint64_t fhash;
                    
                    ana.restart(mode, 1);
                    
                    if ((IS_NF == _YES) || (IS_NF != _NO && field.isNF())) {
                        ASSERT(myHand.exam_hash(), cerr << myHand.toDebugString(););
                        ASSERT(opsHand.exam_hash(), cerr << opsHand.toDebugString(););
                        
                        fhash = knitL2NullFieldHashKey(myHand.hash, opsHand.hash, NullBoardToHashKey(field.bd));
                        res = L2::book.read(fhash);
                        //res = book.read(fhash);
                        if (res != -1) { // 結果が既に登録されていた
#ifdef DEBUG
                            DERR << Space(2 * depth);
                            switch(res) {
                                case L2_WIN: DERR << "-HASHWIN"; break;
                                case L2_DRAW: DERR << "-HASHDRAW"; break;
                                case L2_LOSE: DERR << "-HASHLOSE"; break;
                                default: break;
                            }
                            DERR << endl;
#endif
                            ana.restart(mode, 3);
                            return res;
                        }
                    }
                    
                    ana.restart(mode, 3);
#endif
                    if (E_LEVEL <= 4)break; // fallthrough
                case 5:
                    // 簡易評価
                    ana.restart(mode, 1);
                    
                    // 必敗判定
                    if ((IS_NF == _YES) || (IS_NF != _NO && field.isNF())) {
                        if ((!myHand.seq) && (!(myHand.getPQR() & PQR_234)) && (!myHand.containsJOKER()) && (!myHand.containsS3()) && (!field.bd.isTmpOrderRev())) {
                            int myHR = IntCardToRank4x(pickIntCardHigh(myHand.cards));
                            int opsLR = IntCardToRank4x(pickIntCardLow(opsHand.cards));
                            if (myHR < opsLR) {
                                //DERR << myHand <<" vs " << opsHand << endl; getchar();
                                DERR << Space(2 * depth) << "-EMATELOSE" << endl;
                                ana.restart(mode, 2);
                                return L2_LOSE;
                            } else {
                                Cards tmp = maskCards(opsHand.cards, Rank4xToCards(opsLR));
                                if (tmp) {
                                    opsLR = IntCardToRank4x(pickIntCardLow(tmp));
                                    if (myHR < opsLR) {
                                        //DERR << myHand << " vs " << opsHand << endl; getchar();
                                        DERR << Space(2 * depth) << "-EMATELOSE" << endl;
                                        ana.restart(mode, 2);
                                        return L2_LOSE;
                                    }
                                }
                            }
                        } else {
                            //static StaticCounter sc0;
                            //sc0.add();
                        }
                        
                        //if ( )
                    }
                    
                    ana.restart(mode, 2);
                    if (E_LEVEL <= 5)break; // fallthrough
                default: // 完全探索
                    
                    //DERR << field.bd << endl;
                    //DERR << myHand << endl;
                    //DERR << opsHand << endl;
                    
                    if (nodes > NODE_LIMIT) { DERR << "Last2P Node over!" << endl; failed = 1; return L2_DRAW; }
                    
                    // 合法手の抽出
                    ana.restart(mode, 1);
                    
                    const int NMoves = genMove(mv_buf, myHand, field.getBoard());
                    
                    DERR << "NMoves = " << NMoves << endl;
                    
                    assert(NMoves >= 1);
                    
                    childs += NMoves;
                    
                    ana.restart(mode, 4);
                    
                    if (IS_NF == _NO && field.getBoard().qty() >= myHand.qty && NMoves >= 2) { return L2_WIN; }
                    res = search<2, L2JUDGE_LEVEL_MAX, L2_FINFO>(depth, mv_buf, NMoves, myHand, opsHand, field);
                    
                    if (res >= 0) {
                        ana.restart(mode,1);
#ifdef USE_L2BOOK
                        if (IS_NF) {
                            ASSERT(fhash == knitL2NullFieldHashKey(myHand.hash, opsHand.hash, NullBoardToHashKey(field.bd)), cerr << fhash << endl;);
                            L2::book.regist(L2_WIN, fhash);
                            //book.regist(L2_WIN, fhash);
                        }
                        ana.restart(mode,3);
#endif
                        return L2_WIN;
                    }else if (res == -1) {
                        ana.restart(mode,1);
#ifdef USE_L2BOOK
                        if (IS_NF) {
                            ASSERT(fhash == knitL2NullFieldHashKey(myHand.hash, opsHand.hash, NullBoardToHashKey(field.bd)), cerr << fhash << endl;);
                            L2::book.regist(L2_LOSE, fhash);
                            //book.regist(L2_LOSE,fhash);
                        }
#endif
                        ana.restart(mode,3);
                        return L2_LOSE;
                    }
                    break;
            }
            
            return L2_DRAW;
        }
        
        template<int S_LEVEL, int E_LEVEL, int L2_FINFO, int L2_MINFO>
        int L2Judge::check(const int depth, MoveInfo *const mv_buf, MoveInfo& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field) {
            
            constexpr int IS_NF = (L2_FINFO >> 0) & 3;
            constexpr int IS_PD = (L2_FINFO >> 2) & 3; // パス支配局面
            constexpr int IS_NPDO = (L2_FINFO >> 2) & 3;
            
            constexpr int IS_PASS = (L2_MINFO >> 0) & 3;
            
            // 判定を返す
            
            int res;
            
            ++childs;
            
            if (E_LEVEL > 100) {
                DERR << Space(depth * 2) << "<" << field.getTurnPlayer() << ">" << tmp;
            }
            
            switch(S_LEVEL) {
                case 0: res = L2_NONE; if (E_LEVEL <= 0)break; // fallthrough
                case 1:{
                    // 1手詰み
                    if ((IS_PASS!=_YES) || (!tmp.isPASS())) { // 空場パスは考えない
                        if (tmp.qty() >= myHand.qty) {
                            DERR << Space(depth *2 ) << tmp << " -FIN" << endl;
                            return L2_WIN;
                        }
                    }
                    if (E_LEVEL <= 1)break;
                } // fallthrough
                case 2:{
                    // 支配からの簡単詰み
                    if (((IS_PD == _YES && (IS_NPDO == _YES)) || ((IS_NPDO == _YES) && (IS_PASS == _NO)) || ((IS_PD == _YES) && (IS_PASS == _YES))) // 無条件に支配
                       ||((IS_PASS == _NO) && dominatesCards(tmp.mv(), opsHand.getCards(), field.getBoard()))) { // 他支配チェック
                        tmp.setDomOthers();
                        res = judge_level1<(_YES << 0)>(myHand.qty - (IS_PASS == _YES ? 0 : tmp.qty()));
                        if (res == L2_WIN) {
                            DERR << Space(2 * depth) << "<" << field.getTurnPlayer() << ">" << tmp << " -EMATEWIN" << endl;
                            return res;
                        }
                        ana.restart(mode, 1);
                        Hand nextHand;
                        
                        if (!tmp.isPASS()) {
                            ASSERT(!tmp.mv().isPASS(), DERR << tmp.data() << endl;)
                            makeMove1stHalf(myHand, &nextHand, tmp.mv());
                            
                            ana.restart(mode, 7);
                            res = judge_level2<(_YES << 0)>(nextHand);
                            if (res == L2_WIN) {
                                DERR << Space(2 * depth) << "<" << field.getTurnPlayer() << ">" << tmp << " -EMATEWIN" << endl;
                                return res;
                            }
                            L2Field nextField;
                            ana.restart(mode, 1);
                            procAndFlushL2Field<L2_FINFO, (IS_PASS << 0)>(field, &nextField, tmp.mv());
                            ana.restart(mode, 6);
                            
                            nextHand.hash = myHand.hash ^ CardsToHashKey(tmp.cards());
                            ana.restart(mode, 7);
                            
                            res = judge<3, 4, (_YES << 0)>(depth + 1, mv_buf, nextHand, opsHand, nextField);
                            if (res == L2_WIN) {
                                DERR << Space(2 * depth) << "<" << field.getTurnPlayer() << ">" << tmp << " -EMATEWIN" << endl;
                                return res;
                            }
                        } else {
                            ana.restart(mode, 7);
                            res=judge_level2<(_YES << 0)>(myHand);
                            if (res == L2_WIN) {
                                DERR << Space(2 * depth) << "<" << field.getTurnPlayer() << ">" << tmp << " -EMATEWIN" << endl;
                                return res;
                            }
                            L2Field nextField;
                            ana.restart(mode, 1);
                            procAndFlushL2Field<L2_FINFO, (IS_PASS << 0)>(field, &nextField, tmp.mv());
                            ana.restart(mode, 6);
                            res = judge<3, 4, (_YES << 0)>(depth + 1, mv_buf, myHand, opsHand, nextField);
                            if (res == L2_WIN) {
                                DERR << Space(2 * depth) << "<" << field.getTurnPlayer() << ">" << tmp << " -EMATEWIN" << endl;
                                return res;
                            }
                        }
                        
                    }
                    if (E_LEVEL <= 2)break;
                } // fallthrough
                default:{
                    L2Field nextField;
                    Hand nextHand;
                    int nextPlayer;
                    constexpr int NEXT_S_LEVEL = 1;
                    int conRest = 0;
                    
                    if (IS_PASS == _NO) {
                        if (field.isDConst()) { // 支配拘束中
                            if (!tmp.dominatesOthers()) { // 他支配でない
                                if (hasDWorNFH(myHand.getCards() - tmp.cards(), opsHand.cards, field.tmpOrder(), field.fInfo.isTmpOrderSettled())) { // 残り札に支配保証or空場期待がある
                                    //DERR << tmp << ": DWorNFH_REST FAIL " << myHand << " vs " << opsHand << endl; getchar();
                                    DERR << Space(2 * depth) << "<" << field.getTurnPlayer() << ">" << tmp << " -RESTLOSE" << endl;
                                    return L2_LOSE; // 拘束条件違反で負けとみなす
                                }
                            } else {
                                // 拘束継続
                                conRest = 1;
                            }
                        } else {
                            // 支配保証拘束を掛ける
                            if ((IS_NF == _YES) && field.isTmpOrderSettled() && tmp.dominatesOthers()) {
                                if (hasDWorNFH(myHand.getCards() - tmp.cards(), opsHand.cards, field.tmpOrder(), field.fInfo.isTmpOrderSettled())) { // 残り札に支配保証or空場期待がある
                                    // DERR << tmp << ": DWorNFH_REST START " << myHand << " vs " << opsHand << endl; getchar();
                                    conRest = 1;
                                }
                            }
                        }
                    }
                    
                    // 支配性判定
                    if (IS_PASS == _NO && (field.isLastAwake() || tmp.dominatesOthers())) {
                        if (dominatesCards(tmp.mv(), myHand.getCards(), field.getBoard())) {
                            tmp.setDomMe();
                        }
                    }
                    
                    if (IS_PASS == _NO) {
                        assert(!tmp.isPASS());
                        
                        ana.restart(mode, 1);
                        makeMoveAll(myHand, &nextHand, tmp.mv());
                        DERR << " " << myHand << " -> " << nextHand;
                        ana.restart(mode, 7);
                        
                        nextPlayer = procL2Field<L2_FINFO, L2_MINFO>(field, &nextField, tmp);
                        ana.restart(mode, 6);
                        
                        if (conRest) { nextField.setDConst(); }
                        
                        int nextL2FINFO = genTA_L2FINFO(nextField.isNF() ? _YES : _NO, nextField.isUnrivaled() ? _YES : _NO);
                        
                        if (nextPlayer == 0) {
                            switch(nextL2FINFO) {
                                case genTA_L2FINFO(_NO, _NO):
                                    res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_NO, _NO)>(depth + 1, mv_buf, nextHand, opsHand, nextField); break;
                                case  genTA_L2FINFO(_YES, _NO):
                                    res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _NO)>(depth + 1, mv_buf, nextHand, opsHand, nextField); break;
                                case genTA_L2FINFO(_NO, _YES):
                                    res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_NO, _YES)>(depth + 1, mv_buf, nextHand, opsHand, nextField); break;
                                    //case  genTA_L2FINFO(_YES, _YES):
                                    //res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _YES)>(depth + 1, mv_buf, nextHand, opsHand, nextField); break;
                                default: UNREACHABLE; res = L2_NONE; break;
                            }
                        } else {
                            switch(nextL2FINFO) {
                                case genTA_L2FINFO(_NO,_NO):
                                    res=L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX,genTA_L2FINFO(_NO,_NO)>(depth+1,mv_buf,opsHand,nextHand,nextField);break;
                                case  genTA_L2FINFO(_YES,_NO):
                                    res=L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX,genTA_L2FINFO(_YES,_NO)>(depth+1,mv_buf,opsHand,nextHand,nextField);break;
                                case genTA_L2FINFO(_NO,_YES):
                                    res=L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX,genTA_L2FINFO(_NO,_YES)>(depth+1,mv_buf,opsHand,nextHand,nextField);break;
                                    //case  genTA_L2FINFO(_YES,_YES):
                                    //res = L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _YES)>(depth + 1, mv_buf, opsHand, nextHand, nextField); break;
                                default: UNREACHABLE; res = L2_NONE; break;
                            }
                        }
                    } else { // PASS
                        DERR << " " << myHand;
                        ana.restart(mode, 1);
                        nextPlayer = procL2Field<L2_FINFO, L2_MINFO>(field, &nextField, tmp);
                        ana.restart(mode, 6);
                        
                        if (conRest) { nextField.setDConst(); }
                        
                        int nextL2FINFO = genTA_L2FINFO(nextField.isNF() ? _YES : _NO, nextField.isUnrivaled() ? _YES : _NO);
                        
                        if (nextPlayer == 0) {
                            switch(nextL2FINFO) {
                                case genTA_L2FINFO(_NO, _NO):
                                    res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_NO, _NO)>(depth + 1, mv_buf, myHand, opsHand, nextField); break;
                                case genTA_L2FINFO(_YES, _NO):
                                    res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _NO)>(depth + 1, mv_buf, myHand, opsHand, nextField); break;
                                    //case genTA_L2FINFO(_NO, _YES):
                                    //res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_NO, _YES)>(depth + 1, mv_buf, myHand, opsHand, nextField); break;
                                    //case genTA_L2FINFO(_YES, _YES):
                                    //res = judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _YES)>(depth + 1, mv_buf, myHand, opsHand, nextField); break;
                                default: UNREACHABLE; res = L2_NONE; break;
                            }
                        } else {
                            switch(nextL2FINFO) {
                                case genTA_L2FINFO(_NO, _NO):
                                    res = L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_NO, _NO)>(depth + 1, mv_buf, opsHand, myHand, nextField);break;
                                case  genTA_L2FINFO(_YES, _NO):
                                    res = L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _NO)>(depth + 1, mv_buf, opsHand, myHand, nextField);break;
                                case genTA_L2FINFO(_NO, _YES):
                                    res = L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_NO, _YES)>(depth + 1, mv_buf, opsHand, myHand, nextField);break;
                                    //case  genTA_L2FINFO(_YES, _YES):
                                    // res = L2_WIN + L2_LOSE - judge<NEXT_S_LEVEL, L2JUDGE_LEVEL_MAX, genTA_L2FINFO(_YES, _YES)>(depth + 1, mv_buf, opsHand, myHand, nextField); break;
                                default: UNREACHABLE; res = L2_NONE; break;
                            }
                        }
                    }
                    
                    DERR << Space(depth * 2);
                    switch(res) {
                        case L2_WIN: DERR << "-WIN"; break;
                        case L2_DRAW: DERR << "-DRAW"; break;
                        case L2_LOSE: DERR << "-LOSE"; break;
                        default: break;
                    }
                    DERR << endl;
                    
                    return res;
                    break;
                }
            }
            
            return L2_DRAW;
        }
        
        template<int S_LEVEL, int E_LEVEL, int L2_FINFO>
        int L2Judge::search(const int depth, MoveInfo *const mv_buf, const int NMoves, const Hand& myHand, const Hand& opsHand, const L2Field& field) {
            
            // 反復深化をするのでなければ終了レベルは常に最大で呼ばれると思われる
            
            constexpr int IS_NF = (L2_FINFO >> 0) & 3;//空場
            constexpr int POS_PASS = ((IS_NF == _YES) ? _NO : _YES); // パスの可能性あり
            //const int IS_SFOL = (L2_FINFO >> 2) & 3;//セルフフォロー
            
            //int NActiveMoves = NMoves;
            
            // 勝利手があればインデックス(>=0)、結果不明なら-2、負け確定なら-1を返す
            int unFound = 0;
            int res;
            switch(S_LEVEL) {
                case 0: if (E_LEVEL <= 0)break; // fallthrough
                case 1:
                    // 1手詰み
                    //DERR << "LEVEL 1" << endl;
                    for (int m = NMoves - 1; m >= 0; --m) {
                        MoveInfo& tmp = mv_buf[m];
                        if (POS_PASS == _NO || !tmp.isPASS()) {
                            res = check<1, 1, L2_FINFO, 0>(depth, mv_buf + NMoves, tmp, myHand, opsHand, field);
                        } else {
                            res = check<1, 1, L2_FINFO, (_YES << 0)>(depth, mv_buf + NMoves, tmp, myHand, opsHand, field);
                        }
                        if (res == L2_WIN) { return m; }
                        if ((E_LEVEL <= 1) && res == L2_DRAW) { ++unFound; }
                    }
                    if (E_LEVEL <= 1)break; // fallthrough
                case 2:
                    //DERR << "LEVEL 2" << endl;
                    // 支配からの簡単詰み
                    for (int m = NMoves - 1; m >= 0; --m) {
                        MoveInfo& tmp = mv_buf[m];
                        
                        if (POS_PASS == _NO || !tmp.isPASS()) {
                            res=check<2, 2, L2_FINFO, 0>(depth, mv_buf + NMoves, tmp, myHand, opsHand, field);
                        } else {
                            res=check<2, 2, L2_FINFO, (_YES << 0)>(depth, mv_buf + NMoves, tmp, myHand, opsHand, field);
                        }
                        if (res == L2_WIN) { return m; }
                        if ((E_LEVEL <= 2) && res == L2_DRAW) { ++unFound; }
                    }
                    if (E_LEVEL <= 2)break; // fallthrough
                default:
                    //DERR << "LEVEL 3~" << endl;
                    // 完全な探索
                    
                    Cards dw;
                    if (IS_NF == _YES) {
                        dw = getAllDWCards(myHand.getCards(), opsHand.getCards(), field.tmpOrder(), field.fInfo.isTmpOrderSettled());
                    } else {
                        dw = CARDS_NULL; // 不要だがwarning回避
                    }
                    
                    for (int m = NMoves - 1; m >= 0; --m) {
                        MoveInfo& tmp = mv_buf[m];
                        if (!tmp.isL2GiveUp()) {
                            if (POS_PASS==_NO || !tmp.isPASS()) {
                                if ((IS_NF==_YES) && tmp.dominatesAll() && !(tmp.cards() & dw)) { continue; }
                                
                                res = check<3, L2JUDGE_LEVEL_MAX, L2_FINFO, 0>(depth, mv_buf + NMoves, tmp, myHand, opsHand, field);
                            } else {
                                res = check<3, L2JUDGE_LEVEL_MAX, L2_FINFO, (_YES << 0)>(depth, mv_buf + NMoves, tmp, myHand, opsHand, field);
                            }
                            if (res == L2_WIN) { return m; }
                            if (res == L2_DRAW) {
                                ++unFound;
                            } else {
                                //ASSERT(res == L2_LOSE, cerr << "res = " << res << endl;);
                                if (tmp.dominatesOthers()) {
                                    if (tmp.dominatesMe()) {
                                        if (tmp.containsJOKER() && !tmp.isSingleJOKER()) {
                                            // 支配共役系の枝刈り
                                            for (int mm = m - 1; mm >= 0; --mm) {
                                                if (tmp.cards() == mv_buf[mm].cards()) {
                                                    //DERR << tmp << "-" << mv[mm] << " cut!" << endl;
                                                    mv_buf[mm].setL2GiveUp();
                                                }
                                            }
                                        }
                                    }
                                    
                                    if ((IS_NF == _YES) && field.isTmpOrderSettled()) {
                                        // NFPDで負けた場合、排反なDW持ちのNFPDは全て枝刈り出来る
                                        for (int mm = m - 1; mm >= 0; --mm) {
                                            if (mv_buf[mm].dominatesOthers()) {
                                                Cards anoMvCards = mv_buf[mm].cards();
                                                if ((dw & anoMvCards) && (!(tmp.cards() & anoMvCards))) {
                                                    mv_buf[mm].setL2GiveUp();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
            }
            if (unFound) {
                res = -2;
            } else {
                res = -1;
            }
            return res;
        }
        
        int L2Judge::start_judge(const Hand& myHand, const Hand& opsHand, const Board bd, const FieldAddInfo fInfo) {
            assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
            int res;
            L2Field field;
            mode = 0;
            ana.start();
            init();
            convL2Field(bd, fInfo, &field); // L2型へのチェンジ
            ana.restart(mode, 0);
            if (field.isNF()) {
                res = judge<1, L2JUDGE_LEVEL_MAX, (_YES << 0)>(0, buf, myHand, opsHand, field);
            } else {
                if (fInfo.isSelfFollow()) {
                    res = judge<1, L2JUDGE_LEVEL_MAX, (_YES << 2)>(0, buf, myHand, opsHand, field);
                } else {
                    res = judge<1, L2JUDGE_LEVEL_MAX, 0>(0, buf, myHand, opsHand, field);
                }
            }
            ana.addNodes(nodes, mode);
            ana.addChilds(childs, mode);
            if (failed) { ana.addFailure(mode); }
            ana.end(mode, 1);
            return res;
        }
        
        int L2Judge::start_check(const MoveInfo mi, const Hand& myHand, const Hand& opsHand, const Board bd, const FieldAddInfo fInfo) {
            assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll() );
            int res;
            L2Field field;
            mode = 1;
            ana.start();
            init();
            convL2Field(bd, fInfo, &field); // L2型へのチェンジ
            MoveInfo tmp = mi;
            
            ana.restart(mode, 0);
            if (field.isNF()) {
                res = check<1, L2JUDGE_LEVEL_MAX, (_YES << 0), 0>(0, buf, tmp, myHand, opsHand, field);
            } else {
                if (mi.isPASS()) {
                    res = check<1, L2JUDGE_LEVEL_MAX, (_NO << 0), (_YES << 0)>(0, buf, tmp, myHand, opsHand, field);
                } else {
                    res = check<1, L2JUDGE_LEVEL_MAX, (_NO << 0), (_NO << 0)>(0, buf, tmp, myHand, opsHand, field);
                }
            }
            ana.addNodes(nodes, mode);
            ana.addChilds(childs, mode);
            if (failed) { ana.addFailure(mode); }
            ana.end(mode, 1);
            DERR << "L2Check ";
            switch(res) {
                case L2_WIN: DERR << "-WIN"; break;
                case L2_DRAW: DERR << "-DRAW"; break;
                case L2_LOSE: DERR << "-LOSE"; break;
                default: break;
            }
            DERR << " (" << nodes << " nodes)." << endl;
            return res;
        }
        
    }
}