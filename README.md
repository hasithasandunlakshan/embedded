# IoT Environmental Monitoring System

This project is for the University of Moratuwa EN2853 Programming Assignment 2. It implements an IoT-based environmental monitoring system using ESP32, sensors, and Node-RED for visualization.

## Components Used

- ESP32 DevKit C V4
- DHT22 Temperature and Humidity Sensor
- Photoresistor (LDR) Light Sensor
- Buzzer for alerts
- Servo Motor for physical control

## Wokwi Simulation

The project is set up to run on the Wokwi simulator. The diagram.json file contains the circuit configuration for the simulation.

### Setting up Wokwi

1. Go to [Wokwi](https://wokwi.com/)
2. Create a new ESP32 project
3. Copy the contents of `diagram.json` to your project's diagram.json
4. Copy the contents of `esp32_code.ino` to your project's sketch file
5. Click "Start Simulation"

## Node-RED Dashboard

The project includes a Node-RED flow for visualization and control of the sensors and actuators.

### Setting up Node-RED

1. Install Node-RED if you haven't already:
   ```
   npm install -g node-red
   ```

2. Install required Node-RED packages:
   ```
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

## Features

- Real-time temperature and humidity monitoring with DHT22
- Light level monitoring with photoresistor
- Temperature threshold alerts with buzzer
- Automatic and manual servo control based on light levels
- MQTT communication between ESP32 and Node-RED
- Dashboard with gauges, charts, and controls

## Troubleshooting

- **MQTT Connection Issues**: Make sure the ESP32 and Node-RED are connected to the internet. The project uses the public HiveMQ broker.
- **Sensor Readings**: If you're not getting readings, check the wiring in the Wokwi diagram.
- **Wokwi Simulation**: If the simulation is slow, try reducing the baud rate in the serial monitor.

## Author

Hasitha Sandun 