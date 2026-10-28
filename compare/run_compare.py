"""
Head to head: Eternal (ACI) vs Optuna (TPE) vs Ax (BoTorch) vs random search.

Same functions (verified identical to the C++ ones), same budget, same noise model, same scoring:
after each trial, the true gap to the optimum of the point with the best observed (noisy) value.
Every library runs with its defaults.

  .venv-compare/bin/python compare/run_compare.py [budget] [seeds] [processes] [ax_seeds]

Ax is by far the slowest (about 20 minutes per 100-trial run here), so it can be given fewer seeds.
Runs already in compare/results.json (or results_partial.json) are kept and not repeated.
"""
import json
import logging
import multiprocessing as mp
import subprocess
import sys
import time
import warnings
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from problems import PROBLEMS, ROOT, TOOL, sigma  # noqa: E402

NOISES = [0.0, 0.02]
METHODS = ["eternal", "optuna", "ax", "random"]


def scorer(p, sd, seed):
    """A noisy objective, and a tracker of the true gap of the best observed point."""
    rng = np.random.default_rng(31337 + seed)
    state = {"best": float("inf"), "gap": None, "curve": []}

    def f(x):
        y = p["f"](x) + (rng.normal(0, sd) if sd > 0 else 0.0)
        if y < state["best"]:
            state["best"], state["gap"] = y, p["f"](x) - p["optimum"]
        state["curve"].append(state["gap"])
        return y
    return f, state


def run_eternal(k, noise, seed, budget):
    out = subprocess.run([str(TOOL), "run", str(k), str(seed), str(budget), str(noise)],
                         capture_output=True, text=True, check=True).stdout
    return json.loads(out)["gap"]


def run_random(k, noise, seed, budget):
    p = PROBLEMS[k]
    f, st = scorer(p, sigma(k, noise), seed)
    rng = np.random.default_rng(seed)
    for _ in range(budget):
        f([rng.uniform(lo, hi) for lo, hi in p["bounds"]])
    return st["curve"]


def run_optuna(k, noise, seed, budget):
    import optuna
    optuna.logging.set_verbosity(optuna.logging.ERROR)
    p = PROBLEMS[k]
    f, st = scorer(p, sigma(k, noise), seed)
    study = optuna.create_study(sampler=optuna.samplers.TPESampler(seed=seed))
    study.optimize(lambda t: f([t.suggest_float(f"x{i}", lo, hi) for i, (lo, hi) in enumerate(p["bounds"])]),
                   n_trials=budget)
    return st["curve"]


def run_ax(k, noise, seed, budget):
    warnings.filterwarnings("ignore")
    logging.disable(logging.CRITICAL)
    import torch
    torch.set_num_threads(1)
    from ax import Client, RangeParameterConfig
    p = PROBLEMS[k]
    f, st = scorer(p, sigma(k, noise), seed)
    c = Client(random_seed=seed)
    c.configure_experiment(parameters=[RangeParameterConfig(name=f"x{i}", parameter_type="float", bounds=b)
                                       for i, b in enumerate(p["bounds"])])
    c.configure_optimization(objective="-y")
    d = len(p["bounds"])
    for _ in range(budget):
        for idx, prm in c.get_next_trials(max_trials=1).items():
            c.complete_trial(trial_index=idx, raw_data={"y": f([prm[f"x{i}"] for i in range(d)])})
    return st["curve"]


RUNNERS = dict(eternal=run_eternal, optuna=run_optuna, ax=run_ax, random=run_random)


def job(args):
    method, k, noise, seed, budget = args
    t0 = time.time()
    curve = RUNNERS[method](k, noise, seed, budget)
    return dict(method=method, problem=k, noise=noise, seed=seed, curve=curve, seconds=time.time() - t0)


def main():
    budget = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    seeds = int(sys.argv[2]) if len(sys.argv) > 2 else 10
    procs = int(sys.argv[3]) if len(sys.argv) > 3 else 8
    ax_seeds = int(sys.argv[4]) if len(sys.argv) > 4 else seeds
    n_seeds = lambda m: ax_seeds if m == "ax" else seeds
    results, seen = [], set()
    for name in ("results.json", "results_partial.json"):
        path = ROOT / "compare" / name
        if not path.exists():
            continue
        old = json.load(open(path))
        if old.get("budget") != budget:
            continue
        for r in old["results"]:
            key = (r["method"], r["problem"], r["noise"], r["seed"])
            if key not in seen and r["seed"] < n_seeds(r["method"]):
                seen.add(key)
                results.append(r)
    print(f"resuming with {len(results)} finished runs", flush=True)
    jobs = [(m, k, n, s, budget) for m in METHODS for k in range(len(PROBLEMS)) for n in NOISES for s in range(n_seeds(m))
            if (m, k, n, s) not in seen]
    jobs.sort(key=lambda j: j[0] != "ax")          # the slow ones first
    total = len(results) + len(jobs)
    with mp.get_context("spawn").Pool(procs) as pool:
        for i, r in enumerate(pool.imap_unordered(job, jobs), 1):
            results.append(r)
            if i % 5 == 0 or i == len(jobs):
                print(f"{len(results)}/{total} done", flush=True)
                json.dump(dict(budget=budget, seeds=seeds, ax_seeds=ax_seeds, results=results),
                          open(ROOT / "compare" / "results.json", "w"))
    print("saved compare/results.json")


if __name__ == "__main__":
    main()
