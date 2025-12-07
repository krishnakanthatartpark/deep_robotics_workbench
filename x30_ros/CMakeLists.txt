cmake_minimum_required(VERSION 3.8)
project(x30_ros)

# Compiler options
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------------
# Find dependencies
# -----------------------------------
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(x30_robotserver_sdk REQUIRED)
find_package(x30_interfaces REQUIRED)

# Common include directories
include_directories(
  include
)

# -----------------------------------
# Executable 1: HAL lifecycle node
# -----------------------------------
add_executable(x30_hal_node_exec
  src/main.cpp
  src/x30_hal_node.cpp
  src/state_transition_server.cpp
  src/udp_client.cpp
)

ament_target_dependencies(x30_hal_node_exec
  rclcpp
  rclcpp_lifecycle
  rclcpp_components
  geometry_msgs
  x30_interfaces
)

target_link_libraries(x30_hal_node_exec
  x30_robotserver_sdk::x30_robotserver_sdk
)

# -----------------------------------
# Executable 2: Transition Command Server
# -----------------------------------
add_executable(x30_transition_exec
  src/transition_main.cpp
  src/state_transition_server.cpp
  src/udp_client.cpp
)

ament_target_dependencies(x30_transition_exec
  rclcpp
  x30_interfaces
)

target_link_libraries(x30_transition_exec
  x30_robotserver_sdk::x30_robotserver_sdk
)

# -----------------------------------
# Install executables
# -----------------------------------
install(TARGETS
  x30_hal_node_exec
  x30_transition_exec
  DESTINATION lib/${PROJECT_NAME}
)

# Install public headers
install(DIRECTORY include/
  DESTINATION include
)

# -----------------------------------
# Linting / Unit Tests (Optional)
# -----------------------------------
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
