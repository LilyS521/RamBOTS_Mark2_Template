
#!/usr/bin/env python3
"""
Pixel - ROS 2 Movement Command Subscriber

Project: RamBOTs Mark II (Pixel)
Subsystem: Computer Engineering / ROS 2 Control
ROS Distribution: Jazzy

Purpose:
    Receive simulated movement commands published to
    /pixel/cmd_vel using geometry_msgs/msg/Twist.

    Interpret requested movement and monitor command
    communication for timeouts.

ROS Interface:
    Node:  pixel_movement_subscriber
    Topic: /pixel/cmd_vel
    Type:  geometry_msgs/msg/Twist

Hardware:
    None required. Software-only communication testing.

Note:
    This node does not control motors or enforce a
    physical robot safety state. Timeout detection
    only produces diagnostic messages.
"""

import time

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from rclpy.clock import Clock
from rclpy.clock_type import ClockType


class PixelMovementSubscriber(Node):
    """Receive, interpret, and monitor Pixel movement commands."""

    def __init__(self):
        super().__init__("pixel_movement_subscriber")

        # Subscribe to movement commands from the publisher.
        self.subscription_ = self.create_subscription(
            Twist,
            "/pixel/cmd_vel",
            self.movement_callback,
            10
        )

        # Communication monitoring settings.
        self.timeout_seconds = 3.0

        # Monotonic time is appropriate for elapsed-time checks.
        self.last_message_time = time.monotonic()

        # Tracks whether at least one message has arrived.
        self.message_received = False

        # Prevent repeated timeout warnings.
        self.communication_lost = False

        # Track the number of received commands.
        self.message_count = 0

        # Use a monotonic ROS clock for communication monitoring.
        # This avoids timer disruptions caused by system-clock changes.
        self.watchdog_clock = Clock(
            clock_type=ClockType.STEADY_TIME
        )

        self.watchdog_timer_ = self.create_timer(
            0.5,
            self.check_communication,
            clock=self.watchdog_clock
        )

        self.get_logger().info(
            "Pixel movement subscriber initialized."
        )

        self.get_logger().info(
            "Listening for movement commands on /pixel/cmd_vel."
        )

        self.get_logger().info(
            f"Communication timeout threshold: "
            f"{self.timeout_seconds:.1f} seconds."
        )

    def movement_callback(self, msg):
        """
        Process an incoming Twist command.

        This callback executes whenever a new
        message arrives on /pixel/cmd_vel.
        """

        # Update communication status.
        self.last_message_time = time.monotonic()
        self.message_count += 1

        # Detect recovery following a communication timeout.
        if self.communication_lost:
            self.get_logger().info(
                "COMMUNICATION RECOVERED: "
                "Movement commands are being received again."
            )

            self.communication_lost = False

        self.message_received = True

        # Extract requested linear velocities.
        linear_x = msg.linear.x
        linear_y = msg.linear.y
        linear_z = msg.linear.z

        # Extract requested angular velocities.
        angular_x = msg.angular.x
        angular_y = msg.angular.y
        angular_z = msg.angular.z

        # Determine the requested movement.
        movement = self.identify_movement(
            linear_x,
            linear_y,
            angular_z
        )

        # Report the received command.
        self.get_logger().info(
            f"RECEIVED COMMAND #{self.message_count} | "
            f"Movement: {movement} | "
            f"X: {linear_x:.2f} m/s | "
            f"Y: {linear_y:.2f} m/s | "
            f"Rotation: {angular_z:.2f} rad/s"
        )

        # Reserved for future gait and motor-command integration.
        # No hardware commands are generated here.
        self.process_movement_command(msg)

    def identify_movement(self, x, y, rotation):
        """
        Convert velocity components into a movement description.

        Multiple components may be nonzero simultaneously.
        """

        tolerance = 0.0001
        movements = []

        if x > tolerance:
            movements.append("FORWARD")
        elif x < -tolerance:
            movements.append("BACKWARD")

        if y > tolerance:
            movements.append("STRAFE LEFT")
        elif y < -tolerance:
            movements.append("STRAFE RIGHT")

        if rotation > tolerance:
            movements.append("ROTATE LEFT")
        elif rotation < -tolerance:
            movements.append("ROTATE RIGHT")

        if not movements:
            return "STOPPED"

        return " + ".join(movements)

    def process_movement_command(self, msg):
        """
        Placeholder for a future control interface.

        Eventually, a separate gait-control component
        could interpret velocity commands and generate
        joint targets for the motor-control subsystem.

        This method deliberately does not forward
        commands to any physical hardware.
        """

        # Explicitly unused components for this prototype.
        _ = (
            msg.linear.z,
            msg.angular.x,
            msg.angular.y
        )

        return

    def check_communication(self):
        """
        Detect whether movement commands have stopped arriving.

        This is diagnostic timeout detection only.
        No physical safe-state action is performed.
        """

        elapsed_time = (
            time.monotonic() - self.last_message_time
        )

        if elapsed_time >= self.timeout_seconds:

            if not self.communication_lost:

                self.communication_lost = True

                if not self.message_received:
                    self.get_logger().warning(
                        "COMMUNICATION TIMEOUT: "
                        "No movement commands received since startup."
                    )
                else:
                    self.get_logger().warning(
                        "COMMUNICATION LOST: "
                        f"No movement commands received for "
                        f"{elapsed_time:.2f} seconds."
                    )

                self.get_logger().warning(
                    "SIMULATION ONLY: "
                    "A physical robot would require an "
                    "independent motor-side safety response."
                )


def main(args=None):
    """Initialize and run the ROS 2 subscriber."""

    rclpy.init(args=args)

    node = PixelMovementSubscriber()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        node.get_logger().info(
            "Pixel movement subscriber stopped by user."
        )

    finally:
        node.destroy_node()

        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()