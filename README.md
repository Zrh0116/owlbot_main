# OwlBot Project

This is an owl interactive robot based on ESP32.
Main features:
- Head rotation
- Wing flapping
- Sound playback
- Touch / pressure response
- Sound-triggered interaction

- ## Hardware
- ESP32 Dev Module
- SG92R servos × 3
- MAX98357A amplifier
- Speaker
- Touch / pressure sensor
- Sound sensor
- 5V battery

- ## Pin Configuration
- Head Servo: GPIO13
- Left Wing Servo: GPIO12
- Right Wing Servo: GPIO25
- Touch Sensor: GPIO27
- Sound Sensor: GPIO34
- Button: GPIO26
- I2S BCLK: GPIO14

- ## File Structure
- OwlBot_Main.ino: main control loop
- config.h: pin definitions and settings
- sensors.cpp / sensors.h: sensor reading functions
- actuators.cpp / actuators.h: servo and audio control
- scheduler.cpp / scheduler.h: timing and random action scheduling

- ## How to Run
1. Open `OwlBot_Main.ino` in Arduino IDE
2. Select board: ESP32 Dev Module
3. Install required libraries:
   - ESP32Servo
4. Connect the hardware according to `docs/wiring.png`
5. Upload to ESP32
