// 永遠 ─ 因果最適化ラボ
// The desktop visualizer for the Eternal optimizer. Every visible string is Japanese.
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <implot3d.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <type_traits>
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
const L kEvRecall = reg({"#%d 記憶した良い範囲から探索を開始", "#%d started from the remembered good region"});
const L kUseMemory = reg({"記憶を使う", "Use memory"});
const L kSettings = reg({"設定", "Settings"});
const L kTabOverview = reg({"概要", "Overview"});
const L kTab3D = reg({"3D・因果", "3D & causes"});
const L kTabLearn = reg({"学習", "Learning"});
const L k3DTitle = reg({"3D地形（高さ＝良さ、−log10 差）・表示専用", "3D landscape (height = quality, −log10 gap), display only"});
const L kQuality = reg({"良さ", "quality"});
const L kHint3D = reg({"ドラッグで回転・ホイールで拡大", "Drag to rotate, scroll to zoom"});
const L kGraphTitle = reg({"因果グラフ（太さ＝効果の強さ・橙＝組み合わせ）", "Causal graph (width = effect strength, orange = interacting pair)"});
const L kOutcome = reg({"結果", "Outcome"});
const L kEffect = reg({"効果/ノイズ %.3g", "effect/noise %.3g"});
const L kStudyBars = reg({"研究ごとの最終的な良さ（−log10 差・高いほど良い）", "Final quality per study (−log10 gap, higher is better)"});
const L kStudyCurves = reg({"研究ごとの推移（明るいほど新しい）", "Progress per study (brighter = newer)"});
const L kBudgetPie = reg({"試行の使い道（現在の研究）", "Where the trials went (current study)"});
const L kOverhead = reg({"研究ごとの準備コスト（初期探索＋検査の試行数）", "Setup cost per study (warm-up + screening trials)"});
const L kWithMemory = reg({"記憶あり", "with memory"});
const L kFresh = reg({"記憶なし", "fresh"});
const L kNoStudies = reg({"まだ完了した研究がありません。研究を最後まで進めるか「次の研究へ」を押してください。", "No finished studies yet. Run a study to the end or press Next study."});
const L kStudyAxis = reg({"研究", "Study"});
const L kTabMemory = reg({"記憶", "Memory"});
const L kTabOptimizer = reg({"最適化", "Optimizer"});
const L kTabDisplay = reg({"表示", "Display"});
const L kAutoRemember = reg({"研究が終わったら自動で記憶する", "Remember automatically when a study ends"});
const L kAutoNext = reg({"続けて次の研究を始める（連続学習）", "Start the next study automatically (continuous learning)"});
const L kPersist = reg({"ディスクに保存する（アプリを閉じても残る）", "Save to disk (kept after the app closes)"});
const L kSavedAt = reg({"保存先", "Saved at"});
const L kTrust = reg({"記憶の使い方", "How memory is used"});
const L kRecallShare = reg({"記憶から始める初期試行の割合", "Warm-up trials started from memory"});
const L kRecallSpread = reg({"記憶した点のまわりの広がり（範囲に対する割合）", "Spread around the remembered point (share of range)"});
const L kRecallZoom = reg({"最初の探索範囲（記憶した最良値 ± 範囲の割合）", "Starting search range (remembered best ± share of range)"});
const L kRecallConfirm = reg({"覚えている変数を確かめる介入の回数", "Confirming interventions per remembered knob"});
const L kForgetHL = reg({"忘却：古い研究の重みが半分になるまでの研究数（0＝忘れない）", "Forgetting: studies until an old one counts half (0 = never)"});
const L kContents = reg({"記憶の中身", "What is remembered"});
const L kCauseRate = reg({"原因と判定した割合", "Judged a cause"});
const L kRememberedBest = reg({"記憶した最良値", "Remembered best"});
const L kStudies = reg({"研究数", "Studies"});
const L kEmpty = reg({"まだ何も記憶していません", "Nothing remembered yet"});
const L kClearThis = reg({"この問題の記憶を消す", "Clear memory for this problem"});
const L kClearAll = reg({"すべての記憶を消す", "Clear all memory"});
const L kMemoryNote = reg({"記憶は提案するだけです。覚えた判定は、新しい研究でも必ず介入で確かめ直します。", "Memory only proposes. Every remembered verdict is re-checked by intervention in each new study."});
const L kApplyNote = reg({"変更は次のリセットから有効になります。", "Changes take effect from the next reset."});
const L kWarmupOpt = reg({"初期探索の試行数（0＝自動）", "Warm-up trials (0 = automatic)"});
const L kBins = reg({"変数ごとの区間の数", "Bins per variable"});
const L kGamma = reg({"成功とみなす上位の割合", "Share of trials counted as success"});
const L kKSigma = reg({"原因と判定する閾値（ノイズの何倍）", "Cause threshold (× noise)"});
const L kHalfLife = reg({"証拠の半減期（試行）", "Evidence half-life (trials)"});
const L kLocalShare = reg({"局所探索の割合", "Local search share"});
const L kRecheck = reg({"固定した変数を再検査する間隔（試行）", "Recheck frozen knobs every (trials)"});
const L kStall = reg({"停滞とみなすまでの試行数", "Trials before a stall"});
const L kDefaults = reg({"既定値に戻す", "Restore defaults"});
const L kApplyNow = reg({"今すぐ適用（リセット）", "Apply now (reset)"});
const L kMemoryLine = reg({"記憶", "Memory"});
const L kMemOn = reg({"%d 研究分・オン", "on (%d)"});
const L kMemOff = reg({"オフ", "off"});
const L kMemoryCount = reg({"記憶している研究", "Studies remembered"});
const L kNextStudy = reg({"次の研究へ（この研究を記憶）", "Next study (remember this one)"});
const L kForget = reg({"記憶を消す", "Clear memory"});
const L kEvDoubt = reg({"#%d 『%s』は判定が際どい → 再検査", "#%d %s is borderline → testing again"});
const L kKindNames[] = {reg({"初期探索", "Warm-up"}), reg({"再測定", "Re-measure"}), reg({"介入", "Intervene"}), reg({"活用", "Exploit"}), reg({"探索", "Explore"}), reg({"局所探索", "Local search"}), reg({"ランダム", "Random"}), reg({"記憶から", "From memory"})};
}  // namespace ja

static const ImVec4 kKindColor[] = {
    {0.55f, 0.55f, 0.58f, 1}, {0.95f, 0.85f, 0.30f, 1}, {0.95f, 0.35f, 0.75f, 1}, {0.30f, 0.85f, 0.95f, 1},
    {1.00f, 0.60f, 0.20f, 1}, {0.45f, 0.95f, 0.45f, 1}, {0.70f, 0.70f, 0.70f, 1}, {0.75f, 0.60f, 1.00f, 1}};

// ------------------------------------------------------------------ fonts

static std::string first_existing(std::initializer_list<const char*> paths) {
    for (const char* p : paths)
        if (p && fs::exists(p)) return p;
    return {};
}

// Prefer macOS Hiragino W3/W6 when present; otherwise Noto CJK / Yu Gothic paths
// used on common Linux distros and Windows (#6).
static std::string find_regular_cjk_font() {
    for (const char* dir : {"/System/Library/Fonts", "/Library/Fonts"}) {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(dir, ec)) {
            const std::string n = e.path().filename().string();
            if (n.size() >= 6 && n.compare(n.size() - 6, 6, "W3.ttc") == 0 && n.find("GB") == std::string::npos)
                return e.path().string();
        }
    }
    return first_existing({
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",          // Debian/Ubuntu
            "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",               // Arch
            "/usr/share/google-noto-sans-cjk-fonts/NotoSansCJK-Regular.ttc", // Fedora
            "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
            "C:/Windows/Fonts/YuGothR.ttc",
            "C:/Windows/Fonts/yugothic.ttf",
            "C:/Windows/Fonts/meiryo.ttc",
            "/System/Library/Fonts/Hiragino Sans GB.ttc",
    });
}

static std::string find_bold_cjk_font() {
    for (const char* dir : {"/System/Library/Fonts", "/Library/Fonts"}) {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(dir, ec)) {
            const std::string n = e.path().filename().string();
            if (n.size() >= 6 && n.compare(n.size() - 6, 6, "W6.ttc") == 0 && n.find("GB") == std::string::npos)
                return e.path().string();
        }
    }
    return first_existing({
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc",
            "/usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc",
            "/usr/share/google-noto-sans-cjk-fonts/NotoSansCJK-Bold.ttc",
            "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Bold.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Bold.ttc",
            "C:/Windows/Fonts/YuGothB.ttc",
            "C:/Windows/Fonts/meiryob.ttc",
    });
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

    const std::string regular = find_regular_cjk_font(), bold = find_bold_cjk_font();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    if (!regular.empty()) {
        io.Fonts->AddFontFromFileTTF(regular.c_str(), 16.0f * scale, &cfg, ranges.Data);
    } else {
        // No CJK face: default font cannot draw Japanese. Prefer English UI (#6).
        if (!g_en) {
            std::fprintf(stderr,
                    "eternal: no CJK font found (tried Noto Sans CJK / Yu Gothic / Hiragino); "
                    "starting in English. Pass --lang ja after installing a CJK font.\n");
            g_en = true;
        }
        io.Fonts->AddFontDefault();
    }
    *big = !bold.empty() ? io.Fonts->AddFontFromFileTTF(bold.c_str(), 26.0f * scale, &cfg, ranges.Data)
                         : (!regular.empty() ? io.Fonts->AddFontFromFileTTF(regular.c_str(), 26.0f * scale, &cfg, ranges.Data)
                                             : nullptr);
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
    c[ImGuiCol_TitleBg] = {0.08f, 0.08f, 0.09f, 1};
    c[ImGuiCol_TitleBgActive] = {0.12f, 0.12f, 0.14f, 1};
    c[ImGuiCol_Tab] = {0.12f, 0.12f, 0.14f, 1};
    c[ImGuiCol_TabHovered] = {0.30f, 0.30f, 0.33f, 1};
    c[ImGuiCol_TabSelected] = {0.92f, 0.92f, 0.94f, 1};
    c[ImGuiCol_TabSelectedOverline] = {1, 1, 1, 1};
    c[ImGuiCol_Separator] = {0.22f, 0.22f, 0.25f, 1};
    c[ImGuiCol_ResizeGrip] = {0.25f, 0.25f, 0.28f, 1};
    c[ImGuiCol_HeaderHovered] = {0.22f, 0.22f, 0.25f, 1};
    ImPlotStyle& p = ImPlot::GetStyle();
    p.Colors[ImPlotCol_PlotBg] = {0.02f, 0.02f, 0.03f, 1};
    p.Colors[ImPlotCol_FrameBg] = {0.05f, 0.05f, 0.06f, 1};
    p.PlotPadding = {8, 8};
    ImPlot3DStyle& q = ImPlot3D::GetStyle();
    q.Colors[ImPlot3DCol_PlotBg] = {0.02f, 0.02f, 0.03f, 1};
    q.Colors[ImPlot3DCol_FrameBg] = {0.05f, 0.05f, 0.06f, 1};
    q.Colors[ImPlot3DCol_AxisGrid] = {0.30f, 0.30f, 0.34f, 1};
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
    bool use_memory = true, auto_remember = true, auto_next = false, persist = true;
    float forget_hl = 0.0f;
    aci::Options cfg;                // engine settings, applied at the next reset
    aci::Memory memory;
    int memory_problem = -1;
    bool folded = false, show_settings = false;
    struct StudyRec { double final_gap; int setup; bool memory; std::vector<double> curve; };
    std::vector<StudyRec> studies;
    bool recorded = false;
    int tab_request = -1;
    // the 3D landscape, cached per trial count
    std::vector<double> l3x, l3y, l3z;
    int l3_at = -1;

    void record() {
        if (recorded || !opt || opt->trials().empty() || gap_aci.empty()) return;
        int setup = 0;
        for (const auto& t : opt->trials())
            setup += t.kind == aci::Kind::Warmup || t.kind == aci::Kind::Remeasure || t.kind == aci::Kind::Intervene ||
                     t.kind == aci::Kind::Recall;
        studies.push_back({gap_aci.back(), setup, opt->has_memory(), gap_aci});
        recorded = true;
    }

    const aci::Problem& P() const { return probs[problem]; }

    static fs::path memory_dir() {
        const char* home = std::getenv("HOME");
        fs::path base = home ? fs::path(home) : fs::current_path();
#ifdef __APPLE__
        return base / "Library" / "Application Support" / "Eternal" / "memory";
#else
        return base / ".eternal" / "memory";
#endif
    }
    fs::path memory_path() const {
        std::string n;
        for (char c : P().name_en) n += std::isalnum((unsigned char)c) ? (char)std::tolower((unsigned char)c) : '-';
        return memory_dir() / (n + ".txt");
    }
    void load_memory() {
        memory = {};
        if (!persist) return;
        std::ifstream in(memory_path());
        if (!in) return;
        std::stringstream ss;
        ss << in.rdbuf();
        memory = aci::Memory::load(ss.str());
    }
    void save_memory() const {
        if (!persist) return;
        std::error_code ec;
        fs::create_directories(memory_dir(), ec);
        std::ofstream(memory_path()) << memory.save();
    }
    void clear_memory(bool everything) {
        memory = {};
        std::error_code ec;
        if (everything) fs::remove_all(memory_dir(), ec);
        else fs::remove(memory_path(), ec);
    }

    // fold the current study into memory, once
    void fold() {
        if (folded || !use_memory || !opt || !opt->screened()) return;
        memory = opt->remember(&memory, forget_hl);
        folded = true;
        save_memory();
    }

    // remember this study, then start the next one with a new seed
    void next_study() {
        record();
        fold();
        ++seed;
        reset();
    }

    void reset() {
        if (memory_problem != problem) { memory_problem = problem; load_memory(); studies.clear(); }
        folded = false;
        recorded = false;
        l3_at = -1;
        aci::RandomSearch probe(P().params, 4242);
        double m = 0, s = 0;
        std::vector<double> ys;
        for (int i = 0; i < 400; ++i) ys.push_back(P().f(probe.ask()));
        for (double y : ys) m += y;
        m /= ys.size();
        for (double y : ys) s += (y - m) * (y - m);
        spread = std::sqrt(s / ys.size());
        opt = std::make_unique<aci::Optimizer>(P().params, seed, cfg, use_memory ? &memory : nullptr);
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
        if (done() >= budget) {
            record();
            if (auto_remember) fold();
            if (auto_next && running) next_study();
            else running = false;
            return;
        }
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
    else if (e.what == "recall") std::snprintf(buf, sizeof buf, tr(ja::kEvRecall), e.trial + 1);
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
    if (ImGui::Button(tr(ja::kNextStudy), {-1, 30})) { s.running = false; s.next_study(); }
    if (ImGui::Button(tr(ja::kSettings), {-1, 30})) s.show_settings = true;
    ImGui::PopStyleColor();
    ImGui::TextUnformatted(tr(ja::kMemoryLine));
    ImGui::SameLine(g_en ? 175 : 190);
    if (s.use_memory) ImGui::Text(tr(ja::kMemOn), s.memory.studies);
    else ImGui::TextDisabled("%s", tr(ja::kMemOff));
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
    int shown = 0;
    for (int k : {0, 1, 2, 3, 4, 5, 7}) {
        ImGui::ColorButton("##k", kKindColor[k], ImGuiColorEditFlags_NoTooltip, {12, 12});
        ImGui::SameLine();
        ImGui::TextUnformatted(tr(ja::kKindNames[k]));
        if (shown++ % 2 == 0) ImGui::SameLine(160);
    }
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::TextUnformatted(tr(ja::kLog));
    ImGui::BeginChild("##log", {0, 0}, ImGuiChildFlags_Borders);
    const auto& ev = o.events();
    for (int i = (int)ev.size() - 1; i >= 0 && i >= (int)ev.size() - 40; --i)
        ImGui::TextWrapped("%s", event_text(ev[i], s.P()).c_str());
    ImGui::EndChild();
}

template <class T>
static bool slider(const char* label, const char* id, T* v, T lo, T hi, const char* fmt) {
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    if constexpr (std::is_same_v<T, int>) return ImGui::SliderInt(id, v, lo, hi, fmt);
    else return ImGui::SliderScalar(id, ImGuiDataType_Double, v, &lo, &hi, fmt);
}

static void settings_window(Session& s) {
    if (!s.show_settings) return;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({vp->WorkPos.x + vp->WorkSize.x * 0.5f, vp->WorkPos.y + vp->WorkSize.y * 0.5f}, ImGuiCond_Appearing, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({660, std::min(820.0f, vp->WorkSize.y - 40)}, ImGuiCond_Appearing);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.07f, 1));
    std::string title = std::string(tr(ja::kSettings)) + "###settings";
    if (!ImGui::Begin(title.c_str(), &s.show_settings, ImGuiWindowFlags_NoCollapse)) { ImGui::End(); ImGui::PopStyleColor(); return; }
    if (ImGui::BeginTabBar("##tabs")) {
        static int open_tab = 0;
        auto tab = [&](const char* label, int idx) {
            ImGui::PushStyleColor(ImGuiCol_Text, open_tab == idx ? ImVec4(0, 0, 0, 1) : ImVec4(0.9f, 0.9f, 0.92f, 1));
            const bool on = ImGui::BeginTabItem(label);
            ImGui::PopStyleColor();
            if (on) open_tab = idx;
            return on;
        };
        if (tab(tr(ja::kTabMemory), 0)) {
            ImGui::Spacing();
            if (ImGui::Checkbox(tr(ja::kUseMemory), &s.use_memory)) { s.running = false; s.reset(); }
            ImGui::BeginDisabled(!s.use_memory);
            ImGui::Checkbox(tr(ja::kAutoRemember), &s.auto_remember);
            ImGui::Checkbox(tr(ja::kAutoNext), &s.auto_next);
            if (ImGui::Checkbox(tr(ja::kPersist), &s.persist) && s.persist) s.save_memory();
            if (s.persist) ImGui::TextDisabled("%s: %s", tr(ja::kSavedAt), s.memory_path().string().c_str());
            ImGui::SeparatorText(tr(ja::kTrust));
            slider(tr(ja::kRecallShare), "##rs", &s.cfg.recall_share, 0.0, 1.0, "%.2f");
            slider(tr(ja::kRecallSpread), "##rp", &s.cfg.recall_spread, 0.01, 0.3, "%.2f");
            slider(tr(ja::kRecallZoom), "##rz", &s.cfg.recall_zoom, 0.05, 0.5, "%.2f");
            slider(tr(ja::kRecallConfirm), "##rc", &s.cfg.recall_confirm, 1, 3, "%d");
            ImGui::TextUnformatted(tr(ja::kForgetHL));
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("##fh", &s.forget_hl, 0.0f, 20.0f, "%.1f");
            ImGui::EndDisabled();
            ImGui::TextDisabled("%s", tr(ja::kApplyNote));

            ImGui::SeparatorText(tr(ja::kContents));
            if (s.memory.entries.empty()) ImGui::TextDisabled("%s", tr(ja::kEmpty));
            else if (ImGui::BeginTable("##mem", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, {0, 190})) {
                for (const char* h : {tr(ja::kVar), tr(ja::kCauseRate), tr(ja::kRememberedBest), tr(ja::kStudies)}) ImGui::TableSetupColumn(h);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();
                for (const auto& e : s.memory.entries) {
                    int i = -1;
                    for (int k = 0; k < (int)s.P().params.size(); ++k) if (s.P().params[k].name == e.name) i = k;
                    const double rate = e.seen > 0 ? e.cause_votes / e.seen : 0;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(i >= 0 ? pname(s.P(), i) : e.name.c_str());
                    ImGui::TableNextColumn(); ImGui::ProgressBar((float)rate, {-1, 0}, ""); ImGui::SameLine(8); ImGui::Text("%.0f%%", rate * 100);
                    ImGui::TableNextColumn();
                    if (rate >= 0.5 && i >= 0) ImGui::Text("%.4g", s.P().params[i].lo + e.best_u * (s.P().params[i].hi - s.P().params[i].lo));
                    else ImGui::TextDisabled("─");
                    ImGui::TableNextColumn(); ImGui::Text("%.1f", e.seen);
                }
                ImGui::EndTable();
            }
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
            const float w = (ImGui::GetContentRegionAvail().x - 8) / 2;
            if (ImGui::Button(tr(ja::kClearThis), {w, 30})) { s.clear_memory(false); s.running = false; s.reset(); }
            ImGui::SameLine();
            if (ImGui::Button(tr(ja::kClearAll), {w, 30})) { s.clear_memory(true); s.running = false; s.reset(); }
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::TextWrapped("%s", tr(ja::kMemoryNote));
            ImGui::EndTabItem();
        }
        if (tab(tr(ja::kTabOptimizer), 1)) {
            ImGui::Spacing();
            auto& c = s.cfg;
            slider(tr(ja::kWarmupOpt), "##wu", &c.warmup, 0, 60, "%d");
            slider(tr(ja::kBins), "##bn", &c.bins, 3, 12, "%d");
            slider(tr(ja::kGamma), "##gm", &c.gamma, 0.1, 0.5, "%.2f");
            slider(tr(ja::kKSigma), "##ks", &c.k_sigma, 2.0, 8.0, "%.1f");
            slider(tr(ja::kHalfLife), "##hl", &c.half_life, 8.0, 256.0, "%.0f");
            slider(tr(ja::kLocalShare), "##ls", &c.local_share, 0.0, 0.8, "%.2f");
            slider(tr(ja::kRecheck), "##rk", &c.recheck_every, 20, 300, "%d");
            slider(tr(ja::kStall), "##st", &c.stall_reset, 20, 200, "%d");
            ImGui::TextDisabled("%s", tr(ja::kApplyNote));
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
            const float w = (ImGui::GetContentRegionAvail().x - 8) / 2;
            if (ImGui::Button(tr(ja::kDefaults), {w, 30})) {
                const aci::Options d;
                c.warmup = d.warmup; c.bins = d.bins; c.gamma = d.gamma; c.k_sigma = d.k_sigma; c.half_life = d.half_life;
                c.local_share = d.local_share; c.recheck_every = d.recheck_every; c.stall_reset = d.stall_reset;
            }
            ImGui::SameLine();
            if (ImGui::Button(tr(ja::kApplyNow), {w, 30})) { s.running = false; s.reset(); }
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }
        if (tab(tr(ja::kTabDisplay), 2)) {
            ImGui::Spacing();
            ImGui::TextUnformatted(tr(ja::kLanguage));
            bool lang = false;
            if (ImGui::RadioButton("日本語##2", !g_en)) { g_en = false; lang = true; }
            ImGui::SameLine();
            if (ImGui::RadioButton("English##2", g_en)) { g_en = true; lang = true; }
            if (lang) glfwSetWindowTitle(glfwGetCurrentContext(), tr(ja::kTitle));
            ImGui::TextUnformatted(tr(ja::kSpeed));
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("##speed2", &s.speed, 1.0f, 400.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

// ------------------------------------------------------------------ 3D landscape

static void landscape3d(Session& s, ImVec2 size) {
    s.refresh_slice();
    if (s.slice_i < 0) {
        ImGui::BeginChild("##l3wait", size, ImGuiChildFlags_Borders);
        ImGui::TextUnformatted(tr(ja::k3DTitle));
        ImGui::TextDisabled("%s", s.opt->screened() ? tr(ja::kNeedTwo) : tr(ja::kWaiting));
        ImGui::EndChild();
        return;
    }
    const auto& P = s.P();
    const auto& a = P.params[s.slice_i];
    const auto& b = P.params[s.slice_j];
    const int N = 44;
    auto height = [&](double gap) { return -std::log10(std::max(1e-6, gap)); };
    if (s.l3_at != s.done()) {
        s.l3_at = s.done();
        s.l3x.assign(N * N, 0); s.l3y.assign(N * N, 0); s.l3z.assign(N * N, 0);
        auto x = s.opt->incumbent();
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j) {
                const int k = i * N + j;
                x[s.slice_i] = s.l3x[k] = a.lo + (a.hi - a.lo) * i / (N - 1.0);
                x[s.slice_j] = s.l3y[k] = b.lo + (b.hi - b.lo) * j / (N - 1.0);
                s.l3z[k] = height(P.f(x) - P.optimum);
            }
    }
    const auto [zmn, zmx] = std::minmax_element(s.l3z.begin(), s.l3z.end());
    ImGui::BeginChild("##l3", size, ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(tr(ja::k3DTitle));
    ImGui::SameLine();
    ImGui::TextDisabled("  %s", tr(ja::kHint3D));
    if (ImPlot3D::BeginPlot("##landscape", ImGui::GetContentRegionAvail(), ImPlot3DFlags_NoLegend | ImPlot3DFlags_NoTitle)) {
        ImPlot3D::SetupAxes(pname(P, s.slice_i), pname(P, s.slice_j), tr(ja::kQuality));
        ImPlot3D::SetupAxesLimits(a.lo, a.hi, b.lo, b.hi, *zmn, *zmx + 0.25 * (*zmx - *zmn) + 1e-9, ImPlot3DCond_Always);
        ImPlot3D::SetupBoxRotation(28, -55, false, ImPlot3DCond_Once);
        ImPlot3D::PushColormap(ImPlot3DColormap_Greys);
        ImPlot3D::PlotSurface("##surface", s.l3x.data(), s.l3y.data(), s.l3z.data(), N, N, *zmn, *zmx,
                              ImPlot3DSurfaceFlags_NoMarkers);
        ImPlot3D::PopColormap();
        for (int k : {0, 1, 2, 3, 4, 5, 7}) {
            std::vector<double> tx, ty, tz;
            for (const auto& tr_ : s.opt->trials())
                if ((int)tr_.kind == k) {
                    tx.push_back(tr_.x[s.slice_i]); ty.push_back(tr_.x[s.slice_j]);
                    tz.push_back(std::clamp(height(P.f(tr_.x) - P.optimum), *zmn, *zmx + 0.25 * (*zmx - *zmn)));
                }
            if (tx.empty()) continue;
            ImPlot3D::SetNextMarkerStyle(ImPlot3DMarker_Circle, 3.0f, kKindColor[k], 0.0f);
            ImPlot3D::PlotScatter("##kind", tx.data(), ty.data(), tz.data(), (int)tx.size());
        }
        const auto& inc = s.opt->incumbent();
        double ix = inc[s.slice_i], iy = inc[s.slice_j], iz = std::clamp(height(P.f(inc) - P.optimum), *zmn, *zmx + 0.25 * (*zmx - *zmn));
        ImPlot3D::SetNextMarkerStyle(ImPlot3DMarker_Diamond, 9.0f, {1, 1, 1, 1}, 1.5f, {0, 0, 0, 1});
        ImPlot3D::PlotScatter("##best", &ix, &iy, &iz, 1);
        ImPlot3D::EndPlot();
    }
    ImGui::EndChild();
}

// ------------------------------------------------------------------ causal graph

static void causal_graph(Session& s, ImVec2 size) {
    ImGui::BeginChild("##graph", size, ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(tr(ja::kGraphTitle));
    const auto& P = s.P();
    const int d = (int)P.params.size();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o = ImGui::GetCursorScreenPos(), av = ImGui::GetContentRegionAvail();
    const float row = av.y / d, bw = std::min(280.0f, av.x * 0.46f), bh = std::min(54.0f, row * 0.8f);
    const float lx = o.x + 64, ox = o.x + av.x - 150, oy = o.y + av.y * 0.5f;
    auto knob = [&](int i) { return ImVec2(lx, o.y + row * (i + 0.5f)); };
    const ImU32 white = IM_COL32(240, 240, 244, 255), grey = IM_COL32(80, 80, 88, 255), dim = IM_COL32(120, 120, 128, 255);

    // interacting pairs: arcs on the left
    for (const auto& pr : s.opt->pair_rules(3)) {
        if (pr.z <= 0) continue;
        const ImVec2 a = knob(pr.pi), b = knob(pr.pj);
        const float bulge = 22 + 10 * std::abs(pr.pi - pr.pj);
        const int alpha = (int)std::clamp(90 + 60 * pr.z, 90.0, 240.0);
        dl->AddBezierCubic(a, {a.x - bulge, a.y}, {b.x - bulge, b.y}, b, IM_COL32(255, 153, 51, alpha), 1.5f + std::min(3.0f, (float)pr.z));
    }
    // edges to the outcome
    for (int i = 0; i < d; ++i) {
        const auto& v = s.opt->verdicts()[i];
        const ImVec2 a = {knob(i).x + bw, knob(i).y}, b = {ox, oy};
        const ImVec2 c1 = {a.x + (b.x - a.x) * 0.5f, a.y}, c2 = {a.x + (b.x - a.x) * 0.5f, b.y};
        if (v.tested && v.cause) {
            const float w = 1.5f + 5.0f * std::clamp((float)std::log10(std::max(1.0, v.snr)) / 6.0f, 0.0f, 1.0f);
            dl->AddBezierCubic(a, c1, c2, b, white, w);
        } else {
            // dashed: sample the curve and draw every other piece
            ImVec2 prev = a;
            for (int k = 1; k <= 40; ++k) {
                const float t = k / 40.0f, u = 1 - t;
                const ImVec2 q = {u * u * u * a.x + 3 * u * u * t * c1.x + 3 * u * t * t * c2.x + t * t * t * b.x,
                                  u * u * u * a.y + 3 * u * u * t * c1.y + 3 * u * t * t * c2.y + t * t * t * b.y};
                if (k % 2) dl->AddLine(prev, q, v.tested ? grey : IM_COL32(60, 60, 66, 255), 1.2f);
                prev = q;
            }
        }
    }
    // knob nodes
    for (int i = 0; i < d; ++i) {
        const auto& v = s.opt->verdicts()[i];
        const ImVec2 c = knob(i), tl = {c.x, c.y - bh / 2}, br = {c.x + bw, c.y + bh / 2};
        const bool cause = v.tested && v.cause;
        dl->AddRectFilled(tl, br, cause ? IM_COL32(28, 28, 32, 255) : IM_COL32(14, 14, 16, 255), 4);
        dl->AddRect(tl, br, cause ? white : grey, 4, 0, cause ? 1.5f : 1.0f);
        const float fh = ImGui::GetFontSize();
        const bool two = v.tested && bh >= 2 * fh + 6;
        dl->AddText({tl.x + 10, two ? c.y - fh - 1 : c.y - fh * 0.5f}, cause ? white : dim, pname(P, i));
        if (two) {
            char buf[64];
            if (cause) std::snprintf(buf, sizeof buf, tr(ja::kEffect), v.snr);
            dl->AddText({tl.x + 10, c.y + 1}, cause ? IM_COL32(150, 150, 158, 255) : grey, cause ? buf : tr(ja::kFrozen));
        }
    }
    // the outcome
    const ImVec2 otl = {ox, oy - 34}, obr = {ox + 140, oy + 34};
    dl->AddRectFilled(otl, obr, white, 6);
    const char* oname = tr(ja::kOutcome);
    const ImVec2 osz = ImGui::CalcTextSize(oname);
    dl->AddText({ox + 70 - osz.x / 2, oy - osz.y - 2}, IM_COL32(0, 0, 0, 255), oname);
    char best[48];
    std::snprintf(best, sizeof best, "%.5g", s.opt->trials().empty() ? 0.0 : P.f(s.opt->incumbent()));
    const ImVec2 bsz = ImGui::CalcTextSize(best);
    dl->AddText({ox + 70 - bsz.x / 2, oy + 2}, IM_COL32(0, 0, 0, 255), best);
    ImGui::Dummy(av);
    ImGui::EndChild();
}

// ------------------------------------------------------------------ learning across studies

static void study_bars(Session& s, ImVec2 size) {
    if (s.studies.empty()) {
        ImGui::BeginChild("##sbw", size, ImGuiChildFlags_Borders);
        ImGui::TextUnformatted(tr(ja::kStudyBars));
        ImGui::TextWrapped("%s", tr(ja::kNoStudies));
        ImGui::EndChild();
        return;
    }
    if (ImPlot::BeginPlot(tr(ja::kStudyBars), size)) {
        ImPlot::SetupAxes(tr(ja::kStudyAxis), nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0f");
        ImPlot::SetupAxisTicks(ImAxis_X1, 1, (double)s.studies.size(), (int)s.studies.size());
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);
        std::vector<double> xm, ym, xf, yf;
        for (int i = 0; i < (int)s.studies.size(); ++i)
            (s.studies[i].memory ? xm : xf).push_back(i + 1), (s.studies[i].memory ? ym : yf).push_back(-std::log10(std::max(1e-6, s.studies[i].final_gap)));
        ImPlot::SetNextFillStyle({0.45f, 0.45f, 0.5f, 1});
        if (!xf.empty()) ImPlot::PlotBars(tr(ja::kFresh), xf.data(), yf.data(), (int)xf.size(), 0.6);
        ImPlot::SetNextFillStyle({0.95f, 0.95f, 0.97f, 1});
        if (!xm.empty()) ImPlot::PlotBars(tr(ja::kWithMemory), xm.data(), ym.data(), (int)xm.size(), 0.6);
        ImPlot::EndPlot();
    }
}

static void study_curves(Session& s, ImVec2 size) {
    if (ImPlot::BeginPlot(tr(ja::kStudyCurves), size, ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes(tr(ja::kTrialAxis), nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        const int n = (int)s.studies.size();
        for (int i = 0; i < n; ++i) {
            const auto& c = s.studies[i].curve;
            std::vector<double> x(c.size());
            for (size_t k = 0; k < c.size(); ++k) x[k] = k + 1;
            const float light = 0.25f + 0.75f * (i + 1.0f) / n;
            ImPlot::SetNextLineStyle({light, light, light, 0.9f}, i == n - 1 ? 2.5f : 1.5f);
            ImPlot::PlotLine("##c", x.data(), c.data(), (int)c.size());
        }
        if (!s.xs.empty() && !s.recorded) {   // the study in progress, in its kind colour for memory
            ImPlot::SetNextLineStyle(kKindColor[7], 2.0f);
            ImPlot::PlotLine("##now", s.xs.data(), s.gap_aci.data(), (int)s.xs.size());
        }
        ImPlot::EndPlot();
    }
}

static void budget_pie(Session& s, ImVec2 size) {
    if (ImPlot::BeginPlot(tr(ja::kBudgetPie), size, ImPlotFlags_Equal | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
        std::vector<const char*> labels;
        std::vector<double> vals;
        std::vector<ImVec4> cols;
        std::vector<int> kinds;
        for (int k : {0, 7, 1, 2, 3, 4, 5}) {
            int n = 0;
            for (const auto& t : s.opt->trials()) n += (int)t.kind == k;
            if (!n) continue;
            labels.push_back(tr(ja::kKindNames[k]));
            vals.push_back(n);
            cols.push_back(kKindColor[k]);
            kinds.push_back(k);
        }
        if (!vals.empty()) {
            static ImPlotColormap cmap = -1;
            static std::vector<int> last;
            if (kinds != last) {   // a colormap matching the kinds shown
                last = kinds;
                char name[32];
                static int counter = 0;
                std::snprintf(name, sizeof name, "kinds%d", counter++);
                cmap = cols.size() > 1 ? ImPlot::AddColormap(name, cols.data(), (int)cols.size(), true) : -1;
            }
            if (cmap >= 0) ImPlot::PushColormap(cmap);
            else ImPlot::SetNextFillStyle(cols[0]);
            ImPlot::PlotPieChart(labels.data(), vals.data(), (int)vals.size(), 0.5, 0.5, 0.42, "%.0f", 90);
            if (cmap >= 0) ImPlot::PopColormap();
        }
        ImPlot::EndPlot();
    }
}

static void overhead_bars(Session& s, ImVec2 size) {
    if (ImPlot::BeginPlot(tr(ja::kOverhead), size)) {
        ImPlot::SetupAxes(tr(ja::kStudyAxis), tr(ja::kTrialAxis), ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0f");
        if (!s.studies.empty()) ImPlot::SetupAxisTicks(ImAxis_X1, 1, (double)s.studies.size(), (int)s.studies.size());
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);
        std::vector<double> xm, ym, xf, yf;
        for (int i = 0; i < (int)s.studies.size(); ++i)
            (s.studies[i].memory ? xm : xf).push_back(i + 1), (s.studies[i].memory ? ym : yf).push_back(s.studies[i].setup);
        ImPlot::SetNextFillStyle({0.45f, 0.45f, 0.5f, 1});
        if (!xf.empty()) ImPlot::PlotBars(tr(ja::kFresh), xf.data(), yf.data(), (int)xf.size(), 0.6);
        ImPlot::SetNextFillStyle({0.95f, 0.95f, 0.97f, 1});
        if (!xm.empty()) ImPlot::PlotBars(tr(ja::kWithMemory), xm.data(), ym.data(), (int)xm.size(), 0.6);
        ImPlot::EndPlot();
    }
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
            for (int k : {0, 1, 2, 3, 4, 5, 7}) {
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
        for (int k : {0, 1, 2, 3, 4, 5, 7}) {
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
    int cap_settings = 0, cap_tab = 0, cap_studies = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--capture") capture = argv[++i];
        else if (a == "--steps") cap_steps = std::atoi(argv[++i]);
        else if (a == "--problem") cap_problem = std::atoi(argv[++i]);
        else if (a == "--noise") cap_noise = (float)std::atof(argv[++i]);
        else if (a == "--lang") g_en = std::string(argv[++i]) == "en";
        else if (a == "--settings") cap_settings = std::atoi(argv[++i]);
        else if (a == "--tab") cap_tab = std::atoi(argv[++i]);
        else if (a == "--studies") cap_studies = std::atoi(argv[++i]);
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
    ImPlot3D::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    float sx = 1, sy = 1;
    glfwGetWindowContentScale(win, &sx, &sy);
    Session s;
    ImFont* big = nullptr;
    load_fonts(std::max(1.0f, sx), s.probs, &big);
    style();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    if (!capture.empty()) { s.problem = cap_problem; s.noise = cap_noise; s.budget = cap_steps; s.persist = false; }
    s.reset();
    if (!capture.empty()) {
        s.persist = false; s.memory = {};
        for (int k = 0; k < cap_studies; ++k) {
            if (k) s.next_study();
            for (int i = 0; i < cap_steps; ++i) s.step();
        }
        if (cap_studies > 1) s.record();
        s.fold();
        s.show_settings = cap_settings != 0;
        s.tab_request = cap_tab;
    }
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
        if (ImGui::BeginTabBar("##views")) {
            const char* names[3] = {tr(ja::kTabOverview), tr(ja::kTab3D), tr(ja::kTabLearn)};
            for (int v = 0; v < 3; ++v) {
                const ImGuiTabItemFlags f = s.tab_request == v ? ImGuiTabItemFlags_SetSelected : 0;
                std::string id = std::string(names[v]) + "###view" + std::to_string(v);
                if (!ImGui::BeginTabItem(id.c_str(), nullptr, f)) continue;
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const float W = (avail.x - 8) / 2;
                if (v == 0) {
                    const float H = (avail.y - 16) / 3;
                    history_plot(s, {W, H});      ImGui::SameLine(); screening_plot(s, {W, H});
                    slice_plot(s, {W, H});        ImGui::SameLine(); rules_table(s, {W, H});
                    parallel_plot(s, {W, H});     ImGui::SameLine(); drives_plot(s, {W, H});
                } else if (v == 1) {
                    landscape3d(s, {avail.x * 0.58f, avail.y});
                    ImGui::SameLine();
                    causal_graph(s, {0, avail.y});
                } else {
                    const float H = (avail.y - 8) / 2;
                    study_bars(s, {W, H});   ImGui::SameLine(); study_curves(s, {W, H});
                    budget_pie(s, {W, H});   ImGui::SameLine(); overhead_bars(s, {W, H});
                }
                ImGui::EndTabItem();
            }
            s.tab_request = -1;
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
        ImGui::End();
        settings_window(s);

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
    ImPlot3D::DestroyContext();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
