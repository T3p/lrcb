#ifndef LINEARREP_H
#define LINEARREP_H
#include <random>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include "abstractclasses.h"

using json = nlohmann::json;
using namespace std;
using namespace Eigen;

class FiniteLinearRepresentation : public ContRepresentation<int>
{
public:
    FiniteLinearRepresentation(const FiniteLinearRepresentation& other)
        : FiniteLinearRepresentation(other.features, other.param, other.noise_std, other.seed)
    {}

    FiniteLinearRepresentation(std::vector<MatrixXd> features, VectorXd param,
                               double noise_std=0.1, int seed=0)
        : features(features), param(param), noise_std(noise_std), seed(seed)
    {
#if DEBUG
        for (MatrixXd& x : features)
        {
            // cout << x << endl << endl;
            assert(x.cols() == param.size());
        }
#endif
        uid = uniform_int_distribution<int>(0,features.size()-1);
        nd = normal_distribution<>(0,noise_std);
        rng.seed(seed);
    }

    FiniteLinearRepresentation(std::string json_file, double noise_std=0.1, long seed=0)
        : noise_std(noise_std), seed(seed)
    {
        /*
        Expected json format
        {
            "features":
            [
                [
                    [0.04570723266688925, 0.05741007615948068],
                    [0.6712536279276858, 0.6521873839500452]
                ],
                [
                    [0.5151870882948832, 0.7586773652752291],
                    [0.5365993509774891, 0.7829667875472054]
                ],
                [
                    [0.17244380342440213, 0.06567624806908534],
                    [0.012151281523658741, 0.39743985240386237]
                ]
            ],
            "param": [1.57270357, 0.64796005]
        }
        Example generated from python as follows
        F = (n_context, n_arm, dim)
        F = np.random.rand(3,2,2)
        p = np.random.randn(2)
        data = {"features": F.tolist(), "param": p.tolist()}
        with open('data.txt', 'w') as outfile:
            json.dump(data, outfile)
        */
        std::ifstream ifile(json_file);
        json js;
        ifile >> js;
        // parse features
        std::vector<std::vector<std::vector<double>>> vect = js["features"].get<std::vector<std::vector<std::vector<double>>>>();
        int n_context = vect.size();
        int n_arms = vect[0].size();
        int dim = vect[0][0].size();

        for(int i = 0; i < n_context; i++)
        {
            MatrixXd mat(n_arms, dim);
            for(int j = 0; j < n_arms; ++j)
            {
                assert(dim == vect[i][j].size());
                mat.row(j) = Eigen::Map<VectorXd> (vect[i][j].data(), dim);
            }
            features.push_back(mat);
        }
        // parse params
        std::vector<double> p = js["param"].get<std::vector<double>>();
        param = Eigen::Map<VectorXd> (p.data(), dim);
        uid = uniform_int_distribution<int>(0,features.size()-1);
        nd = normal_distribution<>(0,noise_std);
        rng.seed(seed);
    }

    ~FiniteLinearRepresentation() {}

    FiniteLinearRepresentation copy(long new_seed)
    {
        FiniteLinearRepresentation rep(features, param, noise_std, new_seed);
        return rep;
    }

    int n_arms()
    {
        return features[0].rows();
    }

    int n_contexts()
    {
        return features.size();
    }

    int features_dim()
    {
        return features[0].cols();
    }

    double features_bound()
    {
        double bound = -1;
        for (MatrixXd& x : features)
        {
            double n = x.colwise().lpNorm<2>().maxCoeff();
            bound = max(n, bound);
        }
        return bound;
    }

    double param_bound()
    {
        return param.lpNorm<2>();
    }

    int sample_context()
    {
        return uid(rng);
    }
    double sample_reward(const int& context, int action)
    {
        double r = reward(context, action) + nd(rng) * noise_std;
        return r;
    }
    double reward(const int& context, int action)
    {
        return param.dot(features[context].row(action));
    }

    VectorXd get_features(const int& context, int action)
    {
        return features[context].row(action);
    }

    double optimal_reward(const int& context)
    {
        VectorXd v = features[context] * param;
        return v.maxCoeff();
    }

    void optimal_features_and_arms(std::vector<int>& opt_arms,
                                   MatrixXd& opt_features, VectorXd& opt_rews)
    {
        int n_cont= features.size(), d = features[0].cols();
        for(int i=0; i< n_cont; ++i)
        {
            VectorXd v = features[i] * param;
            int maxElementIndex = 0;
            double maxValue = v(0);
            for(int j = 1; j < v.size(); ++j)
            {
                if (maxValue < v(j))
                {
                    maxValue = v(j);
                    maxElementIndex = j;
                }
            }
            opt_arms[i] = maxElementIndex;
            opt_rews[i] = maxValue;
            for(int j = 0; j < d; ++j)
            {
                opt_features(i,j) = features[i](maxElementIndex, j);
            }
        }
    }

    int rank_optimal_arms()
    {
        int nc  = n_contexts();
        int d = features_dim();
        std::vector<int> opt_arms(nc);
        MatrixXd opt_feats(nc, d);
        VectorXd opt_rews(nc);
        optimal_features_and_arms(opt_arms, opt_feats, opt_rews);
        // Eigen::ColPivHouseholderQR<MatrixXd> dec(opt_feats);
        FullPivLU<MatrixXd> dec(opt_feats);
        // std::cout << opt_feats << std::endl << std::endl;
        // dec.setThreshold(1e-12);
        return dec.rank();
    }

    void save(std::string json_file)
    {
        json j;
        std::vector<std::vector<std::vector<double>>> vfeat;
        for(MatrixXd& el : features)
        {
            std::vector<std::vector<double>> mat;
            for(int i = 0; i < el.rows(); ++i)
            {
                std::vector<double> tmp;
                for(int j = 0; j < el.cols(); ++j)
                {
                    tmp.push_back(el(i,j));
                }
                mat.push_back(tmp);
            }
            vfeat.push_back(mat);
        }
        j["features"] = vfeat;
        std::vector<double> vec(param.data(), param.data() + param.size());
        j["param"] = vec;
        std::ofstream o(json_file);
        o << j << std::endl;
    }

    VectorXd rewards()
    {
        int nc  = n_contexts();
        int na = n_arms();
        VectorXd v(nc*na);
        for(int i = 0; i < nc; ++i)
        {
            auto& h = features[i] * param;
            for(int a = 0; a < na; a++)
            {
                v[i*na + a] = h[a];
            }
        }
        return v;
    }
protected:
    std::vector<MatrixXd> features;
    VectorXd param;
    std::vector<int> actions;
    double noise_std;
    long seed;
    std::uniform_int_distribution<int> uid;
    std::normal_distribution<> nd;
    std::mt19937 rng;

public:
    friend FiniteLinearRepresentation derank_hls(
        FiniteLinearRepresentation& rep, int newrank, bool random_seed,
        bool transform, bool normalize);
    friend FiniteLinearRepresentation random_transform(FiniteLinearRepresentation& rep, bool normalize);
    friend FiniteLinearRepresentation normalize_param(FiniteLinearRepresentation& rep, double scale);
};

FiniteLinearRepresentation normalize_param(FiniteLinearRepresentation& rep, double scale=1.)
{
    VectorXd& param = rep.param;
    double param_norm = param.lpNorm<2>();
    param = param / param_norm * scale;

    auto& features = rep.features;
    for (auto& el : features)
    {
        el *= param_norm / scale;
    }

    return FiniteLinearRepresentation(features, param, rep.noise_std, rep.seed);
}

FiniteLinearRepresentation random_transform(FiniteLinearRepresentation& rep, bool normalize=true)
{
    int dim = rep.features_dim();
    static default_random_engine e(time(0));
    static normal_distribution<double> n(0,1);
    MatrixXd A = MatrixXd::Zero(dim,dim).unaryExpr([](double dummy)
    {
        return n(e);
    });

    if (dim > 1)
    {
        // get orthogonal matrix
        Eigen::ColPivHouseholderQR<MatrixXd> qr(A);
        A = qr.householderQ();
    }

    // Normalize columns
    A.colwise().normalize();
    A.rowwise().normalize();
    // cout << A.colwise().lpNorm<2>() << endl;
    // cout << A.rowwise().lpNorm<2>() << endl;
    //------------------
    std::vector<MatrixXd> features;
    for(auto& el : rep.features)
    {
        features.push_back(el * A);
    }

    assert(features.size() == rep.n_contexts());
    VectorXd param = A.transpose() * rep.param;

    FiniteLinearRepresentation r1(features, param, rep.noise_std, rep.seed);

    if (normalize)
        r1 = normalize_param(r1);
    return r1;

    VectorXd v1 = rep.rewards();
    VectorXd v2 = r1.rewards();
    assert(v2.isApprox(v1));
    return r1;
}

FiniteLinearRepresentation make_random(int n_contexts, int n_arms, int dim, bool normalize=false, double noise_std=0.1, int seed = 0)
{
    static default_random_engine e(time(0));
    static normal_distribution<double> n(0,1);

    std::vector<MatrixXd> features;
    for (int i = 0, ii=n_contexts; i< ii; ++i)
    {
        MatrixXd m = MatrixXd::Zero(n_arms,dim).unaryExpr([](double dummy)
        {
            return n(e);
        });
        features.push_back(m);
    }
    VectorXd param = VectorXd::Random(dim);
    param = 2*param.array()-1.;

    FiniteLinearRepresentation rep(features, param, noise_std, seed);
    if (normalize)
    {
        rep = normalize_param(rep);
    }
    return rep;
}


FiniteLinearRepresentation derank_hls(
    FiniteLinearRepresentation& rep, int newrank=1, bool random_seed=false,
    bool transform=true, bool normalize=true)
{
    int n_cont = rep.n_contexts();
    int d = rep.features_dim();
    int remove = min(max(n_cont - newrank + 1, 0), n_cont);
    std::vector<MatrixXd> f0 = rep.features;
    MatrixXd opt_feats(n_cont, d);
    VectorXd opt_rews(n_cont);
    std::vector<int> opt_arms(n_cont);
    rep.optimal_features_and_arms(opt_arms, opt_feats, opt_rews);

    MatrixXd m = opt_rews.head(remove) * opt_rews.head(remove).transpose();
    double n = opt_rews.head(remove).lpNorm<2>();
    MatrixXd xx = m * opt_feats.block(0,0,remove,d) / (n*n);
    for(int i = 0; i < remove; ++i)
    {
        for(int j=0; j < d; ++j)
        {
            f0[i](opt_arms[i], j) = xx(i,j);
        }
    }
    VectorXd param = rep.param;

    long seed = random_seed ? rand() : rep.seed;
    FiniteLinearRepresentation rep2(f0, param, rep.noise_std, seed);

    if (transform)
    {
        rep2 = random_transform(rep2, normalize);
    }
    else if (normalize)
    {
        rep2 = normalize_param(rep2);
    }


    return rep2;
}

#endif