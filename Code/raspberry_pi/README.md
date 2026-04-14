# Raspberry Pi Control System (Arduino + Pixhawk + ROS 2)

This module runs on the **Raspberry Pi 5** and acts as the central control computer for the system. It manages communication with both the Arduino Uno (USB serial) for servo control and IMU data, and the Pixhawk 2.4.8 (UART via MAVLink) for motor control.

The Raspberry Pi is responsible for sending commands, receiving sensor data, and running ROS 2 nodes that coordinate the entire system.

---

## Core Responsibilities

The Raspberry Pi handles:

- Sending servo commands to the Arduino over USB serial
- Receiving and parsing IMU data from the Arduino
- Communicating with Pixhawk using MAVLink (pymavlink)
- Arming/disarming motors and sending RC override signals
- Running ROS 2 nodes for control and communication
- Acting as the platform for:
  - Teleoperation
  - Sensor processing
  - Future autonomy (CV, navigation)

---

## Hardware Connections

### Arduino (USB Serial)
- Device: /dev/ttyACM0 (may vary)
- Baud rate: 115200

### Pixhawk (UART)
- Device: /dev/ttyAMA0
- Baud rate: 57600
- Connection:
  - Pixhawk TX → Pi RX
  - Pixhawk RX → Pi TX
  - Shared GND

---

## Arduino Serial Communication

Commands are newline-terminated strings:

COMMAND,arg1,arg2,...

### Commands

Drive servos:
DRV,right,left

Camera servos:
CAM,rot,pitch

All servos:
ALL,right,left,rot,pitch

Ping:
PING

### Expected Responses

- ACK,DRV
- ACK,CAM
- ACK,ALL
- PONG
- ERR,BAD_DRV
- ERR,BAD_CAM
- ERR,BAD_ALL
- ERR,UNKNOWN_CMD

---

## IMU Data Stream

The Arduino continuously sends IMU data at ~20 Hz:

IMU,ax,ay,az,gx,gy,gz,mx,my,mz,roll,pitch,yaw

### Field Description

- ax, ay, az → acceleration (g)
- gx, gy, gz → angular velocity (deg/s)
- mx, my, mz → magnetometer (calibrated)
- roll, pitch, yaw → orientation (degrees)

---

## Python Serial Example

```python
import serial

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

ser.write(b'DRV,45,135\n')

while True:
    line = ser.readline().decode().strip()
    if line:
        print(line)
```

---

## Pixhawk Communication (MAVLink)

```python
from pymavlink import mavutil

master = mavutil.mavlink_connection('/dev/ttyAMA0', baud=57600)
master.wait_heartbeat()
print("Connected to Pixhawk")
```

---

## Motor Control

Arm motors:

```python
master.arducopter_arm()
master.motors_armed_wait()
```

Disarm motors:

```python
master.arducopter_disarm()
```

---

## RC Override Example

```python
master.mav.rc_channels_override_send(
    master.target_system,
    master.target_component,
    1500, 1500, 1500, 1500,
    0, 0, 0, 0
)
```

Notes:
- Channel 3 = throttle
- 1000 = minimum
- 1500 = neutral
- 2000 = maximum

---

## ROS 2 Integration (Jazzy)

### Topics

Subscribed:
- /servo_drive_cmd → [right, left]
- /servo_camera_cmd → [rot, pitch]

Published:
- /arduino_imu → sensor_msgs/msg/Imu

---

## Node Responsibilities

Serial Bridge Node:
- Opens /dev/ttyACM0
- Sends DRV / CAM commands
- Reads IMU data
- Publishes IMU messages

MAVLink Node:
- Connects to Pixhawk
- Arms/disarms motors
- Sends RC override commands

---

## Launch File Example

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='helium_arduino_interface',
            executable='serial_bridge',
            name='arduino_serial'
        ),
        Node(
            package='helium_mav_control',
            executable='mav_motor_controller',
            name='pixhawk_control'
        ),
    ])
```

---

## Teleoperation Example

Drive mapping:

- w → forward
- s → backward
- a → left
- d → right
- x → neutral

Camera:
- rotation centered at 90°
- pitch centered at 45°
- incremental control

---

## Startup Behavior

When the Arduino is ready, it sends:

READY

The Raspberry Pi should wait for this before sending commands.

---

## Main Runtime Behavior

The Raspberry Pi continuously:

1. Reads serial input from Arduino
2. Parses IMU and ACK messages
3. Sends servo commands when needed
4. Sends MAVLink commands to Pixhawk
5. Publishes data to ROS 2 topics

---

## Important Notes

- Do not run MAVProxy and ROS node on /dev/ttyAMA0 at the same time
- Only one process should access each serial port
- Always send newline (\n) with commands
- Disarm is the safest way to stop motors
- Ensure correct UART wiring (TX ↔ RX)

---

## Limitations

- No automatic reconnect
- No command retry system
- No synchronization between IMU and Pixhawk data
- No position hold (IMU-only system)
- No Pi-side failsafe

---

## Summary

The Raspberry Pi acts as the central control layer, interfacing with both the Arduino and Pixhawk. It sends actuator commands, receives IMU data, and runs ROS 2 nodes that enable system control, integration, and future expansion.

