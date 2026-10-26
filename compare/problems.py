"""The four benchmark problems, mirrored exactly from include/problems.hpp.
check() verifies the mirror against the C++ build to machine precision."""
import math
import subprocess
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "build" / "eternal_compare"


def _u(lo, hi, v):
    return (v - lo) / (hi - lo)


def ramen(x):
    t, T, s = _u(60, 300, x[0]), _u(60, 99, x[1]), _u(2, 12, x[2])
    o, c = _u(0, 30, x[3]), _u(2, 15, x[4])
    taste = (100 - 60 * (t - 0.55) ** 2 - 80 * (T - 0.8) ** 2 - 120 * (s - 0.45 - 0.3 * (o - 0.5)) ** 2
             - 30 * (o - 0.4) ** 2 - 20 * (c - 0.35) ** 2)
    return -taste


def branin(x):
    a, b, c, r, s, t = 1, 5.1 / (4 * math.pi ** 2), 5 / math.pi, 6, 10, 1 / (8 * math.pi)
    return a * (x[1] - b * x[0] ** 2 + c * x[0] - r) ** 2 + s * (1 - t) * math.cos(x[0]) + s


_AL = [1.0, 1.2, 3.0, 3.2]
_A = [[10, 3, 17, 3.5, 1.7, 8], [0.05, 10, 17, 0.1, 8, 14], [3, 3.5, 1.7, 10, 17, 8], [17, 8, 0.05, 10, 0.1, 14]]
_P = [[1312, 1696, 5569, 124, 8283, 5886], [2329, 4135, 8307, 3736, 1004, 9991],
      [2348, 1451, 3522, 2883, 3047, 6650], [4047, 8828, 8732, 5743, 1091, 381]]


def hartmann6(x):
    y = 0.0
    for i in range(4):
        e = sum(_A[i][j] * (x[j] - _P[i][j] * 1e-4) ** 2 for j in range(6))
        y -= _AL[i] * math.exp(-e)
    return y


def rosenbrock(x):
    return sum(100 * (x[i + 1] - x[i] ** 2) ** 2 + (1 - x[i]) ** 2 for i in range(len(x) - 1))


PROBLEMS = [
    dict(name="ramen", f=ramen, optimum=-100.0,
         bounds=[(60, 300), (60, 99), (2, 12), (0, 30), (2, 15), (-5, 35), (0, 7), (0, 10)]),
    dict(name="branin+6 decoys", f=branin, optimum=0.397887357729738,
         bounds=[(-5, 10), (0, 15)] + [(0, 1)] * 6),
    dict(name="hartmann6", f=hartmann6, optimum=-3.32236801141551, bounds=[(0, 1)] * 6),
    dict(name="rosenbrock4", f=rosenbrock, optimum=0.0, bounds=[(-2, 2)] * 4),
]


def check(n=2000, seed=0):
    rng = np.random.default_rng(seed)
    worst = 0.0
    for k, p in enumerate(PROBLEMS):
        X = [[rng.uniform(lo, hi) for lo, hi in p["bounds"]] for _ in range(n)]
        out = subprocess.run([str(TOOL), "eval", str(k)], input="\n".join(" ".join(repr(v) for v in x) for x in X),
                             capture_output=True, text=True, check=True).stdout.split()
        for x, c in zip(X, out):
            py, cc = p["f"](x), float(c)
            worst = max(worst, abs(py - cc) / max(1.0, abs(cc)))
    return worst


def sigma(k, noise):
    return float(subprocess.run([str(TOOL), "sigma", str(k), str(noise)], capture_output=True, text=True, check=True).stdout)


if __name__ == "__main__":
    print(f"worst relative difference, Python vs C++, over {4 * 2000} points: {check():.2e}")
