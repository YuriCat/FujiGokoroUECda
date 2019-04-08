#pragma once

// プリミティブな型 少し複雑なもの

#include "daifugo.hpp"
#include "card.hpp"
#include "move.hpp"

namespace UECda {
    
    /**************************着手決定のための基本追加場情報**************************/
    
    // 試合結果の宣言情報
    
    constexpr int LCT_FINAL    = 0;
    constexpr int LCT_PW       = 1;
    constexpr int LCT_BNPW     = 2;
    constexpr int LCT_BRPW     = 3;
    constexpr int LCT_MPMATE   = 4;
    constexpr int LCT_L2MATE   = 5;
    constexpr int LCT_MPGIVEUP = 6;
    constexpr int LCT_L2GIVEUP = 7;

    constexpr int LCT_MINNMELDS = 8;

    // 場の一時状況に対するする宣言情報
    constexpr int LCT_SELFFOLLOW = 16;
    constexpr int LCT_UNRIVALED = 17;
    constexpr int LCT_LASTAWAKE = 18;
    constexpr int LCT_FLUSHLEAD = 19;
    constexpr int LCT_NPDOM = 20;
    constexpr int LCT_PDOM = 21;
    constexpr int LCT_BDOMOTHERS = 22;
    constexpr int LCT_BDOMME = 23; // <-NoChanceのこと

    constexpr int LCT_TMPORDSETTLED = 24;
    constexpr int LCT_PRMORDSETTLED = 25;

    constexpr int LCT_DCONST = 28;

    constexpr uint32_t FLAG_MATE   = 0x3fU;
    constexpr uint32_t FLAG_GIVEUP = 0xc0U;

    constexpr int LCT64_FINAL    = 0 + 32;
    constexpr int LCT64_PW       = 1 + 32;
    constexpr int LCT64_BNPW     = 2 + 32;
    constexpr int LCT64_BRPW     = 3 + 32;
    constexpr int LCT64_MPMATE   = 4 + 32;
    constexpr int LCT64_L2MATE   = 5 + 32;
    constexpr int LCT64_MPGIVEUP = 6 + 32;
    constexpr int LCT64_L2GIVEUP = 7 + 32;

    constexpr int LCT64_MINNMELDS = 8 + 32;

    constexpr int LCT64_SELFFOLLOW = 16 + 32;
    constexpr int LCT64_UNRIVALED = 17 + 32;
    constexpr int LCT64_LASTAWAKE = 18 + 32;
    constexpr int LCT64_FLUSHLEAD = 19 + 32;
    constexpr int LCT64_NPDOM = 20 + 32;
    constexpr int LCT64_PDOM = 21 + 32;
    constexpr int LCT64_BDOMOTHERS = 22 + 32;
    constexpr int LCT64_BDOMME = 23 + 32; // <-NoChanceのこと
    
    constexpr int LCT64_TMPORDSETTLED = 24 + 32;
    constexpr int LCT64_PRMORDSETTLED = 25 + 32;

    constexpr int LCT64_DCONST = 28 + 32;

    constexpr int LCT_MINNCARDS = 0;
    constexpr int LCT_MAXNCARDS = 4;
    constexpr int LCT_MINNCARDSAWAKE = 8;
    constexpr int LCT_MAXNCARDSAWAKE = 12;

    constexpr uint64_t FLAG64_MATE   = 0x3fULL << 32;
    constexpr uint64_t FLAG64_GIVEUP = 0xc0ULL << 32;

    constexpr uint64_t MASK64_TMPINFO = (1ULL << LCT64_TMPORDSETTLED) | (1ULL << LCT64_PRMORDSETTLED);
    
    class FieldAddInfo {
        
        // 着手決定のためにこの程度は調べておきたい場情報
        // 着手ごとの情報と被る場合もあるけれども、検索が面倒な場合はこちらに記録しておく
        
        // 8-11 MinNMelds
        
        //
        
        // 16 セルフフォロー
        // 17 独壇場(ALLDOM)
        // 18 LastAwake
        // 19 流れ場主
        // 20 NPDOM
        // 21 PDOM
        // 22 BDOMOTHERS
        // 23 BDOMME
        
        // 24 orderConsolidated
        // 25 prmOrderConsolidated
        // 28 支配保証/空場期待拘束中(ここに書くべきなのだろうか?)
        
        // info1
        // 0-3 MinNCards
        // 4-7 MaxNCards
        // 8-11 MinNCardsAwake
        // 12-15 MaxNCardsAwake
        
    public:
        using data_t = uint64_t;
        
        // set
        void setFinal() {    i_.set(LCT64_FINAL, LCT64_PW, LCT64_MPMATE); }
        void setPW() {       i_.set(LCT64_PW, LCT64_MPMATE); }
        void setBNPW() {     i_.set(LCT64_BNPW, LCT64_MPMATE); }
        void setBRPW() {     i_.set(LCT64_BRPW, LCT64_MPMATE); }
        void setMPMate() {   i_.set(LCT64_MPMATE); }
        void setL2Mate() {   i_.set(LCT64_L2MATE); }
        void setMPGiveUp() { i_.set(LCT64_MPGIVEUP); }
        void setL2GiveUp() { i_.set(LCT64_L2GIVEUP); }
        
        void setSelfFollow() { i_.set(LCT64_SELFFOLLOW,
                                            LCT64_UNRIVALED,
                                            LCT64_LASTAWAKE,
                                            LCT64_FLUSHLEAD,
                                            LCT64_NPDOM,
                                            LCT64_PDOM); }
        void setUnrivaled() { i_.set(LCT64_UNRIVALED,
                                           LCT64_FLUSHLEAD,
                                           LCT64_NPDOM,
                                           LCT64_PDOM); }
        void setLastAwake() { i_.set(LCT64_LASTAWAKE,
                                           LCT64_NPDOM,
                                           LCT64_BDOMOTHERS); }
        void setFlushLead() { i_.set(LCT64_FLUSHLEAD); }
        void setNPDom() { i_.set(LCT64_NPDOM); }
        void setPassDom() { i_.set(LCT64_PDOM); }
        void setBDO() { i_.set(LCT64_BDOMOTHERS); }
        void setBDM() { i_.set(LCT64_BDOMME); }
        void setBDALL() { i_.set(LCT64_BDOMOTHERS,
                                       LCT64_BDOMME); }
        void setNoChance() { setBDM(); }
        
        void settlePrmOrder() { i_.set(LCT64_TMPORDSETTLED); }
        void settleTmpOrder() { i_.set(LCT64_TMPORDSETTLED,
                                             LCT64_PRMORDSETTLED); }
        
        void setDConst() { i_.set(LCT64_DCONST); }
        
        void setMinNMelds(uint32_t n) { i_ |= uint64_t(n & 15U) << LCT64_MINNMELDS; }
        
        void setMinNCards(uint32_t n) { i_ |= n & 15U; }
        void setMaxNCards(uint32_t n) { i_ = (i_ & 0xFFFFFFFFFFFFFF0F) | ((n & 15U) << 4); }
        void setMinNCardsAwake(uint32_t n) { i_ |= (n & 15U) << 8; }
        void setMaxNCardsAwake(uint32_t n) { i_ = (i_ & 0xFFFFFFFFFFFF0FFF) | ((n & 15U) << 12); }
        
        uint64_t test(int n) const { return i_.test(n); }
        
        // get
        // 一時情報
        uint64_t isFinal() const {    return test(LCT64_FINAL); }
        uint64_t isPW() const {       return test(LCT64_PW); }
        uint64_t isBNPW() const {     return test(LCT64_BNPW); }
        uint64_t isBRPW() const {     return test(LCT64_BRPW); }
        uint64_t isMPMate() const {   return test(LCT64_MPMATE); }
        uint64_t isMPGiveUp() const { return test(LCT64_MPGIVEUP); }
        uint64_t isL2Mate() const {   return test(LCT64_L2MATE); }
        uint64_t isL2GiveUp() const { return test(LCT64_L2GIVEUP); }
        uint64_t isMate() const {     return i_ & FLAG64_MATE; }
        uint64_t isGiveUp() const {   return i_ & FLAG64_GIVEUP; }
        
        uint64_t isSelfFollow() const { return test(LCT64_SELFFOLLOW); }
        uint64_t isUnrivaled() const { return test(LCT64_UNRIVALED); }
        uint64_t isLastAwake() const { return test(LCT64_LASTAWAKE); }
        uint64_t isFlushLead() const { return test(LCT64_FLUSHLEAD); }
        uint64_t isNonPassDom() const { return test(LCT64_NPDOM); }
        uint64_t isPassDom() const { return test(LCT64_PDOM); }
        uint64_t isBDO() const { return test(LCT64_BDOMOTHERS); }
        uint64_t isBDM() const { return test(LCT64_BDOMME); }
        bool isBDALL() const { return i_.holds(LCT64_BDOMOTHERS,
                                                     LCT64_BDOMME); }
        uint64_t isNoChance() const { return isBDM(); }
        
        // 永続情報
        uint64_t isOrderSettled() const { return test(LCT64_TMPORDSETTLED); }
        uint64_t isPrmOrderSettled() const { return test(LCT64_PRMORDSETTLED); }
        
        uint64_t isDConst() const { return test(LCT64_DCONST); }
        
        uint32_t getMinNMelds() const { return uint32_t(i_ >> LCT64_MINNMELDS) & 15U; }
        
        uint32_t getMinNCards() const { return i_ & 15U; }
        uint32_t getMaxNCards() const { return (i_ >> 4) & 15U; }
        uint32_t getMinNCardsAwake() const { return (i_ >> 8) & 15U; }
        uint32_t getMaxNCardsAwake() const { return (i_ >> 12) & 15U; }
        
        void initHalf() { i_.reset(); }
        
        void init() {
            // カード枚数については、無設定の場合はmaxが15、minの場合は０になるようにする
            i_ = 0x0000F0F0ULL;
        }
        
        void initTmpInfo() {
            // 一時情報は初期化し、永続情報は残す
            i_ = (i_ & MASK64_TMPINFO) | 0x0000F0F0ULL;
        }
        void procTmpInfo() {
            i_ &= i_ & (MASK64_TMPINFO | 0xFFFFULL);
        }
        
        constexpr data_t data() const { return i_.data(); }
        
        constexpr FieldAddInfo():i_() {}
        constexpr FieldAddInfo(const FieldAddInfo& arg):
        i_(arg.data()) {} // コピー
        
    protected:
        BitSetInRegister<data_t> i_;
    };
    
    static ostream& operator <<(ostream& out, const FieldAddInfo& i) { // 出力
        out << "Field :";
        if (i.isFinal()) out << " -FIN";
        else if (i.isPW()) out << " -PW";
        else if (i.isBNPW()) out << " -BNPW";
        else if (i.isBRPW()) out << " -BRPW";
        else if (i.isMPMate()) out << " -MPMATE";
        
        if (i.isL2Mate()) out << " -L2MATE";
        
        if (i.isMPGiveUp()) out << " -MPGIVEUP";
        if (i.isL2GiveUp()) out << " -L2GIVEUP";
        
        if (i.isSelfFollow()) out << " -SFOL";
        if (i.isUnrivaled()) out << " -UNRIV";
        if (i.isLastAwake()) out << " -LA";
        if (i.isFlushLead()) out << " -FLEAD";
        if (i.isNonPassDom()) out << " -NPD";
        if (i.isPassDom()) out << " -PD";
        
        if (i.isBDALL()) out << " -BDALL";
        else {
            if (i.isBDO()) out << " -BDO";
            if (i.isBDM()) out << " -BDM";
        }
        return out;
    }
    
    inline void flushFieldAddInfo(const FieldAddInfo& fieldInfo,
                           FieldAddInfo *const pnext) {
        pnext->initTmpInfo();
        pnext->setMinNCardsAwake(fieldInfo.getMinNCards());
        pnext->setMaxNCardsAwake(fieldInfo.getMaxNCards());
        pnext->setMinNCards(fieldInfo.getMinNCards());
        pnext->setMaxNCards(fieldInfo.getMaxNCards());
        pnext->setFlushLead();
    }
    inline void procUnrivaled(const FieldAddInfo& fieldInfo,
                       FieldAddInfo *const pnext) {
        *pnext = fieldInfo;
        pnext->procTmpInfo();
        pnext->setUnrivaled();
    }
    
    /**************************着手情報＋追加情報**************************/
    
    // 基本的にはこれを使う
    
    struct MoveInfo : public Move {

        constexpr MoveInfo(): Move() {}
        constexpr MoveInfo(const Move& m): Move(m) {}
        constexpr MoveInfo(const MoveInfo& m): Move(m) {}
        constexpr Move mv() const { return Move(*this); }

        void set(size_t i) { Move::flags |= 1U << i; }
        void set(size_t i0, size_t i1) { set(i0); set(i1); }
        void set(size_t i0, size_t i1, size_t i2) { set(i0); set(i1); set(i2); }
        bool holds(size_t i0, size_t i1) const {
            uint32_t dst = (1U << i0) | (1U << i1);
            return !(~Move::flags & dst);
        }
        bool test(size_t i) const { return Move::flags & (1ULL << i); }

        
        void setFinal() {    set(LCT_FINAL, LCT_PW, LCT_MPMATE); }
        void setPW() {       set(LCT_PW, LCT_MPMATE); }
        void setBNPW() {     set(LCT_BNPW, LCT_MPMATE); }
        void setBRPW() {     set(LCT_BRPW, LCT_MPMATE); }
        void setMPMate() {   set(LCT_MPMATE); }
        void setL2Mate() {   set(LCT_L2MATE); }
        void setMPGiveUp() { set(LCT_MPGIVEUP); }
        void setL2GiveUp() { set(LCT_L2GIVEUP); }

        // 当座支配
        void setDO() { set(12); }
        void setDM() { set(13); }
        void setDALL() { set(12, 13); }
        void setDomOthers() { setDO(); }
        void setDomMe() { setDO(); }
        void setDomAll() { setDO(); }

        // min_melds
        void setIncMinNMelds(uint32_t dec) { Move::flags |= ((uint64_t)(dec))<<(8); }

        // kousoku
        void setDConst() { set(28); }
        
        void init() { Move::flags = 0; }
        
        // get
        uint64_t isFinal() const {    return test(LCT_FINAL); }
        uint64_t isPW() const {       return test(LCT_PW); }
        uint64_t isBNPW() const {     return test(LCT_BNPW); }
        uint64_t isBRPW() const {     return test(LCT_BRPW); }
        uint64_t isMPMate() const {   return test(LCT_MPMATE); }
        uint64_t isMPGiveUp() const { return test(LCT_MPGIVEUP); }
        uint64_t isL2Mate() const {   return test(LCT_L2MATE); }
        uint64_t isL2GiveUp() const { return test(LCT_L2GIVEUP); }
        uint64_t isMate() const {     return Move::flags & FLAG_MATE; }
        uint64_t isGiveUp() const {   return Move::flags & FLAG_GIVEUP; }
        
        uint64_t isDO() const { return test(12); }
        uint64_t isDM() const { return test(13); }
        bool isDALL() const { return holds(12, 13); }
        
        uint64_t dominatesOthers() const { return isDO(); }
        uint64_t dominatesMe() const { return isDM(); }
        uint64_t dominatesAll() const { return isDALL(); }
        
        uint32_t getIncMinNMelds() const { return (Move::flags >> 8) & 15; }
        
        uint64_t isDConst() const { return test((28)); }
    };

    static std::string toInfoString(const MoveInfo& i, const Board b) { // 出力
        std::ostringstream oss;
        // 勝敗
        if (i.isFinal()) oss << " -FIN";
        else if (i.isPW()) oss << " -PW";
        else if (i.isBNPW()) oss << " -BNPW";
        else if (i.isBRPW()) oss << " -BRPW";
        else if (i.isMPMate()) oss << " -MPMATE";
        
        if (i.isL2Mate()) oss << " -L2MATE";
        
        if (i.isMPGiveUp()) oss << " -MPGIVEUP";
        if (i.isL2GiveUp()) oss << " -L2GIVEUP";
        
        // 後場
        if (b.afterTmpOrder(i) != 0) oss << " -TREV";
        if (b.locksSuits(i)) oss << " -SLOCK";
        
        // パラメータ
        oss << " -MNM(" << i.getIncMinNMelds() << ")";
        
        // 当座支配
        if (i.dominatesAll()) oss<< " -DALL";
        else {
            if (i.dominatesOthers()) oss << " -DO";
            if (i.dominatesMe()) oss << " -DM";
        }
        return oss.str();
    }
    
    /**************************プレーヤーの状態**************************/
    
    struct PlayersState {
        // Boardと合わせて、場の状態を表す
        // 0-7   Alive
        // 8-15   NAlive
        // 16-23  Awake
        // 24-31 Nawake
        
        constexpr static int N = N_PLAYERS;
        
        using data_t = uint32_t;
        
        BitArray32<8, 4> i;
        
        constexpr static uint32_t BMASK = 1U; // 基準ビット
        constexpr static uint32_t PMASK = (1 << 8) - 1; // プレーヤー全体
        constexpr static uint32_t REALPMASK = (1 << N) - 1; // 実際にいるプレーヤー全体
        constexpr static uint32_t NMASK = (1 << 8) - 1; // 数全体
        
        constexpr operator uint32_t() const { return (uint32_t)i; }
        
        // set
        void setAsleep(const int p) {
            ASSERT(isAwake(p), cerr << "p = " << p << "," << std::hex << (uint32_t)i << endl;); // 現在Awake
            i -= (BMASK << 24) + ((BMASK << 16) << p);
        }
        void setDead(const int p) {
            // プレーヤーがあがった
            ASSERT(isAwake(p), cerr << "p = " << p <<endl;);
            ASSERT(isAlive(p), cerr << "p = " << p << endl;); // 現在AliveかつAwakeの必要
            i -= (BMASK << 8) + (BMASK << 24) + (((1U << 0) + (1U << 16)) << p);
        }
        void setDeadOnly(const int p) {
            // プレーヤーがあがった
            ASSERT(isAlive(p), cerr << "p = " << p << endl;); // 現在Aliveの必要
            i -= (BMASK << 8) + ((1U << 0) << p);
        }
        void setAwake(const int p) {
            assert(!isAwake(p));
            i += (BMASK << 24) + ((BMASK << 16) << p);
        }
        void setAlive(const int p) {
            // プレーヤー復活
            assert(!isAwake(p)); assert(!isAlive(p));
            i += (BMASK << 8) + (BMASK << 24) + (((1U << 0) + (1U << 16)) << p);
        }
        
        void setAllAsleep() {
            i &= (PMASK << 0) | (NMASK << 8);
        }
        
        void setNAlive(const int n) {
            i.replace(1, n);
        }
        void setNAwake(const int n) {
            i.replace(3, n);
        }
        // get
        
        constexpr data_t isAlive(int p) const { return i & ((BMASK << 0) << p); }
        constexpr data_t isAwake(int p) const { return i & ((BMASK << 16) << p); }
        constexpr bool isExcluded(int p) const { return false; } // あがり以外の除外(都落ち)
        
        constexpr bool isAllAsleepExcept(int p) const { // p以外全員asleep
            return !(i & ((PMASK << 16) ^ ((BMASK << 16) << p)));
        }
        
        uint32_t searchOpsPlayer(int p) const {
            // p以外でaliveなプレーヤーを1人挙げる
            // pがaliveであることは保証される
            assert(isAlive(p));
            assert(getNAlive() >= 2);
            return bsf32(i ^ (BMASK << p));
        }
        
        constexpr data_t getNAlive() const { return i[1]; }
        constexpr data_t getNAwake() const { return i[3]; }
        
        uint32_t countNAlive() const { return countBits(i.get_part(0)); }
        uint32_t countNAwake() const { return countBits(i.get_part(2)); }
        
        constexpr data_t anyAlive() const { return i & (PMASK <<  0); }
        constexpr data_t anyAwake() const { return i & (PMASK << 16); }
        
        bool isSoloAlive() const { return i.get_part(1)  == (1U <<  8); }
        bool isSoloAwake() const { return i.get_part(3)  == (1U << 24); }
        
        constexpr data_t getBestClass() const { return N - getNAlive(); } // 最高の階級 = 全員 - 残っている人数
        constexpr data_t getWorstClass() const { return N - 1; } // 最低の階級 = 全員の最後
        
        uint32_t searchL1Player() const {
            // 最後に残ったプレーヤーを探す
            assert(countBits32(i & PMASK) == 1); // 1人だけ残っている
            return bsf32(i);
        }
        
        void flush() {
            // 場が流れる
            data_t alive = i & ((PMASK << 0) | (NMASK << 8)); // alive情報
            i = alive | (alive << 16); // awake情報をalive情報に置き換える
        }
        void init() {
            i = (REALPMASK << 0) | (N << 8) | (REALPMASK << 16) | (N << 24);
        }
        
        bool exam_alive() const {
            if (getNAlive() <= 0 || N < getNAlive()) {
                cerr << "PlayersState : illegal NAlive " << getNAlive() << endl;
                return false;
            }
            if (getNAlive() != countNAlive()) {
                cerr << "PlayersState : NAlive != count()" << endl;
                return false;
            }
            return true;
        }
        bool exam_awake() const {
            if (getNAwake() <= 0 || N < getNAwake()) {
                cerr << "PlayersState : illegal NAwake " << getNAwake() << endl;
                return false;
            }
            if (getNAwake() != countNAwake()) {
                cerr << "PlayersState : NAwake != count()" << endl;
                return false;
            }
            return true;
        }
        
        //validator
        bool exam() const {
            //各要素
            if (!exam_alive()) return false;
            if (!exam_awake()) return false;
            
            //awakeとaliveの関係
            if (getNAlive() < getNAwake()) {
                cerr << "PlayersState : NAlive < NAwake" << endl;
                return false;
            }
            if (!holdsBits(i[0], i[2])) {
                cerr << "PlayersState : !holds( alive, awake )" << endl;
                return false;
            }
            return true;
        }
        bool examNF() const {
            // awake情報とalive情報が同じはず
            if (((uint32_t)i) >> 16 != ((uint32_t)i & ((1U << 16) - 1))) { return false; }
            return true;
        }
        bool examSemiNF() const {
            return exam();
        }
        
        constexpr PlayersState() : i() {}
        constexpr PlayersState(const PlayersState& arg) : i(arg.i) {}
    };
    
    static std::ostream& operator <<(std::ostream& out, const PlayersState& arg) { // 出力
        // 勝敗
        out << "al{";
        for (int i = 0; i < PlayersState::N; ++i) {
            if (arg.isAlive(i)) {
                out << i;
            }
        }
        out << "}";
        out << " aw{";
        for (int i = 0; i < PlayersState::N; ++i) {
            if (arg.isAwake(i)) {
                out << i;
            }
        }	
        out << "}";
        return out;
    }
}
