cmake_minimum_required(VERSION 3.8)
project(x30_simulator)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------
find_package(ament_cmake REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TINYXML2 REQUIRED tinyxml2)

# ------------------------------------------------------------------------------
# Executable 1: Basic JSON-based Simulator
# ------------------------------------------------------------------------------
add_executable(x30_sim_server src/x30_sim_server.cpp)
target_link_libraries(x30_sim_server
  Boost::system
  Boost::thread
  nlohmann_json::nlohmann_json
)
target_compile_features(x30_sim_server PUBLIC cxx_std_17)

# ------------------------------------------------------------------------------
# Executable 2: XML-based State Transition Simulator
# ------------------------------------------------------------------------------
add_executable(x30_sim_server_state_transitions src/x30_sim_server_state_transitions.cpp)
target_include_directories(x30_sim_server_state_transitions PRIVATE ${TINYXML2_INCLUDE_DIRS})
target_link_libraries(x30_sim_server_state_transitions
  Boost::system
  Boost::thread
  ${TINYXML2_LIBRARIES}
)
target_compile_features(x30_sim_server_state_transitions PUBLIC cxx_std_17)

# ------------------------------------------------------------------------------
# Install Targets
# ------------------------------------------------------------------------------
install(TARGETS
  x30_sim_server
  x30_sim_server_state_transitions
  DESTINATION lib/${PROJECT_NAME}
)

# ------------------------------------------------------------------------------
# Linting & Testing
# ------------------------------------------------------------------------------
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
