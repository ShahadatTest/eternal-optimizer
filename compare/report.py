"""Summarise compare/results.json into compare/REPORT.md and compare/compare.png."""
import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from problems import PROBLEMS, ROOT  # noqa: E402

LABEL = dict(eternal="Eternal (ACI)", optuna="Optuna (TPE)", ax="Ax (BoTorch)", random="Random")
COLOR = dict(eternal="#111111", optuna="#2b7bba", ax="#d9480f", random="#9a9a9a")
ORDER = ["eternal", "optuna", "ax", "random"]


def main():
    data = json.load(open(ROOT / "compare" / "results.json"))
    budget, R = data["budget"], data["results"]
    scen = sorted({(r["problem"], r["noise"]) for r in R})
    by = {}
    for r in R:
        by.setdefault((r["problem"], r["noise"], r["method"]), {})[r["seed"]] = r

    lines = [f"# Eternal vs Optuna vs Ax vs random search\n",
             f"Budget {budget} trials, {data['seeds']} seeds per scenario ({data.get('ax_seeds', data['seeds'])} for Ax, "
             f"the slowest). Score: true gap to the optimum of the best observed point (lower is better). "
             f"Median over seeds; bold is the best median. Ranks use only the seeds every method ran.\n",
             "| Problem | Noise | Trials | " + " | ".join(LABEL[m] for m in ORDER) + " |",
             "|---|---|---|" + "---|" * len(ORDER)]
    rank_rows = []
    totals = {m: [] for m in ORDER}
    fig, axes = plt.subplots(2, 4, figsize=(20, 8.5), sharex=True)
    for ci, (k, noise) in enumerate(scen):
        ax = axes[int(noise > 0)][k]
        for checkpoint in (25, 50, budget):
            meds = {}
            for m in ORDER:
                runs = by.get((k, noise, m), {})
                vals = [r["curve"][checkpoint - 1] for r in runs.values() if len(r["curve"]) >= checkpoint]
                meds[m] = np.median(vals) if vals else float("nan")
            best = min(v for v in meds.values() if v == v)
            cells = [(f"**{meds[m]:.3g}**" if meds[m] == best else f"{meds[m]:.3g}") for m in ORDER]
            lines.append(f"| {PROBLEMS[k]['name']} | {noise:.0%} | {checkpoint} | " + " | ".join(cells) + " |")
        # average rank per seed at the end
        seeds = set.intersection(*(set(by.get((k, noise, m), {})) for m in ORDER))
        ranks = {m: [] for m in ORDER}
        for s in seeds:
            final = {m: by[(k, noise, m)][s]["curve"][-1] for m in ORDER}
            order = sorted(ORDER, key=lambda m: final[m])
            for m in ORDER:
                ranks[m].append(order.index(m) + 1)
        rank_rows.append((k, noise, {m: np.mean(ranks[m]) for m in ORDER}))
        for m in ORDER:
            totals[m] += ranks[m]
            runs = by.get((k, noise, m), {})
            if not runs:
                continue
            C = np.array([np.maximum(r["curve"], 1e-9) for r in runs.values()])
            x = np.arange(1, C.shape[1] + 1)
            med, lo, hi = np.median(C, 0), np.percentile(C, 25, 0), np.percentile(C, 75, 0)
            ax.plot(x, med, color=COLOR[m], lw=2.4 if m == "eternal" else 1.8, label=LABEL[m])
            ax.fill_between(x, lo, hi, color=COLOR[m], alpha=0.10)
        ax.set_yscale("log")
        ax.set_title(f"{PROBLEMS[k]['name']}, noise {noise:.0%}", fontsize=11)
        ax.grid(alpha=0.25)
        if k == 0:
            ax.set_ylabel("gap to optimum (median, IQR)")
        if noise > 0:
            ax.set_xlabel("trial")
    axes[0][0].legend(frameon=False, fontsize=10)
    fig.tight_layout()
    fig.savefig(ROOT / "compare" / "compare.png", dpi=130)

    lines += ["", "## Average rank at the end (1 = best of 4)", "",
              "| Problem | Noise | " + " | ".join(LABEL[m] for m in ORDER) + " |", "|---|---|" + "---|" * len(ORDER)]
    for k, noise, rk in rank_rows:
        best = min(rk.values())
        lines.append(f"| {PROBLEMS[k]['name']} | {noise:.0%} | " +
                     " | ".join(f"**{rk[m]:.2f}**" if rk[m] == best else f"{rk[m]:.2f}" for m in ORDER) + " |")
    lines.append("| **all scenarios** | | " + " | ".join(f"{np.mean(totals[m]):.2f}" for m in ORDER) + " |")
    secs = {m: np.median([r["seconds"] for r in R if r["method"] == m]) for m in ORDER}
    lines += ["", "## Wall-clock time per run (median)", "",
              "| " + " | ".join(LABEL[m] for m in ORDER) + " |", "|" + "---|" * len(ORDER),
              "| " + " | ".join(f"{secs[m]:.2f} s" for m in ORDER) + " |"]
    (ROOT / "compare" / "REPORT.md").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
