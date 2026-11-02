// A real workload for the showcase: a tiled 512x512 matrix multiply, timed on this machine.
//   eternal_matmul tile=<4..256> order=<0..6> unroll=<1..8> color=<any> label=<any>
// tile:   block size (rounded to an integer)
// order:  loop order inside a block (floor: 0 ijk, 1 ikj, 2 jik, 3 jki, 4 kij, 5 kji)
// unroll: how many k steps the innermost loop does per pass (rounded)
// color, label: accepted and ignored; they change nothing.
// Prints the median wall time of 3 runs in milliseconds.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static const int N = 512;

static void block(const float* A, const float* B, float* C, int i0, int j0, int k0, int T, int order, int U) {
    const int i1 = std::min(N, i0 + T), j1 = std::min(N, j0 + T), k1 = std::min(N, k0 + T);
    auto kloop = [&](int i, int j) {
        float s = 0;
        int k = k0;
        for (; k + U <= k1; k += U)
            for (int u = 0; u < U; ++u) s += A[i * N + k + u] * B[(k + u) * N + j];
        for (; k < k1; ++k) s += A[i * N + k] * B[k * N + j];
        C[i * N + j] += s;
    };
    switch (order) {
        case 0: for (int i = i0; i < i1; ++i) for (int j = j0; j < j1; ++j) kloop(i, j); break;
        case 1: for (int i = i0; i < i1; ++i) for (int k = k0; k < k1; ++k) { const float a = A[i * N + k]; for (int j = j0; j < j1; ++j) C[i * N + j] += a * B[k * N + j]; } break;
        case 2: for (int j = j0; j < j1; ++j) for (int i = i0; i < i1; ++i) kloop(i, j); break;
        case 3: for (int j = j0; j < j1; ++j) for (int k = k0; k < k1; ++k) { const float b = B[k * N + j]; for (int i = i0; i < i1; ++i) C[i * N + j] += A[i * N + k] * b; } break;
        case 4: for (int k = k0; k < k1; ++k) for (int i = i0; i < i1; ++i) { const float a = A[i * N + k]; for (int j = j0; j < j1; ++j) C[i * N + j] += a * B[k * N + j]; } break;
        default: for (int k = k0; k < k1; ++k) for (int j = j0; j < j1; ++j) { const float b = B[k * N + j]; for (int i = i0; i < i1; ++i) C[i * N + j] += A[i * N + k] * b; } break;
    }
}

int main(int argc, char** argv) {
    double tile = 64, order = 0, unroll = 1;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (!std::strncmp(a, "tile=", 5)) tile = std::atof(a + 5);
        else if (!std::strncmp(a, "order=", 6)) order = std::atof(a + 6);
        else if (!std::strncmp(a, "unroll=", 7)) unroll = std::atof(a + 7);
        // color= and label= are read by nobody
    }
    const int T = std::clamp((int)std::lround(tile), 4, N);
    const int O = std::clamp((int)order, 0, 5);
    const int U = std::clamp((int)std::lround(unroll), 1, 8);
    std::vector<float> A(N * N), B(N * N), C(N * N);
    for (int i = 0; i < N * N; ++i) { A[i] = (i % 7) * 0.1f; B[i] = (i % 5) * 0.2f; }
    std::vector<double> ms;
    for (int rep = 0; rep < 3; ++rep) {
        std::fill(C.begin(), C.end(), 0.0f);
        const auto t0 = std::chrono::steady_clock::now();
        for (int i0 = 0; i0 < N; i0 += T)
            for (int j0 = 0; j0 < N; j0 += T)
                for (int k0 = 0; k0 < N; k0 += T) block(A.data(), B.data(), C.data(), i0, j0, k0, T, O, U);
        ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    }
    std::sort(ms.begin(), ms.end());
    std::printf("%.3f\n", ms[1]);
    return C[N * N - 1] < -1 ? 1 : 0;   // keep the result alive
}
