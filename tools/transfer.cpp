// Transfer across related tasks: a family of ramen shops.
// Every shop has the same eight knobs. Each shop's ideal recipe is shifted (up to +-0.12 of each
// relevant knob's range) and its sensitivities scaled (x0.7 to x1.3). The weather and the owner's
// mood never matter. Shop index 5 is a trap: there the day of the week DOES matter.
//
//   eternal_transfer eval <family> <shop>                       points on stdin -> f per line
//   eternal_transfer sigma <family> <shop> <noise_frac>
//   eternal_transfer run <family> <shops> <budget> <memory 0|1> <noise_frac> <seed>
//        Eternal over the shops in order, with or without memory carried between them.
//        JSON: per shop, the gap curve and whether the screening matched the truth.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "aci_opt.hpp"
#include "problems.hpp"

struct Shop {
    std::vector<aci::Param> params;
    std::vector<double> centre, weight;
    bool trap;
    double f(const std::vector<double>& x) const {
        std::vector<double> u(8);
        for (int i = 0; i < 8; ++i) u[i] = (x[i] - params[i].lo) / (params[i].hi - params[i].lo);
        double taste = 100 - weight[0] * std::pow(u[0] - centre[0], 2) - weight[1] * std::pow(u[1] - centre[1], 2)
                     - weight[2] * std::pow(u[2] - centre[2] - 0.3 * (u[3] - centre[3]), 2)
                     - weight[3] * std::pow(u[3] - centre[3], 2) - weight[4] * std::pow(u[4] - centre[4], 2);
        if (trap) taste -= 40 * std::pow(u[6] - 0.2, 2);
        return -taste;
    }
    std::vector<bool> relevant() const { return {true, true, true, true, true, false, trap, false}; }
};

static Shop make_shop(int family, int index) {
    Shop s;
    s.params = aci::problems()[0].params;
    std::mt19937_64 g(1000003ULL * family + index);
    std::uniform_real_distribution<double> off(-0.12, 0.12), sc(0.7, 1.3);
    const double base_c[5] = {0.55, 0.8, 0.45, 0.4, 0.35}, base_w[5] = {60, 80, 120, 30, 20};
    for (int i = 0; i < 5; ++i) {
        s.centre.push_back(std::clamp(base_c[i] + off(g), 0.05, 0.95));
        s.weight.push_back(base_w[i] * sc(g));
    }
    s.trap = index == 5;
    return s;
}

static double spread(const Shop& s) {
    aci::RandomSearch r(s.params, 999);
    double m = 0, q = 0;
    std::vector<double> ys;
    for (int i = 0; i < 400; ++i) ys.push_back(s.f(r.ask()));
    for (double y : ys) m += y;
    m /= ys.size();
    for (double y : ys) q += (y - m) * (y - m);
    return std::sqrt(q / ys.size());
}

int main(int argc, char** argv) {
    if (argc < 4) return 2;
    const std::string cmd = argv[1];
    const int family = std::atoi(argv[2]);
    if (cmd == "eval") {
        const Shop s = make_shop(family, std::atoi(argv[3]));
        std::string line;
        while (std::getline(std::cin, line)) {
            std::istringstream in(line);
            std::vector<double> x;
            double v;
            while (in >> v) x.push_back(v);
            if (x.size() == 8) std::printf("%.17g\n", s.f(x)); else std::printf("nan\n");
            std::fflush(stdout);
        }
        return 0;
    }
    if (cmd == "sigma") {
        std::printf("%.17g\n", std::atof(argv[4]) * spread(make_shop(family, std::atoi(argv[3]))));
        return 0;
    }
    if (cmd == "run") {
        const int shops = std::atoi(argv[3]), budget = std::atoi(argv[4]), use_memory = std::atoi(argv[5]);
        const double noise = std::atof(argv[6]);
        const int seed = std::atoi(argv[7]);
        aci::Memory memory;
        std::printf("{\"shops\": [");
        for (int j = 0; j < shops; ++j) {
            const Shop s = make_shop(family, j);
            const double sigma = noise * spread(s);
            std::mt19937_64 g(777 + 13 * seed + j);
            std::normal_distribution<double> N(0, 1);
            aci::Optimizer opt(s.params, seed * 100 + j, {}, use_memory ? &memory : nullptr);
            double best = 1e300, gap = 0;
            std::printf("%s{\"gap\": [", j ? ", " : "");
            for (int t = 0; t < budget; ++t) {
                auto x = opt.ask();
                const double y = s.f(x) + (sigma > 0 ? sigma * N(g) : 0.0);
                opt.tell(y);
                if (y < best) { best = y; gap = s.f(x) + 100.0; }
                std::printf("%s%.17g", t ? ", " : "", gap);
            }
            const auto rel = s.relevant();
            int wrong = 0;
            for (int i = 0; i < 8; ++i) wrong += opt.verdicts()[i].tested && (opt.verdicts()[i].cause != rel[i]);
            std::printf("], \"screening_errors\": %d, \"day_of_week_cause\": %s}", wrong,
                        opt.verdicts()[6].cause ? "true" : "false");
            if (use_memory) memory = opt.remember(&memory);
        }
        std::printf("]}\n");
        return 0;
    }
    return 2;
}
