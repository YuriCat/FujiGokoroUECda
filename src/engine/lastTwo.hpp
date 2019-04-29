#pragma once

// ラスト2人探索
#include "../core/daifugo.hpp"
#include "../core/hand.hpp"
#include "mate.hpp"

using std::string;

namespace UECda {
    namespace L2 {
        // L2関連
        TwoValueBook<(1 << 18)> book;
    }
    
    enum {
        L2_NONE = -1,
        L2_WIN = 0,
        L2_DRAW = 1,
        L2_LOSE = 2
    };
    
    // L2局面表現
    struct L2Field {
        Board b;
        FieldAddInfo info;
#ifdef DEBUG
        int p; // プレーヤー確認用
#endif
        bool isNull() const { return b.isNull(); }
        int order() const { return b.order(); }
        
        uint64_t isLastAwake() const { return info.isLastAwake(); }
        uint64_t isFlushLead() const { return info.isFlushLead(); }
        uint64_t isUnrivaled() const { return info.isUnrivaled(); }
        
        void setSelfFollow() { info.setSelfFollow(); }
        void setLastAwake() { info.setLastAwake(); }
        void setFlushLead() { info.setFlushLead(); }
        
        int turn() const {
#ifdef DEBUG
            return p;
#else
            return 0;
#endif
        }
        
        void flipTurnPlayer() {
#ifdef DEBUG
            p = 1 - p;
#endif
        }
        
        constexpr L2Field(): b(), info()
#ifdef DEBUG
        , p(0)
#endif
        {}
        
    };
    // L2局面表現へのチェンジ
    L2Field convL2Field(const Board& b, const FieldAddInfo& info) {
        L2Field f;
        f.info = info;
        f.b = b;
        return f;
    }
    L2Field procAndFlushL2Field(const L2Field& cur, const Move mv) {
        L2Field f;
        f.b = cur.b;
        f.info = cur.info;
        f.info.initTmpInfo();
        f.b.procAndFlush(mv);
        return f;
    }
    int procL2Field(const L2Field& cur, L2Field *const pnext, const MoveInfo mi) {
        pnext->b = cur.b;
        pnext->info = cur.info;
        pnext->info.initTmpInfo();
#ifdef DEBUG
        pnext->p = cur.p;
#endif
        const Move mv = mi;
        
        if (cur.isUnrivaled()) { // 独壇場
            if (mv.isPASS()) { // pass
                pnext->b.flush();
                DERR << " -FLUSHED" << endl;
            } else {
                if (mi.dominatesMe()) {
                    // 自己支配がかかるので、流れて自分から
                    pnext->b.procAndFlush(mv);
                    DERR << " -FLUSHED" << endl;
                } else {
                    // 流れなければSFが続く
                    pnext->b.proc(mv);
                    if (pnext->b.isNull()) { // renewed
                        DERR << " -FLUSHED" << endl;
                    } else {
                        pnext->setSelfFollow();
                        DERR << endl;
                    }
                }
            }
            return 0;
        } else { // 独壇場でない
            if (cur.isNull()) {
                if (mi.dominatesAll()) {
                    pnext->b.procAndFlush(mv);
                    DERR << " -FLUSHED" << endl; return 0;
                } else {
                    if (mi.dominatesOthers()) {
                        pnext->b.proc(mv);
                        if (pnext->b.isNull()) { // renewed
                            DERR << " -FLUSHED" << endl; return 0;
                        } else {
                            pnext->setSelfFollow();
                            DERR << " -DO" << endl; return 0;
                        }
                    } else {
                        pnext->b.proc(mv);
                        if (pnext->b.isNull()) { // renewed
                            DERR << " -FLUSHED" << endl; return 0;
                        } else {
                            pnext->flipTurnPlayer();
                            DERR << endl; return 1;
                        }
                    }
                }
            } else { // 通常場
                if (mv.isPASS()) { // pass
                    if (cur.isLastAwake()) {
                        pnext->b.flush();
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
                            pnext->b.procAndFlush(mv);
                            DERR << " -FLUSHED" << endl;
                        } else {
                            pnext->b.proc(mv);
                            if (pnext->b.isNull()) { // renewed
                                DERR << " -FLUSHED" << endl;
                            } else {
                                pnext->setSelfFollow();
                                DERR << endl;
                            }
                        }
                        return 0;
                    } else {
                        if (mi.dominatesAll()) {
                            pnext->b.procAndFlush(mv);
                            DERR << " -FLUSHED" << endl; return 0;
                        } else {
                            if (mi.dominatesOthers()) {
                                pnext->b.proc(mv);
                                if (pnext->b.isNull()) { // renewed
                                    DERR << " -FLUSHED" << endl; return 0;
                                } else {
                                    pnext->setSelfFollow();
                                    DERR << " -DO" << endl; return 0;
                                }
                            } else {
                                pnext->b.proc(mv);
                                if (pnext->b.isNull()) { // renewed
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
    
    class L2Judge {
        
        // L2判定人
        // 単純な構造でデータを受け取り、
        
        // judge...局面L2判定を返す
        // check...着手L2判定を返す
        // search...与えられた着手集合に勝利着手があればインデックス、なければ-1を返す
        
    private:
        const int NODE_LIMIT;
        
        MoveInfo *const buf;
        
        // 統計情報
        int nodes, childs, failed;
        
    public:
        void init() {
            nodes = childs = failed = 0;
        }
        
        L2Judge(int nl,MoveInfo *const argMI):
        NODE_LIMIT(nl), buf(argMI) { init(); }
        ~L2Judge() {}

        // 再帰版
        template <int S_LEVEL, int E_LEVEL>
        int judge(const int depth, MoveInfo *const buf, const Hand& myHand, const Hand& opsHand, const L2Field& field);
        int check(const int depth, MoveInfo *const buf, MoveInfo& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy = false);
        int checkDomMate(const int depth, MoveInfo *const buf, MoveInfo& tmp, const Hand& myHand, const Hand& opsHand, const L2Field& field);
        int search(const int depth, MoveInfo *const buf, const int NMoves, const Hand& myHand, const Hand& opsHand, const L2Field& field);
        
        int start_judge(const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fInfo);
        int start_check(const MoveInfo mi, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo fInfo);
    };
    
    template <int S_LEVEL, int E_LEVEL>
    int L2Judge::judge(const int depth, MoveInfo *const buf,
                       const Hand& myHand, const Hand& opsHand, const L2Field& field) {
        // 判定を返す
        int res;
        nodes++;
        
        uint64_t fkey = -1;

        switch (S_LEVEL) {
            case 0: res = L2_NONE; if (E_LEVEL <= 0) break; // fallthrough
            case 1:
                if (field.isNull() && myHand.qty == 1) return L2_WIN;
                if (E_LEVEL <= 1) break; // fallthrough
            case 2:
                if (field.isNull() && judgeMate_Easy_NF(myHand)) return L2_WIN;
                if (E_LEVEL <= 2) break; // fallthrough
            case 3:
                // 局面や相手の手札も考えた必勝判定
                ASSERT(myHand.exam1stHalf(), cerr << myHand.toDebugString(););
                ASSERT(opsHand.exam1stHalf(), cerr << opsHand.toDebugString(););
                if (field.isNull()) {
                    if (judgeHandPW_NF(myHand, opsHand, field.b)) return L2_WIN;
                }
                if (E_LEVEL <= 3) break; // fallthrough
            case 4:
                // 局面登録を検索
#ifdef USE_L2BOOK
                // NFのみ
                if (field.isNull()) {
                    ASSERT(myHand.exam_key(), cerr << myHand.toDebugString(););
                    ASSERT(opsHand.exam_key(), cerr << opsHand.toDebugString(););
                    
                    fkey = knitL2NullFieldHashKey(myHand.key, opsHand.key, NullBoardToHashKey(field.b));
                    res = L2::book.read(fkey);
                    if (res != -1) { // 結果が既に登録されていた
#ifdef DEBUG
                        DERR << string(2 * depth, ' ');
                        switch (res) {
                            case L2_WIN: DERR << "-HASHWIN"; break;
                            case L2_DRAW: DERR << "-HASHDRAW"; break;
                            case L2_LOSE: DERR << "-HASHLOSE"; break;
                            default: break;
                        }
                        DERR << endl;
#endif
                        return res;
                    }
                }
#endif
                if (E_LEVEL <= 4) break; // fallthrough
            case 5:
                // 簡易評価
                // 必敗判定
                if (field.isNull()) {
                    if (!myHand.seq && !(myHand.pqr & PQR_234) && !myHand.jk && !containsS3(myHand.cards) && field.order() == 0) {
                        int myHR = IntCardToRank(pickIntCardHigh(myHand.cards));
                        int opsLR = IntCardToRank(pickIntCardLow(opsHand.cards));
                        if (myHR < opsLR) {
                            //DERR << myHand <<" vs " << opsHand << endl; getchar();
                            DERR << string(2 * depth, ' ') << "-EMATELOSE" << endl;
                            return L2_LOSE;
                        } else {
                            Cards tmp = maskCards(opsHand.cards, RankToCards(opsLR));
                            if (tmp) {
                                opsLR = IntCardToRank(pickIntCardLow(tmp));
                                if (myHR < opsLR) {
                                    //DERR << myHand << " vs " << opsHand << endl; getchar();
                                    DERR << string(2 * depth, ' ') << "-EMATELOSE" << endl;
                                    return L2_LOSE;
                                }
                            }
                        }
                    }
                }
                if (E_LEVEL <= 5) break; // fallthrough
            default: // 完全探索
                if (nodes > NODE_LIMIT) { DERR << "Last2P Node over!" << endl; failed = 1; return L2_DRAW; }
                
                // 合法手の抽出
                const int NMoves = genMove(buf, myHand, field.b);
                DERR << "NMoves = " << NMoves << endl;
                
                childs += NMoves;
                
                if (!field.isNull() && field.b.qty() >= myHand.qty && NMoves >= 2) return L2_WIN;
                res = search(depth, buf, NMoves, myHand, opsHand, field);
                
                if (res >= 0) {
#ifdef USE_L2BOOK
                    if (field.isNull()) {
                        ASSERT(fkey == knitL2NullFieldHashKey(myHand.key, opsHand.key, NullBoardToHashKey(field.b)), cerr << fkey << endl;);
                        L2::book.regist(L2_WIN, fkey);
                    }
#endif
                    return L2_WIN;
                } else if (res == -1) {
#ifdef USE_L2BOOK
                    if (field.isNull()) {
                        ASSERT(fkey == knitL2NullFieldHashKey(myHand.key, opsHand.key, NullBoardToHashKey(field.b)), cerr << fkey << endl;);
                        L2::book.regist(L2_LOSE, fkey);
                    }
#endif
                    return L2_LOSE;
                }
                break;
        }
        
        return L2_DRAW;
    }

    int L2Judge::checkDomMate(const int depth, MoveInfo *const buf, MoveInfo& tmp,
                              const Hand& myHand, const Hand& opsHand, const L2Field& field) {
        if (field.isUnrivaled()
            || tmp.isDO()
            || dominatesCards(tmp, opsHand.cards, field.b)) { // 他支配チェック

            tmp.setDomOthers();
            if (myHand.qty - tmp.qty() <= 1) {
                DERR << string(2 * depth, ' ') << "<" << field.turn() << ">" << tmp << " -EMATEWIN" << endl;
                return L2_WIN;
            }

            int res;
            Hand nextHand;
            if (!tmp.isPASS()) {
                makeMove1stHalf(myHand, &nextHand, tmp);
                if (judgeMate_Easy_NF(nextHand)) {
                    DERR << string(2 * depth, ' ') << "<" << field.turn() << ">" << tmp << " -EMATEWIN" << endl;
                    return L2_WIN;
                }
                L2Field nextField = procAndFlushL2Field(field, tmp);
                nextHand.key = subCardKey(myHand.key, CardsToHashKey(tmp.cards()));
                res = judge<3, 4>(depth + 1, buf, nextHand, opsHand, nextField);
            } else {
                if (judgeMate_Easy_NF(myHand)) {
                    DERR << string(2 * depth, ' ') << "<" << field.turn() << ">" << tmp << " -EMATEWIN" << endl;
                    return L2_WIN;
                }
                L2Field nextField = procAndFlushL2Field(field, tmp);
                res = judge<3, 4>(depth + 1, buf, myHand, opsHand, nextField);
            }
            if (res == L2_WIN) {
                DERR << string(2 * depth, ' ') << "<" << field.turn() << ">" << tmp << " -EMATEWIN" << endl;
                return res;
            }
        }
        return L2_DRAW;
    }

    int L2Judge::check(const int depth, MoveInfo *const buf, MoveInfo& tmp,
                       const Hand& myHand, const Hand& opsHand, const L2Field& field, bool checkedEasy) {
        
        bool pass = tmp.isPASS();
        int res;
        DERR << string(2 * depth, ' ') << "<" << field.turn() << ">" << tmp;

        if (!checkedEasy) {
            if (tmp.qty() >= myHand.qty) {
                DERR << string(2 * depth, ' ') << tmp << " -FIN" << endl;
                return L2_WIN;
            }
            if (checkDomMate(depth, buf, tmp, myHand, opsHand, field) == L2_WIN) return L2_WIN;
        }
        childs++;
        
        // 支配性判定
        if (!pass && (field.isLastAwake() || tmp.dominatesOthers())) {
            if (dominatesCards(tmp, myHand.cards, field.b)) {
                tmp.setDomMe();
            }
        }
        Hand nextHand;
        L2Field nextField;
        int nextPlayer = procL2Field(field, &nextField, tmp);
        if (!pass) {
            makeMoveAll(myHand, &nextHand, tmp);
            DERR << " " << myHand << " -> " << nextHand;
            
            if (nextPlayer == 0) {
                res = judge<1, 1024>(depth + 1, buf, nextHand, opsHand, nextField);
            } else {
                res = L2_WIN + L2_LOSE - judge<1, 1024>(depth + 1, buf, opsHand, nextHand, nextField);
            }
        } else { // PASS
            DERR << " " << myHand;
            nextPlayer = procL2Field(field, &nextField, tmp);
            if (nextPlayer == 0) {
                res = judge<1, 1024>(depth + 1, buf, myHand, opsHand, nextField);
            } else {
                res = L2_WIN + L2_LOSE - judge<1, 1024>(depth + 1, buf, opsHand, myHand, nextField);
            }
        }
        
        DERR << string(2 * depth, ' ');
        switch (res) {
            case L2_WIN: DERR << "-WIN"; break;
            case L2_DRAW: DERR << "-DRAW"; break;
            case L2_LOSE: DERR << "-LOSE"; break;
            default: break;
        }
        DERR << endl;
        return res;
    }

    int L2Judge::search(const int depth, MoveInfo *const buf, const int NMoves,
                        const Hand& myHand, const Hand& opsHand, const L2Field& field) {
        
        // 反復深化をするのでなければ終了レベルは常に最大で呼ばれると思われる
        
        // 勝利手があればインデックス(>=0)、結果不明なら-2、負け確定なら-1を返す
        // 1手詰み
        //for (int m = NMoves - 1; m >= 0; m--) {
        //    if (buf[m].qty() >= myHand.qty) {
        //        DERR << string(2 * depth, ' ') << tmp << " -FIN" << endl; return m;
        //    }
        //}
        // 支配からの簡単詰み
        for (int m = NMoves - 1; m >= 0; m--) {
            int res = checkDomMate(depth, buf + NMoves, buf[m], myHand, opsHand, field);
            if (res == L2_WIN) return m;
        }
        // 探索
        bool unFound = false;
        for (int m = NMoves - 1; m >= 0; m--) {
            MoveInfo& tmp = buf[m];
            //if (tmp.isL2GiveUp()) continue;
            int res = check(depth, buf + NMoves, tmp, myHand, opsHand, field, true);
            if (res == L2_WIN) return m;
            if (res == L2_DRAW) unFound = true;
        }
        if (unFound) return -2;
        else return -1;
    }
    
    int L2Judge::start_judge(const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo info) {
        assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
        init();
        L2Field field = convL2Field(b, info); // L2型へのチェンジ
        int res = judge<1, 1024>(0, buf, myHand, opsHand, field);
        return res;
    }
    
    int L2Judge::start_check(const MoveInfo mi, const Hand& myHand, const Hand& opsHand, const Board b, const FieldAddInfo info) {
        assert(myHand.any() && myHand.examAll() && opsHand.any() && opsHand.examAll());
        init();
        L2Field field = convL2Field(b, info); // L2型へのチェンジ
        MoveInfo tmp = mi;
        int res = check(0, buf, tmp, myHand, opsHand, field);
        DERR << "L2Check ";
        switch (res) {
            case L2_WIN: DERR << "-WIN"; break;
            case L2_DRAW: DERR << "-DRAW"; break;
            case L2_LOSE: DERR << "-LOSE"; break;
            default: break;
        }
        DERR << " (" << nodes << " nodes)." << endl;
        return res;
    }
}