# Bluetooth-Controlled 8x32 LED Matrix (Personal Ambient Notifier)

This project creates a dynamic and sophisticated Ambient Notifier display using an 8x32 LED matrix, controlled wirelessly via Bluetooth. It acts as a smart environment monitor, displaying real-time information such as time, temperature, and custom messages, making it an ideal, non-intrusive solution for modern workspaces.

### 🌟 Key Features

+ Real-Time Clock (RTC): Accurate time and date tracking using the DS1307 module.
Ambient Temperature Monitoring: Displays precise ambient temperature using the LM35 analog sensor.
+ Wireless Control: Complete system configuration and custom message updates via the HC-05 Bluetooth module.
+ Persistent Storage: Uses EEPROM to store the last selected display mode, scrolling effect, and custom message, ensuring settings are preserved across power cycles.
+ Multiple Display Modes: Switch between Time, Date, Temperature, and Custom Message modes.
+ Configurable Effects: Change the text scrolling effect (Left, Right, Up, Down) via Bluetooth.

### 🛠️ Hardware Requirements
| Component                | Quantity   | Notes                                 |
| :---                     | :---:      | :---:                                 |
| Arduino UNO Rev3         | 1          | Microcontroller Unit                  |
| 8x32 LED Matrix          | 1          | (4 cascaded 8x8 MAX72XX modules)      |
| HC-05 Bluetooth Module   | 1          | Wireless control and communication    |
| DS1307 RTC Module        | 1          | Real-Time Clock with battery backup   |
| LM35 Temperature Sensor  | 1          | Analog temperature measurement        |
| Wires, Breadboard        | As needed  | Connect components                    |

### 🔌 Wiring and Pinout
This project uses both SPI (for the LED Matrix) and I2C (for the RTC), plus SoftwareSerial for the Bluetooth module, and an analog pin for the sensor
| Component                 | Pin (Signal)  | Arduino Pin     | Notes                                                |
| :---                      | :---:         | :---:           | :---:                                                |
| LED Matrix (MD_Parola)    | CLK (SCK)     | D13             | Standard SPI Clock                                   |
|                           | DATA (MOSI)   | D11             | Standard SPI Data                                    |
|                           | CS (LOAD)     | D10             | Chip Select                                          |
| HC-05 Bluetooth           | RX            | D3              | Connect to BT_TX                                     |
|                           | TX            | D2              | Connect to BT_RX (via voltage divider if necessary)  |
| DS1307 RTC                | SDA           | A4 (or SDA pin) | I2C Data                                             |
|                           | SCL           | A5 (or SCL pin) | I2C Clock                                            |
| LM35 Sensor               | Analog Out    | A0              | Temperature reading                                  |
| Power/Ground              | VCC/GND       | 5V/GND          |                                                      |


### 💻 Software and Libraries
The following libraries must be installed in the Arduino IDE to compile the code:
1. MD_Parola (by MajicDesigns)
2. MD_MAX72xx (by MajicDesigns)
3. RTClib (by Adafruit)
4. SoftwareSerial (Standard Arduino Library - included by default)
5. Wire (Standard Arduino Library - included by default)
6. SPI (Standard Arduino Library - included by default)
7. EEPROM (Standard Arduino Library - included by default)
Note: Ensure you define the correct hardware type in MD_MAX72xx.h if you are not using the FC16_HW type. The current code defines it as:
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

### ⚙️ Bluetooth Control Commands
Communication with the system is done by sending single-character or formatted string commands via the Bluetooth serial connection (using a smartphone app like Serial Bluetooth Terminal).
| Command Format            | Example            | Function                                                             |
| :---                      | :---:              | :---:                                                                |
| '1'                       | 1                  | Set Mode: Show Time                                                  |
| '2'                       | 2                  | Set Mode: Show Date                                                  |
| '3' + Message             | 3Toi Yeu FPT       | Set Mode: Show Message and update the stored message                 |
| '4'                       | 4                  | Set Mode: Show Temperature                                           |
| '5'                       | 5                  | Set Mode: Manual Time Adjustment (Uses + / - / < / > commands)       |
| 'L'                       | L                  | Set Effect: Scroll Left                                              |
| 'R'                       | R                  | Set Effect: Scroll Right                                             |
| 'U'                       | U                  | Set Effect: Scroll Up                                                |
| 'D'                       | D                  | Set Effect: Scroll Down                                              |
| 'T' + YYYY/MM/DD/HH/MM    | T2025/11/17/10/30  | Adjust RTC Date/Time                                                 |
| '+'                       | +                  | (Mode 5) Increment Manual Hour                                       |
| '-'                       | -                  | (Mode 5) Decrement Manual Hour                                       |
| '>'                       | >                  | (Mode 5) Increment Manual Minute                                     |
| '<'                       | <                  | (Mode 5) Decrement Manual Minute                                     |
| 'O'                       | O                  | (Mode 5) Confirm and set manual time, then return to Time Mode (1)   |

### 👥 Team Contributions
| StudentID | Name              | Role and Contribution                                                     | Contribution Percentage   |
| :---      | :---:             | :---:                                                                     | ---:                      |
| SE192861  | Pham Tuan Anh     | Write report, develop the Arduino code for system functionality           | 30%                       |
| SE190124  | Le Tuan Kiet      | Develop the Arduino code for system functionality, assemble the circuit   | 30%                       |        
| SE190007  | Truong Thao Vi    | Assemble the circuit, draw the circuit schematic                          | 25%                       |
| SE190769  | Dang Hong Phuoc   | Draw block diagram and Flowchart, create presentation                     | 15%                       |