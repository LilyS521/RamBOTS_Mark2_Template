
# Pixel ROS 2 Movement Control Prototype

**Project:** RamBOTs Mark II (Pixel)  
**University:** Colorado State University  
**Subsystem:** Computer Engineering / ROS 2 Software  
**ROS Distribution:** Jazzy  
**Development OS:** Ubuntu 24.04 LTS via WSL 2  
**Status:** Initial publisher/subscriber prototype tested

## 1. Overview

This package provides an initial ROS 2 communication
prototype for Pixel, the RamBOTs Mark II quadruped.

The prototype demonstrates communication between two
independent ROS 2 nodes using velocity commands.

It also demonstrates basic communication monitoring
and diagnostic reporting.

All testing has been performed without physical
motors, a Teensy, or a Jetson.

## 2. Architecture

The current prototype uses:

    Pixel Movement Publisher
              |
              | /pixel/cmd_vel
              | geometry_msgs/msg/Twist
              v
    Pixel Movement Subscriber
              |
              v
    Console Output / Diagnostics

The publisher generates simulated velocity commands.

The subscriber receives the commands, identifies the
requested movement, and monitors communication.

Future integration may connect movement commands to
a gait controller and the Teensy motor interface.

These interfaces are not yet finalized.

## 3. Package Contents

### movement_publisher.py

ROS 2 node: pixel_movement_publisher

Responsibilities:
- Generate simulated movement commands.
- Publish geometry_msgs/msg/Twist messages.
- Cycle through forward, backward, lateral, rotation,
  and stop commands.
- Publish at a target frequency of 2 Hz.

### movement_subscriber.py

ROS 2 node: pixel_movement_subscriber

Responsibilities:
- Subscribe to /pixel/cmd_vel.
- Interpret incoming velocity commands.
- Count and log received messages.
- Detect missing communication.
- Report communication recovery.

The watchdog checks communication every 0.5 seconds
and uses a 3-second timeout threshold.

A steady ROS clock is used for watchdog scheduling,
with Python monotonic time for elapsed-time tracking.

The watchdog provides diagnostics only.

### setup.py

Registers both nodes as ROS 2 executables.

## 4. Dependencies

Required environment:

- Ubuntu 24.04 LTS
- ROS 2 Jazzy
- Python 3
- rclpy
- geometry_msgs
- colcon build tools

The initial implementation was developed on Windows
using WSL 2 and Visual Studio Code.

## 5. Build Instructions

Open an Ubuntu terminal.

Load ROS 2:

    source /opt/ros/jazzy/setup.bash

Navigate to the workspace:

    cd ~/pixel_ros2_ws

Build the package:

    colcon build --packages-select pixel_control --symlink-install

Load the workspace:

    source ~/pixel_ros2_ws/install/setup.bash

Verify registered executables:

    ros2 pkg executables pixel_control

Expected executables:

    pixel_control movement_publisher
    pixel_control movement_subscriber

## 6. Running the Nodes

Open two Ubuntu terminals.

In Terminal 1:

    source ~/pixel_ros2_ws/install/setup.bash
    PYTHONUNBUFFERED=1 ros2 run pixel_control movement_publisher

In Terminal 2:

    source ~/pixel_ros2_ws/install/setup.bash
    PYTHONUNBUFFERED=1 ros2 run pixel_control movement_subscriber

Both nodes should now communicate over:

    /pixel/cmd_vel

Stop either node using Ctrl+C.

## 7. ROS 2 Topic Verification

Open a third Ubuntu terminal and load the workspace:

    source ~/pixel_ros2_ws/install/setup.bash

List topics and message types:

    ros2 topic list -t

Inspect the movement topic:

    ros2 topic info /pixel/cmd_vel --verbose

Measure publishing frequency:

    ros2 topic hz /pixel/cmd_vel

Display messages:

    ros2 topic echo /pixel/cmd_vel

List active nodes:

    ros2 node list

Expected topic type:

    geometry_msgs/msg/Twist

## 8. Simulated Movement Commands

The publisher generates the following movements:

| Movement | Linear X | Linear Y | Angular Z |
|----------|----------|----------|-----------|
| Stop | 0.0 | 0.0 | 0.0 |
| Forward | 0.2 | 0.0 | 0.0 |
| Backward | -0.2 | 0.0 | 0.0 |
| Strafe Left | 0.0 | 0.1 | 0.0 |
| Strafe Right | 0.0 | -0.1 | 0.0 |
| Rotate Left | 0.0 | 0.0 | 0.3 |
| Rotate Right | 0.0 | 0.0 | -0.3 |

Linear units: meters/second
Angular units: radians/second

These are example commands and are not validated
physical operating limits for Pixel.

## 9. Testing Results

The following software-only tests were performed.

| Test | Result |
|------|--------|
| ROS 2 package build | PASS |
| Publisher initialization | PASS |
| Subscriber initialization | PASS |
| ROS 2 topic discovery | PASS |
| Twist message reception | PASS |
| Forward/backward commands | PASS |
| Lateral movement commands | PASS |
| Rotation commands | PASS |
| Stop commands | PASS |
| Publishing frequency | PASS |
| Startup communication timeout | PASS |
| Communication recovery | PASS |
| Communication loss after disconnect | PASS |

Observed publishing rate:

    Approximately 1.993-2.001 Hz

Observed communication-loss detection:

    3.07 seconds after the final received command

The subscriber successfully detected publisher
disconnection and reported the loss.

A steady ROS clock resolved the observed watchdog
timer behavior during testing.

The underlying cause of the original timer stall
has not been conclusively established.

## 10. Safety Limitations

This is a software communication prototype.

It does NOT:
- Directly control RobStride motors.
- Send motor commands to a Teensy.
- Implement gait planning.
- Enforce physical motor shutdown.
- Verify joint positions or torque limits.
- Implement a hardware emergency stop.

A future physical robot safety system will need
independent command-loss handling, motor-side
protection, and hardware validation.

## 11. Future Integration

Potential future communication architecture:

    Operator Controller
            |
            v
    ROS 2 Command Input
            |
            v
    Gait Controller
            |
            v
    Motor Command Interface
            |
            v
    Teensy 4.1
            |
            v
    RobStride Motors

CHAMP is being considered for initial gait control.

OCS2 may be investigated for future advanced control.

The communication protocol between ROS 2 and the
Teensy remains under development.

The existing publisher/subscriber prototype should
be integrated into the team architecture rather
than duplicated in separate motor-control systems.

## 12. Next Steps

1. Review the node architecture with Luke.
2. Coordinate the command interface with Lily.
3. Determine how cmd_vel feeds into gait control.
4. Define ROS 2-to-Teensy communication requirements.
5. Integrate motor status and diagnostic messages.
6. Add automated tests for the ROS 2 nodes.
7. Evaluate simulation integration with MuJoCo.
8. Test actual hardware when available.

## 13. Development Notes

This prototype was developed and tested as part of
the November 4-11, 2026 RamBOTs work period.

It represents the initial ROS 2 node creation,
publisher/subscriber communication, and diagnostics
milestone for Pixel.

It does not represent completed locomotion control.