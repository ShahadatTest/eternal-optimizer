# Eternal vs Optuna vs Ax vs random search

Budget 100 trials, 10 seeds per scenario (5 for Ax, the slowest). Score: true gap to the optimum of the best observed point (lower is better). Median over seeds; bold is the best median. Ranks use only the seeds every method ran.

| Problem | Noise | Trials | Eternal (ACI) | Optuna (TPE) | Ax (BoTorch) | Random |
|---|---|---|---|---|---|---|
| ramen | 0% | 25 | 4.89 | 1.91 | **0.147** | 4.9 |
| ramen | 0% | 50 | 1.91 | 0.714 | **0.00917** | 4.6 |
| ramen | 0% | 100 | 0.725 | 0.173 | **0.00269** | 3.97 |
| ramen | 2% | 25 | 4.89 | 2.69 | **0.358** | 4.9 |
| ramen | 2% | 50 | 3.04 | 1.33 | **0.15** | 4.6 |
| ramen | 2% | 100 | 0.831 | 0.359 | **0.109** | 3.97 |
| branin+6 decoys | 0% | 25 | 1.06 | 1.09 | **0.0175** | 1.36 |
| branin+6 decoys | 0% | 50 | 0.896 | 0.279 | **0.00134** | 1.11 |
| branin+6 decoys | 0% | 100 | 0.0463 | 0.0741 | **0.000206** | 0.353 |
| branin+6 decoys | 2% | 25 | 1.59 | 2.22 | **0.238** | 2.23 |
| branin+6 decoys | 2% | 50 | 1.59 | 0.556 | **0.272** | 1.38 |
| branin+6 decoys | 2% | 100 | 0.666 | 0.474 | **0.299** | 1.07 |
| hartmann6 | 0% | 25 | 2.13 | 0.651 | **0.369** | 2.13 |
| hartmann6 | 0% | 50 | 0.657 | 0.245 | **0.0172** | 1.53 |
| hartmann6 | 0% | 100 | 0.299 | 0.116 | **0.00586** | 1.33 |
| hartmann6 | 2% | 25 | 2.13 | 0.828 | **0.518** | 2.13 |
| hartmann6 | 2% | 50 | 0.776 | 0.223 | **0.0515** | 1.53 |
| hartmann6 | 2% | 100 | 0.26 | 0.0914 | **0.0098** | 1.33 |
| rosenbrock4 | 0% | 25 | 62.5 | 20.6 | **3** | 90 |
| rosenbrock4 | 0% | 50 | 16.6 | 6.37 | **2.97** | 59.6 |
| rosenbrock4 | 0% | 100 | 4.2 | 3.11 | **0.89** | 36.4 |
| rosenbrock4 | 2% | 25 | 62.5 | 44 | **9.12** | 90 |
| rosenbrock4 | 2% | 50 | 36.1 | 20.5 | **9.12** | 66.7 |
| rosenbrock4 | 2% | 100 | 9.45 | 9.92 | **6.06** | 47.7 |

## Average rank at the end (1 = best of 4)

| Problem | Noise | Eternal (ACI) | Optuna (TPE) | Ax (BoTorch) | Random |
|---|---|---|---|---|---|
| ramen | 0% | 2.80 | 2.20 | **1.00** | 4.00 |
| ramen | 2% | 2.80 | 2.00 | **1.20** | 4.00 |
| branin+6 decoys | 0% | 2.20 | 3.20 | **1.00** | 3.60 |
| branin+6 decoys | 2% | 2.20 | 2.80 | **1.80** | 3.20 |
| hartmann6 | 0% | 2.80 | 2.00 | **1.20** | 4.00 |
| hartmann6 | 2% | 2.80 | 2.20 | **1.00** | 4.00 |
| rosenbrock4 | 0% | 2.80 | 2.40 | **1.00** | 3.80 |
| rosenbrock4 | 2% | 2.40 | 2.60 | **1.80** | 3.20 |
| **all scenarios** | | 2.60 | 2.42 | 1.25 | 3.73 |

## Wall-clock time per run (median)

| Eternal (ACI) | Optuna (TPE) | Ax (BoTorch) | Random |
|---|---|---|---|
| 0.02 s | 0.32 s | 217.58 s | 0.02 s |
