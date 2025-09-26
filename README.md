#  ESP32 ADC Web Datalogger 📈

A highly configurable, Wi-Fi enabled, multi-channel ADC data logger running on an ESP32-C3. It features a captive portal and a web interface for on-the-fly sensor calibration and data extraction, requiring no client-side software or internet connection.

---

## 💡 Key Features

- features:
    - wifi_access_point: 📶 Creates its own Wi-Fi network for configuration.
    - captive_portal: 🌐 Automatically redirects any connected device to the configuration portal.
    - web_ui_control: 💻 Fully configurable via a clean web interface from any browser.
    - real_time_calibration: 🔧 Apply a linear function (y = mx + b) to raw ADC values on-the-fly to log real physical units (Voltage, °C, etc.).
    - multi_channel_logging: 🔢 Simultaneously log data from multiple ADC channels.
    - long_duration_sampling: ⏱️ Designed for 24-hour data collection with 1-second intervals.
    - csv_export: 📄 Easy one-click download of collected data in CSV format.
    - visual_feedback: 🟢 An onboard LED signals when the data collection cycle is complete.

---

## ⚙️ How it Works

The project operates in four main phases, creating a seamless, self-contained data logging system.

- workflow:
    - 1_setup_phase:
        - description: On boot, the ESP32 starts a Wi-Fi Access Point and a DNS server.
        - result: Creates a local network (e.g., 'ESP32_Sensor_Setup').
    - 2_configuration_phase:
        - description: A user connects to the Wi-Fi network. The captive portal redirects their browser to the web UI.
        - user_actions:
            - Select ADC pins to monitor.
            - Input calibration parameters (slope 'm' and offset 'b') for each sensor.
            - Start the 24-hour logging session.
        - time_sync: The user's browser sends its current time to the ESP32 for accurate timestamping.
    - 3_logging_phase:
        - description: The ESP32 logs data every second. For each sample, it reads the raw ADC value, applies the user-defined calibration formula, and appends the result to a CSV file on its internal filesystem (SPIFFS).
        - autonomy: This process runs autonomously for 24 hours, no Wi-Fi connection is needed after it starts.
    - 4_extraction_phase:
        - description: At any time (during or after logging), the user can reconnect to the Wi-Fi network and access the web portal.
        - result: The user can download the complete CSV data file with a single click. The onboard LED will turn on to indicate the 24-hour cycle has finished.

---

## 🚀 Getting Started

To get this project up and running, follow these steps.

- setup:
    - 1_clone_repository:
        - command: git clone [https://github.com/sergio-isidoro/ESP32_ADC_WEB_Datalogger.git](https://github.com/sergio-isidoro/ESP32_ADC_WEB_Datalogger.git)
    - 2_configure_arduino_ide:
        - board_manager: Make sure you have the ESP32 board definitions installed. Select "ESP32C3 Dev Module".
        - libraries: Install the required libraries from the Library Manager:
            - ESPAsyncWebServer
            - AsyncTCP
    - 3_connect_hardware:
        - Connect your ADC sensors to the available pins (GPIO 0-4).
        - Connect an LED to the designated pin (default is GPIO 7).
    - 4_upload_code:
        - Open the .ino file in the Arduino IDE and upload it to your ESP32-C3.
    - 5_connect_and_configure:
        - On your phone or laptop, connect to the "ESP32_Sensor_Setup" Wi-Fi network.
        - The captive portal should open automatically. If not, open a browser.
        - Configure your sensors, calibration, and start the collection.

---

## 🔌 Hardware & Software

- requirements:
    - hardware:
        -  microcontroller: ESP32-C3 (or any other ESP32 variant with minor pin changes).
        - sensors: Any analog sensor that outputs a voltage compatible with the ESP32's ADC.
        - indicator: 1x LED.
    - software:
        - framework: Arduino IDE or PlatformIO.
        - libraries: See "Getting Started" section.

---

## 📄 CSV Output Format

The output data is stored in a simple, easy-to-parse CSV format.

- csv_format:
    - header: The first row contains 'timestamp' followed by the selected GPIO pin numbers.
    - data_rows: Each row represents one sample per second.
    - values: All values are the final, calibrated floating-point numbers, not raw ADC readings.
    - timestamp: The time is in UTC format (HH:MM:SS), synchronized from the client's browser at the start of the session.

- example:
```
    # Scenario:
    # - Pins 0 (Voltage sensor, m=0.000806, b=0) and 2 (Temp sensor, m=0.05, b=-20) selected.
    # - Collection started at 14:10:00 UTC.

    timestamp,0,2
    14:10:00,1.2493,7.5000
    14:10:01,1.2509,7.5500
    14:10:02,1.2533,7.6000
```

---

## 🗺️ Roadmap

Future improvements planned for this project:

- roadmap:
    - [ ] Real-Time Clock (RTC) support for persistent timekeeping across reboots.
    - [ ] Deep sleep modes for ultra-low-power battery operation between samples.
    - [ ] Live data visualization on the web portal using charts.
    - [ ] Support for I2C and SPI sensors.
    - [ ] MQTT integration for real-time data streaming to a smart home or IoT platform.
