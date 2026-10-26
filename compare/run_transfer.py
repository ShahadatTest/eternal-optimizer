"""
Transfer across related tasks: eight ramen shops in a row (tools/transfer.cpp), shop 6 a trap where
the day of the week matters.

  eternal_fresh   Eternal, every shop from scratch
  eternal_memory  Eternal with memory carried from shop to shop (proposes; intervention re-checks)
  optuna_fresh    Optuna TPE from scratch
  optuna_warm     Optuna TPE given every trial of the previous shop as history (study.add_trials)
  ax_warm         Ax given every trial of the previous shop as history (attach + complete)
  random          random search

Score per shop: true gap of the best observed point among that shop's own new trials.

  .venv-compare/bin/python compare/run_transfer.py [budget] [shops] [seeds] [processes]
"""
import json
import logging
import multiprocessing as mp
import subprocess
import sys
import warnings
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "build" / "eternal_transfer"
BOUNDS = [(60, 300), (60, 99), (2, 12), (0, 30), (2, 15), (-5, 35), (0, 7), (0, 10)]
NOISES = [0.0, 0.02]
METHODS = ["eternal_fresh", "eternal_memory", "optuna_fresh", "optuna_warm", "ax_warm", "random"]


class ShopEval:
    def __init__(self, family, shop, noise, seed):
        self.p = subprocess.Popen([str(TOOL), "eval", str(family), str(shop)], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True, bufsize=1)
        sd = float(subprocess.run([str(TOOL), "sigma", str(family), str(shop), str(noise)],
                                  capture_output=True, text=True, check=True).stdout)
        self.sd, self.rng = sd, np.random.default_rng(1000 * seed + shop)
        self.best, self.gap, self.curve, self.trials = float("inf"), None, [], []

    def true(self, x):
        self.p.stdin.write(" ".join(repr(float(v)) for v in x) + "\n")
        return float(self.p.stdout.readline())

    def __call__(self, x):
        t = self.true(x)
        y = t + (self.rng.normal(0, self.sd) if self.sd > 0 else 0.0)
        if y < self.best:
            self.best, self.gap = y, t + 100.0
        self.curve.append(self.gap)
        self.trials.append((list(map(float, x)), y))
        return y

    def close(self):
        self.p.stdin.close()
        self.p.wait()


def seq_eternal(memory, family, shops, budget, noise, seed):
    out = subprocess.run([str(TOOL), "run", str(family), str(shops), str(budget), str(int(memory)), str(noise), str(seed)],
                         capture_output=True, text=True, check=True).stdout
    return [s["gap"] for s in json.loads(out)["shops"]]


def seq_random(family, shops, budget, noise, seed):
    curves = []
    rng = np.random.default_rng(seed)
    for j in range(shops):
        f = ShopEval(family, j, noise, seed)
        for _ in range(budget):
            f([rng.uniform(lo, hi) for lo, hi in BOUNDS])
        curves.append(f.curve)
        f.close()
    return curves


def seq_optuna(warm, family, shops, budget, noise, seed):
    import optuna
    optuna.logging.set_verbosity(optuna.logging.ERROR)
    dists = {f"x{i}": optuna.distributions.FloatDistribution(lo, hi) for i, (lo, hi) in enumerate(BOUNDS)}
    curves, previous = [], []
    for j in range(shops):
        f = ShopEval(family, j, noise, seed)
        study = optuna.create_study(sampler=optuna.samplers.TPESampler(seed=seed * 100 + j))
        if warm and previous:
            study.add_trials([optuna.trial.create_trial(params={f"x{i}": v for i, v in enumerate(x)},
                                                        distributions=dists, value=y) for x, y in previous])
        study.optimize(lambda t: f([t.suggest_float(f"x{i}", lo, hi) for i, (lo, hi) in enumerate(BOUNDS)]),
                       n_trials=budget)
        curves.append(f.curve)
        previous = f.trials
        f.close()
    return curves


def seq_ax(family, shops, budget, noise, seed):
    warnings.filterwarnings("ignore")
    logging.disable(logging.CRITICAL)
    import torch
    torch.set_num_threads(1)
    from ax import Client, RangeParameterConfig
    curves, previous = [], []
    for j in range(shops):
        f = ShopEval(family, j, noise, seed)
        c = Client(random_seed=seed * 100 + j)
        c.configure_experiment(parameters=[RangeParameterConfig(name=f"x{i}", parameter_type="float", bounds=b)
                                           for i, b in enumerate(BOUNDS)])
        c.configure_optimization(objective="-y")
        for x, y in previous:
            idx = c.attach_trial(parameters={f"x{i}": v for i, v in enumerate(x)})
            c.complete_trial(trial_index=idx, raw_data={"y": y})
        for _ in range(budget):
            for idx, prm in c.get_next_trials(max_trials=1).items():
                c.complete_trial(trial_index=idx, raw_data={"y": f([prm[f"x{i}"] for i in range(8)])})
        curves.append(f.curve)
        previous = f.trials
        f.close()
    return curves


def job(a):
    method, noise, seed, budget, shops = a
    family = seed + 1
    if method == "eternal_fresh":
        c = seq_eternal(False, family, shops, budget, noise, seed)
    elif method == "eternal_memory":
        c = seq_eternal(True, family, shops, budget, noise, seed)
    elif method == "optuna_fresh":
        c = seq_optuna(False, family, shops, budget, noise, seed)
    elif method == "optuna_warm":
        c = seq_optuna(True, family, shops, budget, noise, seed)
    elif method == "ax_warm":
        c = seq_ax(family, shops, budget, noise, seed)
    else:
        c = seq_random(family, shops, budget, noise, seed)
    return dict(method=method, noise=noise, seed=seed, curves=c)


def main():
    budget = int(sys.argv[1]) if len(sys.argv) > 1 else 60
    shops = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    seeds = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    procs = int(sys.argv[4]) if len(sys.argv) > 4 else 8
    jobs = [(m, n, s, budget, shops) for m in METHODS for n in NOISES for s in range(seeds)]
    jobs.sort(key=lambda j: j[0] != "ax_warm")
    results = []
    with mp.get_context("spawn").Pool(procs) as pool:
        for i, r in enumerate(pool.imap_unordered(job, jobs), 1):
            results.append(r)
            print(f"{i}/{len(jobs)} {r['method']} noise={r['noise']} seed={r['seed']}", flush=True)
            json.dump(dict(budget=budget, shops=shops, seeds=seeds, results=results),
                      open(ROOT / "compare" / "transfer_results.json", "w"))


if __name__ == "__main__":
    main()
