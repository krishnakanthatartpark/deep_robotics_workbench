#!/usr/bin/env python3
"""
X30 Mode Service Dispatcher Node

Provides a unified /x30_mode service that dispatches commands to either:
1. SDK method: via /state_command topic (x30_ros)
2. UDP method: via /state_transition service (x30_state_controller)
"""

import rclpy
from rclpy.node import Node
from x30_interfaces.srv import Mode
from std_msgs.msg import Int32

# Try to import state controller service
try:
    from x30_state_controller.srv import StateTransition
    STATE_CONTROLLER_AVAILABLE = True
except ImportError:
    STATE_CONTROLLER_AVAILABLE = False


class X30ModeServiceNode(Node):
    """Dispatcher node for X30 robot mode commands"""
    
    # Mode to SDK command ID mapping
    MODE_TO_SDK = {
        "stand": 16,
        "sit": 15,
        "estop": 13,
        "step_start": 18,
        "step_stop": 14,
    }
    
    # Mode to UDP command mapping
    MODE_TO_UDP = {
        "stand": "stand",
        "sit": "sit",
        "estop": "estop",  # Note: UDP doesn't have estop, will use stand
        "step_start": "step_start",
        "step_stop": "step_stop",
        "torque": "torque",  # UDP-only command
    }
    
    def __init__(self):
        super().__init__('x30_mode_service_node')
        
        # Declare parameters
        self.declare_parameter('control_method', 'sdk')  # 'sdk' or 'udp'
        self.declare_parameter('service_timeout', 5.0)
        
        # Get parameters
        self.control_method = self.get_parameter('control_method').get_parameter_value().string_value
        self.service_timeout = self.get_parameter('service_timeout').get_parameter_value().double_value
        
        self.get_logger().info(f'X30 Mode Service starting with method: {self.control_method}')
        
        # Create service server
        self.srv = self.create_service(
            Mode,
            '/x30_mode',
            self.handle_mode_request
        )
        
        # Initialize backend clients based on control method
        if self.control_method == 'sdk':
            # Create publisher for /state_command topic
            self.state_cmd_pub = self.create_publisher(Int32, '/state_command', 10)
            self.get_logger().info('Using SDK method via /state_command topic')
        
        elif self.control_method == 'udp':
            if not STATE_CONTROLLER_AVAILABLE:
                self.get_logger().error('UDP method selected but x30_state_controller not available!')
                raise RuntimeError('x30_state_controller service not available')
            
            # Create client for state_transition service
            self.state_client = self.create_client(StateTransition, '/state_transition')
            self.get_logger().info('Using UDP method via /state_transition service')
        
        else:
            self.get_logger().error(f'Invalid control_method: {self.control_method}')
            raise ValueError(f'control_method must be "sdk" or "udp", got: {self.control_method}')
        
        self.get_logger().info('X30 Mode Service ready at /x30_mode')
    
    def handle_mode_request(self, request, response):
        """Handle incoming mode service requests"""
        mode = request.mode.lower()
        self.get_logger().info(f'Received mode command: {mode}')
        
        if self.control_method == 'sdk':
            return self.handle_sdk_mode(mode, response)
        else:  # udp
            return self.handle_udp_mode(mode, response)
    
    def handle_sdk_mode(self, mode, response):
        """Handle mode via SDK method (state_command topic)"""
        if mode not in self.MODE_TO_SDK:
            response.success = False
            response.message = f'Unknown mode for SDK: {mode}. Valid modes: {list(self.MODE_TO_SDK.keys())}'
            self.get_logger().warn(response.message)
            return response
        
        command_id = self.MODE_TO_SDK[mode]
        
        try:
            # Publish to /state_command topic
            msg = Int32()
            msg.data = command_id
            self.state_cmd_pub.publish(msg)
            
            response.success = True
            response.message = f'SDK command {command_id} sent for mode: {mode}'
            self.get_logger().info(response.message)
            
        except Exception as e:
            response.success = False
            response.message = f'Failed to send SDK command: {str(e)}'
            self.get_logger().error(response.message)
        
        return response
    
    def handle_udp_mode(self, mode, response):
        """Handle mode via UDP method (state_transition service)"""
        if mode not in self.MODE_TO_UDP:
            response.success = False
            response.message = f'Unknown mode for UDP: {mode}. Valid modes: {list(self.MODE_TO_UDP.keys())}'
            self.get_logger().warn(response.message)
            return response
        
        udp_command = self.MODE_TO_UDP[mode]
        
        try:
            # Wait for service
            if not self.state_client.wait_for_service(timeout_sec=self.service_timeout):
                response.success = False
                response.message = 'State transition service not available'
                self.get_logger().error(response.message)
                return response
            
            # Call state_transition service
            req = StateTransition.Request()
            req.command = udp_command
            
            future = self.state_client.call_async(req)
            rclpy.spin_until_future_complete(self, future, timeout_sec=self.service_timeout)
            
            if future.result() is not None:
                result = future.result()
                response.success = result.success
                response.message = f'UDP: {result.message}'
                if result.success:
                    self.get_logger().info(response.message)
                else:
                    self.get_logger().warn(response.message)
            else:
                response.success = False
                response.message = 'State transition service call failed'
                self.get_logger().error(response.message)
        
        except Exception as e:
            response.success = False
            response.message = f'Failed to call state transition service: {str(e)}'
            self.get_logger().error(response.message)
        
        return response


def main(args=None):
    rclpy.init(args=args)
    node = X30ModeServiceNode()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
