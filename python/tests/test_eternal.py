import math

import pytest

import eternal


def branin_with_decoys(x, y, d1, d2, d3):
    """Branin in (x, y); d1..d3 do nothing."""
    a, b, c, r, s, t = 1, 5.1 / (4 * math.pi ** 2), 5 / math.pi, 6, 10, 1 / (8 * math.pi)
    return a * (y - b * x * x + c * x - r) ** 2 + s * (1 - t) * math.cos(x) + s


BOUNDS = {"x": (-5, 10), "y": (0, 15), "d1": (0, 1), "d2": (0, 1), "d3": (0, 1)}
BRANIN_MIN = 0.397887


def test_ask_repeats_until_tell():
    opt = eternal.Optimizer(BOUNDS, seed=1)
    a, b = opt.ask(), opt.ask()
    assert a == b
    assert set(a) == set(BOUNDS)


def test_tell_without_ask_raises():
    opt = eternal.Optimizer(BOUNDS, seed=1)
    opt.tell(branin_with_decoys(**opt.ask()))
    with pytest.raises(RuntimeError):
        opt.tell(0.0)


def test_points_stay_in_bounds():
    opt = eternal.Optimizer(BOUNDS, seed=2)
    for _ in range(150):
        x = opt.ask()
        for k, (lo, hi) in BOUNDS.items():
            assert lo <= x[k] <= hi
        opt.tell(branin_with_decoys(**x))


def test_minimize_finds_causes_and_freezes_decoys():
    r = eternal.minimize(branin_with_decoys, BOUNDS, budget=200, seed=3)
    assert r.best - BRANIN_MIN < 0.5
    assert set(r.causes) == {"x", "y"}
    assert set(r.frozen) == {"d1", "d2", "d3"}
    assert len(r.trials) == 200
    assert {t["kind"] for t in r.trials} <= {
        "warmup", "remeasure", "intervene", "exploit", "explore", "local", "random", "recall"}


def test_beats_random_search_on_median():
    ours, theirs = [], []
    for seed in range(5):
        ours.append(eternal.minimize(branin_with_decoys, BOUNDS, budget=150, seed=seed).best)
        rs = eternal.RandomSearch(BOUNDS, seed=seed)
        for _ in range(150):
            rs.tell(branin_with_decoys(**rs.ask()))
        theirs.append(rs.best)
    assert sorted(ours)[2] < sorted(theirs)[2]


def test_memory_roundtrip_and_warm_start():
    r = eternal.minimize(branin_with_decoys, BOUNDS, budget=150, seed=4)
    mem = r.optimizer.remember()
    assert mem.studies == 1
    mem2 = eternal.Memory.load(mem.save())
    assert mem2.save() == mem.save()
    opt = eternal.Optimizer(BOUNDS, seed=5, memory=mem2)
    assert opt._core.has_memory
    for _ in range(60):
        opt.tell(branin_with_decoys(**opt.ask()))


def test_bad_bounds():
    with pytest.raises(ValueError):
        eternal.Optimizer({"x": (1, 1)})
    with pytest.raises(ValueError):
        eternal.Optimizer([("x", 0, 1), ("x", 0, 2)])
    with pytest.raises(ValueError):
        eternal.Optimizer({})


def test_nan_rejected():
    opt = eternal.Optimizer(BOUNDS)
    opt.ask()
    with pytest.raises(ValueError):
        opt.tell(float("nan"))
