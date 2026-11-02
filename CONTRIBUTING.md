# Contributing to Eternal

Thanks for helping! Issues, benchmarks, docs and code are all welcome — in English or Japanese.

## Good places to start

- Issues labelled [`good first issue`](https://github.com/phyrobyte/eternal-optimizer/labels/good%20first%20issue)
- A benchmark where Eternal loses (open a *Benchmark result* issue — losses are just as useful as wins)
- Docs and translations between `README.md` (Japanese) and `README.en.md` (English)

## Building

C++ engine and tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DETERNAL_GUI=OFF
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

Python package:

```bash
python -m venv .venv && . .venv/bin/activate
pip install -e ".[test]"   # or: pip install ".[test]"
pytest -q
```

## Changing the engine

The engine is `include/aci_opt.hpp` and `src/aci_opt.cpp`, with no dependencies. If you change how
it searches, re-run the benchmarks and paste the before/after numbers in your pull request:

```bash
./build/eternal_bench 20 200
```

Results are reported honestly, including where Eternal loses — please keep it that way.

## Pull requests

- Keep each PR to one change, and add a test when you fix a bug.
- Add a line under *Unreleased* in `CHANGELOG.md`.
- CI builds and tests on Linux, macOS and Windows; it must be green.

By contributing you agree that your contribution is licensed under the MIT License.
