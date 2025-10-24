#!/bin/bash
set -e

echo "============================="
echo "🚀 Starting ROS1 <-> ROS2 Bridge"
echo "============================="

# Source environments
source /opt/ros/humble/setup.bash
source /ros-humble-ros1-bridge/install/local_setup.bash

# ROS networking env
export ROS_MASTER_URI=${ROS_MASTER_URI:-http://192.168.1.208:11311}
export ROS_HOSTNAME=${ROS_HOSTNAME:-$(hostname -I | awk '{print $1}')}
export ROS_IP=${ROS_IP:-$ROS_HOSTNAME}

echo "ROS_MASTER_URI=$ROS_MASTER_URI"
echo "ROS_IP=$ROS_IP"

# Wait for roscore to be up
echo "Waiting for ROS Master..."
until nc -z $(echo $ROS_MASTER_URI | awk -F[/:] '{print $4}') 11311; do
  echo "ROS master not available yet..."
  sleep 2
done

# Run bridge
exec ros2 run ros1_bridge dynamic_bridge --bridge-all-topics
