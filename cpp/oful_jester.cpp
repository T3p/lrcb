// standard includes and external libraries
#include <iostream>
#include <random>
#include <Eigen/Dense>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <ctime>
// project includes
#include "abstractclasses.h"
#include "bandit.h"
#include "oful.h"
#include "finitelinrep.h"
#include "utils.h"
#include "gzip.h"

using json = nlohmann::json;
using namespace std;
using namespace Eigen;

size_t PREC = 4;   // for saving numbers are rounded to PREC decimals
size_t EVERY = 1;  // save EVERY round

int main()
{

    int n_derank = 0;
    int low_rank = 15;

    const char *files[7] = { "../../problem_data/jester/33/1/jester_post_d33_span33.npz", "../../problem_data/jester/33/1/jester_post_d26_span26.npz", "../../problem_data/jester/33/1/jester_post_d24_span24.npz", "../../problem_data/jester/33/1/jester_post_d23_span23.npz", "../../problem_data/jester/33/1/jester_post_d20_span20.npz", "../../problem_data/jester/33/1/jester_post_d17_span17.npz", "../../problem_data/jester/33/1/jester_post_d16_span16.npz" };

    const char *names[9] = {"d=33", "d=26", "d=24", "d=23", "d=20", "d=17", "d=16", "d=16 (derank)", "d=17 (derank)"};

    std::time_t t = std::time(nullptr);
    char MY_TIME[100];
    std::strftime(MY_TIME, sizeof(MY_TIME), "%Y%m%d%H%M%S", std::localtime(&t));
    std::cout << MY_TIME << '\n';

    typedef std::vector<std::vector<double>> vec2double;

    int seed = time(NULL);
    srand (seed);
    cout << "seed: " << seed << endl;
    int n_runs = 50, T = 1000000;
    double delta = 0.01;
    double reg_val = 1.;
    double noise_std = 1.0;
    double bonus_scale = 1.;
    bool adaptive_ci = true;

    std::vector<long> seeds(n_runs);
    std::generate(seeds.begin(), seeds.end(), [] ()
    {
        return rand();
    });

    for (int j = 0; j < 7 + n_derank; j++) {

    // load reference representation

    auto start = TIC();
    // FiniteLinearRepresentation reference_rep=flr_loadjson("jester_post_d33_span33.json", noise_std, seed);
    FiniteLinearRepresentation reference_rep=flr_loadnpz("../../problem_data/jester/33/1/jester_post_d33_span33.npz", noise_std, seed,"features", "theta");
    auto tottime = TOC(start);
    int reference_rep_dim = reference_rep.features_dim();
    cout << "Loaded in " << tottime << endl;
    cout << "Ref_rep.dim: " << reference_rep_dim << endl;

    // load representation for OFUL
    start = TIC();
    // FiniteLinearRepresentation oful_rep = flr_loadjson("A.json", noise_std, seed);

    int l = j;
    if(j >= 7) {
	l = 7 + 6 - j;
    }
    FiniteLinearRepresentation oful_rep = flr_loadnpz(files[l], noise_std, seed, "features", "theta");

    if(j >= 7) {
        oful_rep = derank_hls(oful_rep, low_rank, false, true, true);
    }
    //oful_rep.normalize_features(10);

    tottime = TOC(start);
    int oful_rep_dim = oful_rep.features_dim();
    cout << "Loaded in " << tottime << endl;
    cout << "OFUL_rep.dim: " << oful_rep_dim << endl;


    cout << "Equal? " << reference_rep.is_equal(oful_rep, 1e-3) << endl;

    //just OFUL
    vec2double regrets, pseudo_regrets;

    #pragma omp parallel for
    for (int i = 0; i < n_runs; ++i)
    {
        OFUL<int> localg(oful_rep, reg_val, noise_std, bonus_scale, delta, adaptive_ci);
        // create same representation but with different seed
        FiniteLinearRepresentation lrep = reference_rep.copy(seeds[i]);
        ContBanditProblem<int> prb(lrep, localg);
        prb.reset();
        auto start = TIC();
        prb.run(T);
        auto tottime = TOC(start);
        cout << "time(" << i << "): " << tottime << endl;

        // store regret and pseudo regret
        regrets.push_back(prb.instant_regret);
        pseudo_regrets.push_back(prb.exp_instant_regret);

        // save in compressed json
        save_vector_csv_gzip(regrets, "OFUL-"+std::string(names[j])+"_regrets.csv.gz", EVERY, PREC);
        save_vector_csv_gzip(pseudo_regrets, "OFUL-"+std::string(names[j])+"_pseudoregrets.csv.gz", EVERY, PREC);
    }
    }

    return 0;
}
