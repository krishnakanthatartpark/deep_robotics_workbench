#!/usr/bin/env python3
"""
Test client for x30_state_controller service
Demonstrates calling the state transition service from Python
"""
import rclpy
from rclpy.node import Node
from x30_state_controller.srv import StateTransition
import time


class StateTransitionClient(Node):
    def __init__(self):
        super().__init__('state_transition_client')
        self.client = self.create_client(StateTransition, 'state_transition')
        
        while not self.client.wait_for_service(timeout_sec=2.0):
            self.get_logger().info('Waiting for state_transition service...')

    def send_command(self, command):
        request = StateTransition.Request()
        request.command = command
        
        self.get_logger().info(f'→ Sending command: {command}')
        future = self.client.call_async(request)
        rclpy.spin_until_future_complete(self, future)
        
        result = future.result()
        if result.success:
            self.get_logger().info(f'✓ {result.message}')
            self.get_logger().info(f'  Response: code=0x{result.response_code:X}, value={result.response_value}')
        else:
            self.get_logger().warn(f'✗ {result.message}')
        
        return result.success


def main():
    rclpy.init()
    client = StateTransitionClient()
    
    print("\n=== X30 State Transition Test ===\n")
    
    # Test sequence
    commands = [
        ("stand", 2),
        ("sit", 2),
        ("stand", 2),
        ("step_start", 3),
        ("step_stop", 2),
        ("torque", 2),
    ]
    
    for cmd, delay in commands:
        success = client.send_command(cmd)
        if success:
            time.sleep(delay)
        else:
            break
    
    print("\n=== Test Complete ===\n")
    client.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
