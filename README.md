# Medibox with Node-RED Integration

This project implements a smart medication box (Medibox) using ESP32 with Node-RED dashboard integration for monitoring environmental conditions and controlling a shaded sliding window.

## Features

- Real-time temperature and humidity monitoring
- Environmental alerts for medication storage conditions
- Time synchronization using NTP
- Visual and audible alerts
- **Light intensity monitoring with configurable sampling intervals**
- **Automatic shaded sliding window control using servo motor**
- **Adaptive environment control based on light and temperature conditions**

## Components Used

- ESP32 DevKit C V4
- DHT22 Temperature and Humidity Sensor
- OLED Display (128x64)
- Buzzer for alerts
- Push buttons for user interface
- LED for indication
- **LDR (Light Dependent Resistor) for light intensity monitoring**
- **Servo motor for controlling the shaded sliding window**

## Wokwi Simulation

The project is set up to run on the Wokwi simulator. The diagram.json file contains the circuit configuration.

## MQTT Topics

The Medibox communicates with Node-RED using the following MQTT topics:

- `medibox/temperature` - Current temperature reading
- `medibox/humidity` - Current humidity reading
- `medibox/status` - Device status information
- `medibox/light` - Light intensity readings
- `medibox/motor` - Servo motor angle updates
- `medibox/config` - Configuration parameters

## Node-RED Dashboard

The Node-RED dashboard provides the following functionality:

1. **Environmental Metrics**
   - Temperature and humidity gauges
   - Historical charts

2. **System Status**
   - Device connectivity
   - Current time
   - Environmental status

3. **Light Intensity Monitoring**
   - Real-time light intensity display
   - Historical light intensity chart
   - Configurable sampling intervals
   - Configurable data sending intervals

4. **Shade Controller**
   - Servo angle display
   - Minimum angle configuration (θoffset)
   - Controlling factor adjustment (γ)
   - Ideal temperature setting (Tmed)

## Shaded Sliding Window Control Algorithm

The servo motor angle for the shaded sliding window is calculated using the formula:

θ = θoffset + (180 - θoffset) × I × γ × ln((ts/tu) × (T/Tmed))

Where:
- θ is the motor angle (degrees)
- θoffset is the minimum angle (default 30°)
- I is the light intensity (0-1)
- γ is the controlling factor (default 0.75)
- ts is the sampling interval (seconds)
- tu is the sending interval (seconds)
- T is the measured temperature (°C)
- Tmed is the ideal storage temperature (default 30°C)

This formula ensures that the shade adjusts based on both light intensity and temperature conditions to maintain optimal storage conditions for light-sensitive medicines.

## Setup Instructions

1. Upload the ESP32 code to your ESP32 board or run it in the Wokwi simulator
2. Import the Node-RED flow into your Node-RED instance
3. Ensure the MQTT broker is accessible (default: test.mosquitto.org)
4. Open the Node-RED dashboard to interact with the Medibox

## Configuration

You can configure the Medibox parameters directly from the Node-RED dashboard:

- Light sampling interval (1-60 seconds)
- Light data sending interval (10-600 seconds)
- Minimum servo angle (0-120 degrees)
- Control factor (0-1)
- Ideal medicine storage temperature (10-40°C)

## Usage

1. The Medibox automatically monitors temperature, humidity, and light intensity
2. The servo motor adjusts the shaded window based on environmental conditions
3. Use the physical buttons on the ESP32 to navigate the local menu if needed
4. Adjust configuration parameters based on specific environmental requirements

## Troubleshooting

- **MQTT Connection Issues**: Make sure both the ESP32 and the computer running Node-RED have internet connectivity.
- **ESP32 Not Responding to Dashboard Commands**: Check the MQTT connection status on the dashboard.
- **Missing Library Errors**: Make sure all required libraries are installed in the Arduino IDE or Wokwi.
- **Dashboard Not Showing Data**: Verify that the ESP32 is publishing to the correct MQTT topics.

## Future Enhancements

- Add medication inventory tracking
- Implement multiple user profiles
- Add secure authentication for remote access
- SMS or email notifications for missed medications
- Mobile app integration 