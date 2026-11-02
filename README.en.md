# Eternal — a causal optimizer

[日本語](README.md) · [Contributing](CONTRIBUTING.md) · [Changelog](CHANGELOG.md)

**Give it something you can measure. It finds out how to make it better — and which knobs actually matter — without being told how the system works.**

Eternal turns Lunaform's causal AI (ACI) into a black-box optimizer. It uses the same ask/tell
interface as Optuna or Ax. The difference: before it fits any statistical model, it **intervenes**.

```bash
pip install eternal-optimizer
```

```python
import eternal

def bake(temp, pressure, time, color, label):   # color and label do nothing
    return (temp - 210) ** 2 / 100 + (pressure - 3.2) ** 2 + abs(time - 35) / 10

r = eternal.minimize(bake, {"temp": (150, 250), "pressure": (1, 5), "time": (10, 60),
                            "color": (0, 1), "label": (0, 1)}, budget=200, seed=1)
print(r.best, r.best_params)
print("causes:", r.causes)   # ['temp', 'pressure', 'time']
print("frozen:", r.frozen)   # ['color', 'label']
```

Ask/tell, for when each measurement is an experiment you run yourself:

```python
opt = eternal.Optimizer({"temp": (150, 250), "pressure": (1, 5)}, seed=1)
for _ in range(100):
    x = opt.ask()            # {"temp": ..., "pressure": ...}
    opt.tell(measure(**x))   # smaller is better; negate to maximize
print(opt.verdicts)          # per knob: cause or not, effect size, signal/noise
```

## How it works

1. **Intervene** — around the current best point, move one parameter at a time. Re-measure the best point to learn the noise.
2. **Necessity** — a parameter whose interventions never move the result beyond the noise is "not a cause" and is frozen.
3. **Rules** — the remaining ranges are split into bins; each bin (and pair of bins) keeps decayed Beta counts of landing in the top γ.
4. **Gate** — per parameter, exploration shrinks as confidence z grows: ρ = min(ε, 1/(1+z²)), ε = 0.15 + 0.5·C(t). A stronger pair rule wins.
5. **Crystallize** — when one bin is clearly best, the parameter's range zooms onto it.
6. **Recheck** — frozen parameters are re-tested from time to time, so a wrong call can be undone.

## Benchmarks (budget 200, 20 seeds, vs. random search)

| Problem | Noise | Eternal gap (median) | Random gap | Wins | Irrelevant knobs frozen |
|---|---|---|---|---|---|
| Ramen taste | 0% | 0.13 | 2.80 | 20/20 | 20/20 |
| Ramen taste | 2% | 0.62 | 2.97 | 19/20 | 20/20 |
| Branin + 6 dummy dims | 0% | 0.016 | 0.227 | 17/20 | 20/20 |
| Branin + 6 dummy dims | 2% | 0.31 | 0.34 | 13/20 | 20/20 |
| Hartmann 6 | 0% | 0.15 | 1.00 | 20/20 | — |
| Hartmann 6 | 2% | 0.15 | 1.00 | 20/20 | — |
| Rosenbrock 4 | 0% | 2.26 | 16.1 | 20/20 | — |
| Rosenbrock 4 | 2% | 13.4 | 24.3 | 16/20 | — |

**Weaknesses:** with noise, when the remaining improvement near the optimum is smaller than the noise
(noisy Branin), Eternal is about even with random search. A cause weaker than the noise can be frozen
(it comes back on recheck).

## Versus Optuna and Ax (same problems, noise and scoring)

**One-shot optimization** (budget 100, 10 seeds, Ax 5): mean final rank among 4 methods (1 = best).
Details in [compare/REPORT.md](compare/REPORT.md).

| Eternal (ACI) | Optuna (TPE) | Ax (BoTorch) | Random |
|---|---|---|---|
| 2.60 | 2.42 | **1.25** | 3.73 |
| 0.02 s/iter | 0.32 s/iter | 218 s/iter | 0.02 s/iter |

Honestly: solving one problem from scratch, Ax wins all 8 scenarios. Eternal is on par with Optuna and about 10,000× faster than Ax.

**A sequence of related problems** (8 ramen shops × 60 trials, 5 seeds; shop 6 is a trap where the day of
the week matters). Optuna and Ax get every previous trial as history. Mean final rank for shops 2–8 (6 methods):

| Noise | Eternal, from scratch | **Eternal, with memory** | Optuna | Optuna + history | Ax + history | Random |
|---|---|---|---|---|---|---|
| 0% | 4.66 | **2.20** | 3.11 | 3.14 | 2.14 | 5.74 |
| 2% | 4.71 | 2.74 | 2.71 | 3.11 | 2.06 | 5.66 |

- Eternal with memory beats Eternal from scratch in 67 of 70 runs.
- On the trap shop, Optuna with history is dragged off course (gap 11.5 vs 0.76 from scratch). Eternal treats memory as a *proposal* and re-checks it by intervention: gap 0.45, best of all methods.

In Python, carry memory between studies with `opt.remember()` and `eternal.Optimizer(..., memory=mem)`.

## Demo: an AI makes real code faster

`showcase/ai_tunes_matmul.py` drives Eternal over MCP to speed up a 512×512 matrix multiply, measured
live. Knobs: tile, order, unroll — plus color and label, which do nothing. It is not told how anything works.

| Seed | Eternal best | Random best | Eternal's verdict |
|---|---|---|---|
| 7 | 8.52 ms | 8.44 ms | causes = tile, order / frozen = unroll, color, label |
| 8 | 8.73 ms | 8.83 ms | same |
| 9 | 9.40 ms | 9.08 ms | same |

The textbook setting (tile=64, ijk) takes 67.7 ms; Eternal's best is about **7.8× faster**. Random search
is just as fast here — one knob (loop order) dominates, so luck finds it. What Eternal adds is that it can
**explain what matters**.

## MCP server (let an AI run experiments)

`build/eternal_mcp` is an MCP server. Claude and other agents can drive Eternal with tools:

| Tool | Role |
|---|---|
| `create_study` | create a study from parameter names and ranges |
| `ask` | get the next point (warm-up, intervention, exploit, explore, local) |
| `tell` | report the measured value |
| `status` | best value, causes, frozen knobs, rules, events |
| `list_problems` / `benchmark` | compare with random search on built-in problems |

Claude Code picks up this repository's `.mcp.json` automatically.

## C++

```cpp
#include "aci_opt.hpp"

aci::Optimizer opt({{"temp", 150, 250}, {"pressure", 1, 5}, {"time", 10, 60}}, /*seed=*/1);
for (int i = 0; i < 200; ++i) {
    auto x = opt.ask();
    opt.tell(measure(x));    // smaller is better
}
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # -DETERNAL_GUI=OFF to skip the desktop app
cmake --build build -j 8
./build/eternal            # desktop app (Japanese / English, --lang en)
./build/eternal_tests      # tests
./build/eternal_bench 20 200
```

The engine (`include/aci_opt.hpp`, `src/aci_opt.cpp`) has no dependencies. GLFW 3.4, Dear ImGui 1.91.5
and ImPlot 0.16 are fetched at pinned versions for the app only.

## License

MIT — see [LICENSE](LICENSE).
