#pragma once

// プリミティブな型 少し複雑なもの

#include "daifugo.hpp"

/**************************着手決定のための基本追加場情報**************************/

// 試合結果の宣言情報

constexpr int LCT_FINAL    = 0;
constexpr int LCT_PW       = 1;
constexpr int LCT_MPMATE   = 2;
constexpr int LCT_L2MATE   = 3;
constexpr int LCT_MPGIVEUP = 4;
constexpr int LCT_L2GIVEUP = 5;

// 場の一時状況に対するする宣言情報
constexpr int LCT_SELFFOLLOW = 6;
constexpr int LCT_UNRIVALED = 7;
constexpr int LCT_LASTAWAKE = 8;
constexpr int LCT_FLUSHLEAD = 9;
constexpr int LCT_NPDOM = 10;
constexpr int LCT_PDOM = 11;
constexpr int LCT_DOMOTHERS = 12;
constexpr int LCT_DOMME = 13;

constexpr int LCT_CHECKED = 15;

struct FieldAddInfo {
    // 着手決定のためにこの程度は調べておきたい場情報
    // 着手ごとの情報と被る場合もあるけれども、検索が面倒な場合はこちらに記録しておく

    uint32_t data;

    // set
    void setFinal() {    set(LCT_FINAL, LCT_PW, LCT_MPMATE); }
    void setPW() {       set(LCT_PW, LCT_MPMATE); }
    void setMPMate() {   set(LCT_MPMATE); }
    void setL2Mate() {   set(LCT_L2MATE); }
    void setMPGiveUp() { set(LCT_MPGIVEUP); }
    void setL2GiveUp() { set(LCT_L2GIVEUP); }

    void setSelfFollow() { set(LCT_SELFFOLLOW, LCT_UNRIVALED, LCT_LASTAWAKE, LCT_FLUSHLEAD, LCT_NPDOM, LCT_PDOM, LCT_DOMOTHERS); }
    void setUnrivaled() { set(LCT_UNRIVALED, LCT_FLUSHLEAD, LCT_NPDOM, LCT_PDOM, LCT_DOMOTHERS); }
    void setLastAwake() { set(LCT_LASTAWAKE, LCT_NPDOM, LCT_DOMOTHERS); }
    void setFlushLead() { set(LCT_FLUSHLEAD); }
    void setNPDom() { set(LCT_NPDOM); }
    void setPassDom() { set(LCT_PDOM); }
    void setBDO() { set(LCT_DOMOTHERS); }
    void setBDM() { set(LCT_DOMME); }
    void setBDALL() { set(LCT_DOMOTHERS, LCT_DOMME); }
    void setNoChance() { setBDM(); }

    void setMinNumCards(uint32_t n) { assert(n < 16U); data = (data & 0xFFF0FFFFU) | (n << 16); }
    void setMaxNumCards(uint32_t n) { assert(n < 16U); data = (data & 0xFF0FFFFFU) | (n << 20); }
    void setMinNumCardsAwake(uint32_t n) { assert(n < 16U); data = (data & 0xF0FFFFFFU) | (n << 24); }
    void setMaxNumCardsAwake(uint32_t n) { assert(n < 16U); data = (data & 0x0FFFFFFFU) | (n << 28); }

    // get
    // 一時情報
    bool isFinal() const {    return test(LCT_FINAL); }
    bool isPW() const {       return test(LCT_PW); }
    bool isMPMate() const {   return test(LCT_MPMATE); }
    bool isMPGiveUp() const { return test(LCT_MPGIVEUP); }
    bool isL2Mate() const {   return test(LCT_L2MATE); }
    bool isL2GiveUp() const { return test(LCT_L2GIVEUP); }
    bool isMate() const {     return test(LCT_FINAL, LCT_PW, LCT_MPMATE, LCT_L2MATE); }
    bool isGiveUp() const {   return test(LCT_MPGIVEUP, LCT_L2GIVEUP); }

    bool isSelfFollow() const { return test(LCT_SELFFOLLOW); }
    bool isUnrivaled() const { return test(LCT_UNRIVALED); }
    bool isLastAwake() const { return test(LCT_LASTAWAKE); }
    bool isFlushLead() const { return test(LCT_FLUSHLEAD); }
    bool isNonPassDom() const { return test(LCT_NPDOM); }
    bool isPassDom() const { return test(LCT_PDOM); }
    bool isBDO() const { return test(LCT_DOMOTHERS); }
    bool isBDM() const { return test(LCT_DOMME); }
    bool isBDALL() const { return holds(LCT_DOMOTHERS, LCT_DOMME); }
    bool isNoChance() const { return isBDM(); }

    uint32_t minNumCards() const { return (data >> 16) & 15U; }
    uint32_t maxNumCards() const { return (data >> 20) & 15U; }
    uint32_t minNumCardsAwake() const { return (data >> 24) & 15U; }
    uint32_t maxNumCardsAwake() const { return (data >> 28) & 15U; }

    void init() {
        // カード枚数については、無設定の場合はmaxが15、minの場合は0になるようにする
        data = 0xF0F00000U;
    }
    void procTmpInfo() {
        data &= 0xFFFF0000U;
    }

    constexpr FieldAddInfo(): data() {}
    constexpr FieldAddInfo(const FieldAddInfo& arg): data(arg.data) {}

    void set(size_t i) { data |= 1U << i; }
    template <class... args_t>
    void set(size_t i0, args_t... args) { set(i0); set(args...); }
    bool holds(size_t i0, size_t i1) const {
        uint32_t dst = (1U << i0) | (1U << i1);
        return !(~data & dst);
    }
    bool test(size_t i) const { return data & (1U << i); }
    template <class... args_t>
    bool test(size_t i0, args_t... args) const { return test(i0) || test(args...); }
};

static std::ostream& operator <<(std::ostream& out, const FieldAddInfo& i) { // 出力
    out << "Field :";
    if (i.isFinal()) out << " -FIN";
    else if (i.isPW()) out << " -PW";
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
    pnext->setMinNumCardsAwake(fieldInfo.minNumCards());
    pnext->setMaxNumCardsAwake(fieldInfo.maxNumCards());
    pnext->setMinNumCards(fieldInfo.minNumCards());
    pnext->setMaxNumCards(fieldInfo.maxNumCards());
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
    bool test(size_t i) const { return Move::flags & (1U << i); }
    template <class... args_t>
    bool test(size_t i0, args_t... args) const { return test(i0) || test(args...); }

    void setFinal() {    set(LCT_FINAL, LCT_PW, LCT_MPMATE); }
    void setPW() {       set(LCT_PW, LCT_MPMATE); }
    void setMPMate() {   set(LCT_MPMATE); }
    void setL2Mate() {   set(LCT_L2MATE); }
    void setMPGiveUp() { set(LCT_MPGIVEUP); }
    void setL2GiveUp() { set(LCT_L2GIVEUP); }

    // 当座支配
    void setDO() { set(LCT_DOMOTHERS); }
    void setDM() { set(LCT_DOMME); }
    void setDALL() { set(LCT_DOMOTHERS, LCT_DOMME); }
    void setDomOthers() { setDO(); }
    void setDomMe() { setDM(); }
    void setDomAll() { setDALL(); }
    void setChecked() { set(LCT_CHECKED); }

    void init() { Move::flags = 0; }

    // get
    bool isFinal() const {    return test(LCT_FINAL); }
    bool isPW() const {       return test(LCT_PW); }
    bool isMPMate() const {   return test(LCT_MPMATE); }
    bool isMPGiveUp() const { return test(LCT_MPGIVEUP); }
    bool isL2Mate() const {   return test(LCT_L2MATE); }
    bool isL2GiveUp() const { return test(LCT_L2GIVEUP); }
    bool isMate() const {     return test(LCT_FINAL, LCT_PW, LCT_MPMATE, LCT_L2MATE); }
    bool isGiveUp() const {   return test(LCT_MPGIVEUP, LCT_L2GIVEUP); }

    bool isDO() const { return test(LCT_DOMOTHERS); }
    bool isDM() const { return test(LCT_DOMME); }
    bool isDALL() const { return holds(LCT_DOMME, LCT_DOMOTHERS); }

    bool dominatesOthers() const { return isDO(); }
    bool dominatesMe() const { return isDM(); }
    bool dominatesAll() const { return isDALL(); }

    bool isChecked() const { return test(LCT_CHECKED); }
};

static std::string toInfoString(const MoveInfo& i, const Board b) { // 出力
    std::ostringstream oss;
    // 勝敗
    if (i.isFinal()) oss << " -FIN";
    else if (i.isPW()) oss << " -PW";
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