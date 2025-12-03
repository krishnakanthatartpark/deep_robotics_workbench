# Deep Robotics X30 Interface

1. First connect to robot's wifi

2. Then run ros2 run x30_ros x30_hal_node_exec on your laptop.

3. Then activate the lifecycle node




1. GUI Integration
2. /cmd_vel robot control (Done)
    1. ros2 run x30_ros x30_hal_node_exec 
    2. ros2 run x30_simulator x30_sim_server 
    3. ros2 lifecycle set /x30_hal_lifecycle_node configure
    4. ros2 lifecycle set /x30_hal_lifecycle_node activate



3. State Transitions of robot (In progress)
4. Video Feed 
    1. Run the ros noetic docker container and inside run roslaunch laptop_video_feed camera_stream.launch to publish /camera_feed topic from /dev/video0
    2. Run the docker compose from ros-humble-ros1-bridge-builder
    3. Then run the mediamtx docker contianer as given below 
        docker run --rm -it   -p 8554:8554   -p 1935:1935   -p 8888:8888   -p 8889:8889   -p 8890:8890/udp   -v ~/mediamtx/mediamtx.yml:/mediamtx.yml:ro   --name mediamtx   bluenviron/mediamtx:latest
    4. Then run ros2 run streaming video_streamer 
    5. Then run ros2 launch robot_commander rosbridge_websocket.launch.py
    6. Then start the GUI via npm run dev
    7. Then run the follower service call "ros2 service call /start_stream streaming/srv/StartStream "{stream_name: 'x30_feed', srt_uri: 'srt://localhost:8890', source: '/camera_feed', source_type: 'ros_topic', width: 640, height: 480, framerate: 30}"
    

     
