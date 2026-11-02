cmake_minimum_required(VERSION 3.20)
project(EternalOptimizer CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(ETERNAL_GUI "Build the desktop app (fetches GLFW, Dear ImGui, ImPlot)" ON)
option(ETERNAL_TOOLS "Build the benchmarks, MCP server, showcase and tests" ON)
option(ETERNAL_PYTHON "Build the Python module (pybind11)" OFF)

include(FetchContent)

# ---- the engine: no dependencies
add_library(aci_opt src/aci_opt.cpp)
target_include_directories(aci_opt PUBLIC include)
if(NOT MSVC)
  target_compile_options(aci_opt PRIVATE -Wall -Wextra)
endif()
set_target_properties(aci_opt PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- the Python module (built by scikit-build-core via pyproject.toml)
if(ETERNAL_PYTHON)
  find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(_core python/eternal_bindings.cpp)
  target_link_libraries(_core PRIVATE aci_opt)
  install(TARGETS _core DESTINATION eternal)
endif()

if(ETERNAL_TOOLS)

add_executable(eternal_bench tools/bench.cpp)
target_link_libraries(eternal_bench PRIVATE aci_opt)

# ---- the MCP server (JSON via nlohmann/json, pinned) and the showcase workload
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
                     URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
FetchContent_MakeAvailable(json)
add_executable(eternal_mcp tools/mcp_server.cpp)
target_link_libraries(eternal_mcp PRIVATE aci_opt nlohmann_json::nlohmann_json)

add_executable(eternal_compare tools/compare_tools.cpp)
target_link_libraries(eternal_compare PRIVATE aci_opt)

add_executable(eternal_transfer tools/transfer.cpp)
target_link_libraries(eternal_transfer PRIVATE aci_opt)

add_executable(eternal_matmul tools/matmul_bench.cpp)
target_compile_options(eternal_matmul PRIVATE -O2)

enable_testing()
add_executable(eternal_tests tests/test_engine.cpp)
target_link_libraries(eternal_tests PRIVATE aci_opt)
add_test(NAME engine COMMAND eternal_tests)
endif()

# ---- the desktop app
if(ETERNAL_GUI)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.5 GIT_SHALLOW TRUE)
  FetchContent_Declare(implot GIT_REPOSITORY https://github.com/epezent/implot.git GIT_TAG v0.16 GIT_SHALLOW TRUE)
  FetchContent_Declare(implot3d GIT_REPOSITORY https://github.com/brenocq/implot3d.git GIT_TAG v0.3 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glfw imgui implot implot3d)

  add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${implot_SOURCE_DIR}/implot.cpp ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot3d_SOURCE_DIR}/implot3d.cpp ${implot3d_SOURCE_DIR}/implot3d_items.cpp ${implot3d_SOURCE_DIR}/implot3d_meshes.cpp)
  target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${implot_SOURCE_DIR} ${implot3d_SOURCE_DIR})
  target_link_libraries(imgui_lib PUBLIC glfw)
  if(APPLE)
    target_link_libraries(imgui_lib PUBLIC "-framework OpenGL")
    target_compile_definitions(imgui_lib PUBLIC GL_SILENCE_DEPRECATION)
  else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui_lib PUBLIC OpenGL::GL)
  endif()

  add_executable(eternal app/main.cpp)
  target_link_libraries(eternal PRIVATE aci_opt imgui_lib)
endif()
