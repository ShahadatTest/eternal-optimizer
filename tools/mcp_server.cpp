// 永遠 MCP サーバー ─ MCP (Model Context Protocol) over stdio.
// Lets a language model drive the optimizer: open a study, ask for the next trial, report the
// measured value, and read what the causal screening and the rules have learned.
// Messages are newline-delimited JSON-RPC 2.0 on stdin/stdout; logs go to stderr.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "aci_opt.hpp"
#include "problems.hpp"

using json = nlohmann::json;

namespace {

const char* kKind[] = {"初期探索", "再測定", "介入", "活用", "探索", "局所探索", "ランダム", "記憶から"};

struct Study {
    std::vector<aci::Param> params;
    std::unique_ptr<aci::Optimizer> opt;
    bool maximize = false;
    bool waiting = false;
};

std::map<std::string, Study> studies;
int next_id = 1;

json point(const Study& s, const std::vector<double>& x) {
    json p = json::object();
    for (size_t i = 0; i < x.size(); ++i) p[s.params[i].name] = x[i];
    return p;
}

std::string event_text(const aci::Event& e, const Study& s) {
    const std::string n = e.param >= 0 ? "『" + s.params[e.param].name + "』" : "";
    if (e.what == "freeze") return "#" + std::to_string(e.trial + 1) + " " + n + "は原因ではない → 固定";
    if (e.what == "revive") return "#" + std::to_string(e.trial + 1) + " " + n + "が再び効き始めた → 復活";
    if (e.what == "recall") return "#" + std::to_string(e.trial + 1) + " 記憶した良い範囲から探索を開始";
    if (e.what == "doubt") return "#" + std::to_string(e.trial + 1) + " " + n + "は判定が際どい → 再検査";
    if (e.what == "zoom") return "#" + std::to_string(e.trial + 1) + " " + n + "の範囲を絞り込み（結晶化）";
    return "#" + std::to_string(e.trial + 1) + " 停滞したので範囲を再び広げた";
}

Study& get(const json& a) {
    const std::string id = a.at("study").get<std::string>();
    auto it = studies.find(id);
    if (it == studies.end()) throw std::runtime_error("研究が見つかりません: " + id);
    return it->second;
}

// ------------------------------------------------------------------ tools

json tool_create(const json& a) {
    Study s;
    for (const auto& p : a.at("params")) {
        const double lo = p.at("low").get<double>(), hi = p.at("high").get<double>();
        if (!(hi > lo)) throw std::runtime_error("high は low より大きくしてください: " + p.at("name").get<std::string>());
        s.params.push_back({p.at("name").get<std::string>(), lo, hi});
    }
    s.maximize = a.value("maximize", false);
    s.opt = std::make_unique<aci::Optimizer>(s.params, a.value("seed", 1));
    const std::string id = "study-" + std::to_string(next_id++);
    studies.emplace(id, std::move(s));
    return {{"study", id}, {"message", "研究を作成しました。ask で次の試行を受け取り、測定値を tell で返してください。"}};
}

json tool_ask(const json& a) {
    Study& s = get(a);
    const auto x = s.opt->ask();
    s.waiting = true;
    return {{"trial", s.opt->trials().size() + 1}, {"kind", kKind[(int)s.opt->pending_kind()]}, {"params", point(s, x)}};
}

json tool_tell(const json& a) {
    Study& s = get(a);
    if (!s.waiting) throw std::runtime_error("先に ask を呼んでください");
    const double v = a.at("value").get<double>();
    s.opt->tell(s.maximize ? -v : v);
    s.waiting = false;
    const double best = s.maximize ? -s.opt->best() : s.opt->best();
    return {{"trials", s.opt->trials().size()}, {"best", best}, {"best_params", point(s, s.opt->incumbent())}};
}

json tool_status(const json& a) {
    Study& s = get(a);
    const auto& o = *s.opt;
    json causes = json::array(), frozen = json::array(), untested = json::array();
    for (size_t i = 0; i < s.params.size(); ++i) {
        const auto& v = o.verdicts()[i];
        json e = {{"name", s.params[i].name}, {"effect_over_noise", v.snr}};
        (!v.tested ? untested : v.cause ? causes : frozen).push_back(e);
    }
    json rules = json::array();
    for (const auto& r : o.bin_rules())
        if (r.best) rules.push_back({{"name", s.params[r.param].name}, {"range", {r.lo, r.hi}},
                                     {"success_rate", r.mean}, {"evidence", r.n}, {"z", r.z}});
    json pairs = json::array();
    for (const auto& p : o.pair_rules(3))
        pairs.push_back({{"pair", {s.params[p.pi].name, s.params[p.pj].name}}, {"success_rate", p.mean}, {"z", p.z}});
    json events = json::array();
    for (const auto& e : o.events()) events.push_back(event_text(e, s));
    std::map<std::string, int> kinds;
    for (const auto& t : o.trials()) kinds[kKind[(int)t.kind]]++;
    return {{"trials", o.trials().size()},
            {"best", o.trials().empty() ? json(nullptr) : json(s.maximize ? -o.best() : o.best())},
            {"best_params", o.trials().empty() ? json::object() : point(s, o.incumbent())},
            {"noise_estimate", o.noise()},
            {"causes", causes}, {"frozen_as_irrelevant", frozen}, {"untested", untested},
            {"rules", rules}, {"pair_rules", pairs}, {"events", events}, {"trial_kinds", kinds},
            {"curiosity", o.curiosity()}, {"exploration_rate", o.last_rho()}};
}

json tool_problems(const json&) {
    json out = json::array();
    int i = 0;
    for (const auto& p : aci::problems()) {
        json names = json::array();
        for (const auto& q : p.params) names.push_back(q.name);
        out.push_back({{"index", i++}, {"name", p.name}, {"description", p.blurb}, {"params", names}, {"optimum", p.optimum}});
    }
    return out;
}

json tool_benchmark(const json& a) {
    const auto probs = aci::problems();
    const int k = a.value("problem", 0), budget = a.value("budget", 200), seeds = a.value("seeds", 10);
    const double noise = a.value("noise", 0.0);
    if (k < 0 || k >= (int)probs.size()) throw std::runtime_error("問題番号が範囲外です");
    const auto& p = probs[k];
    aci::RandomSearch probe(p.params, 4242);
    double m = 0, ss = 0;
    std::vector<double> ys;
    for (int i = 0; i < 400; ++i) ys.push_back(p.f(probe.ask()));
    for (double y : ys) m += y;
    m /= ys.size();
    for (double y : ys) ss += (y - m) * (y - m);
    const double sigma = noise * std::sqrt(ss / ys.size());
    std::vector<double> ga, gr;
    int wins = 0, exact = 0;
    for (int s = 0; s < seeds; ++s) {
        aci::Optimizer opt(p.params, 1000 + s);
        aci::RandomSearch rnd(p.params, 5000 + s);
        aci::Noisy fa(p, sigma, 7000 + s), fr(p, sigma, 9000 + s);
        std::vector<double> xa, xr;
        double ya = 1e300, yr = 1e300;
        for (int t = 0; t < budget; ++t) {
            auto x = opt.ask(); double v = fa(x); opt.tell(v); if (v < ya) { ya = v; xa = x; }
            auto r = rnd.ask(); double w = fr(r); rnd.tell(w); if (w < yr) { yr = w; xr = r; }
        }
        ga.push_back(p.f(xa) - p.optimum); gr.push_back(p.f(xr) - p.optimum);
        wins += ga.back() < gr.back();
        bool ok = true;
        for (size_t i = 0; i < p.params.size(); ++i) ok &= (!opt.verdicts()[i].cause) == !p.relevant[i];
        exact += ok;
    }
    std::sort(ga.begin(), ga.end()); std::sort(gr.begin(), gr.end());
    return {{"problem", p.name}, {"budget", budget}, {"seeds", seeds}, {"noise", noise},
            {"median_gap_eternal", ga[seeds / 2]}, {"median_gap_random", gr[seeds / 2]},
            {"wins", wins}, {"screening_exact", exact}};
}

json schema(json props, std::vector<std::string> required) {
    return {{"type", "object"}, {"properties", std::move(props)}, {"required", std::move(required)}};
}

const json kTools = json::array({
    {{"name", "create_study"},
     {"description", "新しい最適化研究を作る。測れる値（小さいほど良い。maximize=true なら大きいほど良い）を、"
                     "仕組みを知らずに改善する。各パラメータの名前と下限・上限を渡す。"},
     {"inputSchema", schema({{"params", {{"type", "array"}, {"items", schema({{"name", {{"type", "string"}}},
                                                                            {"low", {{"type", "number"}}},
                                                                            {"high", {{"type", "number"}}}},
                                                                           {"name", "low", "high"})}}},
                             {"maximize", {{"type", "boolean"}}}, {"seed", {{"type", "integer"}}}},
                            {"params"})}},
    {{"name", "ask"},
     {"description", "次に試すパラメータを受け取る。種類（初期探索・介入・活用・探索・局所探索など）も返す。"},
     {"inputSchema", schema({{"study", {{"type", "string"}}}}, {"study"})}},
    {{"name", "tell"},
     {"description", "ask で受け取ったパラメータで測った値を返す。"},
     {"inputSchema", schema({{"study", {{"type", "string"}}}, {"value", {{"type", "number"}}}}, {"study", "value"})}},
    {{"name", "status"},
     {"description", "研究の現状：最良値と最良パラメータ、原因と判定した変数・無関係として固定した変数、"
                     "学習したルール、出来事の記録。"},
     {"inputSchema", schema({{"study", {{"type", "string"}}}}, {"study"})}},
    {{"name", "list_problems"},
     {"description", "内蔵のベンチマーク問題の一覧。"},
     {"inputSchema", schema(json::object(), {})}},
    {{"name", "benchmark"},
     {"description", "内蔵問題で、永遠とランダム探索を同じ予算で比べる。"},
     {"inputSchema", schema({{"problem", {{"type", "integer"}}}, {"budget", {{"type", "integer"}}},
                             {"noise", {{"type", "number"}}}, {"seeds", {{"type", "integer"}}}}, {})}},
});

json call(const std::string& name, const json& args) {
    if (name == "create_study") return tool_create(args);
    if (name == "ask") return tool_ask(args);
    if (name == "tell") return tool_tell(args);
    if (name == "status") return tool_status(args);
    if (name == "list_problems") return tool_problems(args);
    if (name == "benchmark") return tool_benchmark(args);
    throw std::runtime_error("不明なツール: " + name);
}

}  // namespace

int main() {
    std::ios::sync_with_stdio(false);
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        json msg;
        try { msg = json::parse(line); } catch (...) {
            std::cout << json{{"jsonrpc", "2.0"}, {"id", nullptr}, {"error", {{"code", -32700}, {"message", "parse error"}}}}.dump() << "\n" << std::flush;
            continue;
        }
        if (!msg.contains("id")) continue;   // notifications need no reply
        const json id = msg["id"];
        const std::string method = msg.value("method", "");
        json reply = {{"jsonrpc", "2.0"}, {"id", id}};
        try {
            if (method == "initialize") {
                const std::string version = msg["params"].value("protocolVersion", "2025-06-18");
                reply["result"] = {{"protocolVersion", version}, {"capabilities", {{"tools", json::object()}}},
                                   {"serverInfo", {{"name", "eternal"}, {"version", "0.1.0"}}},
                                   {"instructions", "永遠：介入して、効くものだけを残す因果最適化エンジン。create_study → ask → 測定 → tell を繰り返し、status で何が効くかを読む。"}};
            } else if (method == "ping") {
                reply["result"] = json::object();
            } else if (method == "tools/list") {
                reply["result"] = {{"tools", kTools}};
            } else if (method == "tools/call") {
                const auto& p = msg["params"];
                try {
                    const json out = call(p.at("name").get<std::string>(), p.value("arguments", json::object()));
                    reply["result"] = {{"content", {{{"type", "text"}, {"text", out.dump(2)}}}}};
                } catch (const std::exception& e) {
                    reply["result"] = {{"content", {{{"type", "text"}, {"text", std::string("エラー: ") + e.what()}}}}, {"isError", true}};
                }
            } else {
                reply["error"] = {{"code", -32601}, {"message", "method not found: " + method}};
            }
        } catch (const std::exception& e) {
            reply["error"] = {{"code", -32603}, {"message", e.what()}};
        }
        std::cout << reply.dump() << "\n" << std::flush;
    }
    return 0;
}
