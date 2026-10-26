// 永遠 ─ 因果最適化ラボ
// The desktop visualizer for the Eternal optimizer. Every visible string is Japanese.
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "aci_opt.hpp"
#include "problems.hpp"

namespace fs = std::filesystem;

// ------------------------------------------------------------------ every string on screen, in both languages
struct L { const char* ja; const char* en; };
static bool g_en = false;   // the language selector
static const char* tr(const L& l) { return g_en ? l.en : l.ja; }
static std::vector<L>& all_strings() { static std::vector<L> v; return v; }
static L reg(L l) { all_strings().push_back(l); return l; }

namespace ja {
const L kTitle = reg({"永遠 ─ 因果最適化ラボ", "Eternal ─ Causal Optimization Lab"});
const L kHeading = reg({"永遠 ─ 因果最適化ラボ", "Eternal Optimizer"});
const L kTagline = reg({"介入して、効くものだけを残す。", "Intervene, and keep only what works."});
const L kLanguage = reg({"言語", "Language"});
const L kProblem = reg({"問題", "Problem"});
const L kNoise = reg({"測定ノイズ（ばらつきに対する割合）", "Measurement noise (share of spread)"});
const L kBudget = reg({"試行の予算", "Trial budget"});
const L kSpeed = reg({"速度（試行/秒）", "Speed (trials/s)"});
const L kSeed = reg({"乱数シード", "Random seed"});
const L kStart = reg({"開始", "Start"});
const L kPause = reg({"一時停止", "Pause"});
const L kStep = reg({"1手進める", "Step once"});
const L kReset = reg({"リセット", "Reset"});
const L kTrials = reg({"試行数", "Trials"});
const L kBest = reg({"最良値", "Best value"});
const L kOptimum = reg({"既知の最適値", "Known optimum"});
const L kGap = reg({"最適値との差", "Gap to optimum"});
const L kRandomGap = reg({"ランダム探索の差", "Random search gap"});
const L kSigma = reg({"推定ノイズ σ", "Noise estimate σ"});
const L kCuriosity = reg({"好奇心 C", "Curiosity C"});
const L kRho = reg({"探索率 ρ", "Exploration rate ρ"});
const L kActive = reg({"原因と判定された変数", "Causes found"});
const L kKinds = reg({"試行の種類", "Trial kinds"});
const L kLog = reg({"出来事", "Events"});
const L kHistory = reg({"最適化の推移（最適値との差・対数）", "Progress (gap to optimum, log scale)"});
const L kAciLine = reg({"永遠（ACI）", "Eternal (ACI)"});
const L kRandomLine = reg({"ランダム探索", "Random search"});
const L kScreen = reg({"因果スクリーニング（介入効果 ÷ ノイズ）", "Causal screening (intervention effect ÷ noise)"});
const L kThreshold = reg({"判定の閾値", "Decision threshold"});
const L kCause = reg({"原因", "cause"});
const L kFrozen = reg({"無関係 → 固定", "irrelevant → frozen"});
const L kUntested = reg({"未検査", "untested"});
const L kSlice = reg({"断面図（表示専用・試行数に含まない）", "Cross-section (display only, not counted as trials)"});
const L kIncumbent = reg({"現在の最良点", "Current best"});
const L kRules = reg({"学習したルール", "Learned rules"});
const L kVar = reg({"変数", "Variable"});
const L kRange = reg({"区間", "Range"});
const L kMean = reg({"成功率（事後平均）", "Success rate"});
const L kEvidence = reg({"証拠量", "Evidence"});
const L kZ = reg({"確信度 z", "z"});
const L kPairs = reg({"組み合わせルール", "Pair rules"});
const L kPairFmt = reg({"『%s』×『%s』  成功率 %.2f  z %.2f", "%s × %s  success %.2f  z %.2f"});
const L kRangeFmt = reg({"%.3g 〜 %.3g", "%.3g – %.3g"});
const L kParallel = reg({"平行座標（直近の試行・明るいほど良い）", "Parallel coordinates (recent trials, brighter is better)"});
const L kDrives = reg({"動機（好奇心）と探索率", "Drive (curiosity) and exploration rate"});
const L kTrialAxis = reg({"試行", "Trial"});
const L kNeedTwo = reg({"原因の変数が2つ以上になると表示されます", "Shown once two or more variables are causes"});
const L kWaiting = reg({"スクリーニング後に表示されます", "Shown after screening"});
const L kDone = reg({"予算を使い切りました", "Budget used up"});
const L kRunning = reg({"実行中", "Running"});
const L kPaused = reg({"停止中", "Paused"});
const L kTickTop = reg({"10⁶以上", "10⁶+"});
const L kEvFreeze = reg({"#%d 『%s』は原因ではない → 固定", "#%d %s is not a cause → frozen"});
const L kEvRevive = reg({"#%d 『%s』が再び効き始めた → 復活", "#%d %s matters again → revived"});
const L kEvZoom = reg({"#%d 『%s』の範囲を絞り込み（結晶化）", "#%d %s: range narrowed (crystallised)"});
const L kEvExpand = reg({"#%d 停滞したので範囲を再び広げた", "#%d stalled, so the ranges were widened again"});
const L kEvDoubt = reg({"#%d 『%s』は判定が際どい → 再検査", "#%d %s is borderline → testing again"});
const L kKindNames[] = {reg({"初期探索", "Warm-up"}), reg({"再測定", "Re-measure"}), reg({"介入", "Intervene"}), reg({"活用", "Exploit"}), reg({"探索", "Explore"}), reg({"局所探索", "Local search"}), reg({"ランダム", "Random"})};
}  // namespace ja

static const ImVec4 kKindColor[] = {
    {0.55f, 0.55f, 0.58f, 1}, {0.95f, 0.85f, 0.30f, 1}, {0.95f, 0.35f, 0.75f, 1}, {0.30f, 0.85f, 0.95f, 1},
    {1.00f, 0.60f, 0.20f, 1}, {0.45f, 0.95f, 0.45f, 1}, {0.70f, 0.70f, 0.70f, 1}};

// ------------------------------------------------------------------ fonts

static std::string find_font(const char* suffix) {
    for (const char* dir : {"/System/Library/Fonts", "/Library/Fonts"}) {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(dir, ec)) {
            const std::string n = e.path().filename().string();
            if (n.size() >= std::strlen(suffix) && n.compare(n.size() - std::strlen(suffix), std::strlen(suffix), suffix) == 0 &&
                n.find("GB") == std::string::npos)
                return e.path().string();
        }
    }
    for (const char* p : {"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "C:/Windows/Fonts/meiryo.ttc",
                          "/System/Library/Fonts/Hiragino Sans GB.ttc"})
        if (fs::exists(p)) return p;
    return {};
}

static void load_fonts(float scale, const std::vector<aci::Problem>& probs, ImFont** big) {
    ImGuiIO& io = ImGui::GetIO();
    ImFontGlyphRangesBuilder b;
    b.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    for (const L& l : all_strings()) { b.AddText(l.ja); b.AddText(l.en); }
    b.AddText("σρ─→・（）÷『』℃°³⁴⁵⁶以上〜×–’日本語");
    for (const auto& p : probs) {
        b.AddText(p.name.c_str());
        b.AddText(p.blurb.c_str());
        for (const auto& q : p.params) b.AddText(q.name.c_str());
    }
    static ImVector<ImWchar> ranges;
    b.BuildRanges(&ranges);

    const std::string regular = find_font("W3.ttc"), bold = find_font("W6.ttc");
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    if (!regular.empty()) io.Fonts->AddFontFromFileTTF(regular.c_str(), 16.0f * scale, &cfg, ranges.Data);
    else io.Fonts->AddFontDefault();
    *big = !bold.empty() ? io.Fonts->AddFontFromFileTTF(bold.c_str(), 26.0f * scale, &cfg, ranges.Data) : nullptr;
    io.FontGlobalScale = 1.0f / scale;
}

static void style() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.FrameRounding = 2; s.GrabRounding = 2; s.ChildRounding = 0;
    s.WindowPadding = {14, 12}; s.ItemSpacing = {8, 7}; s.FramePadding = {8, 5};
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = {0.02f, 0.02f, 0.03f, 1};
    c[ImGuiCol_ChildBg] = {0.05f, 0.05f, 0.06f, 1};
    c[ImGuiCol_Border] = {0.16f, 0.16f, 0.18f, 1};
    c[ImGuiCol_FrameBg] = {0.10f, 0.10f, 0.12f, 1};
    c[ImGuiCol_FrameBgHovered] = {0.18f, 0.18f, 0.20f, 1};
    c[ImGuiCol_Button] = {0.92f, 0.92f, 0.94f, 1};
    c[ImGuiCol_ButtonHovered] = {1, 1, 1, 1};
    c[ImGuiCol_ButtonActive] = {0.75f, 0.75f, 0.78f, 1};
    c[ImGuiCol_SliderGrab] = {0.92f, 0.92f, 0.94f, 1};
    c[ImGuiCol_CheckMark] = {1, 1, 1, 1};
    c[ImGuiCol_Header] = {0.14f, 0.14f, 0.16f, 1};
    c[ImGuiCol_TableHeaderBg] = {0.10f, 0.10f, 0.12f, 1};
    ImPlotStyle& p = ImPlot::GetStyle();
    p.Colors[ImPlotCol_PlotBg] = {0.02f, 0.02f, 0.03f, 1};
    p.Colors[ImPlotCol_FrameBg] = {0.05f, 0.05f, 0.06f, 1};
    p.PlotPadding = {8, 8};
}

// ------------------------------------------------------------------ the running session

struct Session {
    int problem = 0, budget = 300, seed = 1;
    float noise = 0.0f, speed = 40.0f;
    bool running = false;
    std::vector<aci::Problem> probs = aci::problems();
    std::unique_ptr<aci::Optimizer> opt;
    std::unique_ptr<aci::RandomSearch> rnd;
    std::unique_ptr<aci::Noisy> fa, fr;
    double spread = 1, rnd_best_true = 1e300;
    std::vector<double> xs, gap_aci, gap_rnd, curiosity, rho;
    std::vector<double> slice;
    int slice_i = -1, slice_j = -1, slice_at = -1;
    double carry = 0;

    const aci::Problem& P() const { return probs[problem]; }

    void reset() {
        aci::RandomSearch probe(P().params, 4242);
        double m = 0, s = 0;
        std::vector<double> ys;
        for (int i = 0; i < 400; ++i) ys.push_back(P().f(probe.ask()));
        for (double y : ys) m += y;
        m /= ys.size();
        for (double y : ys) s += (y - m) * (y - m);
        spread = std::sqrt(s / ys.size());
        opt = std::make_unique<aci::Optimizer>(P().params, seed);
        rnd = std::make_unique<aci::RandomSearch>(P().params, seed + 77777);
        fa = std::make_unique<aci::Noisy>(P(), noise * spread, seed + 1);
        fr = std::make_unique<aci::Noisy>(P(), noise * spread, seed + 2);
        rnd_best_true = 1e300;
        xs.clear(); gap_aci.clear(); gap_rnd.clear(); curiosity.clear(); rho.clear();
        slice_at = -1;
        carry = 0;
    }

    int done() const { return static_cast<int>(opt->trials().size()); }

    void step() {
        if (done() >= budget) { running = false; return; }
        auto x = opt->ask();
        opt->tell((*fa)(x));
        auto r = rnd->ask();
        const double yr = (*fr)(r);
        rnd->tell(yr);
        if (yr <= rnd->best()) rnd_best_true = P().f(r);
        xs.push_back(done());
        gap_aci.push_back(std::max(1e-9, P().f(opt->incumbent()) - P().optimum));
        gap_rnd.push_back(std::max(1e-9, rnd_best_true - P().optimum));
        curiosity.push_back(opt->curiosity());
        rho.push_back(opt->last_rho());
    }

    // the two most clearly causal parameters, for the slice
    void pick_slice() {
        std::vector<std::pair<double, int>> v;
        for (int i = 0; i < (int)P().params.size(); ++i)
            if (opt->verdicts()[i].tested && opt->verdicts()[i].cause) v.push_back({opt->verdicts()[i].snr, i});
        std::sort(v.rbegin(), v.rend());
        slice_i = v.size() >= 2 ? v[0].second : -1;
        slice_j = v.size() >= 2 ? v[1].second : -1;
    }

    void refresh_slice() {
        if (!opt->screened()) { slice_i = -1; return; }
        if (slice_at == done()) return;
        pick_slice();
        slice_at = done();
        if (slice_i < 0) return;
        const int N = 64;
        slice.assign(N * N, 0);
        auto x = opt->incumbent();
        const auto& a = P().params[slice_i];
        const auto& b = P().params[slice_j];
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c) {
                x[slice_i] = a.lo + (a.hi - a.lo) * (c + 0.5) / N;
                x[slice_j] = b.hi - (b.hi - b.lo) * (r + 0.5) / N;   // heatmap rows run top to bottom
                slice[r * N + c] = std::log10(std::max(1e-9, P().f(x) - P().optimum));
            }
    }
};

static const char* pname(const aci::Problem& p, int i) { return g_en ? p.param_en[i].c_str() : p.params[i].name.c_str(); }
static const char* prob_name(const aci::Problem& p) { return g_en ? p.name_en.c_str() : p.name.c_str(); }
static const char* prob_blurb(const aci::Problem& p) { return g_en ? p.blurb_en.c_str() : p.blurb.c_str(); }

static std::string event_text(const aci::Event& e, const aci::Problem& p) {
    char buf[256];
    const char* name = e.param >= 0 ? pname(p, e.param) : "";
    if (e.what == "freeze") std::snprintf(buf, sizeof buf, tr(ja::kEvFreeze), e.trial + 1, name);
    else if (e.what == "revive") std::snprintf(buf, sizeof buf, tr(ja::kEvRevive), e.trial + 1, name);
    else if (e.what == "doubt") std::snprintf(buf, sizeof buf, tr(ja::kEvDoubt), e.trial + 1, name);
    else if (e.what == "zoom") std::snprintf(buf, sizeof buf, tr(ja::kEvZoom), e.trial + 1, name);
    else std::snprintf(buf, sizeof buf, tr(ja::kEvExpand), e.trial + 1);
    return buf;
}

// ------------------------------------------------------------------ panels

static void controls(Session& s, ImFont* big) {
    if (big) ImGui::PushFont(big);
    ImGui::TextUnformatted(tr(ja::kHeading));
    if (big) ImGui::PopFont();
    ImGui::TextDisabled("%s", tr(ja::kTagline));
    ImGui::TextUnformatted(tr(ja::kLanguage));
    ImGui::SameLine();
    bool lang = false;
    if (ImGui::RadioButton("日本語", !g_en)) { g_en = false; lang = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("English", g_en)) { g_en = true; lang = true; }
    if (lang) glfwSetWindowTitle(glfwGetCurrentContext(), tr(ja::kTitle));
    ImGui::Separator();

    bool changed = false;
    ImGui::TextUnformatted(tr(ja::kProblem));
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##problem", prob_name(s.P()))) {
        for (int i = 0; i < (int)s.probs.size(); ++i)
            if (ImGui::Selectable(prob_name(s.probs[i]), i == s.problem)) { s.problem = i; changed = true; }
        ImGui::EndCombo();
    }
    ImGui::TextWrapped("%s", prob_blurb(s.P()));
    ImGui::TextUnformatted(tr(ja::kNoise));
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::SliderFloat("##noise", &s.noise, 0.0f, 0.10f, "%.2f");
    ImGui::TextUnformatted(tr(ja::kBudget));
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##budget", &s.budget, 50, 1500);
    ImGui::TextUnformatted(tr(ja::kSpeed));
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##speed", &s.speed, 1.0f, 400.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
    ImGui::TextUnformatted(tr(ja::kSeed));
    ImGui::SetNextItemWidth(-1);
    changed |= ImGui::InputInt("##seed", &s.seed);
    if (changed) { s.running = false; s.reset(); }

    ImGui::Spacing();
    const float w = (ImGui::GetContentRegionAvail().x - 8) / 2;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    if (ImGui::Button(s.running ? tr(ja::kPause) : tr(ja::kStart), {w, 34})) s.running = !s.running;
    ImGui::SameLine();
    if (ImGui::Button(tr(ja::kStep), {w, 34})) { s.running = false; s.step(); }
    if (ImGui::Button(tr(ja::kReset), {-1, 30})) { s.running = false; s.reset(); }
    ImGui::PopStyleColor();
    ImGui::TextDisabled("%s", s.done() >= s.budget ? tr(ja::kDone) : s.running ? tr(ja::kRunning) : tr(ja::kPaused));
    ImGui::Separator();

    auto row = [](const char* k, const char* fmt, double v) {
        ImGui::TextUnformatted(k);
        ImGui::SameLine(g_en ? 175 : 190);
        ImGui::Text(fmt, v);
    };
    const auto& o = *s.opt;
    row(tr(ja::kTrials), "%.0f", s.done());
    row(tr(ja::kBest), "%.5g", o.trials().empty() ? 0.0 : s.P().f(o.incumbent()));
    row(tr(ja::kOptimum), "%.5g", s.P().optimum);
    row(tr(ja::kGap), "%.3g", s.gap_aci.empty() ? 0.0 : s.gap_aci.back());
    row(tr(ja::kRandomGap), "%.3g", s.gap_rnd.empty() ? 0.0 : s.gap_rnd.back());
    row(tr(ja::kSigma), "%.3g", o.noise());
    row(tr(ja::kCuriosity), "%.2f", o.curiosity());
    row(tr(ja::kRho), "%.2f", o.last_rho());
    ImGui::TextUnformatted(tr(ja::kActive));
    ImGui::SameLine(g_en ? 175 : 190);
    ImGui::Text("%d / %d", o.active_count(), (int)s.P().params.size());
    ImGui::Separator();

    ImGui::TextUnformatted(tr(ja::kKinds));
    for (int k = 0; k < 6; ++k) {
        ImGui::ColorButton("##k", kKindColor[k], ImGuiColorEditFlags_NoTooltip, {12, 12});
        ImGui::SameLine();
        ImGui::TextUnformatted(tr(ja::kKindNames[k]));
        if (k % 2 == 0) ImGui::SameLine(160);
    }
    ImGui::Separator();
    ImGui::TextUnformatted(tr(ja::kLog));
    ImGui::BeginChild("##log", {0, 0}, ImGuiChildFlags_Borders);
    const auto& ev = o.events();
    for (int i = (int)ev.size() - 1; i >= 0 && i >= (int)ev.size() - 40; --i)
        ImGui::TextWrapped("%s", event_text(ev[i], s.P()).c_str());
    ImGui::EndChild();
}

static void history_plot(Session& s, ImVec2 size) {
    if (ImPlot::BeginPlot(tr(ja::kHistory), size)) {
        ImPlot::SetupAxes(tr(ja::kTrialAxis), nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);
        if (!s.xs.empty()) {
            ImPlot::SetNextLineStyle({0.6f, 0.6f, 0.6f, 1}, 2);
            ImPlot::PlotLine(tr(ja::kRandomLine), s.xs.data(), s.gap_rnd.data(), (int)s.xs.size());
            ImPlot::SetNextLineStyle({1, 1, 1, 1}, 2.5f);
            ImPlot::PlotLine(tr(ja::kAciLine), s.xs.data(), s.gap_aci.data(), (int)s.xs.size());
            // every trial, coloured by kind, at its own true gap
            for (int k = 0; k < 6; ++k) {
                std::vector<double> tx, ty;
                for (const auto& t : s.opt->trials())
                    if ((int)t.kind == k) { tx.push_back(t.id + 1); ty.push_back(std::max(1e-9, s.P().f(t.x) - s.P().optimum)); }
                if (tx.empty()) continue;
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.5f, kKindColor[k], 0);
                ImPlot::PlotScatter("##kind", tx.data(), ty.data(), (int)tx.size());
            }
        }
        ImPlot::EndPlot();
    }
}

static void screening_plot(Session& s, ImVec2 size) {
    const auto& P = s.P();
    const int d = (int)P.params.size();
    if (ImPlot::BeginPlot(tr(ja::kScreen), size, ImPlotFlags_NoLegend)) {
        std::vector<const char*> names;
        std::vector<double> pos;
        for (int i = 0; i < d; ++i) { names.push_back(pname(P, i)); pos.push_back(i); }
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_None, ImPlotAxisFlags_Invert);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.1, 1e6, ImPlotCond_Always);
        static const double xt[] = {0.1, 1, 10, 100, 1e3, 1e4, 1e5, 1e6};
        const char* xl[] = {"0.1", "1", "10", "100", "10³", "10⁴", "10⁵", tr(ja::kTickTop)};
        ImPlot::SetupAxisTicks(ImAxis_X1, xt, 8, xl);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.7, d - 0.3, ImPlotCond_Always);
        ImPlot::SetupAxisTicks(ImAxis_Y1, pos.data(), d, names.data());
        for (int i = 0; i < d; ++i) {
            const auto& v = s.opt->verdicts()[i];
            if (!v.tested) continue;
            double val = std::max(0.11, std::min(1e6, v.snr)), y = i;
            const ImVec4 col = v.cause ? ImVec4(1, 1, 1, 1) : ImVec4(0.35f, 0.35f, 0.38f, 1);
            ImPlot::SetNextFillStyle(col);
            ImPlot::PlotBars("##b", &val, &y, 1, 0.6, ImPlotBarsFlags_Horizontal);
            const ImVec2 sz = ImGui::CalcTextSize(v.cause ? tr(ja::kCause) : tr(ja::kFrozen));
            ImPlot::PushStyleColor(ImPlotCol_InlayText, v.cause ? ImVec4(0, 0, 0, 1) : ImVec4(0.85f, 0.85f, 0.88f, 1));
            ImPlot::PlotText(v.cause ? tr(ja::kCause) : tr(ja::kFrozen), 0.1, y, {sz.x / 2 + 10, 0});
            ImPlot::PopStyleColor();
        }
        if (s.opt->screened()) {
            const double thr = 4.0;
            ImPlot::SetNextLineStyle({0.95f, 0.35f, 0.75f, 1}, 1.5f);
            ImPlot::PlotInfLines(tr(ja::kThreshold), &thr, 1);
        }
        ImPlot::EndPlot();
    }
}

static void slice_plot(Session& s, ImVec2 size) {
    s.refresh_slice();
    if (s.slice_i < 0) {
        ImGui::BeginChild("##slice_wait", size, ImGuiChildFlags_Borders);
        ImGui::TextUnformatted(tr(ja::kSlice));
        ImGui::TextDisabled("%s", s.opt->screened() ? tr(ja::kNeedTwo) : tr(ja::kWaiting));
        ImGui::EndChild();
        return;
    }
    const auto& a = s.P().params[s.slice_i];
    const auto& b = s.P().params[s.slice_j];
    if (ImPlot::BeginPlot(tr(ja::kSlice), size, ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes(pname(s.P(), s.slice_i), pname(s.P(), s.slice_j), ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxesLimits(a.lo, a.hi, b.lo, b.hi, ImPlotCond_Always);
        const auto [mn, mx] = std::minmax_element(s.slice.begin(), s.slice.end());
        ImPlot::PushColormap(ImPlotColormap_Greys);
        ImPlot::PlotHeatmap("##h", s.slice.data(), 64, 64, *mx, *mn, nullptr, {a.lo, b.lo}, {a.hi, b.hi});
        ImPlot::PopColormap();
        // the rule regions, and the trials projected onto this slice
        const auto lo = s.opt->region_lo(), hi = s.opt->region_hi();
        double rx[5] = {lo[s.slice_i], hi[s.slice_i], hi[s.slice_i], lo[s.slice_i], lo[s.slice_i]};
        double ry[5] = {lo[s.slice_j], lo[s.slice_j], hi[s.slice_j], hi[s.slice_j], lo[s.slice_j]};
        ImPlot::SetNextLineStyle({0.30f, 0.85f, 0.95f, 1}, 1.5f);
        ImPlot::PlotLine("##region", rx, ry, 5);
        for (int k = 0; k < 6; ++k) {
            std::vector<double> tx, ty;
            for (const auto& t : s.opt->trials())
                if ((int)t.kind == k) { tx.push_back(t.x[s.slice_i]); ty.push_back(t.x[s.slice_j]); }
            if (tx.empty()) continue;
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.5f, kKindColor[k], 0);
            ImPlot::PlotScatter("##kind", tx.data(), ty.data(), (int)tx.size());
        }
        const auto& inc = s.opt->incumbent();
        double ix = inc[s.slice_i], iy = inc[s.slice_j];
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 9, {1, 1, 1, 1}, 1.5f, {0, 0, 0, 1});
        ImPlot::PlotScatter(tr(ja::kIncumbent), &ix, &iy, 1);
        ImPlot::EndPlot();
    }
}

static void rules_table(Session& s, ImVec2 size) {
    ImGui::BeginChild("##rules", size, ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(tr(ja::kRules));
    if (!s.opt->screened()) { ImGui::TextDisabled("%s", tr(ja::kWaiting)); ImGui::EndChild(); return; }
    const auto rules = s.opt->bin_rules();
    if (ImGui::BeginTable("##t", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
                          {0, size.y * 0.62f})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (const char* h : {tr(ja::kVar), tr(ja::kRange), tr(ja::kMean), tr(ja::kEvidence), tr(ja::kZ)}) ImGui::TableSetupColumn(h);
        ImGui::TableHeadersRow();
        for (const auto& r : rules) {
            if (!r.best) continue;
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(pname(s.P(), r.param));
            ImGui::TableNextColumn(); ImGui::Text(tr(ja::kRangeFmt), r.lo, r.hi);
            ImGui::TableNextColumn();
            ImGui::ProgressBar((float)r.mean, {-1, 0}, "");
            ImGui::SameLine(8); ImGui::Text("%.2f", r.mean);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", r.n);
            ImGui::TableNextColumn(); ImGui::Text("%.2f", r.z);
        }
        ImGui::EndTable();
    }
    ImGui::TextUnformatted(tr(ja::kPairs));
    for (const auto& pr : s.opt->pair_rules(4)) {
        const auto& P = s.P().params;
        ImGui::BulletText(tr(ja::kPairFmt), pname(s.P(), pr.pi), pname(s.P(), pr.pj), pr.mean, pr.z);
    }
    ImGui::EndChild();
}

static void parallel_plot(Session& s, ImVec2 size) {
    const auto& P = s.P();
    const int d = (int)P.params.size();
    if (ImPlot::BeginPlot(tr(ja::kParallel), size, ImPlotFlags_NoLegend)) {
        std::vector<const char*> names;
        std::vector<double> pos;
        static std::vector<std::string> shorts;
        shorts.clear();
        for (int i = 0; i < d; ++i) {
            std::string n = pname(P, i);
            if (auto k = n.find(" ("); k != std::string::npos) n = n.substr(0, k);
            shorts.push_back(i % 2 ? "\n" + n : n);
        }
        for (int i = 0; i < d; ++i) { names.push_back(shorts[i].c_str()); pos.push_back(i); }
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_None, ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(-0.45, d - 0.3, -0.02, 1.02, ImPlotCond_Always);
        ImPlot::SetupAxisTicks(ImAxis_X1, pos.data(), d, names.data());
        const auto& T = s.opt->trials();
        const int from = std::max(0, (int)T.size() - 150);
        std::vector<double> ys;
        for (int t = from; t < (int)T.size(); ++t) ys.push_back(T[t].y);
        std::vector<double> sorted = ys;
        std::sort(sorted.begin(), sorted.end());
        for (int t = from; t < (int)T.size(); ++t) {
            const double rank = (std::lower_bound(sorted.begin(), sorted.end(), T[t].y) - sorted.begin()) / std::max(1.0, sorted.size() - 1.0);
            std::vector<double> u(d);
            for (int i = 0; i < d; ++i) u[i] = aci::unit(P.params[i], T[t].x[i]);
            const float light = (float)(1.0 - rank);
            ImPlot::SetNextLineStyle({light, light, light, 0.15f + 0.75f * light * light}, 1.0f + 1.5f * light);
            ImPlot::PlotLine("##p", pos.data(), u.data(), d);
        }
        ImPlot::EndPlot();
    }
}

static void drives_plot(Session& s, ImVec2 size) {
    if (ImPlot::BeginPlot(tr(ja::kDrives), size)) {
        ImPlot::SetupAxes(tr(ja::kTrialAxis), nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.05, ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);
        if (!s.xs.empty()) {
            ImPlot::SetNextLineStyle({1.0f, 0.6f, 0.2f, 1}, 2);
            ImPlot::PlotLine(tr(ja::kCuriosity), s.xs.data(), s.curiosity.data(), (int)s.xs.size());
            ImPlot::SetNextLineStyle({0.3f, 0.85f, 0.95f, 1}, 2);
            ImPlot::PlotLine(tr(ja::kRho), s.xs.data(), s.rho.data(), (int)s.xs.size());
        }
        ImPlot::EndPlot();
    }
}

// ------------------------------------------------------------------ main

// --capture <file.rgba> [--steps N] [--problem K] [--noise F]: run N trials, render one frame
// in a hidden window, write raw RGBA pixels (width, height in the first 8 bytes) and exit.
int main(int argc, char** argv) {
    std::string capture;
    int cap_steps = 250, cap_problem = 0;
    float cap_noise = 0.0f;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--capture") capture = argv[++i];
        else if (a == "--steps") cap_steps = std::atoi(argv[++i]);
        else if (a == "--problem") cap_problem = std::atoi(argv[++i]);
        else if (a == "--noise") cap_noise = (float)std::atof(argv[++i]);
        else if (a == "--lang") g_en = std::string(argv[++i]) == "en";
    }
    if (!glfwInit()) return 1;
    if (!capture.empty()) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    GLFWwindow* win = glfwCreateWindow(1600, 980, tr(ja::kTitle), nullptr, nullptr);
    if (!win) return 1;
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    float sx = 1, sy = 1;
    glfwGetWindowContentScale(win, &sx, &sy);
    Session s;
    ImFont* big = nullptr;
    load_fonts(std::max(1.0f, sx), s.probs, &big);
    style();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    if (!capture.empty()) { s.problem = cap_problem; s.noise = cap_noise; s.budget = std::max(s.budget, cap_steps); }
    s.reset();
    if (!capture.empty()) for (int i = 0; i < cap_steps; ++i) s.step();
    int frames = 0;

    auto last = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::chrono::duration<double>(now - last).count();
        last = now;
        if (s.running) {
            s.carry += dt * s.speed;
            for (int i = 0; i < 2000 && s.carry >= 1.0; ++i) { s.step(); s.carry -= 1.0; }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::BeginChild("##left", {330, 0});
        controls(s, big);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##right", {0, 0});
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float W = (avail.x - 8) / 2, H = (avail.y - 16) / 3;
        history_plot(s, {W, H});      ImGui::SameLine(); screening_plot(s, {W, H});
        slice_plot(s, {W, H});        ImGui::SameLine(); rules_table(s, {W, H});
        parallel_plot(s, {W, H});     ImGui::SameLine(); drives_plot(s, {W, H});
        ImGui::EndChild();
        ImGui::End();

        ImGui::Render();
        int fw, fh;
        glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!capture.empty() && ++frames == 4) {
            std::vector<unsigned char> px((size_t)fw * fh * 4);
            glReadPixels(0, 0, fw, fh, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            if (FILE* f = std::fopen(capture.c_str(), "wb")) {
                const int dims[2] = {fw, fh};
                std::fwrite(dims, sizeof dims, 1, f);
                std::fwrite(px.data(), 1, px.size(), f);
                std::fclose(f);
            }
            break;
        }
        glfwSwapBuffers(win);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
