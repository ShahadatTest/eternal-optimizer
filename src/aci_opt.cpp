#include "aci_opt.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace aci {

namespace {
double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }
}  // namespace

Optimizer::Optimizer(std::vector<Param> params, uint64_t seed, Options opt, const Memory* memory)
    : params_(std::move(params)), opt_(opt), rng_(seed), d_(static_cast<int>(params_.size())) {
    if (d_ == 0) throw std::invalid_argument("no parameters");
    if (memory && memory->studies > 0) {
        prior_.assign(d_, -1);
        prior_best_.assign(d_, 0.5);
        int known = 0;
        for (int i = 0; i < d_; ++i)
            if (const auto* e = memory->find(params_[i].name); e && e->seen > 0) {
                prior_[i] = e->cause_votes / e->seen >= 0.5 ? 1 : 0;
                prior_best_[i] = e->best_u;
                ++known;
            }
        if (known == 0) prior_.clear();
    }
    if (opt_.warmup <= 0) opt_.warmup = has_memory() ? std::max(4, d_ / 2) : std::max(10, 2 * d_);
    if (has_memory()) local_step_ = 0.05;
    verdict_.assign(d_, Verdict{});
    for (const auto& p : params_) { lo_.push_back(p.lo); hi_.push_back(p.hi); }
}

std::vector<double> Optimizer::uniform_point(std::mt19937_64& g) const {
    std::vector<double> x(d_);
    for (int i = 0; i < d_; ++i) x[i] = std::uniform_real_distribution<double>(params_[i].lo, params_[i].hi)(g);
    return x;
}

int Optimizer::active_count() const {
    int n = 0;
    for (const auto& v : verdict_) n += v.cause;
    return n;
}

// ------------------------------------------------------------------ ask / tell

std::vector<double> Optimizer::ask() {
    if (has_pending_) return pending_.x;
    const int T = static_cast<int>(trials_.size());
    if (!queue_.empty()) {
        pending_ = queue_.front();
        queue_.erase(queue_.begin());
    } else if (T < opt_.warmup) {
        const bool recall = has_memory() && std::ceil((T + 1) * opt_.recall_share) > std::ceil(T * opt_.recall_share);
        if (recall) {                                  // recall: start near what worked before
            auto x = uniform_point(rng_);
            std::normal_distribution<double> N(0.0, opt_.recall_spread);
            for (int i = 0; i < d_; ++i)
                if (prior_[i] >= 0) {
                    const double u = std::clamp(prior_best_[i] + (T == 0 ? 0.0 : N(rng_)), 0.0, 1.0);
                    x[i] = params_[i].lo + u * (params_[i].hi - params_[i].lo);
                }
            pending_ = {x, Kind::Recall, -1};
        } else {
            pending_ = {uniform_point(rng_), Kind::Warmup, -1};
        }
    } else if (!screened_) {
        std::vector<int> all(d_);
        std::iota(all.begin(), all.end(), 0);
        plan_screening(all);
        pending_ = queue_.front();
        queue_.erase(queue_.begin());
    } else {
        std::vector<int> frozen;
        for (int i = 0; i < d_; ++i) if (!verdict_[i].cause) frozen.push_back(i);
        if (!frozen.empty() && T - last_recheck_ >= opt_.recheck_every) {
            plan_screening(frozen);
            pending_ = queue_.front();
            queue_.erase(queue_.begin());
        } else {
            pending_ = propose();
        }
    }
    has_pending_ = true;
    return pending_.x;
}

void Optimizer::tell(double y) {
    if (!has_pending_) throw std::logic_error("tell() without ask()");
    has_pending_ = false;
    const double before = best_y_;
    const int id = static_cast<int>(trials_.size());
    trials_.push_back({id, pending_.x, y, pending_.kind, pending_.param});
    if (y < best_y_) {
        best_y_ = y;
        best_x_ = pending_.x;
        best_at_ = id;
    }
    if (id + 1 == opt_.warmup) {           // the spread of the warm-up sets the scale
        double m = 0, s = 0;
        for (const auto& t : trials_) m += t.y;
        m /= trials_.size();
        for (const auto& t : trials_) s += (t.y - m) * (t.y - m);
        scale_ = std::max(1e-12, std::sqrt(s / trials_.size()));
    }
    if (pending_.kind == Kind::Remeasure) base_ys_.push_back(y);
    if (pending_.kind == Kind::Local) {
        local_step_ = clampd(y < before ? local_step_ * 1.5 : local_step_ * 0.93, 1e-5, 0.3);
    }
    if (screening_ && queue_.empty()) finish_screening();
    drives(before, best_y_);
    if (screened_) {
        maybe_zoom();
        if (id - best_at_ >= opt_.stall_reset) {   // stuck: widen every active range again
            for (int i = 0; i < d_; ++i)
                if (verdict_[i].cause) { lo_[i] = params_[i].lo; hi_[i] = params_[i].hi; }
            events_.push_back({id, "expand", -1});
            C_ = std::min(1.0, C_ + 0.3);
            best_at_ = id;
        }
    }
}

// ------------------------------------------------------------------ 1-2. intervene, necessity

void Optimizer::plan_screening(const std::vector<int>& which, bool full) {
    screening_ = true;
    screen_params_ = which;
    screen_start_ = static_cast<int>(trials_.size());
    screen_base_x_ = best_x_;
    base_ys_ = {best_y_};
    const int remeasures = has_memory() && !full ? 3 : 5;
    for (int r = 0; r < remeasures; ++r) queue_.push_back({screen_base_x_, Kind::Remeasure, -1});
    for (int p : which) {
        const double span = params_[p].hi - params_[p].lo;
        // a remembered verdict needs one confirming intervention; anything else gets three
        if (!full && has_memory() && prior_[p] >= 0) {
            static const double fracs[3] = {0.3, -0.3, 0.15};
            for (int r = 0; r < std::clamp(opt_.recall_confirm, 1, 3); ++r) {
                auto x = screen_base_x_;
                double v = x[p] + fracs[r] * span;
                if (v > params_[p].hi || v < params_[p].lo) v = x[p] - fracs[r] * span;
                x[p] = clampd(v, params_[p].lo, params_[p].hi);
                queue_.push_back({x, Kind::Intervene, p});
            }
            continue;
        }
        for (double frac : {0.25, -0.25}) {
            auto x = screen_base_x_;
            double v = x[p] + frac * span;
            if (v > params_[p].hi || v < params_[p].lo) v = x[p] - frac * span;
            x[p] = clampd(v, params_[p].lo, params_[p].hi);
            queue_.push_back({x, Kind::Intervene, p});
        }
        auto x = screen_base_x_;
        x[p] = std::uniform_real_distribution<double>(params_[p].lo, params_[p].hi)(rng_);
        queue_.push_back({x, Kind::Intervene, p});
    }
    last_recheck_ = static_cast<int>(trials_.size());
}

void Optimizer::finish_screening() {
    screening_ = false;
    double m = 0;
    for (double v : base_ys_) m += v;
    m /= base_ys_.size();
    for (double v : base_ys_) sigma_samples_.push_back(v - m);
    ++sigma_batches_;
    double ss = 0;
    for (double v : sigma_samples_) ss += v * v;
    // pooled over every screening batch; one degree of freedom is spent on each batch's mean
    const double dof = std::max(1.0, double(sigma_samples_.size()) - sigma_batches_);
    sigma_ = std::sqrt(ss / dof);
    const double floor = 1e-9 * (std::abs(m) + scale_) + 1e-15;
    const double sig = std::max(sigma_, floor);
    const int T = static_cast<int>(trials_.size());
    // widen the bar while the noise estimate itself is uncertain (few degrees of freedom)
    const double bar = opt_.k_sigma * (1.0 + 2.0 / dof) * sig;
    const bool confirming = confirming_;
    confirming_ = false;
    std::vector<int> borderline;
    for (int p : screen_params_) {
        double effect = 0;
        for (int t = screen_start_; t < T; ++t)
            if (trials_[t].kind == Kind::Intervene && trials_[t].param == p) effect = std::max(effect, std::abs(trials_[t].y - m));
        Verdict& v = verdict_[p];
        // in a confirmation round a clear echo is enough: noise rarely fakes an effect twice
        const bool was = v.cause, cause = effect > (confirming ? 0.6 : 1.0) * bar;
        // a verdict that contradicts memory, or a cause that only just clears the bar, must reproduce
        // in a fresh, full round before it counts
        const bool contradicts = has_memory() && prior_[p] >= 0 && (prior_[p] == 1) != cause;
        if (!confirming && (contradicts || (cause && effect < 3.0 * bar))) {
            borderline.push_back(p);
            events_.push_back({T - 1, "doubt", p});
            continue;
        }
        v.tested = true;
        v.effect = effect;
        v.snr = effect / sig;
        v.checks += 1;
        v.cause = cause;
        if (was && !cause) {
            lo_[p] = hi_[p] = screen_base_x_[p];
            events_.push_back({T - 1, "freeze", p});
        } else if (!was && cause) {
            lo_[p] = params_[p].lo;
            hi_[p] = params_[p].hi;
            events_.push_back({T - 1, "revive", p});
        }
    }
    if (!borderline.empty()) {
        confirming_ = true;
        plan_screening(borderline, true);
        return;
    }
    if (!screened_ && has_memory() && !regions_from_memory_) {   // start zoomed on the remembered region
        for (int p = 0; p < d_; ++p) {
            if (!verdict_[p].cause || prior_[p] != 1) continue;
            const double span = params_[p].hi - params_[p].lo;
            const double c = std::clamp(best_x_[p], params_[p].lo, params_[p].hi);
            lo_[p] = std::max(params_[p].lo, c - opt_.recall_zoom * span);
            hi_[p] = std::min(params_[p].hi, c + opt_.recall_zoom * span);
        }
        regions_from_memory_ = true;
        events_.push_back({static_cast<int>(trials_.size()) - 1, "recall", -1});
    }
    screened_ = true;
}

// ------------------------------------------------------------------ 3. rules

double Optimizer::weight(int age) const { return std::pow(0.5, age / opt_.half_life); }

double Optimizer::threshold() const {
    std::vector<double> ys;
    ys.reserve(trials_.size());
    for (const auto& t : trials_) ys.push_back(t.y);
    if (ys.empty()) return 0;
    size_t k = static_cast<size_t>(std::floor(opt_.gamma * (ys.size() - 1)));
    std::nth_element(ys.begin(), ys.begin() + k, ys.end());
    return ys[k];
}

int Optimizer::bin_of(int p, double v) const {
    const double w = (hi_[p] - lo_[p]) / opt_.bins;
    if (w <= 0) return 0;
    return std::clamp(static_cast<int>((v - lo_[p]) / w), 0, opt_.bins - 1);
}

void Optimizer::counts(int p, std::vector<double>& s, std::vector<double>& f) const {
    s.assign(opt_.bins, 0.0);
    f.assign(opt_.bins, 0.0);
    const double q = threshold();
    const int T = static_cast<int>(trials_.size());
    for (int t = 0; t < T; ++t) {
        const double v = trials_[t].x[p];
        if (v < lo_[p] || v > hi_[p]) continue;
        const double w = weight(T - 1 - t);
        (trials_[t].y <= q ? s : f)[bin_of(p, v)] += w;
    }
}

namespace {
struct Stat { double m, v, n; int b; };
Stat beta_stat(double s, double f, int b) {
    const double n = s + f, m = (s + 1) / (n + 2);
    return {m, m * (1 - m) / (n + 3), n, b};
}
double z_of(std::vector<Stat> st) {
    if (st.size() < 2) return 0;
    std::sort(st.begin(), st.end(), [](const Stat& a, const Stat& b) { return a.m > b.m; });
    return (st[0].m - st[1].m) / std::sqrt(st[0].v + st[1].v + 1e-12);
}
}  // namespace

std::vector<BinRule> Optimizer::bin_rules() const {
    std::vector<BinRule> out;
    if (!screened_) return out;
    std::vector<double> s, f;
    for (int p = 0; p < d_; ++p) {
        if (!verdict_[p].cause) continue;
        counts(p, s, f);
        std::vector<Stat> st;
        for (int b = 0; b < opt_.bins; ++b) st.push_back(beta_stat(s[b], f[b], b));
        const double z = z_of(st);
        int best = 0;
        for (int b = 1; b < opt_.bins; ++b) if (st[b].m > st[best].m) best = b;
        const double w = (hi_[p] - lo_[p]) / opt_.bins;
        for (int b = 0; b < opt_.bins; ++b)
            out.push_back({p, b, lo_[p] + b * w, lo_[p] + (b + 1) * w, st[b].m, st[b].n, b == best ? z : 0.0, b == best});
    }
    return out;
}

std::vector<PairRule> Optimizer::pair_rules(int top) const {
    std::vector<PairRule> out;
    if (!screened_) return out;
    const double q = threshold();
    const int T = static_cast<int>(trials_.size()), B = opt_.bins;
    for (int i = 0; i < d_; ++i) {
        if (!verdict_[i].cause) continue;
        for (int j = i + 1; j < d_; ++j) {
            if (!verdict_[j].cause) continue;
            std::vector<double> s(B * B, 0.0), f(B * B, 0.0);
            for (int t = 0; t < T; ++t) {
                const auto& x = trials_[t].x;
                if (x[i] < lo_[i] || x[i] > hi_[i] || x[j] < lo_[j] || x[j] > hi_[j]) continue;
                const int c = bin_of(i, x[i]) * B + bin_of(j, x[j]);
                (trials_[t].y <= q ? s : f)[c] += weight(T - 1 - t);
            }
            std::vector<Stat> st;
            for (int c = 0; c < B * B; ++c) st.push_back(beta_stat(s[c], f[c], c));
            const double z = z_of(st);
            const auto it = std::max_element(st.begin(), st.end(), [](const Stat& a, const Stat& b) { return a.m < b.m; });
            out.push_back({i, it->b / B, j, it->b % B, it->m, it->n, z});
        }
    }
    std::sort(out.begin(), out.end(), [](const PairRule& a, const PairRule& b) { return a.z > b.z; });
    if (static_cast<int>(out.size()) > top) out.resize(top);
    return out;
}

// ------------------------------------------------------------------ 4. the gate

Optimizer::Plan Optimizer::propose_local() {
    auto x = best_x_;
    std::normal_distribution<double> N(0.0, 1.0);
    for (int p = 0; p < d_; ++p) {
        if (!verdict_[p].cause) continue;
        x[p] = clampd(x[p] + N(rng_) * local_step_ * (params_[p].hi - params_[p].lo), params_[p].lo, params_[p].hi);
    }
    return {x, Kind::Local, -1};
}

Optimizer::Plan Optimizer::propose() {
    ++proposals_;
    std::uniform_real_distribution<double> U(0.0, 1.0);
    if (U(rng_) < opt_.local_share) return propose_local();

    auto x = best_x_;
    const double eps = 0.15 + 0.5 * C_;
    const int B = opt_.bins;
    std::vector<int> bin(d_, -1);
    std::vector<double> zs(d_, 0.0);
    bool explored = false;
    double rho_sum = 0;
    int rho_n = 0;
    std::vector<double> s, f;
    for (int p = 0; p < d_; ++p) {
        if (!verdict_[p].cause) continue;
        counts(p, s, f);
        std::vector<Stat> st;
        for (int b = 0; b < B; ++b) st.push_back(beta_stat(s[b], f[b], b));
        zs[p] = z_of(st);
        const double rho = std::min(eps, 1.0 / (1.0 + zs[p] * zs[p]));
        rho_sum += rho;
        ++rho_n;
        if (U(rng_) > rho) {
            bin[p] = std::max_element(st.begin(), st.end(), [](const Stat& a, const Stat& b) { return a.m < b.m; })->b;
        } else {                                   // Thompson draw from each bin's posterior
            double best = -1;
            for (int b = 0; b < B; ++b) {
                std::gamma_distribution<double> ga(1 + s[b], 1.0), gb(1 + f[b], 1.0);
                const double a1 = ga(rng_), b1 = gb(rng_), draw = a1 / (a1 + b1);
                if (draw > best) { best = draw; bin[p] = b; }
            }
            explored = true;
        }
    }
    last_rho_ = rho_n ? rho_sum / rho_n : 0.0;

    // the most specific level wins: a pair rule overrides two single rules when its z is larger
    for (const auto& pr : pair_rules(3)) {
        if (pr.z > std::max(zs[pr.pi], zs[pr.pj]) && pr.n >= 4 && U(rng_) > std::min(eps, 1.0 / (1.0 + pr.z * pr.z))) {
            bin[pr.pi] = pr.bi;
            bin[pr.pj] = pr.bj;
        }
    }
    for (int p = 0; p < d_; ++p) {
        if (bin[p] < 0) continue;
        const double w = (hi_[p] - lo_[p]) / B, a = lo_[p] + bin[p] * w, b = a + w;
        if (best_x_[p] >= a && best_x_[p] <= b)
            x[p] = clampd(std::normal_distribution<double>(best_x_[p], w / 3)(rng_), a, b);
        else
            x[p] = std::uniform_real_distribution<double>(a, b)(rng_);
    }
    return {x, explored ? Kind::Explore : Kind::Exploit, -1};
}

// ------------------------------------------------------------------ 5. crystallise

void Optimizer::maybe_zoom() {
    const int T = static_cast<int>(trials_.size());
    if (T % 10 != 0) return;
    std::vector<double> s, f;
    for (int p = 0; p < d_; ++p) {
        if (!verdict_[p].cause) continue;
        counts(p, s, f);
        std::vector<Stat> st;
        for (int b = 0; b < opt_.bins; ++b) st.push_back(beta_stat(s[b], f[b], b));
        const double z = z_of(st);
        const auto best = *std::max_element(st.begin(), st.end(), [](const Stat& a, const Stat& b) { return a.m < b.m; });
        const double w = (hi_[p] - lo_[p]) / opt_.bins;
        const double a = lo_[p] + best.b * w, b = a + w;
        if (z < opt_.zoom_z || best.n < opt_.zoom_min_n) continue;
        if (best_x_[p] < a - w || best_x_[p] > b + w) continue;
        const double nlo = std::max(params_[p].lo, std::min(a, best_x_[p]) - w);
        const double nhi = std::min(params_[p].hi, std::max(b, best_x_[p]) + w);
        if (nhi - nlo > 0.7 * (hi_[p] - lo_[p]) || nhi - nlo < 1e-6 * (params_[p].hi - params_[p].lo)) continue;
        lo_[p] = nlo;
        hi_[p] = nhi;
        events_.push_back({T - 1, "zoom", p});
    }
}

// ------------------------------------------------------------------ 6. drives

void Optimizer::drives(double before, double after) {
    const double g = 10.0 * std::max(0.0, (before < 1e299 ? before - after : 0.0) / scale_);
    C_ += 0.1 * (g - C_) - 0.05 * C_ * eta_;
    eta_ += 0.1 * (1.0 - eta_);
    const double q = threshold();
    int n = 0, ok = 0;
    for (int t = static_cast<int>(trials_.size()) - 1; t >= 0 && n < 20; --t, ++n) ok += trials_[t].y <= q;
    const double dkl = 1.0 - (n ? double(ok) / n : 0.0);
    kappa_ += 0.1 * (1.0 - dkl) - 0.05 * kappa_;
    C_ = clampd(C_, 0, 1);
    eta_ = clampd(eta_, 0, 1);
    kappa_ = clampd(kappa_, 0, 1);
}

// ------------------------------------------------------------------ memory

const Memory::Entry* Memory::find(const std::string& name) const {
    for (const auto& e : entries) if (e.name == name) return &e;
    return nullptr;
}

std::string Memory::save() const {
    std::string out = "studies " + std::to_string(studies) + "\n";
    char buf[64];
    for (const auto& e : entries) {
        std::snprintf(buf, sizeof buf, "\t%.17g\t%.17g\t%.17g\n", e.cause_votes, e.seen, e.best_u);
        out += e.name + buf;
    }
    return out;
}

Memory Memory::load(const std::string& text) {
    Memory m;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(pos, end - pos);
        pos = end + 1;
        if (line.rfind("studies ", 0) == 0) { m.studies = std::atoi(line.c_str() + 8); continue; }
        const size_t a = line.find('\t');
        if (a == std::string::npos) continue;
        Entry e;
        e.name = line.substr(0, a);
        std::sscanf(line.c_str() + a + 1, "%lf\t%lf\t%lf", &e.cause_votes, &e.seen, &e.best_u);
        m.entries.push_back(e);
    }
    return m;
}

Memory Optimizer::remember(const Memory* previous, double forget_half_life) const {
    Memory m = previous ? *previous : Memory{};
    if (trials_.empty()) return m;
    if (forget_half_life > 0) {                      // older studies fade: each counts half after h studies
        const double keep = std::pow(0.5, 1.0 / forget_half_life);
        for (auto& e : m.entries) { e.cause_votes *= keep; e.seen *= keep; }
    }
    for (int i = 0; i < d_; ++i) {
        if (!verdict_[i].tested) continue;
        Memory::Entry* e = nullptr;
        for (auto& x : m.entries) if (x.name == params_[i].name) e = &x;
        if (!e) { m.entries.push_back({params_[i].name}); e = &m.entries.back(); e->best_u = 0; }
        const double u = (best_x_[i] - params_[i].lo) / (params_[i].hi - params_[i].lo);
        if (verdict_[i].cause) e->best_u = (e->best_u * e->cause_votes + u) / (e->cause_votes + 1);
        e->cause_votes += verdict_[i].cause;
        e->seen += 1;
    }
    m.studies += 1;
    return m;
}

// ------------------------------------------------------------------ random baseline

std::vector<double> RandomSearch::ask() {
    last_.resize(params_.size());
    for (size_t i = 0; i < params_.size(); ++i)
        last_[i] = std::uniform_real_distribution<double>(params_[i].lo, params_[i].hi)(rng_);
    return last_;
}

void RandomSearch::tell(double y) {
    best_ = std::min(best_, y);
    hist_.push_back(best_);
}

}  // namespace aci
