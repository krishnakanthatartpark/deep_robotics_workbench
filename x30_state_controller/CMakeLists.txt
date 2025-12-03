cmake_minimum_required(VERSION 3.8)
project(x30_state_controller)

# Compiler settings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Generate service interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/StateTransition.srv"
)

# Include directories
include_directories(
  include
  ${Boost_INCLUDE_DIRS}
)

# UDP Client library
add_library(udp_client
  src/udp_client.cpp
)
target_include_directories(udp_client PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(udp_client
  ${Boost_LIBRARIES}
)

# State Manager library
add_library(state_manager
  src/state_manager.cpp
)
target_include_directories(state_manager PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(state_manager
  udp_client
  ${Boost_LIBRARIES}
)

# ROS2 Node executable
add_executable(state_transition_node
  src/state_transition_node.cpp
)
target_include_directories(state_transition_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(state_transition_node
  rclcpp
)
target_link_libraries(state_transition_node
  udp_client
  state_manager
  ${Boost_LIBRARIES}
)

# Link service interfaces to node
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)
target_link_libraries(state_transition_node "${cpp_typesupport_target}")

# Install targets
install(TARGETS
  udp_client
  state_manager
  state_transition_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install headers
install(DIRECTORY include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_dependencies(rosidl_default_runtime)
ament_package()
