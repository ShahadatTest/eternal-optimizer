// Benchmark problems. Every one is minimised; `optimum` is its known best value.
// `relevant[i]` records whether parameter i really matters, for checking the screening only;
// the optimizer never sees it.
#pragma once

#include <cmath>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "aci_opt.hpp"

namespace aci {

struct Problem {
    std::string name;        // shown in the app
    std::string blurb;
    std::vector<Param> params;
    std::vector<bool> relevant;
    double optimum;
    std::function<double(const std::vector<double>&)> f;
};

inline double unit(const Param& p, double v) { return (v - p.lo) / (p.hi - p.lo); }

inline std::vector<Problem> problems() {
    std::vector<Problem> out;

    {   // A hidden "taste" with an interaction (salt depends on oil) and three knobs that do nothing.
        Problem p;
        p.name = "ラーメンの味";
        p.blurb = "隠れた味の関数。8つのつまみのうち3つは味に無関係。";
        p.params = {{"茹で時間 (秒)", 60, 300}, {"スープ温度 (℃)", 60, 99}, {"塩分 (g)", 2, 12},
                    {"背脂 (ml)", 0, 30}, {"チャーシューの厚さ (mm)", 2, 15},
                    {"外の気温 (℃)", -5, 35}, {"曜日", 0, 7}, {"店主の機嫌", 0, 10}};
        p.relevant = {true, true, true, true, true, false, false, false};
        p.optimum = -100.0;
        auto ps = p.params;
        p.f = [ps](const std::vector<double>& x) {
            const double t = unit(ps[0], x[0]), T = unit(ps[1], x[1]), s = unit(ps[2], x[2]);
            const double o = unit(ps[3], x[3]), c = unit(ps[4], x[4]);
            const double taste = 100 - 60 * std::pow(t - 0.55, 2) - 80 * std::pow(T - 0.8, 2)
                               - 120 * std::pow(s - 0.45 - 0.3 * (o - 0.5), 2) - 30 * std::pow(o - 0.4, 2)
                               - 20 * std::pow(c - 0.35, 2);
            return -taste;
        };
        out.push_back(p);
    }
    {   // Branin in two dimensions, plus six decoy dimensions.
        Problem p;
        p.name = "ブラニン＋ダミー6次元";
        p.blurb = "定番の2次元関数に、効かない次元を6つ足したもの。";
        p.params = {{"x1", -5, 10}, {"x2", 0, 15}};
        for (int i = 1; i <= 6; ++i) p.params.push_back({"ダミー" + std::to_string(i), 0, 1});
        p.relevant = {true, true, false, false, false, false, false, false};
        p.optimum = 0.397887357729738;
        p.f = [](const std::vector<double>& x) {
            const double a = 1, b = 5.1 / (4 * M_PI * M_PI), c = 5 / M_PI, r = 6, s = 10, t = 1 / (8 * M_PI);
            return a * std::pow(x[1] - b * x[0] * x[0] + c * x[0] - r, 2) + s * (1 - t) * std::cos(x[0]) + s;
        };
        out.push_back(p);
    }
    {   // Hartmann-6.
        Problem p;
        p.name = "ハルトマン6";
        p.blurb = "6次元・多峰性の定番テスト関数。";
        for (int i = 1; i <= 6; ++i) p.params.push_back({"x" + std::to_string(i), 0, 1});
        p.relevant.assign(6, true);
        p.optimum = -3.32236801141551;
        p.f = [](const std::vector<double>& x) {
            static const double al[4] = {1.0, 1.2, 3.0, 3.2};
            static const double A[4][6] = {{10, 3, 17, 3.5, 1.7, 8}, {0.05, 10, 17, 0.1, 8, 14},
                                           {3, 3.5, 1.7, 10, 17, 8}, {17, 8, 0.05, 10, 0.1, 14}};
            static const double P[4][6] = {{1312, 1696, 5569, 124, 8283, 5886}, {2329, 4135, 8307, 3736, 1004, 9991},
                                           {2348, 1451, 3522, 2883, 3047, 6650}, {4047, 8828, 8732, 5743, 1091, 381}};
            double y = 0;
            for (int i = 0; i < 4; ++i) {
                double e = 0;
                for (int j = 0; j < 6; ++j) e += A[i][j] * std::pow(x[j] - P[i][j] * 1e-4, 2);
                y -= al[i] * std::exp(-e);
            }
            return y;
        };
        out.push_back(p);
    }
    {   // Rosenbrock-4: a long curved valley.
        Problem p;
        p.name = "ローゼンブロック4";
        p.blurb = "細く曲がった谷。どの変数も効くが、互いに強く絡む。";
        for (int i = 1; i <= 4; ++i) p.params.push_back({"x" + std::to_string(i), -2, 2});
        p.relevant.assign(4, true);
        p.optimum = 0.0;
        p.f = [](const std::vector<double>& x) {
            double y = 0;
            for (size_t i = 0; i + 1 < x.size(); ++i) y += 100 * std::pow(x[i + 1] - x[i] * x[i], 2) + std::pow(1 - x[i], 2);
            return y;
        };
        out.push_back(p);
    }
    return out;
}

// A problem evaluated with Gaussian measurement noise of standard deviation `sigma`.
class Noisy {
public:
    Noisy(const Problem& p, double sigma, uint64_t seed) : p_(p), sigma_(sigma), rng_(seed) {}
    double operator()(const std::vector<double>& x) {
        return p_.f(x) + (sigma_ > 0 ? std::normal_distribution<double>(0, sigma_)(rng_) : 0.0);
    }
private:
    const Problem& p_;
    double sigma_;
    std::mt19937_64 rng_;
};

}  // namespace aci
