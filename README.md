# Medibox with Node-RED Integration

This project implements a smart medication box (Medibox) using ESP32 with Node-RED dashboard integration for remote monitoring and control of medication alarms.

## Features

- Set and manage medication alarms through ESP32 or Node-RED dashboard
- Real-time temperature and humidity monitoring
- Environmental alerts for medication storage conditions
- Remote alarm management via MQTT and Node-RED
- Time synchronization using NTP
- Visual and audible medication reminders
- **Light intensity monitoring with configurable sampling intervals**
- **Automatic shaded sliding window control using servo motor**
- **Adaptive environment control based on light and temperature conditions**

## Components Used

- ESP32 DevKit C V4
- DHT22 Temperature and Humidity Sensor
- OLED Display (128x64)
- Buzzer for alerts
- Push buttons for user interface
- LED for alarm indication
- **LDR (Light Dependent Resistor) for light intensity monitoring**
- **Servo motor for controlling the shaded sliding window**

## Wokwi Simulation

The project is set up to run on the Wokwi simulator. The diagram.json file contains the circuit configuration for the simulation.

### Setting up Wokwi

1. Go to [Wokwi](https://wokwi.com/)
2. Create a new ESP32 project
3. Copy the contents of `diagram.json` to your project's diagram.json
4. Copy the contents of `esp32_code.ino` to your project's sketch file
5. Add the required libraries in the Libraries manager:
   - DHTesp
   - Adafruit GFX Library
   - Adafruit SSD1306
   - PubSubClient
   - ArduinoJson (version 6.x)
6. Click "Start Simulation"

## Node-RED Dashboard

The project includes a Node-RED flow for visualization and control of the Medibox system.

### Setting up Node-RED

1. Install Node-RED if you haven't already:
   ```
   npm install -g node-red
   ```

2. Install required Node-RED packages:
   ```
   cd ~/.node-red
   npm install node-red-dashboard
   npm install node-red-contrib-ui-led
   ```

3. Start Node-RED:
   ```
   node-red
   ```

4. Access Node-RED in your browser at `http://localhost:1880`

5. Import the flow by selecting Menu → Import → Clipboard, and paste the contents of `node-red-flow.json`

6. Deploy the flow and access the dashboard at `http://localhost:1880/ui`

## MQTT Communication

This project uses MQTT for communication between the ESP32 and Node-RED. We're using the HiveMQ public broker for simplicity.

### MQTT Topics

- `medibox/temperature` - Current temperature readings
- `medibox/humidity` - Current humidity readings
- `medibox/alarms` - Information about configured alarms
- `medibox/status` - Overall system status
- `medibox/command` - Used to send commands to the ESP32
- `medibox/light` - Light intensity readings
- `medibox/motor` - Servo motor angle updates
- `medibox/config` - Configuration parameters

## Dashboard Features

The Node-RED dashboard provides:

1. **Environmental Metrics**
   - Real-time temperature and humidity gauges
   - Historical charts of environmental conditions

2. **Alarm Management**
   - View and set medication alarms
   - Add new alarms through the interface
   - View active alarm status

3. **System Status**
   - Connection status
   - Current time
   - Environmental status indicators

4. **Light Intensity Monitoring**
   - Real-time light intensity display
   - Historical light intensity chart
   - Configurable sampling intervals
   - Configurable data sending intervals

5. **Shade Controller**
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
3. Set alarms for medication times using the physical buttons or Node-RED dashboard
4. Receive visual and audible alerts when it's time to take medication
5. Adjust configuration parameters based on specific medication requirements

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