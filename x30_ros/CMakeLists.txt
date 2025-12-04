cmake_minimum_required(VERSION 3.8)
project(x30_ros)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# Find dependencies
# -----------------------------
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(x30_robotserver_sdk REQUIRED)

# -----------------------------
# Executable (single-binary approach)
# -----------------------------
# Note: list both main and the lifecycle implementation .cpp so the constructor and all methods
# are compiled into the executable (this avoids undefined reference errors).
add_executable(x30_hal_node_exec
  src/main.cpp
  src/x30_hal_node.cpp   # <-- implementation of X30HalLifecycleNode

  src/transition_main.cpp
  src/state_transition_server.cpp
  src/udp_client.cpp
  # If your implementation file has a different name, put that here instead.
)

# include our public headers so #include "x30_ros/..." works while building
target_include_directories(x30_hal_node_exec PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Link the SDK imported target (plain signature)
target_link_libraries(x30_hal_node_exec
  x30_robotserver_sdk::x30_robotserver_sdk
)

# Add ROS 2 dependencies
ament_target_dependencies(x30_hal_node_exec
  rclcpp
  rclcpp_lifecycle
  rclcpp_components
  geometry_msgs
)

# -----------------------------
# Install executable so ros2 run can find it
# -----------------------------
install(TARGETS x30_hal_node_exec
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# install headers if you have any public headers under include/
install(DIRECTORY include/ DESTINATION include)

# -----------------------------
# Linting / testing
# -----------------------------
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
