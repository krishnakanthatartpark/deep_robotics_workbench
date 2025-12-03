cmake_minimum_required(VERSION 3.8)
project(rf_link_proc)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

find_package(PkgConfig REQUIRED)
find_package(rosidl_default_generators REQUIRED)
pkg_check_modules(LIBNL REQUIRED IMPORTED_TARGET libnl-3.0 libnl-genl-3.0)

include_directories(
  include/rf_link_proc
)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RFInfo.msg"
  DEPENDENCIES std_msgs
)



# Add executable
add_executable(rf_link_processor
  src/main.cpp
  src/rf_signal_handler.cpp
  src/nl80211_client.cpp
  src/rf_signal_node.cpp
)


# Link libraries
target_link_libraries(rf_link_processor
  PkgConfig::LIBNL
)

# Link against generated interfaces
rosidl_target_interfaces(rf_link_processor
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

ament_target_dependencies(rf_link_processor
  rclcpp
  std_msgs
)



# Install the target
install(TARGETS rf_link_processor
  DESTINATION lib/${PROJECT_NAME}
)

# Install headers
install(DIRECTORY include/
  DESTINATION include/
)

ament_package()
