// Eternal optimizer: an ACI-style causal optimization loop.
//
// Black-box minimisation over a box of continuous parameters, driven by ask/tell.
// The loop follows ACI:
//   1. intervene  - around the incumbent, set one parameter at a time and re-measure the
//                   incumbent to learn the noise;
//   2. necessity  - a parameter whose interventions never move the objective beyond the
//                   noise is not a cause; it is frozen at the incumbent's value;
//   3. rules      - each active parameter's range is split into bins (tokens); a trial is a
//                   success when it lands in the best gamma quantile; every token and every
//                   pair of tokens keeps decayed Beta counts;
//   4. gate       - per parameter, exploit the best bin with probability 1 - rho,
//                   rho = min(eps, 1 / (1 + z^2)), eps = 0.15 + 0.5 C(t), otherwise sample a
//                   bin from its Beta posterior; a pair rule with a larger z wins;
//   5. crystallise - when a bin is clearly best, zoom the parameter's range onto it;
//   6. recheck    - frozen parameters are tested again from time to time.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace aci {

struct Param {
    std::string name;
    double lo, hi;
};

enum class Kind { Warmup, Remeasure, Intervene, Exploit, Explore, Local, Random, Recall };

struct Trial {
    int id;
    std::vector<double> x;
    double y;
    Kind kind;
    int param = -1;          // the parameter intervened on, if any
};

struct Verdict {             // the outcome of the causal screening of one parameter
    bool tested = false;
    bool cause = true;       // active (true) or frozen (false)
    double effect = 0.0;     // largest |change| seen under intervention
    double snr = 0.0;        // effect / noise
    int checks = 0;
};

struct BinRule {             // what the rule table knows about one token
    int param, bin;
    double lo, hi, mean, n, z;
    bool best;
};

struct PairRule {
    int pi, bi, pj, bj;
    double mean, n, z;
};

struct Event {               // a crystallisation, freeze, revival or expansion, for the log
    int trial;
    std::string what;        // freeze, revive, doubt, zoom or expand; the UI translates it
    int param;
};

// What past studies taught: per parameter name, how often it proved a cause and where the best
// value sat (as a share of its range). Memory only proposes; every remembered verdict is
// re-tested by intervention in the new study, and a contradiction triggers the full test.
struct Memory {
    struct Entry { std::string name; double cause_votes = 0, seen = 0, best_u = 0.5; };
    std::vector<Entry> entries;
    int studies = 0;
    const Entry* find(const std::string& name) const;
    std::string save() const;                       // a small text format, one parameter per line
    static Memory load(const std::string& text);
};

struct Options {
    int warmup = 0;          // 0: max(10, 2 d)
    int bins = 6;
    double gamma = 0.25;     // success = best gamma share of trials
    double half_life = 64;   // trials
    double k_sigma = 4.0;    // necessity threshold in noise units
    double zoom_z = 2.0;
    double zoom_min_n = 6;
    int recheck_every = 80;
    int stall_reset = 60;
    double local_share = 0.35;
    // how memory is used (only when a Memory is passed in)
    double recall_share = 0.5;    // share of warm-up trials placed near the remembered best
    double recall_spread = 0.08;  // their spread, as a share of each range
    double recall_zoom = 0.2;     // starting search range: remembered best +- this share of the range
    int recall_confirm = 1;       // confirming interventions per remembered knob (a fresh knob gets 3)
};

class Optimizer {
public:
    Optimizer(std::vector<Param> params, uint64_t seed, Options opt = {}, const Memory* memory = nullptr);

    // Fold this study into a memory (pass the previous memory to accumulate across studies).
    // forget_half_life: in studies; older studies count half as much after this many (0: never forget)
    Memory remember(const Memory* previous = nullptr, double forget_half_life = 0) const;
    bool has_memory() const { return !prior_.empty(); }

    std::vector<double> ask();
    void tell(double y);

    // read-only views for the UI and tests
    const std::vector<Param>& params() const { return params_; }
    const std::vector<Trial>& trials() const { return trials_; }
    const std::vector<Verdict>& verdicts() const { return verdict_; }
    const std::vector<Event>& events() const { return events_; }
    std::vector<BinRule> bin_rules() const;
    std::vector<PairRule> pair_rules(int top = 8) const;
    const std::vector<double>& incumbent() const { return best_x_; }
    double best() const { return best_y_; }
    double noise() const { return sigma_; }
    double curiosity() const { return C_; }
    double last_rho() const { return last_rho_; }
    std::vector<double> region_lo() const { return lo_; }
    std::vector<double> region_hi() const { return hi_; }
    int active_count() const;
    bool screened() const { return screened_; }
    Kind pending_kind() const { return pending_.kind; }

private:
    struct Plan { std::vector<double> x; Kind kind; int param; };

    std::vector<double> uniform_point(std::mt19937_64& g) const;
    void plan_screening(const std::vector<int>& which, bool full = false);
    void finish_screening();
    Plan propose();
    Plan propose_local();
    void counts(int p, std::vector<double>& s, std::vector<double>& f) const;
    double threshold() const;
    double weight(int age) const;
    int bin_of(int p, double v) const;
    void maybe_zoom();
    void drives(double before, double after);

    std::vector<Param> params_;
    Options opt_;
    std::mt19937_64 rng_;
    int d_;

    std::vector<Trial> trials_;
    std::vector<Plan> queue_;
    Plan pending_{};
    bool has_pending_ = false;

    std::vector<double> best_x_;
    double best_y_ = 1e300;
    int best_at_ = 0;

    // screening
    bool screened_ = false;
    bool screening_ = false;
    std::vector<int> screen_params_;
    int screen_start_ = 0;
    bool confirming_ = false;
    std::vector<double> base_ys_;
    std::vector<double> screen_base_x_;
    std::vector<double> sigma_samples_;
    int sigma_batches_ = 0;
    double sigma_ = 0.0;
    int last_recheck_ = 0;
    std::vector<Verdict> verdict_;

    // the rule region (trust region) per parameter
    std::vector<double> lo_, hi_;

    // drives (ACI's homeostatic state)
    double C_ = 0.5, eta_ = 0.1, kappa_ = 0.9;
    double scale_ = 1.0;
    double local_step_ = 0.1;

    // memory: per parameter, the remembered verdict (-1 unknown, 0 irrelevant, 1 cause) and best value
    std::vector<int> prior_;
    std::vector<double> prior_best_;
    bool regions_from_memory_ = false;
    double last_rho_ = 0.0;
    int proposals_ = 0;

    std::vector<Event> events_;
};

// Pure random search, the baseline every result is compared against.
class RandomSearch {
public:
    RandomSearch(std::vector<Param> params, uint64_t seed) : params_(std::move(params)), rng_(seed) {}
    std::vector<double> ask();
    void tell(double y);
    double best() const { return best_; }
    const std::vector<double>& history() const { return hist_; }
private:
    std::vector<Param> params_;
    std::mt19937_64 rng_;
    std::vector<double> last_;
    double best_ = 1e300;
    std::vector<double> hist_;
};

}  // namespace aci
