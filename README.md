# Deep Robotics X30 Workbench

Comprehensive ROS2 integration and control suite for the Deep Robotics X30 quadruped robot.

## 📦 Overview

This workbench provides multiple control interfaces and examples for the X30 robot, including:

- **SDK Integration** - Native C++ integration with X30 RobotServer SDK
- **ROS2 HAL Node** - Hardware abstraction layer with lifecycle management
- **State Controller** - UDP-based state transition control
- **GUI Integration** - Web-based control interface via Strider
- **Learning Examples** - Progressive tutorials for SDK usage

---

## 📁 Package Structure

```
deep_robotics_workbench/
├── x30_robotserver_sdk/     # X30 SDK wrapper
├── x30_ros/                 # ROS2 HAL node with mode service
├── x30_state_controller/    # UDP state transition controller
├── x30_interfaces/          # ROS2 service definitions
├── x30_simulator/           # TCP simulator (if present)
└── udp_scripts/             # Original Python UDP scripts
```

---

## 🎯 Packages

### 1. x30_robotserver_sdk

C++ wrapper for the Deep Robotics X30 RobotServer SDK.

**Features:**
- TCP/IP communication (port 30000)
- Motion control commands
- Status querying
- Async callback-based API

**Examples:**
- [`lesson1_connect.cpp`](x30_robotserver_sdk/examples/basic/lesson1_connect.cpp) - Connect and get status
- [`lesson2_motion.cpp`](x30_robotserver_sdk/examples/basic/lesson2_motion.cpp) - Motion control (stand, sit, e-stop)

**Build:**
```bash
colcon build --packages-select x30_robotserver_sdk
```

**Run Examples:**
```bash
# Connect and get status
ros2 run x30_robotserver_sdk lesson1_connect

# Motion control
ros2 run x30_robotserver_sdk lesson2_motion
```

---

### 2. x30_ros

ROS2 Hardware Abstraction Layer (HAL) with lifecycle node management.

**Features:**
- ✅ **cmd_vel subscription** - Manual velocity control
- ✅ **/state_command topic** - Integer-based state commands
- ✅ **/x30_mode service** - String-based mode control (NEW!)
- ✅ **SDK connection management** - Lifecycle-based connection
- ✅ **Thread-safe operation** - Mutex-protected SDK access

**Supported Modes:**
| Mode String | Command ID | Description |
|-------------|-----------|-------------|
| `stand` | 16 | Stand up |
| `sit` | 15 | Sit down |
| `estop` | 13 | Emergency stop |
| `step_start` | 18 | Start walking |
| `step_stop` | 14 | Stop walking |
| `switch_gait` | 20 | Switch gait mode |

**Usage:**
```bash
# Start HAL node
ros2 run x30_ros x30_hal_node_exec

# Activate lifecycle
ros2 lifecycle set /x30_hal_lifecycle_node configure
ros2 lifecycle set /x30_hal_lifecycle_node activate

# Send velocity commands
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.5}}"

# Call mode service
ros2 service call /x30_mode x30_interfaces/srv/Mode "{mode: 'stand'}"
```

**Parameters:**
- `sdk_host` (default: "127.0.0.1") - SDK server IP
- `sdk_port` (default: 30000) - SDK server port

---

### 3. x30_state_controller

UDP-based state transition controller (alternative to SDK method).

**Features:**
- Direct UDP communication (port 43893)
- Simple binary protocol (12-byte packets)
- Service interface for state transitions

**Supported Commands:**
- `stand` - Stand up
- `sit` - Sit down  
- `torque` - Torque control mode
- `step_start` - Start stepping
- `step_stop` - Stop stepping

**Usage:**
```bash
# Start state controller
ros2 run x30_state_controller state_transition_node \
  --ros-args \
  -p robot_ip:=192.168.1.103 \
  -p robot_port:=43893

# Test with service
ros2 service call /state_transition x30_state_controller/srv/StateTransition \
  "{command: 'stand'}"

# Test with Python client
python3 src/deep_robotics_workbench/x30_state_controller/scripts/test_client.py
```

**Test Server (for development):**
```bash
# Run UDP echo server
python3 src/deep_robotics_workbench/x30_state_controller/scripts/udp_test_server.py
```

---

### 4. x30_interfaces

ROS2 service interface definitions.

**Services:**
- `Mode.srv` - Mode control service interface

**Definition:**
```
# Request
string mode    # Mode name (e.g., "stand", "sit")
---
# Response
bool success
string message
```

**Usage in C++:**
```cpp
#include "x30_interfaces/srv/mode.hpp"

auto client = node->create_client<x30_interfaces::srv::Mode>("/x30_mode");
auto request = std::make_shared<x30_interfaces::srv::Mode::Request>();
request->mode = "stand";
auto result = client->async_send_request(request);
```

**Usage in Python:**
```python
from x30_interfaces.srv import Mode

client = node.create_client(Mode, '/x30_mode')
request = Mode.Request()
request.mode = 'stand'
future = client.call_async(request)
```

---

## 🏗️ Architecture

### Control Flow

```
┌─────────────┐
│   Strider   │ Web GUI Interface
│     GUI     │
└──────┬──────┘
       │ HTTP POST /robot/x30/mode
       ▼
┌─────────────────┐
│  API Endpoint   │ FastAPI Backend
│ (endpoints.py)  │
└──────┬──────────┘
       │ navigator._send_x30_mode_command()
       ▼
┌──────────────────────────────────────────┐
│         ROS2 Service Layer               │
│                                          │
│  /x30_mode service                       │
│  (x30_interfaces::srv::Mode)             │
└──────┬───────────────────────────────────┘
       │
       ├──────────────────┬─────────────────┐
       ▼                  ▼                 ▼
┌─────────────┐   ┌──────────────┐   ┌──────────┐
│  x30_ros    │   │x30_state_    │   │  Custom  │
│  HAL Node   │   │ controller   │   │  Client  │
│  (SDK)      │   │  (UDP)       │   │          │
└──────┬──────┘   └──────┬───────┘   └────┬─────┘
       │                 │                 │
       │ TCP 30000       │ UDP 43893       │
       ▼                 ▼                 ▼
┌──────────────────────────────────────────┐
│           X30 Robot / Simulator          │
└──────────────────────────────────────────┘
```

### Communication Protocols

| Protocol | Port | Used By | Pros | Cons |
|----------|------|---------|------|------|
| **SDK (TCP)** | 30000 | x30_ros | Full features, reliable | Requires SDK setup |
| **UDP** | 43893 | x30_state_controller | Simple, direct | Limited commands |

---

## 🚀 Quick Start

### Prerequisites

```bash
# ROS2 Humble
sudo apt install ros-humble-desktop

# Python dependencies
pip install empy<4 catkin_pkg lark

# Workspace setup
cd ~/X30_ws
source /opt/ros/humble/setup.bash
```

### Build All Packages

```bash
cd ~/X30_ws
colcon build --packages-select \
  x30_robotserver_sdk \
  x30_interfaces \
  x30_ros \
  x30_state_controller

source install/setup.bash
```

### Connect to Robot

**Option 1: Using SDK (Recommended)**
```bash
# Terminal 1: Start HAL node
ros2 run x30_ros x30_hal_node_exec \
  --ros-args \
  -p sdk_host:=192.168.1.103 \
  -p sdk_port:=30000

# Terminal 2: Activate
ros2 lifecycle set /x30_hal_lifecycle_node configure
ros2 lifecycle set /x30_hal_lifecycle_node activate

# Terminal 3: Control
ros2 service call /x30_mode x30_interfaces/srv/Mode "{mode: 'stand'}"
```

**Option 2: Using UDP**
```bash
# Terminal 1: State controller
ros2 run x30_state_controller state_transition_node \
  --ros-args \
  -p robot_ip:=192.168.1.103 \
  -p robot_port:=43893

# Terminal 2: Control
ros2 service call /state_transition \
  x30_state_controller/srv/StateTransition "{command: 'stand'}"
```

---

## 🌐 GUI Integration (Strider)

The workbench integrates with the Strider web interface for graphical robot control.

### Backend Setup

**File: `strider-robot-commander/robot_commander/nav_client/navigator.py`**
- Added `X30Mode` service client
- Implemented `_send_x30_mode_command()` method

**File: `strider-robot-commander/robot_commander/api/endpoints.py`**
- Added `POST /robot/x30/mode` endpoint

### Frontend Setup

**File: `strider-webapp/src/lib/robot-config.ts`**
```typescript
services: {
  mode: { service_name: "/mode", service_type: "go2_interfaces/srv/Mode" },
  x30_mode: { service_name: "/x30_mode", service_type: "x30_interfaces/srv/Mode" },
}
```

**File: `strider-webapp/src/features/robot-control/components/JoystickWidget.tsx`**

Update to use X30 mode:
```typescript
// Line 50: Change service
const { callService: callModeService, ... } = useServiceCall('x30_mode');

// Line 320: Stand up
await callModeService({ mode: 'stand' });

// Line 367: Sit down
await callModeService({ mode: 'sit' });
```

### Run Full Stack

```bash
# Terminal 1: ROS2 HAL
ros2 run x30_ros x30_hal_node_exec
ros2 lifecycle set /x30_hal_lifecycle_node activate

# Terminal 2: Strider Backend
cd ~/strider-robot-commander
source ~/X30_ws/install/setup.bash
python3 -m robot_commander

# Terminal 3: Strider Frontend
cd ~/strider-webapp
npm run dev
```

Access GUI at: `http://localhost:3000`

---

## 📚 Learning Path

### Beginner: SDK Basics
1. Run `lesson1_connect` - Learn SDK connection
2. Run `lesson2_motion` - Learn motion commands
3. Read code comments in examples

### Intermediate: ROS2 Integration
1. Study `x30_hal_node.cpp` - Understand lifecycle pattern
2. Try `/x30_mode` service - Test mode control
3. Monitor with `ros2 topic echo /cmd_vel`

### Advanced: Custom Development
1. Extend `x30_hal_node` with new features
2. Create custom state machine in `x30_state_controller`
3. Integrate with Nav2 for autonomous navigation

---

## 🔧 Troubleshooting

### SDK Connection Issues

**Problem:** `lesson1_connect` shows Error Code 2
```bash
Error: Failed to get status. Error: 2
```

**Solution:**
1. Ensure simulator or robot is running
2. Check IP address and port
3. Verify network connectivity:
   ```bash
   ping 192.168.1.103
   telnet 192.168.1.103 30000
   ```

### Build Errors

**Problem:** `undefined reference to x30_robotserver_sdk`

**Solution:**
```bash
# Check CMakeLists.txt has correct library name
target_link_libraries(target_name x30_robotserver_sdk::x30_robotserver_sdk)
```

**Problem:** `empy` version issues

**Solution:**
```bash
pip install empy<4 --force-reinstall
```

### Service Not Available

**Problem:** `/x30_mode` service not found

**Solution:**
```bash
# Check if node is running
ros2 node list

# Check lifecycle state
ros2 lifecycle get /x30_hal_lifecycle_node

# Ensure it's activated
ros2 lifecycle set /x30_hal_lifecycle_node activate
```

### UDP Timeout

**Problem:** "Timeout waiting for robot response"

**Solution:**
1. Check robot IP/port configuration
2. Test with UDP echo server first
3. Verify firewall settings:
   ```bash
   sudo ufw allow 43893/udp
   ```

---

## 📖 API Reference

### x30_ros Services

#### /x30_mode
```
Service Type: x30_interfaces/srv/Mode
Request:
  string mode
Response:
  bool success
  string message
```

**Modes:** stand, sit, estop, step_start, step_stop, switch_gait

### x30_state_controller Services

#### /state_transition
```
Service Type: x30_state_controller/srv/StateTransition
Request:
  string command
Response:
  bool success
  string message
  uint32 response_code
  uint32 response_value
```

**Commands:** stand, sit, torque, step_start, step_stop

### Topics

#### /cmd_vel
```
Type: geometry_msgs/msg/Twist
Rate: 10 Hz
Description: Velocity commands for manual control
```

#### /state_command
```
Type: std_msgs/msg/Int32
Description: Integer command IDs for state transitions
Values: 13 (estop), 15 (sit), 16 (stand), 18 (step_start), 14 (step_stop)
```

---

## 🧪 Testing

### Unit Tests
```bash
colcon test --packages-select x30_ros x30_state_controller
colcon test-result --verbose
```

### Integration Test
```bash
# Test full pipeline
./scripts/integration_test.sh
```

### Manual Testing Checklist
- [ ] SDK connection successful
- [ ] Stand/sit commands work
- [ ] Emergency stop functional
- [ ] Velocity control responsive
- [ ] GUI buttons operational
- [ ] Service calls return properly

---

## 🤝 Contributing

### Code Style
- C++: Follow [ROS2 C++ Style Guide](https://docs.ros.org/en/humble/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)
- Python: PEP 8
- Use `ament_lint` for validation

### Adding New Features

1. Create feature branch
2. Add to appropriate package
3. Update this README
4. Add tests
5. Submit PR

---

## 📄 License

Apache 2.0 - See LICENSE file

---

## 🙏 Acknowledgments

- Deep Robotics for X30 SDK
- ROS2 Humble community
- Strider robotics interface framework

---

## 📞 Support

For issues and questions:
- Check [troubleshooting section](#-troubleshooting)
- Review SDK documentation
- Open GitHub issue

---

**Last Updated:** December 2025  
**ROS2 Version:** Humble  
**X30 SDK Version:** Latest
