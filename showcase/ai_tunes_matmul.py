"""
デモ：AI が MCP 経由で「永遠」を操り、本物のコードを速くする。

対象は eternal_matmul（512×512 の行列積）。実際にこの機械で時間を測る。つまみは
タイルの大きさ・ループの順序・展開数、そして何もしない「色」と「ラベル」。
永遠には仕組みを一切教えない。同じ予算でランダム探索とも比べる。

  python showcase/ai_tunes_matmul.py [予算]
"""
import asyncio
import json
import random
import statistics
import subprocess
import sys
from pathlib import Path

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

ROOT = Path(__file__).resolve().parents[1]
SERVER = ROOT / "build" / "eternal_mcp"
MATMUL = ROOT / "build" / "eternal_matmul"
BUDGET = int(sys.argv[1]) if len(sys.argv) > 1 else 60

PARAMS = [
    {"name": "tile", "low": 4, "high": 256},
    {"name": "order", "low": 0, "high": 5.999},
    {"name": "unroll", "low": 1, "high": 8},
    {"name": "color", "low": 0, "high": 1},
    {"name": "label", "low": 0, "high": 100},
]
ORDERS = ["ijk", "ikj", "jik", "jki", "kij", "kji"]


def run(p):
    args = [str(MATMUL)] + [f"{k}={v}" for k, v in p.items()]
    return float(subprocess.run(args, capture_output=True, text=True, check=True).stdout)


def text(res):
    return json.loads(res.content[0].text)


async def tune(seed, verbose):
    """One study driven entirely through MCP. Returns its best-so-far curve, best config and status."""
    params = StdioServerParameters(command=str(SERVER), args=[])
    async with stdio_client(params) as (r, w):
        async with ClientSession(r, w) as s:
            info = await s.initialize()
            if verbose:
                tools = await s.list_tools()
                print(f"MCP 接続: {info.serverInfo.name} {info.serverInfo.version}  ツール: {[t.name for t in tools.tools]}")
            study = text(await s.call_tool("create_study", {"params": PARAMS, "seed": seed}))["study"]
            curve, best = [], float("inf")
            for _ in range(BUDGET):
                a = text(await s.call_tool("ask", {"study": study}))
                ms = run(a["params"])
                t = text(await s.call_tool("tell", {"study": study, "value": ms}))
                best = t["best"]
                curve.append(best)
                if verbose:
                    p = a["params"]
                    print(f"#{a['trial']:3d} {a['kind']:<5s} tile={p['tile']:6.1f} order={ORDERS[min(5, int(p['order']))]} "
                          f"unroll={p['unroll']:4.1f} color={p['color']:.2f} label={p['label']:5.1f}  →  {ms:7.2f} ms")
            st = text(await s.call_tool("status", {"study": study}))
    return curve, {k: st["best_params"][k] for k in ("tile", "order", "unroll", "color", "label")}, st


def random_search(seed):
    rng = random.Random(seed)
    curve, best, best_cfg = [], float("inf"), None
    for _ in range(BUDGET):
        cfg = {q["name"]: rng.uniform(q["low"], q["high"]) for q in PARAMS}
        ms = run(cfg)
        if ms < best:
            best, best_cfg = ms, cfg
        curve.append(best)
    return curve, best_cfg


def first_within(curve, target):
    return next((i + 1 for i, v in enumerate(curve) if v <= target), None)


async def main():
    seeds = [7, 8, 9]
    print(f"対象：512×512 行列積の実測時間。つまみ5つ（うち2つは何もしない）。予算 {BUDGET} 試行 × シード {len(seeds)}。\n")
    runs = []
    for i, seed in enumerate(seeds):
        e_curve, e_cfg, st = await tune(seed, verbose=(i == 0))
        r_curve, r_cfg = random_search(seed)
        runs.append((seed, e_curve, e_cfg, st, r_curve, r_cfg))
        if i == 0:
            print()

    default = {"tile": 64, "order": 0, "unroll": 1, "color": 0.5, "label": 50}
    retime = lambda cfg: statistics.median(run(cfg) for _ in range(5))
    t_default = retime(default)
    rows = []
    for seed, e_curve, e_cfg, st, r_curve, r_cfg in runs:
        te, tr = retime(e_cfg), retime(r_cfg)
        goal = 1.10 * min(e_curve[-1], r_curve[-1])
        rows.append((seed, te, tr, first_within(e_curve, goal), first_within(r_curve, goal), st))

    print("── 結果（最良設定を各5回再計測した中央値）──")
    print(f"教科書どおりの設定（tile=64, ijk, 展開なし）：{t_default:.2f} ms\n")
    print("シード   永遠の最良   ランダムの最良   最良の10%以内に届いた試行（永遠 / ランダム）")
    for seed, te, tr, ne, nr, st in rows:
        print(f"{seed:4d}   {te:8.2f} ms   {tr:10.2f} ms        {ne if ne else '未到達':>4} / {nr if nr else '未到達'}")
    med_e = statistics.median(r[1] for r in rows)
    print(f"\n永遠の最良（中央値）は教科書設定の {t_default / med_e:.1f} 倍速")
    print("\n── 永遠が判定したこと（各シード）──")
    for seed, te, tr, ne, nr, st in rows:
        causes = ", ".join(c["name"] for c in st["causes"])
        frozen = ", ".join(c["name"] for c in st["frozen_as_irrelevant"]) or "なし"
        print(f"シード {seed}：原因 = {causes}  ／  無関係として固定 = {frozen}")
    print("\n出来事（シード 7）：")
    for e in rows[0][5]["events"]:
        print("  ", e)
    json.dump({"budget": BUDGET, "default_ms": t_default,
               "runs": [{"seed": s, "eternal_ms": te, "random_ms": tr, "eternal_trials_to_goal": ne,
                         "random_trials_to_goal": nr, "status": st} for s, te, tr, ne, nr, st in rows]},
              open(ROOT / "showcase" / "result.json", "w"), ensure_ascii=False, indent=1)


if __name__ == "__main__":
    asyncio.run(main())
