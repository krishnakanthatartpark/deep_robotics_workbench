#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32
import time

class StateTransitionTester(Node):
    def __init__(self):
        super().__init__('state_transition_tester')
        self.publisher_ = self.create_publisher(Int32, '/state_command', 10)
        self.get_logger().info('State Transition Tester Node Started')

    def send_command(self, command_id, description):
        msg = Int32()
        msg.data = command_id
        self.publisher_.publish(msg)
        self.get_logger().info(f'Sent command: {command_id} ({description})')

    def run_test_sequence(self):
        # Allow some time for connection
        time.sleep(2)

        # 1. Stand Up (16)
        self.send_command(16, "Stand Up")
        time.sleep(3)

        # 2. Sit Down (15)
        self.send_command(15, "Sit Down")
        time.sleep(3)

        # 3. Stand Up again (16)
        self.send_command(16, "Stand Up")
        time.sleep(3)

        # 4. Start Stepping (18)
        self.send_command(18, "Start Stepping")
        time.sleep(3)

        # 5. Stop Stepping (14)
        self.send_command(14, "Stop Stepping")
        time.sleep(3)

        # 6. Emergency Stop (13)
        self.send_command(13, "Emergency Stop")
        time.sleep(2)

        self.get_logger().info('Test sequence completed.')

def main(args=None):
    rclpy.init(args=args)
    tester = StateTransitionTester()
    
    try:
        tester.run_test_sequence()
    except KeyboardInterrupt:
        pass
    finally:
        tester.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
