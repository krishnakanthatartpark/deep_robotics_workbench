cmake_minimum_required(VERSION 3.8)
project(x30_simulator)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json REQUIRED)

# Add your node
add_executable(x30_sim_server src/x30_sim_server.cpp)
target_link_libraries(x30_sim_server
  Boost::system
  Boost::thread
  nlohmann_json::nlohmann_json
)
target_compile_features(x30_sim_server PUBLIC cxx_std_17)

# Install the executable
install(TARGETS x30_sim_server
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
