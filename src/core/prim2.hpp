#pragma once

// プリミティブな型 少し複雑なもの

#include "daifugo.hpp"

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

// 場の一時状況に対するする宣言情報
constexpr int LCT_SELFFOLLOW = 16;
constexpr int LCT_UNRIVALED = 17;
constexpr int LCT_LASTAWAKE = 18;
constexpr int LCT_FLUSHLEAD = 19;
constexpr int LCT_NPDOM = 20;
constexpr int LCT_PDOM = 21;
constexpr int LCT_BDOMOTHERS = 22;
constexpr int LCT_BDOMME = 23; // <-NoChanceのこと

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

constexpr int LCT64_SELFFOLLOW = 16 + 32;
constexpr int LCT64_UNRIVALED = 17 + 32;
constexpr int LCT64_LASTAWAKE = 18 + 32;
constexpr int LCT64_FLUSHLEAD = 19 + 32;
constexpr int LCT64_NPDOM = 20 + 32;
constexpr int LCT64_PDOM = 21 + 32;
constexpr int LCT64_BDOMOTHERS = 22 + 32;
constexpr int LCT64_BDOMME = 23 + 32; // <-NoChanceのこと

constexpr int LCT_MINNCARDS = 0;
constexpr int LCT_MAXNCARDS = 4;
constexpr int LCT_MINNCARDSAWAKE = 8;
constexpr int LCT_MAXNCARDSAWAKE = 12;

constexpr uint64_t FLAG64_MATE   = 0x3fULL << 32;
constexpr uint64_t FLAG64_GIVEUP = 0xc0ULL << 32;

class FieldAddInfo {

    // 着手決定のためにこの程度は調べておきたい場情報
    // 着手ごとの情報と被る場合もあるけれども、検索が面倒な場合はこちらに記録しておく

    // 16 セルフフォロー
    // 17 独壇場(ALLDOM)
    // 18 LastAwake
    // 19 流れ場主
    // 20 NPDOM
    // 21 PDOM
    // 22 BDOMOTHERS
    // 23 BDOMME

    // info1
    // 0-3 MinNCards
    // 4-7 MaxNCards
    // 8-11 MinNCardsAwake
    // 12-15 MaxNCardsAwake

public:
    using data_t = uint64_t;

    // set
    void setFinal() {    set(LCT64_FINAL, LCT64_PW, LCT64_MPMATE); }
    void setPW() {       set(LCT64_PW, LCT64_MPMATE); }
    void setBNPW() {     set(LCT64_BNPW, LCT64_MPMATE); }
    void setBRPW() {     set(LCT64_BRPW, LCT64_MPMATE); }
    void setMPMate() {   set(LCT64_MPMATE); }
    void setL2Mate() {   set(LCT64_L2MATE); }
    void setMPGiveUp() { set(LCT64_MPGIVEUP); }
    void setL2GiveUp() { set(LCT64_L2GIVEUP); }

    void setSelfFollow() { set(LCT64_SELFFOLLOW,
                                        LCT64_UNRIVALED,
                                        LCT64_LASTAWAKE,
                                        LCT64_FLUSHLEAD,
                                        LCT64_NPDOM,
                                        LCT64_PDOM); }
    void setUnrivaled() { set(LCT64_UNRIVALED,
                                       LCT64_FLUSHLEAD,
                                       LCT64_NPDOM,
                                       LCT64_PDOM); }
    void setLastAwake() { set(LCT64_LASTAWAKE,
                                       LCT64_NPDOM,
                                       LCT64_BDOMOTHERS); }
    void setFlushLead() { set(LCT64_FLUSHLEAD); }
    void setNPDom() { set(LCT64_NPDOM); }
    void setPassDom() { set(LCT64_PDOM); }
    void setBDO() { set(LCT64_BDOMOTHERS); }
    void setBDM() { set(LCT64_BDOMME); }
    void setBDALL() { set(LCT64_BDOMOTHERS,
                                   LCT64_BDOMME); }
    void setNoChance() { setBDM(); }
    void setMinNCards(uint32_t n) { i_ |= n & 15U; }
    void setMaxNCards(uint32_t n) { i_ = (i_ & 0xFFFFFFFFFFFFFF0F) | ((n & 15U) << 4); }
    void setMinNCardsAwake(uint32_t n) { i_ |= (n & 15U) << 8; }
    void setMaxNCardsAwake(uint32_t n) { i_ = (i_ & 0xFFFFFFFFFFFF0FFF) | ((n & 15U) << 12); }

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
    bool isBDALL() const { return holds(LCT64_BDOMOTHERS,
                                                 LCT64_BDOMME); }
    uint64_t isNoChance() const { return isBDM(); }

    uint32_t getMinNCards() const { return i_ & 15U; }
    uint32_t getMaxNCards() const { return (i_ >> 4) & 15U; }
    uint32_t getMinNCardsAwake() const { return (i_ >> 8) & 15U; }
    uint32_t getMaxNCardsAwake() const { return (i_ >> 12) & 15U; }

    void initHalf() { i_ = 0; }

    void init() {
        // カード枚数については、無設定の場合はmaxが15、minの場合は０になるようにする
        i_ = 0x0000F0F0ULL;
    }
    void procTmpInfo() {
        i_ &= 0xFFFFULL;
    }

    constexpr data_t data() const { return i_; }
    constexpr FieldAddInfo(): i_() {}
    constexpr FieldAddInfo(const FieldAddInfo& arg): i_(arg.data()) {}

protected:
    data_t i_;

    void set(size_t i) { i_ |= 1ULL << i; }
    template <class... args_t>
    void set(size_t i0, args_t... args) { set(i0); set(args...); }
    bool holds(size_t i0, size_t i1) const {
        uint64_t dst = (1ULL << i0) | (1ULL << i1);
        return !(~i_ & dst);
    }
    uint64_t test(size_t i) const { return i_ & (1ULL << i); }
};

static std::ostream& operator <<(std::ostream& out, const FieldAddInfo& i) { // 出力
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
    pnext->init();
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

    void set(size_t i) { Move::flags |= 1U << i; }
    template <class... args_t>
    void set(size_t i0, args_t... args) { set(i0); set(args...); }
    bool holds(size_t i0, size_t i1) const {
        uint32_t dst = (1U << i0) | (1U << i1);
        return !(~Move::flags & dst);
    }
    uint32_t test(size_t i) const { return Move::flags & (1U << i); }

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
    void setDomMe() { setDM(); }
    void setDomAll() { setDALL(); }
    void setChecked() { set(31); }

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

    bool isChecked() const { return test(31); }
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
    if (b.nextOrder(i) != 0) oss << " -TREV";
    if (b.locksSuits(i)) oss << " -SLOCK";

    // 当座支配
    if (i.dominatesAll()) oss<< " -DALL";
    else {
        if (i.dominatesOthers()) oss << " -DO";
        if (i.dominatesMe()) oss << " -DM";
    }
    return oss.str();
}