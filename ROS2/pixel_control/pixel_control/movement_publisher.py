
#!/usr/bin/env python3
"""
Pixel - ROS 2 Movement Command Publisher

Project: RamBOTs Mark II (Pixel)
Subsystem: Computer Engineering / ROS 2 Control
ROS Distribution: Jazzy

Purpose:
    Publish simulated movement commands for Pixel using
    geometry_msgs/msg/Twist.

    The commands represent requested robot velocities, not
    individual motor positions or torques.

ROS Interface:
    Node:  pixel_movement_publisher
    Topic: /pixel/cmd_vel
    Type:  geometry_msgs/msg/Twist
    Rate:  2 Hz

Hardware:
    Not required. Simulation-only communication testing.
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from rclpy.clock import Clock
from rclpy.clock_type import ClockType


class PixelMovementPublisher(Node):
    """Publish a repeating sequence of simulated movement commands."""

    def __init__(self):
        super().__init__("pixel_movement_publisher")

        # ROS 2 publisher for commanded robot velocity.
        self.publisher_ = self.create_publisher(
            Twist,
            "/pixel/cmd_vel",
            10
        )

        # Use a steady clock to avoid disruptions from
        # system clock adjustments in WSL 2.
        self.publisher_clock = Clock(
            clock_type=ClockType.STEADY_TIME
        )

        self.timer_ = self.create_timer(
            0.5,
            self.publish_movement_command,
            clock=self.publisher_clock
        )

        # Commands are linear x, linear y, and angular z velocities.
        #
        # Linear values: meters per second (m/s)
        # Angular values: radians per second (rad/s)
        #
        # These are demonstration values, not validated
        # safe limits for the physical robot.
        self.commands = [
            ("STANDING / STOPPED", 0.0, 0.0, 0.0),
            ("WALK FORWARD", 0.2, 0.0, 0.0),
            ("WALK BACKWARD", -0.2, 0.0, 0.0),
            ("STRAFE LEFT", 0.0, 0.1, 0.0),
            ("STRAFE RIGHT", 0.0, -0.1, 0.0),
            ("ROTATE LEFT", 0.0, 0.0, 0.3),
            ("ROTATE RIGHT", 0.0, 0.0, -0.3),
            ("STOP", 0.0, 0.0, 0.0),
        ]

        self.command_index = 0

        self.get_logger().info(
            "Pixel movement publisher initialized."
        )
        self.get_logger().info(
            "Publishing simulated movement commands "
            "on /pixel/cmd_vel at 2 Hz."
        )

    def publish_movement_command(self):
        """Construct and publish the next Twist command."""

        description, linear_x, linear_y, angular_z = (
            self.commands[self.command_index]
        )

        msg = Twist()

        msg.linear.x = float(linear_x)
        msg.linear.y = float(linear_y)
        msg.linear.z = 0.0

        msg.angular.x = 0.0
        msg.angular.y = 0.0
        msg.angular.z = float(angular_z)

        self.publisher_.publish(msg)

        self.get_logger().info(
            f"Command: {description} | "
            f"linear.x={msg.linear.x:.2f} m/s | "
            f"linear.y={msg.linear.y:.2f} m/s | "
            f"angular.z={msg.angular.z:.2f} rad/s"
        )

        # Move to the next command, wrapping back to zero.
        self.command_index = (
            self.command_index + 1
        ) % len(self.commands)


def main(args=None):
    """Initialize ROS 2 and run the movement publisher."""

    rclpy.init(args=args)

    node = PixelMovementPublisher()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        node.get_logger().info(
            "Movement publisher stopped by user."
        )

    finally:
        node.destroy_node()

        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
