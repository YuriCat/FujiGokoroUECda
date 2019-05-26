#include <map>
#include "daifugo.hpp"

using namespace std;

// ランク

const string rankChar  = "-3456789TJQKA2+:";

ostream& operator <<(ostream& out, const OutRank& arg) {
    out << rankChar[arg.r];
    return out;
}

ostream& operator <<(ostream& ost, const RankRange& arg) {
    for (int r = arg.r0; r <= arg.r1; r++) ost << rankChar[r];
    return ost;
}

int CharToRank(char c) {
    int r = rankChar.find(c);
    if (r != string::npos) return r;
    r = rankChar.find(toupper(c));
    if (r != string::npos) return r;
    return RANK_NONE;
}

// スート

const string suitNumChar  = "CDHSX";
    
ostream& operator <<(ostream& ost, const OutSuitNum& arg) {
    ost << suitNumChar[arg.sn];
    return ost;
}

int CharToSuitNum(char c) {
    int sn = suitNumChar.find(c);
    if (sn != string::npos) return sn;
    sn = suitNumChar.find(toupper(c));
    if (sn != string::npos) return sn;
    return SUITNUM_NONE;
}

ostream& operator <<(ostream& out, const OutSuits& arg) { // 出力の時だけ第５のスートは16として対応している
    for (int sn = 0; sn < N_SUITS + 1; sn++) {
        if (arg.s & SuitNumToSuits(sn)) {
            out << suitNumChar[sn];
        }
    }
    return out;
}

// (スート, スート)のパターン
uint8_t sSIndex[16][16];
uint8_t S2Index[16][16];
uint8_t SSIndex[16][16];
// (スート, スート, スート)のパターン
uint8_t sSSIndex[16][16][16];
uint16_t SSSIndex[16][16][16];

void initSuits() {
    
    // (suits suits) pattern index (exchangable) 0 ~ 21
    int twoSuitsCountIndex[5][5][5] = {0};
    int cnt = 0;
    for (int c0 = 0; c0 <= 4; c0++) {
        for (int c1 = 0; c1 <= c0; c1++) {
            for (int c01 = max(0, c0 + c1 - 4); c01 <= min(c0, c1); c01++) {
                DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c01 << endl;
                twoSuitsCountIndex[c0][c1][c01] = cnt++;
            }
        }
    }
    ASSERT(cnt == N_PATTERNS_2SUITS, cerr << cnt << " <-> " << N_PATTERNS_2SUITS << endl;);
    
    // (suits, suits) pattern index 0 ~ 34
    int suitsSuitsCountIndex[5][5][5] = {0};
    cnt = 0;
    for (int c0 = 0; c0 <= 4; c0++) {
        for (int c1 = 0; c1 <= 4; c1++) {
            for (int c01 = max(0, c0 + c1 - 4); c01 <= min(c0, c1); c01++) {
                DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c01 << endl;
                suitsSuitsCountIndex[c0][c1][c01] = cnt++;
            }
        }
    }
    ASSERT(cnt == N_PATTERNS_SUITS_SUITS,
           cerr << cnt << " <-> " << N_PATTERNS_SUITS_SUITS << endl;);
    
    for (unsigned s0 = 0; s0 < 16; s0++) {
        for (unsigned s1 = 0; s1 < 16; s1++) {
            unsigned s01 = s0 & s1;
            int c0 = popcnt(s0), c1 = popcnt(s1), c01 = popcnt(s01);
            int cmin = min(c0, c1), cmax = max(c0, c1);
            SSIndex[s0][s1] = suitsSuitsCountIndex[c0][c1][c01];
            S2Index[s0][s1] = twoSuitsCountIndex[cmax][cmin][c01];
        }
    }
    
    // (suit, suits) pattern index 0 ~ 7
    int suitSuitsCountIndex[5][2] = {0};
    cnt = 0;
    for (int c1 = 0; c1 <= 4; c1++) {
        for (int c01 = max(0, 1 + c1 - 4); c01 <= min(c1, 1); c01++) {
            assert(c01 == 0 || c01 == 1);
            DERR << "pattern " << cnt << " = " << c1 << ", " << c01 << endl;
            suitSuitsCountIndex[c1][c01] = cnt++;
        }
    }
    ASSERT(cnt == N_PATTERNS_SUIT_SUITS, cerr << cnt << " <-> " << N_PATTERNS_SUIT_SUITS << endl;);
    
    for (int sn0 = 0; sn0 < 4; sn0++) {
        for (unsigned s1 = 0; s1 < 16; s1++) {
            unsigned s0 = SuitNumToSuits(sn0);
            unsigned s01 = s0 & s1;
            int c1 = popcnt(s1), c01 = popcnt(s01);
            sSIndex[s0][s1] = suitSuitsCountIndex[c1][c01];
        }
    }
    
    // (suits, suits, suits) pattern index
    map<array<int, ipow(2, 3) - 1>, int> sssMap;
    for (unsigned s0 = 0; s0 < 16; s0++) {
        for (unsigned s1 = 0; s1 < 16; s1++) {
            for (unsigned s2 = 0; s2 < 16; s2++) {
                unsigned s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                unsigned s012 = s0 & s1 & s2;
                int c0 = popcnt(s0), c1 = popcnt(s1), c2 = popcnt(s2);
                int c01 = popcnt(s01), c02 = popcnt(s02), c12 = popcnt(s12);
                int c012 = popcnt(s012);
                array<int, ipow(2, 3) - 1> pattern = {c0, c1, c2, c01, c02, c12, c012};
                int cnt;
                if (sssMap.count(pattern) == 0) {
                    cnt = sssMap.size();
                    sssMap[pattern] = cnt;
                    DERR << "pattern " << cnt << " = " << c0 << ", " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << ", " << c012 << endl;
                } else {
                    cnt = sssMap[pattern];
                }
                SSSIndex[s0][s1][s2] = cnt;
            }
        }
    }
    ASSERT(sssMap.size() == N_PATTERNS_SUITS_SUITS_SUITS,
           cerr << sssMap.size() << " <-> " << N_PATTERNS_SUITS_SUITS_SUITS << endl;);

    map<array<int, ipow(2, 3) - 3>, int> s1ssMap;
    for (unsigned s0 = 1; s0 < 16; s0 <<= 1) {
        for (unsigned s1 = 0; s1 < 16; ++s1) {
            for (unsigned s2 = 0; s2 < 16; ++s2) {
                unsigned s01 = s0 & s1, s02 = s0 & s2, s12 = s1 & s2;
                unsigned s012 = s0 & s1 & s2;
                int c1 = popcnt(s1), c2 = popcnt(s2);
                int c01 = popcnt(s01), c02 = popcnt(s02), c12 = popcnt(s12);
                array<int, ipow(2, 3) - 3> pattern = {c1, c2, c01, c02, c12};
                int cnt;
                if (s1ssMap.count(pattern) == 0) {
                    cnt = s1ssMap.size();
                    s1ssMap[pattern] = cnt;
                    DERR << "pattern " << cnt << " = " << c1 << ", " << c2 << ", " << c01 << ", " << c02 << ", " << c12 << endl;
                } else {
                    cnt = s1ssMap[pattern];
                }
                sSSIndex[s0][s1][s2] = cnt;
            }
        }
    }
}

SuitsInitializer suitsInitializer;

// カード

ostream& operator <<(ostream& out, const OutIntCard& arg) {
    if (arg.ic == INTCARD_JOKER) {
        out << "JO";
    } else {
        out << OutSuitNum(IntCardToSuitNum(arg.ic)) << OutRank(IntCardToRank(arg.ic));
    }
    return out;
}

IntCard StringToIntCard(const string& str) {
    if (str.size() != 2) return INTCARD_NONE;
    if (toupper(str) == "JO") return INTCARD_JOKER;
    int sn = CharToSuitNum(str[0]);
    int r = CharToRank(str[1]);
    if (sn == SUITNUM_NONE || r == RANK_NONE) return INTCARD_NONE;
    return RankSuitNumToIntCard(r, sn);
}

Cards::Cards(const string& str) {
    clear();
    vector<string> v = split(str, ' ');
    for (const string& s : v) {
        IntCard ic = StringToIntCard(s);
        if (ic != INTCARD_NONE) insert(ic);
    }
}

string Cards::toString() const {
    ostringstream oss;
    oss << "{";
    for (IntCard ic : *this) oss << " " << OutIntCard(ic);
    oss << " }";
    return oss.str();
}

ostream& operator <<(ostream& out, const Cards& c) {
    out << c.toString();
    return out;
}

ostream& operator <<(ostream& out, const OutCardTables& arg) {
    // テーブル形式で見やすく
    // ２つを横並びで表示
    for (int i = 0; i < (int)arg.cv.size(); i++) {
        out << " ";
        for (int r = RANK_3; r <= RANK_2; r++) out << " " << OutRank(r);
        out << " X    ";
    }
    out << endl;
    for (int sn = 0; sn < N_SUITS; sn++) {
        for (Cards c : arg.cv) {
            out << OutSuitNum(sn) << " ";
            for (int r = RANK_3; r <= RANK_2; r++) {
                IntCard ic = RankSuitNumToIntCard(r, sn);
                if (c.contains(ic)) out << "O ";
                else out << ". ";
            }
            if (sn == 0) {
                if (containsJOKER(c)) out << "O ";
                else out << ". ";
                out << "   ";
            } else {
                out << "     ";
            }
        }
        out << endl;
    }
    return out;
} 

BitCards ORQ_NDTable[2][16][8]; // (order, rank, qty - 1)
uint64_t HASH_CARDS_ALL;

void initCards() {
    // カード集合関係の初期化
    HASH_CARDS_ALL = CardsToHashKey(CARDS_ALL);
    // nd計算に使うテーブル
    for (int r = 0; r < 16; r++) {
        // オーダー通常
        ORQ_NDTable[0][r][0] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1;
        ORQ_NDTable[0][r][1] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_12;
        ORQ_NDTable[0][r][2] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_123;
        ORQ_NDTable[0][r][3] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
        for (int q = 4; q < 8; q++) {
            ORQ_NDTable[0][r][q] = RankRangeToCards(RANK_IMG_MIN, r) & PQR_1234;
        }
        // オーダー逆転
        ORQ_NDTable[1][r][0] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1;
        ORQ_NDTable[1][r][1] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_12;
        ORQ_NDTable[1][r][2] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_123;
        ORQ_NDTable[1][r][3] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
        for (int q = 4; q < 8; q++) {
            ORQ_NDTable[1][r][q] = RankRangeToCards(r, RANK_IMG_MAX) & PQR_1234;
        }
        // 複数ジョーカーには未対応
    }
}

CardsInitializer cardsInitializer;

// 着手

ostream& operator <<(ostream& out, const MeldChar& m) { // MeldChar出力
    if (m.isPASS()) {
        out << "PASS";
    } else if (m.isSingleJOKER()) {
        out << "JOKER";
    } else {
        // スート
        if (m.isQuintuple()) { // クインタ特別
            out << OutSuits(SUITS_CDHSX);
        } else {
            out << OutSuits(m.suits());
        }
        out << "-";
        
        // ランク
        int r = m.rank();
        if (m.isSeq()) {
            int q = m.qty();
            out << RankRange(r, r + q - 1);
        } else {
            out << OutRank(r);
        }
    }
    return out;
}

ostream& operator <<(ostream& out, const Move& m) { // Move出力
    out << MeldChar(m) << m.cards();
    return out;
}

string toRecordString(Move m) {
    ostringstream oss;
    if (m.isPASS()) {
        oss << "P";
    } else if (m.isSingleJOKER()) {
        oss << "JK";
    } else {
        int r = m.rank();
        if (m.isSeq()) {
            oss << OutSuits(m.suits());
            oss << "-";
            int q = m.qty();
            oss << RankRange(r, r + q - 1);
            // ジョーカー
            if (m.containsJOKER()) {
                oss << "(" << OutRank(m.jokerRank()) << ")";
            }
        } else {
            if (m.isQuintuple()) oss << OutSuits(SUITS_CDHSX);
            else oss << OutSuits(m.suits());
            oss << "-";
            oss << OutRank(r);
            if (m.containsJOKER()) {
                unsigned jks = m.jokerSuits();
                if (jks == SUITS_CDHS) jks = SUIT_X;
                oss << "(" << OutSuits(jks) << ")";
            }
        }
    }
    return tolower(oss.str());
}
    
Move CardsToMove(const Cards chara, const Cards used) {
    // 性質 chara 構成札 used のカードから着手への変換
    Move m = MOVE_NULL;
    if (chara == CARDS_NULL) return MOVE_PASS;
    if (chara == CARDS_JOKER) {
        m.setSingleJOKER();
        return m;
    }
    IntCard ic = chara.lowest();
    int r = IntCardToRank(ic);
    unsigned s = chara[r];
    unsigned ps = used[r]; // ジョーカーなしのスート
    int q = countCards(chara);
    if (!polymRanks<2>(chara)) { // グループ系
        m.setGroup(q, r, s);
        unsigned js = s - ps;
        if (q > 4) js = 15;
        if (js) m.setJokerSuits(js);
    } else { // 階段系
        m.setSeq(q, r, s);
        if (containsJOKER(used)) {
            IntCard jic = Cards(subtrCards(chara, used.plain())).lowest();
            int jr = IntCardToRank(jic);
            m.setJokerRank(jr);
            m.setJokerSuits(s);
        }
    }
    DERR << "chara " << chara << " used " << used << " -> " << MeldChar(m) << endl;
    return m;
}

Move StringToMoveM(const string& str) {
    // 入力文字列からMove型への変更
    Move mv = MOVE_NULL;
    bool jk = false; // joker used
    unsigned s = SUITS_NULL;
    int rank = RANK_NONE;
    unsigned ns = 0; // num of suits
    int nr = 0; // num of ranks
    size_t i = 0;
    
    // special
    if (str == "p") return MOVE_PASS;
    if (str == "jk") {
        mv.setSingleJOKER();
        return mv;
    }
    // suits
    for (; i < str.size(); i++) {
        char c = str[i];
        if (c == '-') { i++; break; }
        int sn = CharToSuitNum(c);
        if (sn == SUITNUM_NONE) {
            CERR << "illegal suit number" << endl;
            return MOVE_NONE;
        }
        if (sn != SUITNUM_X) s |= SuitNumToSuits(sn);
        else jk = true; // クインタプル
        ns++;
    }
    // rank
    for (; i < str.size(); i++) {
        char c = str[i];
        if (c == '(') { jk = true; i++; break; }
        int r = CharToRank(c);
        if (r == RANK_NONE) {
            CERR << "illegal rank" << endl;
            return MOVE_NONE;
        }
        if (rank == RANK_NONE) rank = r;
        nr++;
    }
    // invalid
    if (s == SUITS_NULL) { CERR << "null suits" << endl; return MOVE_NONE; }
    if (!ns) { CERR << "zero suits" << endl; return MOVE_NONE; }
    if (rank == RANK_NONE) { CERR << "null lowest-rank" << endl; return MOVE_NONE; }
    if (!nr) { CERR << "zero ranks" << endl; return MOVE_NONE; }
    // seq or group?
    if (nr > 1) mv.setSeq(nr, rank, s);
    else if (ns == 1) mv.setSingle(rank, s);
    else  mv.setGroup(ns, rank, s);
    // joker
    if (jk) {
        if (!mv.isSeq()) {
            unsigned jks = SUITS_NULL;
            for (; i < str.size(); i++) {
                char c = str[i];
                if (c == ')') break;
                int sn = CharToSuitNum(c);
                if (sn == SUITNUM_NONE) {
                    CERR << "illegal joker-suit" << c << " from " << str << endl;
                    return MOVE_NONE;
                }
                jks |= SuitNumToSuits(sn);
            }
            if (jks == SUITS_NULL || (jks & SUIT_X)) jks = SUITS_CDHS; // クインタのときはスート指定なくてもよい
            mv.setJokerSuits(jks);
        } else {
            int jkr = RANK_NONE;
            for (; i < str.size(); i++) {
                char c = str[i];
                if (c == ')') break;
                int r = CharToRank(c);
                if (r == RANK_NONE) {
                    CERR << "illegal joker-rank " << c << " from " << str << endl;
                    return MOVE_NONE;
                }
                jkr = r;
            }
            mv.setJokerRank(jkr);
            mv.setJokerSuits(mv.suits());
        }
    }
    return mv;
}

ostream& operator <<(ostream& out, const Board& b) { // Board出力
    if (b.isNull()) out << "NULL";
    else out << b.move();
    // オーダー...一時オーダーのみ
    out << "  Order : ";
    if (b.order() == 0) out << "NORMAL";
    else out << "REVERSED";
    out << "  Suits : ";
    if (b.suitsLocked()) out << "LOCKED";
    else out << "FREE";
    return out;
}

bool isSubjectivelyValid(Board b, Move mv, const Cards& c, const int q) {
    // 不完全情報の上での合法性判定
    // c はそのプレーヤーが所持可能なカード
    // q はそのプレーヤーの手札枚数（公開されている情報）
    if (mv.isPASS()) return true;
    // 枚数オーバー
    if (mv.qty() > q) return false;
    // 持っていないはずの札を使った場合
    if (!holdsCards(c, mv.cards())) return false;
    if (!b.isNull()) {
        if (b.type() != mv.type()) return false; // 型違い
        if (b.isSeq()) {
            if (!isValidSeqRank(mv.rank(), b.order(), b.rank(), mv.qty())) {
                return false;
            }
            if (b.suitsLocked()) {
                if (b.suits() != mv.suits()) return false;
            }
        } else {
            if (b.isSingleJOKER()) {
                return mv.isS3();
            }
            if (mv.isSingleJOKER()) {
                if (!b.isSingle()) return false;
            } else {
                if (!isValidGroupRank(mv.rank(), b.order(), b.rank())) {
                    return false;
                }
                if (b.suitsLocked()) {
                    if (b.suits() != mv.suits()) return false;
                }
            }
        }
    }
    return true;
}