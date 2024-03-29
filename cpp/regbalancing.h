#ifndef OFULBAL_H
#define OFULBAL_H

#include <Eigen/Dense>
#include "abstractclasses.h"
#include <algorithm>

using namespace std;
using namespace Eigen;

/**
 * Implementation of bandit model selection by regret balancing from
 *
 * Yasin Abbasi-Yadkori, Aldo Pacchiano, My Phan:
 * Regret Balancing for Bandit and RL Model Selection
 * https://arxiv.org/abs/2006.05491
 *
 * This is a specific implementation for selecting the representation of
 * OFUL algorithm.
 */
template<typename X>
class RegretBalance : public Algo<X>
{
public:

    RegretBalance(
        std::vector<std::shared_ptr<Algo<X>>>& base_algs,
        bool update_all=false
    )
        : Algo<X>("RegretBalance"), base_algs(base_algs), update_all(update_all)
    {
        reset();
    }
    ~RegretBalance() {}

    void reset()
    {
        // reset all base algs
        for(auto& ba : base_algs)
        {
            ba->reset();
        }
        cum_rewards.resize(base_algs.size());
        std::fill(cum_rewards.begin(), cum_rewards.end(), 0);
        num_selection.resize(base_algs.size());
        std::fill(num_selection.begin(), num_selection.end(), 0);
        t = 1;
        // all the representations are active [0, M]
        active_reps.resize(this->base_algs.size());
        std::iota(active_reps.begin(), active_reps.end(), 0);
    }

    double _upper_bound(int i)
    {
        // if (this->base_algs[i]->name == "OFUL") {
        //     std::shared_ptr<OFUL<X>> tmp = std::dynamic_pointer_cast<OFUL<X>>(base_algs[i]);
        //     return -log(tmp->inv_A.determinant() * tmp->delta) * sqrt(this->num_selection[i]);
        // }
        return base_algs[i]->upper_bound();
    }

    int action(const X& context)
    {
        // int M = base_algs.size();
        if (this->active_reps.size() == 0) {
            std::cout << "!!! No active representation" << std::endl;
            throw(this->active_reps.size());
        }

        // compute optimistic base algorithm
        int opt_base = 0;
        double opt_value = std::numeric_limits<double>::min();
        for (int i : active_reps)
        {
            if (num_selection[i] == 0) {
                last_selected_algo = i;
                t++;
                return base_algs[last_selected_algo]->action(context);
            }

            double ub = _upper_bound(i);
            double u = (cum_rewards[i] + ub) / num_selection[i];
            if (u > opt_value)
            {
                opt_value = u;
                opt_base = i;
            }
        }

        double min_empreg = std::numeric_limits<double>::max();
        for (int i : active_reps)
        {
            double emp_regret = num_selection[i] * opt_value - cum_rewards[i];
            if (emp_regret < min_empreg)
            {
                last_selected_algo = i;
                min_empreg = emp_regret;
            }
        }

        double action = base_algs[last_selected_algo]->action(context);
        t++;
        return action;
    }

    std::vector<double> action_distribution(const X& context)
    {
        int n_arms = base_algs.size();
        std::vector<double> proba(n_arms);
        proba[action(context)] = 1;
        return proba;
    }

    void update(const X& context, int action, double reward)
    {
        if (update_all)
        {
            for (int i : active_reps)
            {
                base_algs[i]->update(context, action, reward);
            }
        }
        else
        {
            base_algs[last_selected_algo]->update(context, action, reward);
        }
        // we update number of selection and cumulative rewards only
        // for the selected algorithm
        num_selection[last_selected_algo]++;
        cum_rewards[last_selected_algo] += reward;
    }

public:
    std::vector<std::shared_ptr<Algo<X>>>& base_algs;
    std::vector<double> cum_rewards;
    std::vector<int> num_selection;
    double t;
    int last_selected_algo;
    std::vector<int> active_reps;
    bool update_all;
};

/**
 * Implementation of bandit model selection by regret balancing
 * and elimination from
 *
 * Aldo Pacchiano, Christoph Dann, Claudio Gentile, Peter Bartlett:
 * Regret Bound Balancing and Elimination for Model Selection in Bandits and RL
 * https://arxiv.org/abs/2012.13045
 *
 * This is a specific implementation for selecting the representation of
 * OFUL algorithm.
 */
template<typename X>
class RegretBalanceAndEliminate : public RegretBalance<X>
{
public:
    RegretBalanceAndEliminate(
        std::vector<std::shared_ptr<Algo<X>>>& base_algs,
        double delta, bool update_all=false
    )
        : RegretBalance<X>(base_algs, update_all), delta(delta)
    {
        this->name = "RegretBalanceAndEliminate";
    }
    ~RegretBalanceAndEliminate() {}

    int action(const X& context)
    {
        //select base learner
        double min_ub = std::numeric_limits<double>::max();
        for(int i : this->active_reps)
        {
            double ub = this->_upper_bound(i);
            if (ub < min_ub)
            {
                this->last_selected_algo = i;
                min_ub = ub;
            }
        }
        double action = this->base_algs[this->last_selected_algo]->action(context);
        this->t++;
        return action;
    }

    void update(const X& context, int action, double reward)
    {
        RegretBalance<X>::update(context, action, reward);
        double delta = this->delta;

        double XXX = 1., M = this->base_algs.size();
        //eliminate representation
        double max_value = std::numeric_limits<double>::min();
        for(int i : this->active_reps)
        {
            // wait until all the base algorithms have been pulled at least once
            if (this->num_selection[i] == 0) {return;}
            
            double N = this->num_selection[i];
            double value = this->cum_rewards[i] / N - XXX * sqrt(log(M * log(N/delta))/ N);
            if (max_value < value)
            {
                max_value = value;
            }
        }
        std::vector<int> new_active_reps;
        for(int i : this->active_reps)
        {
            double N = this->num_selection[i];
            double ub = this->_upper_bound(i);
            double lhs = this->cum_rewards[i] / N + XXX * sqrt(log(M * log(N/delta))/ N) + ub / N;
            if (lhs >= max_value)
            {
                new_active_reps.push_back(i);
            }
            else
            {
                cout << "t" << this->t <<": eliminated " << i << " since " << lhs << " < " << max_value << endl;
            }
        }
        if (new_active_reps.size() != this->active_reps.size())
        {
            std::cout << "new active set: [ ";
            for(auto& u : new_active_reps)
            {
                std::cout << u << " ";
            }
            std::cout << " ]\n" << std::endl;
        }
        this->active_reps = new_active_reps;
    }

public:
    double delta;
};
#endif
