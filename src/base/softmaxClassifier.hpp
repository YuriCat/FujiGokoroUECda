#pragma once

#include <vector>
#include <utility>
#include <fstream>
#include <cmath>
#include <map>
#include <mutex>

struct MomentQuadruple{
    // one state
    std::vector<std::vector<std::pair<int, double>>> feature_;
    std::vector<double> score_;
    double score_sum_;
    int chosenIndex_; // which action was chosen?
};

template<int _N_PARAMS_, int _N_PHASES_ = 1, int _N_STAGES_ = 1, typename _real_t = double>
class SoftmaxClassifier{
private:
    static void assert_index(int i)noexcept{ ASSERT(0 <= i && i < N_PARAMS_, cerr << i << endl;); }
    static void assert_stage(int st)noexcept{ ASSERT(0 <= st && st < N_STAGES_, cerr << st << endl;); }
    mutable std::mutex mutex_;
    
public:
    using real_t = _real_t;
    
    constexpr static int N_PARAMS_ = _N_PARAMS_; // パラメータの数
    constexpr static int N_PHASES_ = _N_PHASES_; // ドメインの数(複数のドメインに同じパラメータを使い回す場合)
    constexpr static int N_STAGES_ = _N_STAGES_; // 第1分岐の数(パラメータを分ける)
    
    double T_; // temperature
    
    real_t param_[N_STAGES_ * N_PARAMS_];
    
    constexpr static int params()noexcept{ return N_PARAMS_; }
    constexpr static int phases()noexcept{ return N_PHASES_; }
    constexpr static int stages()noexcept{ return N_STAGES_; }
    
    real_t temperature()const noexcept{ return T_; }
    
    real_t param(int i, int st = 0)const{
        assert_index(i); assert_stage(st);
        return param_[st * params() + i];
    }
    
    void setLearningMode()const{}
    
    bool isLearning()const{ return false; }
    
    void initFeedingFeatureValue()const{}
    
    // learner 用の関数
    void initObjValue()const{}
    void initLearning()const{}
    void initCalculatingScore(int candidates)const{}
    void initCalculatingCandidateScore()const{}
    void feedFeatureScore(int c, int f, double v)const{}
    void feedCandidateScore(int c, double s)const{}
    void finishCalculatingScore()const{}
    
    // 学習用の排他制御
    void lock()const{ mutex_.lock(); }
    void unlock()const{ mutex_.unlock(); }

    std::string toString(int start, int end)const{
        std::ostringstream oss;
        for(int i = start; i < end; ++i){
            oss << param_[i] << " ";
        }
        return oss.str();
    }
    
    int fin(const std::string& fName){
        memset(param_, 0, sizeof(param_));
        std::ifstream ifs(fName, std::ios::in);
        if(!ifs){
            cerr << "SoftmaxClassifier::fin() : failed to import! " << fName << endl;
            return -1;
        }
        for(int i = 0; ifs && i < N_STAGES_ * N_PARAMS_; ++i){
            ifs >> param_[i];
        }
        return 0;
    }
    
    int bin(const std::string& fName){
        memset(param_, 0, sizeof(param_));
        FILE *const pf = fopen(fName.c_str(), "rb");
        if(pf == nullptr){
            cerr << "SoftmaxClassifier::bin() : failed to import! " << fName << endl;
            return -1;
        }
        fread(param_, sizeof(param_), 1, pf);
        fclose(pf);
        return 0;
    }
    
    int fout(const std::string& fName)const{ // 標準出入力型
        std::ofstream ofs(fName, std::ios::out);
        if(!ofs){ return -1; }
        for(int i = 0; i < N_STAGES_ * N_PARAMS_ - 1; ++i){
            ofs << param_[i] << " ";
        }
        ofs << param_[N_STAGES_ * N_PARAMS_ - 1];
        return 0;
    }
    
    int bout(const std::string& fName)const{ // バイナリ型
        FILE *const pf = fopen(fName.c_str(), "wb");
        if(pf == nullptr){ return -1; }
        fwrite(param_, sizeof(param_), 1, pf);
        fclose(pf);
        return 0;
    }
    
    int hout(const std::string& fName, const std::string& tableName)const{ // ヘッダファイル型
        std::ofstream ofs(fName, std::ios::out);
        if(!ofs){ return -1; }
        ofs << "double " << tableName << "[] = {" << endl;
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            ofs << param_[i] << "," << endl;
        }
        ofs << "};";
        return 0;
    }
    
    void setParam(const std::vector<real_t>& apv){
        for(int i = 0, n = apv.size(); i < n && i < N_STAGES_ * N_PARAMS_; ++i){
            param_[i] = apv[i];
        }
    }
    void setParam(const real_t* ap){
        memmove(param_, ap, sizeof(param_));
    }
    template<class dice_t>
    void setRandomParam(double mean, double sigma, dice_t& dice){
        std::normal_distribution<real_t> nd(mean, sigma);
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            param_[i] = nd(dice);
        }
    }
    
    void setTemperature(double at)noexcept{
        T_ = at;
    }
    
    SoftmaxClassifier(const real_t* ap):
    T_(1.0)
    {
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            param_[i] = ap[i];
        }
    }
    
    SoftmaxClassifier():
    T_(1.0)
    {
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            param_[i] = 0;
        }
    }
    
    bool exam()const noexcept{
        // nan, inf
        auto valid = [](real_t d)->bool{ return !std::isnan(d) && !std::isinf(d); };
        if(!valid(T_)){
            return false;
        }
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            if(!valid(param_[i])){
                return false;
            }
        }
        return true;
    }
    
    std::string toOverviewString()const{
        std::ostringstream oss;
        oss << params() << " params, " << phases() << " phases, " << stages() << " stages";
        return oss.str();
    }
};

template<class classifier_t>
class SoftmaxClassifyLearner{
private:
    constexpr static int N_PARAMS_ = classifier_t::N_PARAMS_; // パラメータの数
    constexpr static int N_PHASES_ = classifier_t::N_PHASES_; // ドメインの数(複数のドメインに同じパラメータを使い回す場合)
    constexpr static int N_STAGES_ = classifier_t::N_STAGES_; // 第1分岐の数(パラメータを分ける)
    
    constexpr static double PARAM_SUM_VALUE_MAX = 256; // 大きい値になりすぎるのを防ぐ
    constexpr static double VAR_ALPHA_MIN = 0.0001; // 分散が小さい要素に対応するパラメータの学習率が上がり過ぎないようにする
    
    static void assert_index(int i)noexcept{ ASSERT(0 <= i && i < N_PARAMS_, cerr << i << endl;); }
    static void assert_phase(int i)noexcept{ ASSERT(0 <= i && i < N_PHASES_, cerr << i << endl;); }
    static void assert_stage(int i)noexcept{ ASSERT(0 <= i && i < N_STAGES_, cerr << i << endl;); }
    
    classifier_t *pclassifier_;
    
public:
    using real_t = typename classifier_t::real_t;
    
    constexpr static int params()noexcept{ return N_PARAMS_; }
    constexpr static int phases()noexcept{ return N_PHASES_; }
    constexpr static int stages()noexcept{ return N_STAGES_; }
    
    double temperature()const noexcept{ return T_; }
    
    // temporary variable
    std::vector<double> score_;
    double score_sum_;
    int correctIndex_;
    std::vector<std::vector<std::pair<int, double>>> feature_;
    
    // temporary variables for learning
    // double param_sum_;
    // double param2_sum_;
    double gradient_[N_STAGES_][N_PARAMS_];
    int64_t turns_;
    int64_t tmpBatch_;
    
    // objective function
    int64_t trials_[N_PHASES_][N_STAGES_];
    int64_t unfoundTrials_[N_PHASES_][N_STAGES_];
    double meanHitRateSum_[N_PHASES_][N_STAGES_];
    double bestHitRateSum_[N_PHASES_][N_STAGES_];
    double KLDivergenceSum_[N_PHASES_][N_STAGES_];
    double entropySum_[N_PHASES_][N_STAGES_];
    
    // about feature
    double feature_size_[N_PHASES_][N_STAGES_];
    float feature_count_[N_PHASES_][N_STAGES_][N_PARAMS_];
    float feature_sum_[N_PHASES_][N_STAGES_][N_PARAMS_];
    float feature_sum2_[N_PHASES_][N_STAGES_][N_PARAMS_];
    //std::map<std::string, std::size_t> teacher_[N_PHASES_];
    
    // about record
    int64_t records_[N_PHASES_][N_STAGES_];
    int64_t unfoundRecords_[N_PHASES_][N_STAGES_];
    double branchSum_[N_PHASES_][N_STAGES_];
    double invBranchSum_[N_PHASES_][N_STAGES_];
    
    // L1, L2 standardation
    float baseParam_[N_STAGES_ * N_PARAMS_];
    
    // learning param
    double E_;
    double L1_;
    double L2_;
    
    double T_;
    int batch_;
    
    bool sparseUpdate;
    
    // value stock for after learning
    std::vector<MomentQuadruple> stock_;
    
    struct ImaginaryTragectory{
        std::vector<MomentQuadruple> trg;
        double reward;
    };
    
    // imaginary value stock for after learning (1 turn)
    std::vector<ImaginaryTragectory> imaginaryTragectories_;
    
    struct ImaginaryTragectories{
        std::vector<ImaginaryTragectory> tragectory_;
        int realChosenIndex_;
    };
    
    // stock of imaginary tragectories
    std::vector<ImaginaryTragectories> imaginaryStock_;
    
    SoftmaxClassifyLearner():
    pclassifier_(nullptr){
        initBaseParam();
        initFeatureValue();
        initObjValue();
        initLearnParam();
        initLearning();
    }
    
    ~SoftmaxClassifyLearner(){
        pclassifier_ = nullptr;
    }
    
    real_t param(int i, int st = 0)const{
        if(pclassifier_ != nullptr){
            return pclassifier_->param(i, st);
        }else{
            return 0.0;
        }
    }
    double baseParam(int i, int st = 0)const{
        assert_stage(st); assert_index(i);
        return baseParam_[st * params() + i];
    }
    double frequency(int i, int ph = 0, int st = 0)const{
        assert_stage(st); assert_phase(ph); assert_index(i); assert(feature_size_[ph][st] > 0);
        return feature_count_[ph][st][i] / feature_size_[ph][st];
    }
    double mean(int i, int ph = 0, int st = 0)const{
        assert_stage(st); assert_phase(ph); assert_index(i); assert(feature_size_[ph][st] > 0);
        return feature_sum_[ph][st][i] / feature_size_[ph][st];
    }
    double var(int i, int ph = 0, int st = 0)const{
        assert_stage(st); assert_phase(ph); assert_index(i); assert(feature_size_[ph][st] > 0);
        double me = mean(i, ph, st);
        return feature_sum2_[ph][st][i] / feature_size_[ph][st] - me * me;
    }
    double all_phase_size(int st = 0)const{
        assert_stage(st);
        double sum_size = 0;
        for(int ph = 0; ph < N_PHASES_; ++ph){
            sum_size += feature_size_[ph][st];
        }
        return sum_size;
    }
    double all_phase_freq(int i, int st = 0)const{
        assert_stage(st); assert_index(i);
        double sum_count = 0;
        for(int ph = 0; ph < N_PHASES_; ++ph){
            sum_count += feature_count_[ph][st][i];
        }
        double sum_size = all_phase_size(st);
        assert(sum_size > 0);
        return sum_count / sum_size;
    }
    double all_phase_mean(int i, int st = 0)const{
        assert_stage(st); assert_index(i);
        double sum_sum = 0;
        for(int ph = 0; ph < N_PHASES_; ++ph){
            sum_sum += feature_sum_[ph][st][i];
        }
        double sum_size = all_phase_size(st);
        assert(sum_size > 0);
        return sum_sum / sum_size;
    }
    double all_phase_var(int i, int st = 0)const{
        assert_stage(st); assert_index(i);
        double sum_sum2 = 0;
        for(int ph = 0; ph < N_PHASES_; ++ph){
            sum_sum2 += feature_sum2_[ph][st][i];
        }
        double sum_size = all_phase_size(st);
        assert(sum_size > 0);
        double me = all_phase_mean(i, st);
        return sum_sum2 / sum_size - me * me;
    }
    double limit(int i, int st = 0)const{
        assert_stage(st); assert_index(i);
        // パラメータ数が多いとき
        // 特徴量の値の分散が大きいとき
        // にはパラメータの値を小さめに制限
        return PARAM_SUM_VALUE_MAX / sqrt(sqrt(N_PARAMS_)) / sqrt(max(all_phase_var(i, st), 1.0));
    }
    
    void setClassifier(classifier_t *const apc)noexcept{
        pclassifier_ = apc;
    }
    void resetClassifier()noexcept{
        pclassifier_ = nullptr;
    }

    void initLearnParam()noexcept{
        T_ = 1.0;
        E_ = 0.0001;
        L1_ = 0.0;
        L2_ = 0.0;
        batch_ = 1;
    }

    void setLearnParam(double at, double ae, double al1, double al2, int ab)noexcept{
        T_ = at;
        E_ = ae;
        L1_ = al1;
        L2_ = al2;
        batch_ = ab;
        if(batch_ == 1){
            sparseUpdate = true;
        }
    }
    
    void initFeatureValue()noexcept{
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; st < N_STAGES_; ++st){
                //teacher_[ph][st].clear();
                //feature_size_[ph] = 0.00000000000000001;
                //feature_size_[ph] = 4;
                feature_size_[ph][st] = 1;
                for(int i = 0; i < N_PARAMS_; ++i){
                    feature_count_[ph][st][i] = 0;
                    feature_sum_[ph][st][i] = 0;
                    feature_sum2_[ph][st][i] = feature_size_[ph][st];
                }
                records_[ph][st] = 0;
                unfoundRecords_[ph][st] = 0;
                branchSum_[ph][st] = 0;
                invBranchSum_[ph][st] = 0;
            }
        }
    }
    
    void closeFeatureValue()noexcept{}
    
    void mergeFeatureValue(const SoftmaxClassifyLearner& alearner)noexcept{
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; st < N_STAGES_; ++st){
                //teacher_[ph]; どしよ
                feature_size_[ph][st] += alearner.feature_size_[ph][st];
                for(int i = 0; i < N_PARAMS_; ++i){
                    feature_count_[ph][st][i] += alearner.feature_count_[ph][st][i];
                    feature_sum_[ph][st][i] += alearner.feature_sum_[ph][st][i];
                    feature_sum2_[ph][st][i] += alearner.feature_sum2_[ph][st][i];
                }
                records_[ph][st] += alearner.records_[ph][st];
                unfoundRecords_[ph][st] += alearner.unfoundRecords_[ph][st];
                branchSum_[ph][st] += alearner.branchSum_[ph][st];
                invBranchSum_[ph][st] += alearner.invBranchSum_[ph][st];
            }
        }
    }
    
    void initBaseParam()noexcept{
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            baseParam_[i] = 0;
        }
    }
    void setBaseParam(const double *const ap){
        for(int i = 0; i < N_STAGES_ * N_PARAMS_; ++i){
            baseParam_[i] = ap[i];
        }
    }
    
    void initObjValue()noexcept{
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; st < N_STAGES_; ++st){
                trials_[ph][st] = 0;
                unfoundTrials_[ph][st] = 0;
                meanHitRateSum_[ph][st] = 0;
                bestHitRateSum_[ph][st] = 0;
                KLDivergenceSum_[ph][st] = 0;
                entropySum_[ph][st] = 0;
            }
        }
    }
    void mergeObjValue(const SoftmaxClassifyLearner& alearner)noexcept{
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; st < N_STAGES_; ++st){
                trials_[ph][st] += alearner.trials_[ph][st];
                unfoundTrials_[ph][st] += alearner.unfoundTrials_[ph][st];
                meanHitRateSum_[ph][st] += alearner.meanHitRateSum_[ph][st];
                bestHitRateSum_[ph][st] += alearner.bestHitRateSum_[ph][st];
                KLDivergenceSum_[ph][st] += alearner.KLDivergenceSum_[ph][st];
                entropySum_[ph][st] += alearner.entropySum_[ph][st];
            }
        }
    }
    
    int64_t trials(int ph = 0, int st = 0)const noexcept{
        return trials_[ph][st];
    }
    int64_t unfoundTrials(int ph = 0, int st = 0)const noexcept{
        return unfoundTrials_[ph][st];
    }
    double calcMeanHitRate(int ph = 0, int st = 0)const noexcept{
        assert_stage(st); assert_phase(ph);
        return (trials_[ph][st] + unfoundTrials_[ph][st]) > 0 ?
        meanHitRateSum_[ph][st] / (trials_[ph][st] + unfoundTrials_[ph][st]) : 0.0;
    }
    double calcBestHitRate(int ph = 0, int st = 0)const noexcept{
        assert_stage(st); assert_phase(ph);
        return (trials_[ph][st] + unfoundTrials_[ph][st]) > 0 ?
        bestHitRateSum_[ph][st] / (trials_[ph][st] + unfoundTrials_[ph][st]) : 0.0;
    }
    double calcKLDivergence(int ph = 0, int st = 0)const noexcept{
        assert_stage(st); assert_phase(ph);
        return trials_[ph][st] > 0 ? KLDivergenceSum_[ph][st] / trials_[ph][st] : 0.0;
    }
    double calcEntropy(int ph = 0, int st = 0)const noexcept{
        assert_stage(st); assert_phase(ph);
        return trials_[ph][st] > 0 ? entropySum_[ph][st] / trials_[ph][st] : 0.0;
    }
    
    double calcMeanBranches(int ph = 0, int st = 0)const noexcept{
        assert_stage(st); assert_phase(ph);
        return records_[ph][st] > 0 ? branchSum_[ph][st] / records_[ph][st] : 0.0;
    }
    double calcMeanInvBranches(int ph = 0, int st = 0)const noexcept{
        assert_stage(st); assert_phase(ph);
        return records_[ph][st] > 0 ? invBranchSum_[ph][st] / records_[ph][st] : 0.0;
    }
    
    void initLearning()noexcept{
        // initialize
        turns_ = 0;
        for(int st = 0; st < N_STAGES_; ++st){
            for(int i = 0; i < N_PARAMS_; ++i){
                gradient_[st][i] = 0;
            }
        }
        tmpBatch_ = 0;
    }
    
    void initCalculatingScore(int candidates){
        feature_.clear();
        score_.clear();
        if(candidates >= 0){
            feature_.reserve(candidates);
            score_.reserve(candidates);
            for(int c = 0; c < candidates; ++c){
                feature_.push_back(std::vector<std::pair<int, double>>());
                score_.push_back(0);
            }
        }
        score_sum_ = 0;
    }
    void initCalculatingCandidateScore(){}
    void feedFeatureScore(int c, int f, double v){
        // 候補 c 特徴 f 係数 v
        feature_.at(c).emplace_back(std::pair<int, double>(f, v));
    }
    
    void feedCandidateScore(int c, double s){
        score_.at(c) += s;
        score_sum_ += s;
    }
    
    void finishCalculatingScore(bool stock = false){
        // if after-learning mode, value should be stocked
        if(stock){
            stock_.emplace_back(MomentQuadruple{
                feature_,
                score_,
                score_sum_,
                -1
            });
        }
    }
    
    void feedChosenActionIndexToLatestStock(int idx, int ph = 0){
        if(stock_.size() > 0){
            stock_.back().chosenIndex_ = idx;
        }
    }
    
    void clearStocks(){
        stock_.clear();
    }
    
    void clearImaginaryStocks(){
        imaginaryStock_.clear();
    }
    
    void feedReward(double reward, int ph = 0, int st = 0){ // reinforcement learning
        for(auto& q : stock_){
            if(q.feature_.size() <= 1){ return; }
            
            for (const auto& element : q.feature_[q.chosenIndex_]){ // chosen action
                double dg = element.second / (var(element.first, ph) + VAR_ALPHA_MIN) * reward;
                
                FASSERT(dg, cerr << "elm = " << element.first << " " << element.second
                        << " / " << var(element.first, ph) << endl;);
                
                gradient_[st][element.first] += dg;
            }
            
            for (int m = 0, n = q.feature_.size(); m < n; ++m){ // all actions
                double possibility = (q.score_sum_ > 0) ? (q.score_[m] / q.score_sum_) : (1.0 / n);
                
                FASSERT(possibility, cerr << q.score_[m] << " / " << q.score_sum_ << endl;);
                for (const auto& element : q.feature_[m]){ // all features
                    double dg = -element.second / (var(element.first, ph) + VAR_ALPHA_MIN) * possibility * reward;
                    
                    FASSERT(dg, cerr << "elm = " << element.first << " grd = " << dg
                            << " val = " << element.second << " p = " << possibility << " var = " << var(element.first, ph) << endl;);
                    
                    gradient_[st][element.first] += dg;
                }
            }
        }
        clearStocks();
    }
    
    void feedImaginaryReward(double reward, int ph = 0){
        //imaginaryStock.back().emplace_back(make_tuple(stock_, reward));
    }
    
    /*void feedChosenActionIndexToLatestImagnaryStock(int idx, int ph = 0){
        if(stock_.size() > 0){
            stock_.back().chosenIndex_ = idx;
        }
    }*/
    
    void feedRealChosenIndex(int idx){
        imaginaryStock_.emplace_back(ImaginaryTragectories{
            std::move(imaginaryTragectories_),
            idx,
        });
    }
    
    void feedRealReward(double reward, int ph = 0){
        /*for(auto& ts : imaginaryStock_){
            for(auto& t : ts.tragectories){
                
            }
        }
        for(auto& q : stock_){
            if(q.feature_.size() <= 1){ return; }
            
            for (const auto& element : q.feature_[q.chosenIndex_]){ // chosen action
                double dg = element.second / var(element.first, ph) * reward;
                
                FASSERT(dg, cerr << "elm = " << element.first << " " << element.second
                        << " / " << var(element.first, ph) << endl;);
                
                gradient_[element.first] += dg;
            }
            
            for (int m = 0, n = q.feature_.size(); m < n; ++m){ // all actions
                double possibility = (q.score_sum_ > 0) ? (q.score_[m] / q.score_sum_) : (1 / (double)n);
                
                FASSERT(possibility, cerr << q.score_[m] << " / " << q.score_sum_ << endl;);
                for (const auto& element : q.feature_[m]){ // all features
                    double dg = -element.second / var(element.first, ph) * possibility * reward;
                    
                    FASSERT(dg, cerr << "elm = " << element.first << " grd = " << dg
                            << " val = " << element.second << " p = " << possibility << " var = " << var(element.first, ph) << endl;);
                    
                    gradient_[element.first] += dg;
                }
            }
        }*/
        clearImaginaryStocks();
    }
    
    void updateParams(int ph = 0, int st = 0){
        
        ++tmpBatch_;
        if(tmpBatch_ >= batch_){
            tmpBatch_ = 0;
        }else{
            return;
        }
        
        const double T = temperature();
        const double e = E_;
        
        if(e == 0.0 || pclassifier_ == nullptr){ return; }
        
        real_t *const param = pclassifier_->param_;
        if(sparseUpdate){ // 勾配を使用した特徴1つずつで更新する場合
            if(feature_.size() <= 1){ return; }
            const double lam1 = L1_, lam2 = L2_;
            for(int m = 0, n = feature_.size(); m < n; ++m){ // all candidates
                double possibility = (score_sum_ > 0) ? (score_[m] / score_sum_) : (1.0 / n);
                FASSERT(possibility, cerr << score_[m] << " / " << score_sum_ << endl;);
                
                double correct = (m == correctIndex_) ? 1.0 : 0.0;
                for(const auto& element : feature_[m]){
                    const int pi = element.first + st * params();
                    double dg = (correct - possibility) * element.second / (var(element.first, ph, st) + VAR_ALPHA_MIN);
                    FASSERT(dg, cerr << "elm = " << element.first << " grd = " << dg
                            << " val = " << element.second << " p = " << possibility << " var = " << var(element.first, ph, st) << endl;);
                    
                    param[pi] += e / T * dg;
                    
                    FASSERT(param[pi],); FASSERT(baseParam_[pi],);
                    
                    // 正則化
                    /*if(lam1 || lam2){
                        const double weight = 1.0 / sqrt(frequency(element.first, ph, st)) / sqrt(feature_.size());
                        const double l1 = param[pi] > baseParam_[pi] ? (-weight) : weight; // L1
                        const double l2 = -2 * weight * (param[pi] - baseParam_[pi]); // L2
                        
                        // 正則化項によるパラメータ更新は、baseを追い越すときはbaseにする(FOBOS)
                        double nrm = l1 * lam1 + l2 * lam2;
                        if((param[pi] - baseParam_[pi]) * (param[pi] + nrm - baseParam_[pi]) <= 0){
                            param[pi] = baseParam_[pi];
                        }else{
                            param[pi] += nrm;
                        }
                    }*/
                    
                    // 絶対値が大きい場合は丸める
                    double paramLimit = limit(element.first, st);
                    param[pi] = max(-paramLimit, min(paramLimit, (double)param[pi]));
                    FASSERT(param[pi],);
                }
            }
        }else{ // 勾配をパラメータ数分の配列にためておいて計算する場合
            pclassifier_->lock();
            const double weight = sqrt(batch_);
            const double lam1 = L1_ * weight, lam2 = L2_ * weight;
            
            for(int i = 0; i < N_PARAMS_; ++i){
                int pi = i + st * params();
                double omg = e / T * gradient_[st][i];
                param[pi] += omg; // 最急降下法によるパラメータ更新
                double tmp = param[pi];
                
                FASSERT(tmp,); FASSERT(baseParam_[pi],);
                
                if(lam1 || lam2){
                    const double l1 = tmp > baseParam_[pi] ? (-1) : (1); // L1
                    const double l2 = -2 * (tmp - baseParam_[pi]); // L2
                    
                    // 正則化項によるパラメータ更新は、baseを追い越すときはbaseにする(FOBOS)
                    double nrm = l1 * lam1 + l2 * lam2;
                    if((tmp - baseParam_[pi]) * (tmp + nrm - baseParam_[pi]) <= 0){
                        param[pi] = baseParam_[pi];
                    }else{
                        param[pi] += nrm;
                    }
                    //if(nrm > 100){ cerr << nrm << endl; }
                }
                
                // 絶対値が大きい場合は丸める
                double paramLimit = limit(i, st);
                param[pi] = max(-paramLimit, min(paramLimit, (double)param[pi]));
                FASSERT(param[pi],);
            }
            pclassifier_->unlock();
            initLearning();
        }
        ASSERT(pclassifier_->exam(),);
    }
    void feedUnfoundFeatureValue(int ph = 0, int st = 0){
        ++unfoundRecords_[ph][st];
    }
    void feedFeatureValue(int ph = 0, int st = 0){
        if(feature_.size() <= 1){ return; }
        
        const double weight = 1.0 / feature_.size(); // 候補クラスが少ないときほど重要
        
        for(int m = 0, n = feature_.size(); m < n; ++m){
            for(auto element : feature_[m]){
                feature_count_[ph][st][element.first] += weight;
                feature_sum_[ph][st][element.first] += element.second * weight;
                feature_sum2_[ph][st][element.first] += element.second * element.second * weight;
            }
        }
        
        ++feature_size_[ph][st];
        ++records_[ph][st];
        branchSum_[ph][st] += feature_.size();
        invBranchSum_[ph][st] += 1.0 / feature_.size();
    }
    
    void feedTeacherName(const std::string& name, int ph = 0){
        //teacher_[ph][name]++;
    }
    
    void feedSupervisedActionIndex(int idx, int ph = 0, int st = 0){ // in supervised learning mode
        if(feature_.size() <= 1){ return; }
        correctIndex_ = idx;
        if(sparseUpdate)return;
        for(const auto& element : feature_[idx]){ // correct answer
            double dg = element.second / (var(element.first, ph, st) + VAR_ALPHA_MIN);
            
            FASSERT(dg, cerr << "elm = " << element.first << " " << element.second
                   << " / " << var(element.first, ph, st) << endl;);
            
            gradient_[st][element.first] += dg;
        }
        for(int m = 0, n = feature_.size(); m < n; ++m){ // all answers
            double possibility = (score_sum_ > 0) ? (score_[m] / score_sum_) : (1 / (double)n);
            
            FASSERT(possibility, cerr << score_[m] << " / " << score_sum_ << endl;);
            
            for (const auto& element : feature_[m]){
                double dg = -element.second / (var(element.first, ph, st) + VAR_ALPHA_MIN) * possibility;
                
                FASSERT(dg, cerr << "elm = " << element.first << " grd = " << dg
                       << " val = " << element.second << " p = " << possibility << " var = " << var(element.first, ph, st) << endl;);
                
                gradient_[st][element.first] += dg;
            }
        }
        ++turns_;
    }
    
    void feedObjValue(int idx, int ph = 0, int st = 0){
        if(feature_.size() <= 1){ return; }
        if(idx >= 0){
            for(int m = 0, n = feature_.size(); m < n; ++m){
                double possibility = (score_sum_ > 0) ? (score_[m] / score_sum_) : (1 / (double)n);
                
                FASSERT(possibility, cerr << score_[m] << " / " << score_sum_ << endl;);
                
                if(possibility > 0){
                    entropySum_[ph][st] += - possibility * log(possibility) / log(2.0);
                }
            }
            {
                std::vector<int> bestIdx;
                bestIdx.push_back(0);
                double bestScore = score_[0];
                for(int m = 1, n = feature_.size(); m < n; ++m){
                    if(score_[m] > bestScore){
                        bestIdx.clear();
                        bestIdx.push_back(m);
                        bestScore = score_[m];
                    }else if(score_[m] == bestScore){
                        bestIdx.push_back(m);
                    }
                }
                bool best = false;
                for(int m = 0; m < (int)bestIdx.size(); ++m){
                    if(idx == bestIdx[m]){
                        best = true;
                        break;
                    }
                }
                if(score_sum_ > 0){
                    meanHitRateSum_[ph][st] += score_[idx] / score_sum_;
                    bestHitRateSum_[ph][st] += best ? (1.0 / bestIdx.size()) : 0;
                    
                    const double dkl = -(log(score_[idx] / score_sum_) / log(2.0));
                    FASSERT(dkl, cerr << " dkl" << endl;);
                    
                    KLDivergenceSum_[ph][st] += dkl;
                }else{
                    meanHitRateSum_[ph][st] += 1.0 / feature_.size();
                    bestHitRateSum_[ph][st] += 1.0 / feature_.size();
                    KLDivergenceSum_[ph][st] += -(log(1.0 / feature_.size()) / log(2.0));
                    //cerr << " n- " << -(log(1.0 / feature_.size()) / log(2.0)) << endl;
                }
                //cerr << KLDivergenceSum_[ph] << endl;
            }
            ++trials_[ph][st];
        }else{
            ++unfoundTrials_[ph][st];
        }
    }
    
    double calcBaseDistance(int ph = 0, int st = 0)const{
        double dsum = 0;
        for(int i = 0; i < N_PARAMS_; ++i){
            const double d = (param(i, st) - baseParam(i, st)) * var(i, ph, st);
            dsum += fabs(d);
            FASSERT(dsum,);
        }
        return dsum;
    }
    
    double calcBaseDistance2(int ph = 0, int st = 0)const{
        double dsum = 0;
        for(int i = 0; i < N_PARAMS_; ++i){
            const double d = (param(i, st) - baseParam(i, st)) * var(i, ph, st);
            dsum += d * d;
            FASSERT(dsum,);
        }
        return dsum;
    }
    
    int finFeatureSurvey(const std::string& fName, double af_size = 10000){

        std::ifstream ifs(fName, std::ios::in);
        if(!ifs){
            cerr << "SoftmaxClassifyLearner::finFeatureSurvey() : failed to import! " << fName << endl;
            return -1;
        }
        
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; st < N_STAGES_; ++st){
                feature_size_[ph][st] = af_size;
                for(int i = 0; i < N_PARAMS_; ++i){
                    feature_count_[ph][st][i] = 0;
                    feature_sum_[ph][st][i] = 0;
                    feature_sum2_[ph][st][i] = feature_size_[ph][st];
                }
            }
        }
        
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; ifs && st < N_STAGES_; ++st){
                for(int i = 0; ifs && i < N_PARAMS_; ++i){
                    float tfreq, tmean, tvar;
                    ifs >> tfreq >> tmean >> tvar;
                    
                    feature_count_[ph][st][i] = tfreq * feature_size_[ph][st];
                    feature_sum_[ph][st][i] = tmean * feature_size_[ph][st];
                    if(tvar > 0){
                        feature_sum2_[ph][st][i] = (tvar + tmean * tmean) * feature_size_[ph][st];
                    }
                    
                    ASSERT(var(i, ph, st) > 0,);
                }
            }
        }
        return 0;
    }
    
    int foutFeatureSurvey(const std::string& fName)const{
        std::ofstream ofs(fName, std::ios::out);
        if(!ofs){ return -1; }
        for(int ph = 0; ph < N_PHASES_; ++ph){
            for(int st = 0; ofs && st < N_STAGES_; ++st){
                for(int i = 0; ofs && i < N_PARAMS_; ++i){
                    ofs << frequency(i, ph, st) << " " << mean(i, ph, st) << " " << var(i, ph, st) << endl;
                }
            }
        }
        return 0;
    }
    
    std::string toOverviewString()const{
        std::ostringstream oss;
        oss << params() << " params, " << phases() << " phases, " << stages() << " stages";
        return oss.str();
    }
    
    std::string toFeatureString()const{
        std::ostringstream oss;
        for(int i = 0, n = feature_.size(); i < n; ++i){
            oss << "action " << i << " : ";
            for(int j = 0, l = feature_[i].size(); j < l; ++j){
                oss << feature_[i][j].first;
                if(feature_[i][j].second != 1.0){
                    oss << "(" << feature_[i][j].second << ")";
                }
                oss << " ";
            }
            oss << endl;
        }
        return oss.str();
    }
    
    std::string toRecordString(int ph = 0, int st = 0)const{
        std::ostringstream oss;
        oss << "MB = " << calcMeanBranches(ph, st);
        oss << ", MIB = " << calcMeanInvBranches(ph, st) << " in " << records_[ph][st];
        oss << " records. (and " << unfoundRecords_[ph][st] << " unfound)";
        return oss.str();
    }
    
    std::string toObjValueString(int ph = 0, int st = 0)const{
        std::ostringstream oss;
        oss << "MHR(BHR) = " << calcMeanHitRate(ph, st);
        oss << "(" << calcBestHitRate(ph, st) << ")";
        oss << ", KLD = "<< calcKLDivergence(ph, st);
        oss << ", ENT = "<< calcEntropy(ph, st);
        return oss.str();
    }
    
    /*std::string toTeacherString(int ph = 0, int st = 0)const{
        std::ostringstream oss;
        for(auto& t : teacher_){
            oss << t.first << " : " << t.second << endl;
        }
        return oss.str();
    }*/
};

template<class param_t>
double calcParamDistance(const param_t& p0, const param_t& p1){
    double dsum = 0;
    for(int st = 0; st < param_t::stages(); ++st){
        for(int i = 0; i < param_t::params(); ++i){
            const double d = fabs(p0.param(i, st) - p1.param(i, st));
            dsum += d;
        }
    }
    return dsum;
}

template<class param_t>
double calcParamDistance2(const param_t& p0, const param_t& p1){
    double dsum = 0;
    for(int st = 0; st < param_t::stages(); ++st){
        for(int i = 0; i < param_t::params(); ++i){
            const double d = p0.param(i, st) - p1.param(i, st);
            dsum += d * d;
        }
    }
    return dsum;
}