// Python bindings for the Eternal causal optimizer (module eternal._core).
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <tuple>

#include "aci_opt.hpp"

namespace py = pybind11;

static const char* kind_name(aci::Kind k) {
    switch (k) {
        case aci::Kind::Warmup: return "warmup";
        case aci::Kind::Remeasure: return "remeasure";
        case aci::Kind::Intervene: return "intervene";
        case aci::Kind::Exploit: return "exploit";
        case aci::Kind::Explore: return "explore";
        case aci::Kind::Local: return "local";
        case aci::Kind::Random: return "random";
        case aci::Kind::Recall: return "recall";
    }
    return "unknown";
}

static std::vector<aci::Param> to_params(const std::vector<std::tuple<std::string, double, double>>& ps) {
    std::vector<aci::Param> out;
    for (const auto& [name, lo, hi] : ps) {
        if (!(lo < hi)) throw py::value_error("parameter '" + name + "': lower bound must be below upper bound");
        out.push_back({name, lo, hi});
    }
    if (out.empty()) throw py::value_error("at least one parameter is required");
    return out;
}

PYBIND11_MODULE(_core, m) {
    m.doc() = "Eternal: a causal black-box optimizer (C++ core)";

    py::class_<aci::Options>(m, "Options")
        .def(py::init<>())
        .def_readwrite("warmup", &aci::Options::warmup)
        .def_readwrite("bins", &aci::Options::bins)
        .def_readwrite("gamma", &aci::Options::gamma)
        .def_readwrite("half_life", &aci::Options::half_life)
        .def_readwrite("k_sigma", &aci::Options::k_sigma)
        .def_readwrite("zoom_z", &aci::Options::zoom_z)
        .def_readwrite("zoom_min_n", &aci::Options::zoom_min_n)
        .def_readwrite("recheck_every", &aci::Options::recheck_every)
        .def_readwrite("stall_reset", &aci::Options::stall_reset)
        .def_readwrite("local_share", &aci::Options::local_share)
        .def_readwrite("recall_share", &aci::Options::recall_share)
        .def_readwrite("recall_spread", &aci::Options::recall_spread)
        .def_readwrite("recall_zoom", &aci::Options::recall_zoom)
        .def_readwrite("recall_confirm", &aci::Options::recall_confirm);

    py::class_<aci::Memory>(m, "Memory")
        .def(py::init<>())
        .def_readonly("studies", &aci::Memory::studies)
        .def("save", &aci::Memory::save, "Serialize to a small text format.")
        .def_static("load", &aci::Memory::load, py::arg("text"), "Parse text written by save().")
        .def("__repr__", [](const aci::Memory& mem) {
            return "<eternal.Memory studies=" + std::to_string(mem.studies) +
                   " params=" + std::to_string(mem.entries.size()) + ">";
        });

    py::class_<aci::Optimizer>(m, "_Optimizer")
        .def(py::init([](const std::vector<std::tuple<std::string, double, double>>& params, uint64_t seed,
                         const aci::Options& options, const aci::Memory* memory) {
                 return new aci::Optimizer(to_params(params), seed, options, memory);
             }),
             py::arg("params"), py::arg("seed") = 0, py::arg("options") = aci::Options{},
             py::arg("memory") = nullptr)
        .def("ask", &aci::Optimizer::ask)
        .def("tell", &aci::Optimizer::tell, py::arg("y"))
        .def("remember", &aci::Optimizer::remember, py::arg("previous") = nullptr,
             py::arg("forget_half_life") = 0.0)
        .def_property_readonly("names", [](const aci::Optimizer& o) {
            std::vector<std::string> n;
            for (const auto& p : o.params()) n.push_back(p.name);
            return n;
        })
        .def_property_readonly("best", &aci::Optimizer::best)
        .def_property_readonly("incumbent", &aci::Optimizer::incumbent)
        .def_property_readonly("noise", &aci::Optimizer::noise)
        .def_property_readonly("screened", &aci::Optimizer::screened)
        .def_property_readonly("has_memory", &aci::Optimizer::has_memory)
        .def_property_readonly("region_lo", &aci::Optimizer::region_lo)
        .def_property_readonly("region_hi", &aci::Optimizer::region_hi)
        .def_property_readonly("pending_kind", [](const aci::Optimizer& o) { return kind_name(o.pending_kind()); })
        .def_property_readonly("verdicts", [](const aci::Optimizer& o) {
            py::list out;
            const auto& ps = o.params();
            const auto& vs = o.verdicts();
            for (size_t i = 0; i < vs.size(); ++i) {
                py::dict d;
                d["name"] = ps[i].name;
                d["tested"] = vs[i].tested;
                d["cause"] = vs[i].cause;
                d["effect"] = vs[i].effect;
                d["snr"] = vs[i].snr;
                d["checks"] = vs[i].checks;
                out.append(d);
            }
            return out;
        })
        .def_property_readonly("trials", [](const aci::Optimizer& o) {
            py::list out;
            for (const auto& t : o.trials()) {
                py::dict d;
                d["id"] = t.id;
                d["x"] = t.x;
                d["y"] = t.y;
                d["kind"] = kind_name(t.kind);
                d["param"] = t.param;
                out.append(d);
            }
            return out;
        })
        .def_property_readonly("events", [](const aci::Optimizer& o) {
            py::list out;
            const auto& ps = o.params();
            for (const auto& e : o.events()) {
                py::dict d;
                d["trial"] = e.trial;
                d["what"] = e.what;
                d["param"] = (e.param >= 0 && e.param < (int)ps.size()) ? py::cast(ps[e.param].name) : py::none();
                out.append(d);
            }
            return out;
        });

    py::class_<aci::RandomSearch>(m, "_RandomSearch")
        .def(py::init([](const std::vector<std::tuple<std::string, double, double>>& params, uint64_t seed) {
                 return new aci::RandomSearch(to_params(params), seed);
             }),
             py::arg("params"), py::arg("seed") = 0)
        .def("ask", &aci::RandomSearch::ask)
        .def("tell", &aci::RandomSearch::tell, py::arg("y"))
        .def_property_readonly("best", &aci::RandomSearch::best);
}
