cmake_minimum_required(VERSION 3.8)
project(streaming)

# Enable warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(rf_link_proc REQUIRED)
find_package(rosidl_default_generators REQUIRED)


# Generate messages and services
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/StartStream.srv"
  "srv/StopStream.srv"
  "srv/ListStreams.srv"
  DEPENDENCIES std_msgs geometry_msgs
)




# -----------------------------------------------------------------------------
# GStreamer modules
# -----------------------------------------------------------------------------
pkg_check_modules(GST REQUIRED
  gstreamer-1.0
  gstreamer-base-1.0
  gstreamer-app-1.0
  gstreamer-video-1.0
  gstreamer-audio-1.0
  glib-2.0
  gobject-2.0
)

message(STATUS "GStreamer include dirs: ${GST_INCLUDE_DIRS}")
message(STATUS "GStreamer library dirs: ${GST_LIBRARY_DIRS}")
message(STATUS "GStreamer libraries: ${GST_LIBRARIES}")

# -----------------------------------------------------------------------------
# Include and link directories
# -----------------------------------------------------------------------------
include_directories(
  ${GST_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

link_directories(
  ${GST_LIBRARY_DIRS}
)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------
add_executable(video_streamer src/main.cpp src/stream_manager.cpp src/stream_session.cpp src/streaming_server_node.cpp)


# ROS dependencies
ament_target_dependencies(video_streamer
  rclcpp
  sensor_msgs
  cv_bridge
  rf_link_proc
)

rosidl_target_interfaces(video_streamer
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

# GStreamer + OpenCV libraries
target_link_libraries(video_streamer
  ${GST_LIBRARIES}
  ${OpenCV_LIBRARIES}
)

target_include_directories(video_streamer PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/streaming>
  $<INSTALL_INTERFACE:include>
  ${GST_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------
install(TARGETS video_streamer
  DESTINATION lib/${PROJECT_NAME}
)

# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
