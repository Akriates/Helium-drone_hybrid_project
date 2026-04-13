## Model

The parts within this folder correspond to the **final tested prototype** used during system validation. This includes the full mechanical structure that supports the propulsion system, electronics, and camera assembly.

While this design was functional and enabled successful testing, it should be considered an **early iteration**. A redesign is recommended to better account for practical considerations such as:
- Wiring integration and routing  
- Structural strength and weight distribution  
- Ease of assembly and maintenance  
- Mounting points for electronics and sensors  

---

### Full Assembly

The complete assembled prototype is shown below:

![Full Assembly](../../Images/Full_assembly_front_view.jpeg)

---

## Electronics

The following components were used in the final prototype to achieve control, sensing, and actuation.

### Components Used

**Control Systems**
- Raspberry Pi 5 (main processor, ROS 2 control)
- Arduino Uno (servo control and IMU processing)
- Pixhawk 2.4.8 (motor control)

**Actuation**
- MG996R Servos (motor tilting and camera gimbal)
- A2212 Brushless Motors
- Electronic Speed Controllers (ESCs)

**Sensing**
- MPU9250 IMU

**Wiring and Power**
- Jumper wires (male-to-male, male-to-female)
- Servo extension cables  
- Custom XT60 cables (~1m, male-to-female)  
- Custom power distribution wiring  
- Various additional connection wires  

---

### Wiring Layout

The full wiring configuration is shown below:

![Full Wiring](Electronics/full_wiring.jpeg)

The full wiring diagram as a drawing:

![Full Wiring](Electronics/full_wiring_diagram.jpeg)

---

### Design Considerations

The current electronics setup is functional but not optimized. Future iterations should focus on:

- Proper cable management and routing  
- Dedicated power distribution (avoid powering servos from microcontroller rails)  
- Reducing reliance on temporary jumper connections  
- Improving connector reliability  
- Designing the mechanical structure with integrated wiring paths  
