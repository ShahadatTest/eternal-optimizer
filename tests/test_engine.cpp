// Engine tests: ask/tell discipline, screening freezes decoys and keeps causes,
// and the loop beats random search on every benchmark at a fixed budget.
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "aci_opt.hpp"
#include "problems.hpp"

static int failures = 0;
#define CHECK(cond, msg)                                                   \
    do {                                                                   \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; }       \
        else { std::printf("ok:   %s\n", msg); }                           \
    } while (0)

static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

int main() {
    const auto probs = aci::problems();

    {   // ask twice returns the same point; tell without ask throws
        aci::Optimizer opt(probs[0].params, 1);
        auto a = opt.ask(), b = opt.ask();
        CHECK(a == b, "ask() before tell() repeats the pending point");
        opt.tell(probs[0].f(a));
        bool threw = false;
        try { opt.tell(0.0); } catch (const std::logic_error&) { threw = true; }
        CHECK(threw, "tell() without ask() throws");
    }

    {   // every point stays inside the box
        aci::Optimizer opt(probs[3].params, 2);
        bool inside = true;
        for (int t = 0; t < 300; ++t) {
            auto x = opt.ask();
            for (size_t i = 0; i < x.size(); ++i)
                inside &= x[i] >= probs[3].params[i].lo && x[i] <= probs[3].params[i].hi;
            opt.tell(probs[3].f(x));
        }
        CHECK(inside, "all proposals stay within bounds");
    }

    for (int pi : {0, 1}) {   // screening: decoys frozen, real causes kept
        int ok = 0;
        for (int s = 0; s < 10; ++s) {
            aci::Optimizer opt(probs[pi].params, 100 + s);
            for (int t = 0; t < 120; ++t) { auto x = opt.ask(); opt.tell(probs[pi].f(x)); }
            bool good = true;
            for (size_t i = 0; i < probs[pi].params.size(); ++i)
                good &= (!opt.verdicts()[i].cause) == !probs[pi].relevant[i];
            ok += good;
        }
        char msg[160];
        std::snprintf(msg, sizeof msg, "%s: screening exact in %d/10 seeds", probs[pi].name.c_str(), ok);
        CHECK(ok == 10, msg);
    }

    for (const auto& p : probs) {   // beats random search at the median, 15 seeds, 200 evaluations
        std::vector<double> ga, gr;
        for (int s = 0; s < 15; ++s) {
            aci::Optimizer opt(p.params, 300 + s);
            aci::RandomSearch rnd(p.params, 600 + s);
            for (int t = 0; t < 200; ++t) {
                auto x = opt.ask(); opt.tell(p.f(x));
                auto r = rnd.ask(); rnd.tell(p.f(r));
            }
            ga.push_back(opt.best() - p.optimum);
            gr.push_back(rnd.best() - p.optimum);
        }
        char msg[200];
        std::snprintf(msg, sizeof msg, "%s: median gap %.3g vs random %.3g", p.name.c_str(), median(ga), median(gr));
        CHECK(median(ga) < median(gr), msg);
    }

    std::printf(failures ? "\n%d FAILED\n" : "\nall passed\n", failures);
    return failures ? 1 : 0;
}
