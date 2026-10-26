// Headless benchmark: the ACI loop against random search, many seeds, same budget.
// Reports the gap to the known optimum (true function value of the best point found).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "aci_opt.hpp"
#include "problems.hpp"

int main(int argc, char** argv) {
    const int seeds = argc > 1 ? std::atoi(argv[1]) : 20;
    const int budget = argc > 2 ? std::atoi(argv[2]) : 200;
    std::printf("%-26s %6s %5s  %12s %12s  %6s  %s\n", "problem", "noise", "seeds", "ACI gap", "random gap", "wins", "decoys frozen");
    for (const auto& prob : aci::problems()) {
        for (double noise_frac : {0.0, 0.02}) {
            // noise as a share of the function's spread over random points
            double spread;
            {
                aci::RandomSearch r(prob.params, 999);
                std::vector<double> ys;
                for (int i = 0; i < 400; ++i) ys.push_back(prob.f(r.ask()));
                double m = 0, s = 0;
                for (double y : ys) m += y;
                m /= ys.size();
                for (double y : ys) s += (y - m) * (y - m);
                spread = std::sqrt(s / ys.size());
            }
            const double sigma = noise_frac * spread;
            std::vector<double> ga, gr;
            int wins = 0, frozen_ok = 0, wrong_frozen = 0;
            for (int s = 0; s < seeds; ++s) {
                aci::Optimizer opt(prob.params, 1000 + s);
                aci::RandomSearch rnd(prob.params, 5000 + s);
                aci::Noisy fa(prob, sigma, 7000 + s), fr(prob, sigma, 9000 + s);
                std::vector<double> best_xa, best_xr;
                double ya = 1e300, yr = 1e300;
                for (int t = 0; t < budget; ++t) {
                    auto xa = opt.ask(); double va = fa(xa); opt.tell(va);
                    if (va < ya) { ya = va; best_xa = xa; }
                    auto xr = rnd.ask(); double vr = fr(xr); rnd.tell(vr);
                    if (vr < yr) { yr = vr; best_xr = xr; }
                }
                const double a = prob.f(best_xa) - prob.optimum, r = prob.f(best_xr) - prob.optimum;
                ga.push_back(a); gr.push_back(r);
                wins += a < r;
                bool ok = true;
                for (size_t i = 0; i < prob.params.size(); ++i) {
                    const bool frozen = !opt.verdicts()[i].cause;
                    if (!prob.relevant[i] && !frozen) ok = false;
                    if (prob.relevant[i] && frozen) ++wrong_frozen;
                }
                frozen_ok += ok;
            }
            std::sort(ga.begin(), ga.end()); std::sort(gr.begin(), gr.end());
            std::printf("%-26s %6.0f%% %5d  %12.4g %12.4g  %3d/%-2d  %d/%d (real causes frozen: %d)\n",
                        prob.name.c_str(), noise_frac * 100, seeds, ga[seeds / 2], gr[seeds / 2], wins, seeds,
                        frozen_ok, seeds, wrong_frozen);
        }
    }
    return 0;
}
