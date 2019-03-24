/*
 carts_test.cc
 Katsuki Ohto
 */

// カード集合系データ構造の動作テスト
// カード集合の前のランクやスートのテストも含む

#include "../include.h"

using namespace UECda;

Clock cl;

constexpr BitCards rankCardsTable[16] = {
    CARDS_U, CARDS_3, CARDS_4, CARDS_5,
    CARDS_6, CARDS_7, CARDS_8, CARDS_9,
    CARDS_T, CARDS_J, CARDS_Q, CARDS_K,
    CARDS_A, CARDS_2, CARDS_O, CARDS_OO,
};

constexpr BitCards suitCardsTable[16] = {
    CARDS_NULL, CARDS_C,   CARDS_D,   CARDS_CD,
    CARDS_H,    CARDS_CH,  CARDS_DH,  CARDS_CDH,
    CARDS_S,    CARDS_CS,  CARDS_DS,  CARDS_CDS,
    CARDS_HS,   CARDS_CHS, CARDS_DHS, CARDS_CDHS,
};

int testSuitSuits(){
    // (Suit, Suits)関係テスト
    int cnt[16 * 16] = {0};
    for(int sn = 0; sn < 4; ++sn){
        uint32_t s0 = SuitNumToSuits(sn);
        for(uint32_t s1 = 0; s1 < 16; ++s1){
            int index = getSuitSuitsIndex(s0, s1);
            if(index < 0 || N_PATTERNS_SUIT_SUITS <= index){
                cerr << "out of limit. " << index << " in " << N_PATTERNS_SUIT_SUITS;
                cerr << " (" << s0 << ", " << s1 << ")" << endl;
                return -1;
            }
            for(int osn = 0; osn < 4; ++osn){
                uint32_t os0 = SuitNumToSuits(osn);
                for(uint32_t os1 = 0; os1 < 16; ++os1){
                    int oindex = getSuitSuitsIndex(os0, os1);
                    bool equivalence = countBits(s1) == countBits(os1) && countBits(s0 & s1) == countBits(os0 & os1);
                    if(equivalence != (index == oindex)){
                        cerr << "inconsistent pattern index. ";
                        cerr << index << " (" << s0 << ", " << s1 << ")" << " <-> ";
                        cerr << oindex << " (" << os0 << ", " << os1 << ")" << endl;
                        return -1;
                    }
                }
            }
            ++cnt[index];
        }
    }
    return 0;
}

int test2Suits(){
    // (Suits Suits)関係テスト
    int cnt[16 * 16] = {0};
    for(uint32_t s0 = 0; s0 < 16; ++s0){
        for(uint32_t s1 = 0; s1 < 16; ++s1){
            int index = get2SuitsIndex(s0, s1);
            if(index < 0 || N_PATTERNS_2SUITS <= index){
                cerr << "out of limit. " << index << " in " << N_PATTERNS_2SUITS;
                cerr << " (" << s0 << ", " << s1 << ")" << endl;
                return -1;
            }
            for(uint32_t os0 = 0; os0 < 16; ++os0){
                for(uint32_t os1 = 0; os1 < 16; ++os1){
                    int oindex = get2SuitsIndex(os0, os1);
                    bool equivalence = (countBits(s0) + countBits(s1)) == (countBits(os0) + countBits(os1))
                    && abs((int)countBits(s0) - (int)countBits(s1)) == abs((int)countBits(os0) - (int)countBits(os1))
                    && countBits(s0 & s1) == countBits(os0 & os1);
                    if(equivalence != (index == oindex)){
                        cerr << "inconsistent pattern index. ";
                        cerr << index << " (" << s0 << ", " << s1 << ")" << " <-> ";
                        cerr << oindex << " (" << os0 << ", " << os1 << ")" << endl;
                        return -1;
                    }
                }
            }
            ++cnt[index];
        }
    }
    return 0;
}

int testSuitsSuits(){
    // (Suits, Suits)関係テスト
    int cnt[16 * 16] = {0};
    for(uint32_t s0 = 0; s0 < 16; ++s0){
        for(uint32_t s1 = 0; s1 < 16; ++s1){
            int index = getSuitsSuitsIndex(s0, s1);
            if(index < 0 || N_PATTERNS_SUITS_SUITS <= index){
                cerr << "out of limit. " << index << " in " << N_PATTERNS_SUITS_SUITS;
                cerr << " (" << s0 << ", " << s1 << ")" << endl;
                return -1;
            }
            for(uint32_t os0 = 0; os0 < 16; ++os0){
                for(uint32_t os1 = 0; os1 < 16; ++os1){
                    int oindex = getSuitsSuitsIndex(os0, os1);
                    bool equivalence = countBits(s0) == countBits(os0)
                    && countBits(s1) == countBits(os1)
                    && countBits(s0 & s1) == countBits(os0 & os1);
                    if(equivalence != (index == oindex)){
                        cerr << "inconsistent pattern index. ";
                        cerr << index << " (" << s0 << ", " << s1 << ")" << " <-> ";
                        cerr << oindex << " (" << os0 << ", " << os1 << ")" << endl;
                        return -1;
                    }
                }
            }
            ++cnt[index];
        }
    }
    return 0;
}

int testRankCards(){
    uint64_t time[3] = {0};
    for(int r = RANK_U; r <= RANK_O; ++r){
        cl.start();
        Cards test = RankToCards(r);
        time[0] += cl.restart();
        Cards ans = rankCardsTable[r];
        time[1] += cl.stop();
        if(test != ans){
            cerr << "inconsistent Rank -> Cards conversion!" << endl;
            cerr << OutRank(r) << " : " << OutCards(test) << " <-> " << OutCards(ans) << endl;
            return -1;
        }
    }
    for(int r = RANK_U; r <= RANK_O; ++r){
        Cards ans = CARDS_NULL;
        for(int rr = r; rr <= RANK_O; ++rr){
            cl.start();
            Cards test = RankRangeToCards(r, rr);
            time[2] += cl.stop();
            ans |= rankCardsTable[rr];
            if(test != ans){
                cerr << "inconsistent [Rank, Rank] -> Cards conversion!" << endl;
                cerr << "[" << OutRank(r) << ", " << OutRank(rr) << "]";
                cerr << " : " << OutCards(test) << " <-> " << OutCards(ans) << endl;
                return -1;
            }
        }
    }
    cerr << "r      -> c test : " << time[0] << " clock" << endl;
    cerr << "r      -> c ans  : " << time[1] << " clock" << endl;
    cerr << "[r, r] -> c test : " << time[2] << " clock" << endl;
    return 0;
}

int testRank4xCards(){
    uint64_t time[3] = {0};
    for(int r4x = RANK4X_U; r4x <= RANK4X_O; r4x += 4){
        cl.start();
        Cards test = Rank4xToCards(r4x);
        time[0] += cl.restart();
        Cards ans = rankCardsTable[r4x / 4];
        time[1] += cl.stop();
        if(test != ans){
            cerr << "inconsistent Rank4x -> Cards conversion!" << endl;
            cerr << OutRank(r4x / 4) << " : " << OutCards(test) << " <-> " << OutCards(ans) << endl;
            return -1;
        }
    }
    for(int r4x = RANK4X_U; r4x <= RANK4X_O; r4x += 4){
        Cards ans = CARDS_NULL;
        for(int rr4x = r4x; rr4x <= RANK4X_O; rr4x += 4){
            cl.start();
            Cards test = RankRange4xToCards(r4x, rr4x);
            time[2] += cl.stop();
            ans |= rankCardsTable[rr4x / 4];
            if(test != ans){
                cerr << "inconsistent [Rank4x, Rank4x] -> Cards conversion!" << endl;
                cerr << "[" << OutRank(r4x / 4) << ", " << OutRank(rr4x / 4) << "]";
                cerr << " : " << OutCards(test) << " <-> " << OutCards(ans) << endl;
                return -1;
            }
        }
    }
    cerr << "r4x        -> c test : " << time[0] << " clock" << endl;
    cerr << "r4x        -> c ans  : " << time[1] << " clock" << endl;
    cerr << "[r4x, r4x] -> c test : " << time[2] << " clock" << endl;
    return 0;
}

int testSuitCards(){
    // スート->カード集合変換テスト
    uint64_t time[2] = {0};
    for(uint32_t s = 0; s < 16; ++s){
        cl.start();
        Cards test = SuitsToCards(s);
        time[0] += cl.restart();
        Cards ans = suitCardsTable[s];
        time[1] += cl.stop();
        if(test != ans){
            cerr << "inconsistent Suits -> Cards conversion!" << endl;
            cerr << OutSuits(s) << " : " << OutCards(test) << " <-> " << OutCards(ans) << endl;
            return -1;
        }
    }
    cerr << "s  -> c test : " << time[0] << " clock" << endl;
    cerr << "s  -> c ans  : " << time[1] << " clock" << endl;
    return 0;
}

int testQR(const std::vector<Cards>& sample){
    // QR（枚数型）のテスト
    uint64_t time[2] = {0};
    for(Cards c : sample){
        cl.start();
        BitArray64<4> test = CardsToQR(c);
        time[0] += cl.restart();
        BitArray64<4> ans = CardsToQR_slow(c);
        time[1] += cl.stop();
        if(test != ans){
            cerr << "inconsistent Cards -> QR conversion!" << endl;
            cerr << OutCards(c) << " : " << test << " <-> " << ans << endl;
            return -1;
        }
    }
    cerr << "c -> qr test : " << time[0] << " clock" << endl;
    cerr << "c -> qr ans  : " << time[1] << " clock" << endl;
    return 0;
}

int testPQR(const std::vector<Cards>& sample){
    // PQR（枚数位置型）のテスト
    uint64_t time[4] = {0};
    for(Cards c : sample){
        cl.start();
        Cards test0 = CardsToPQR(c);
        time[0] += cl.stop();
        Cards qr = CardsToQR(c);
        cl.start();
        Cards test1 = QRToPQR(qr);
        time[1] += cl.restart();
        Cards ans = QRToPQR_slow(qr);
        time[2] += cl.stop();
        if(test0 != ans){
            cerr << "inconsistent Cards -> PQR conversion!" << endl;
            cerr << OutCards(c) << " : " << test0 << " <-> " << ans << endl;
            return -1;
        }
        if(test1 != ans){
            cerr << "inconsistent QR -> PQR conversion!" << endl;
            cerr << OutCards(c) << " : " << test1 << " <-> " << ans << endl;
            return -1;
        }
    }
    cerr << "c  -> pqr test : " << time[0] << " clock" << endl;
    cerr << "qr -> pqr test : " << time[1] << " clock" << endl;
    cerr << "qr -> pqr ans  : " << time[2] << " clock" << endl;
    return 0;
}

int testENR(const std::vector<Cards>& sample){
    // ENR(ランクにN枚以上存在)のテスト N = 1 ~ 3
    uint64_t time[6] = {0};
    for(Cards c : sample){
        cl.start();
        BitArray64<4> test1 = CardsToER(c);
        time[0] += cl.restart();
        BitArray64<4> ans1 = CardsToENR_slow(c, 1);
        time[1] += cl.stop();
        
        if(test1 != ans1){
            cerr << "inconsistent Cards -> E1R conversion!" << endl;
            cerr << OutCards(c) << " : " << test1 << " <-> " << ans1 << endl;
            return -1;
        }
        
        /*cl.start();
        BitArray64<4> test3 = convCards_3R(c);
        time[6] += cl.restart();
        BitArray64<4> ans3 = convCards_NR_slow(c, 3);
        time[7] += cl.stop();
        
        if(test0 != ans0){
            cerr << "inconsistent Cards -> E3R conversion!" << endl;
            cerr << OutCards(c) << " : " << test3 << " <-> " << ans3 << endl;
            return -1;
        }*/
    }
    for(int n = 1; n < 4; ++n){
        cerr << "c -> e" << n << "r test : " << time[(n - 1) * 2] << " clock" << endl;
        cerr << "c -> e" << n << "r ans  : " << time[(n - 1) * 2 + 1] << " clock" << endl;
    }
    return 0;
}

int testNR(const std::vector<Cards>& sample){
    // NR(ランクにN枚存在)のテスト N = 0 ~ 4
    uint64_t time[10] = {0};
    for(Cards c : sample){
        for(int n = 0; n <= 4; ++n){
            cl.start();
            BitArray64<4> test = CardsToNR(c, n);
            time[n * 2] += cl.restart();
            BitArray64<4> ans = CardsToNR_slow(c, n);
            time[n * 2 + 1] += cl.stop();
            
            if(test != ans){
                cerr << "inconsistent Cards -> " << n << "R conversion!" << endl;
                cerr << OutCards(c) << " : " << test << " <-> " << ans << endl;
                return -1;
            }
        }
    }
    for(int n = 0; n <= 4; ++n){
        cerr << "c -> " << n << "r test : " << time[n * 2] << " clock" << endl;
        cerr << "c -> " << n << "r ans  : " << time[n * 2 + 1] << " clock" << endl;
    }
    return 0;
}

int testSC(const std::vector<Cards>& sample){
    // SC（スート圧縮型）のテスト
    uint64_t time[4] = {0};
    for(Cards c : sample){
        Cards qr = CardsToQR(c);
        Cards pqr = QRToPQR(qr);
        
        cl.start();
        Cards test = PQRToSC(pqr);
        time[0] += cl.restart();
        Cards ans = QRToSC_slow(qr);
        time[1] += cl.stop();
        if(test != ans){
            cerr << "inconsistent PQR (QR) -> SC conversion!" << endl;
            cerr << OutCards(c) << " : " << test << " <-> " << ans << endl;
            return -1;
        }
    }
    cerr << "pqr -> sc test : " << time[0] << " clock" << endl;
    cerr << "qr  -> sc ans  : " << time[1] << " clock" << endl;
    return 0;
}

int testND(const std::vector<Cards>& sample){
    // ND（無支配型）のテスト
    return 0;
}

int main(int argc, char* argv[]){
    
    std::vector<Cards> sample;
    XorShift64 dice((unsigned int)time(NULL));
    
    for(int i = 0; i < 50000; ++i){
        int n = dice.rand() % N_MAX_OWNED_CARDS_CHANGE;
        sample.push_back(pickNBits64(CARDS_ALL, n, N_CARDS - n, &dice));
    }
    
    if(testSuitSuits()){
        cerr << "failed (Suit, Suits) test." << endl;
        return -1;
    }
    cerr << "passed (Suit, Suits) test." << endl;
    
    if(test2Suits()){
        cerr << "failed (Suits Suits) test." << endl;
        return -1;
    }
    cerr << "passed (Suits Suits) test." << endl;
    
    if(testSuitsSuits()){
        cerr << "failed (Suits, Suits) test." << endl;
        return -1;
    }
    cerr << "passed (Suits, Suits) test." << endl << endl;
    
    if(testRankCards()){
        cerr << "failed Rank -> Cards test." << endl;
        return -1;
    }
    cerr << "passed Rank -> Cards test." << endl << endl;
    
    if(testRank4xCards()){
        cerr << "failed Rank4x -> Cards test." << endl;
        return -1;
    }
    cerr << "passed Rank4x -> Cards test." << endl << endl;
    
    if(testSuitCards()){
        cerr << "failed Suits -> Cards test." << endl;
        return -1;
    }
    cerr << "passed Suits -> Cards test." << endl << endl;
    
    if(testQR(sample)){
        cerr << "failed QR test." << endl;
        return -1;
    }
    cerr << "passed QR test." << endl << endl;
    
    if(testPQR(sample)){
        cerr << "failed PQR test." << endl;
        return -1;
    }
    cerr << "passed PQR test." << endl << endl;
    
    if(testSC(sample)){
        cerr << "failed SC test." << endl;
        return -1;
    }
    cerr << "passed SC test." << endl << endl;
    
    if(testNR(sample)){
        cerr << "failed NR test." << endl;
        return -1;
    }
    cerr << "passed NR test." << endl << endl;
    
    if(testENR(sample)){
        cerr << "failed ENR test." << endl;
        return -1;
    }
    cerr << "passed ENR test." << endl << endl;
    
    return 0;
}
