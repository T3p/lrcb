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
#include "leader.h"
#include "regbalancing.h"
#include "adversarial_master.h"
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

    // load reference representation
    // FiniteLinearRepresentation reference_rep=flr_loadjson("linrep3.json", noise_std, seed);
    FiniteLinearRepresentation reference_rep=flr_loadnpz("../../problem_data/jester/33/1/jester_post_d33_span33.npz", noise_std, seed, "features", "theta");
    cout << "Ref_rep.dim: " << reference_rep.features_dim() << endl;
    cout << "Ref_rep.feat_bound=" << reference_rep.features_bound() << endl;

    // other representations
    std::vector<FiniteLinearRepresentation> reps;
    for (int i = 0; i < 7 + n_derank; ++i) {

        int l = i;
	if(i >= 7) {
		l = 7 + 6 - i;
	}
	FiniteLinearRepresentation rr = flr_loadnpz(files[l], noise_std, seed, "features", "theta");

	if(i >= 7) {
		rr = derank_hls(rr, low_rank, false, true, true);
	}
        //rr.normalize_features(10);

        cout << "phi_" << i << ".dim=" << rr.features_dim() << endl;
        cout << "phi_" << i << ".feat_bound=" << rr.features_bound() << endl;
        bool flag = reference_rep.is_equal(rr, 0.05);
        cout << "phi_" << i << ".equal_ref=" << flag << endl;
        if (!flag) {
            std::cout << "Error: " << i << "is a non realizable representation" << std::endl;
            exit(1);
        }
        reps.push_back(rr);
    }

    // add also reference representation
    // reps.push_back(reference_rep);

    vec2double regrets, pseudo_regrets;
    #pragma omp parallel for
    for (int i = 0; i < n_runs; ++i)
    {
        std::vector<std::shared_ptr<ContRepresentation<int>>> lreps;
        for(auto& ll : reps)
        {
            auto tmp = std::make_shared<FiniteLinearRepresentation>(ll.copy(seeds[i]));
            lreps.push_back(tmp);
        }
        LEADER<int> localg(lreps, reg_val, noise_std, bonus_scale, delta/lreps.size(), adaptive_ci);

        // create same representation but witth different seed
        FiniteLinearRepresentation cpRefRep = reference_rep.copy(seeds[i]);
        ContBanditProblem<int> prb(cpRefRep, localg);
        prb.reset();
        auto start = TIC();
        prb.run(T);
        auto tottime = TOC(start);
        cout << "time(" << i << "): " << tottime << endl;
        regrets.push_back(prb.instant_regret);
        pseudo_regrets.push_back(prb.exp_instant_regret);

        save_vector_csv_gzip(regrets, "LEADER-"+std::string(MY_TIME)+"_regrets.csv.gz", EVERY, PREC);
        save_vector_csv_gzip(pseudo_regrets, "LEADER-"+std::string(MY_TIME)+"_pseudoregrets.csv.gz", EVERY, PREC);
    }

    return 0;
}
