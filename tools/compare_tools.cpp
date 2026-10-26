// Helpers for the head-to-head comparison (compare/run_compare.py).
//   eternal_compare eval <problem>                         read points from stdin, print f(x) per line
//   eternal_compare sigma <problem> <noise_frac>           the noise sd that noise_frac means for this problem
//   eternal_compare run <problem> <seed> <budget> <noise_frac>
//        one Eternal run; prints JSON {"gap": [...]}: after each trial, the true gap to the optimum of
//        the point with the best observed (noisy) value so far - the same scoring every method gets.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "aci_opt.hpp"
#include "problems.hpp"

static double spread(const aci::Problem& p) {
    aci::RandomSearch r(p.params, 999);
    std::vector<double> ys;
    for (int i = 0; i < 400; ++i) ys.push_back(p.f(r.ask()));
    double m = 0, s = 0;
    for (double y : ys) m += y;
    m /= ys.size();
    for (double y : ys) s += (y - m) * (y - m);
    return std::sqrt(s / ys.size());
}

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: see header\n"); return 2; }
    const std::string cmd = argv[1];
    const auto probs = aci::problems();
    const auto& p = probs.at(std::atoi(argv[2]));
    if (cmd == "eval") {
        std::string line;
        while (std::getline(std::cin, line)) {
            std::istringstream in(line);
            std::vector<double> x;
            double v;
            while (in >> v) x.push_back(v);
            if (x.size() == p.params.size()) std::printf("%.17g\n", p.f(x));
        }
        return 0;
    }
    if (cmd == "sigma") {
        std::printf("%.17g\n", std::atof(argv[3]) * spread(p));
        return 0;
    }
    if (cmd == "run") {
        const int seed = std::atoi(argv[3]), budget = std::atoi(argv[4]);
        const double sigma = std::atof(argv[5]) * spread(p);
        aci::Optimizer opt(p.params, seed);
        aci::Noisy f(p, sigma, 31337 + seed);
        double best = 1e300, gap = 0;
        std::printf("{\"gap\": [");
        for (int t = 0; t < budget; ++t) {
            auto x = opt.ask();
            const double y = f(x);
            opt.tell(y);
            if (y < best) { best = y; gap = p.f(x) - p.optimum; }
            std::printf("%s%.17g", t ? ", " : "", gap);
        }
        std::printf("]}\n");
        return 0;
    }
    return 2;
}
