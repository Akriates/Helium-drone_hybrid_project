# System Overview

This system is built around a Raspberry Pi 5 acting as the main controller. It interfaces with an Arduino Uno over USB serial for servo control and IMU data, and a Pixhawk 2.4.8 over UART (MAVLink) for motor control.

The Raspberry Pi sends commands to control motor tilt and camera servos, receives real-time orientation data (roll, pitch, yaw), and manages motor arming and throttle through the Pixhawk.

ROS 2 is used to structure the system, with nodes handling serial communication, MAVLink control, and data publishing. This setup provides a foundation for teleoperation and future extensions such as computer vision and autonomous control.
