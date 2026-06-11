# Smart Healthcare IoT Monitor System

A real-time edge fall detection and patient activity classifier powered by the **ESP32-S3** and **TinyML**. 

This system integrates four physical sensors (MPU6050 Accelerometer & Gyroscope, HC-SR04 Ultrasonic Distance, PIR Motion, and IR Obstacle Avoidance) to monitor patient behavior, calculate a running **Activity Score**, detect fall events using both a rule-based algorithm and a quantized neural network, and publish live telemetry to an MQTT broker over Wi-Fi.

---

## 1. Project Architecture

The system utilizes an **Arduino Uno** as a dedicated I2C-to-UART bridge for the MPU6050 to prevent I2C bus locking on the ESP32-S3. The main application runs entirely on the **ESP32-S3 N16R8**, which reads all sensor inputs, runs the TinyML model, and handles Wi-Fi/MQTT communications.

```mermaid
graph TD
    subgraph Sensors
        MPU[MPU6050 Accel/Gyro] -- I2C -- > Uno[Arduino Uno]
        HC[HC-SR04 Ultrasonic] -- Echo/Trig Pulse --> ESP[ESP32-S3 N16R8]
        PIR[PIR Motion Sensor] -- Digital HIGH/LOW --> ESP
        IR[IR Obstacle Sensor] -- Digital active LOW --> ESP
    end

    Uno -- 115200 Baud UART --> ESP

    subgraph ESP32-S3 Processing Engine
        ESP --> SM[Sensor Manager]
        SM --> FD[Rule-Based Fall Detector]
        SM --> ML[TinyML Classifier Inference]
        ML --> AS[Activity Score Tracker]
    end

    ESP -- Wi-Fi (MQTT) --> Broker[MQTT Broker]
    ESP -- USB CDC --> Serial[Arduino Serial Monitor]
```

---

## 2. Hardware Wiring Pinout

| Sensor/Module | Pin Name | Connected to | Logic / Protocol |
| :--- | :--- | :--- | :--- |
| **MPU6050** | VCC / GND | Arduino Uno 5V / GND | Power |
| | SDA / SCL | Arduino Uno A4 / A5 | I2C Protocol |
| **Arduino Uno** | Pin 3 (TX) | ESP32-S3 GPIO 18 (RX1) | 115200 Baud UART |
| | Pin 2 (RX) | ESP32-S3 GPIO 17 (TX1) | 115200 Baud UART |
| | GND | ESP32-S3 GND | Common Ground |
| **HC-SR04** | VCC / GND | ESP32-S3 5V (VBUS) / GND | Power |
| | TRIG / ECHO | ESP32-S3 GPIO 4 / GPIO 5 | Digital Pulse |
| **PIR Sensor** | VCC / GND | ESP32-S3 5V / GND | Power |
| | OUT | ESP32-S3 GPIO 45 | Digital (active High) |
| **IR Sensor** | VCC / GND | ESP32-S3 5V / GND | Power |
| | OUT | ESP32-S3 GPIO 39 | Digital (active Low) |

---

## 3. Directory Structure

```
├── HealthcareSystem/          # Main Arduino firmware directory
│   ├── HealthcareSystem.ino   # Main application sketch entry point
│   ├── config.h               # Pin definitions, WiFi/MQTT settings, and thresholds
│   ├── sensor_manager.h/.cpp  # Unified sensor reading, UART parsing & timeout checks
│   ├── fall_detector.h/.cpp   # Rule-based fall detection FSM logic
│   ├── tinyml_classifier.h    # StandardScaler, TFLite Micro runtime, and Activity Score logic
│   ├── model.h                # Quantized 3.5 KB TensorFlow Lite model byte-array
│   ├── scaler_params.h        # Pre-calculated dataset StandardScaler coefficients
│   ├── communication_manager.h/.cpp # WiFi auto-reconnection and MQTT telemetry publisher
│   └── (utility files)
│
├── ML/                        # TinyML dataset generation and model training pipeline
│   ├── download_datasets.py   # Downloads UCI HAPT & SisFall raw datasets
│   ├── preprocess.py          # Cleans data, normalizes formats, and generates synthetic features
│   ├── train_model.py         # Trains the Dense neural network and exports quantized model.h
│   └── model.tflite           # Compiled TensorFlow Lite binary model
│
├── SamplePrograms/            # Standalone test files for physical wiring diagnostics
│   ├── HCSR04_Test/
│   ├── IR_Test/
│   ├── MPU6050_Arduino_Sender/
│   ├── MPU6050_ESP32_Receiver/
│   ├── PIR_Test/
│   └── WiFi_MQTT_Test/
│
├── delete_library.bat         # Utility script to clean compile cache on Windows
└── README.md                  # This documentation
```

---

## 4. TinyML & Classifier Details

* **Input Features (9 parameters in exact order)**: 
  `accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, distance_cm, pir_motion (0.0/1.0), ir_obstacle (0.0/1.0)`
* **Data Normalization**: 
  Calculated on the ESP32-S3 using preloaded `SCALER_MEAN[9]` and `SCALER_STD[9]` arrays:
  $$\text{Scaled Value} = \frac{\text{Raw Value} - \text{Mean}}{\text{Standard Deviation}}$$
* **Model Size**: `3540 bytes` (~3.5 KB) compiled and quantized to run in an `8 KB` static memory arena.
* **Output Classification Categories**:
  - `0 = Normal` (Active walking/movement)
  - `1 = Resting` (Sitting still / relaxed)
  - `2 = Bed Exit` (Transitioning / moving away from sensor)
  - `3 = Inactive` (Lying motionless)
  - `4 = Fall` (Sudden high-impact fall)
* **Running Activity Score (0 - 100)**:
  - Normal classified: `+2`
  - Resting classified: `-1`
  - Inactive classified: `-3`
  - Fall classified: `-20`
  - Inactivity penalty: `-5` points every 5 seconds if PIR motion detects no movement for over 5 minutes.

---

## 5. WiFi & MQTT Telemetry Topics

The device connects to a configured Wi-Fi network and publishes JSON telemetry payloads to an MQTT broker over 8 channels:

1. `healthcare/mpu` $\to$ `{"accel_x":...,"accel_y":...,"accel_z":...,"gyro_x":...,"gyro_y":...,"gyro_z":...,"svm":...}`
2. `healthcare/distance` $\to$ `{"distance_cm":...}`
3. `healthcare/pir` $\to$ `{"pir_motion":true/false}`
4. `healthcare/ir` $\to$ `{"ir_obstacle":true/false}`
5. `healthcare/fall` $\to$ `{"fall_detected":true/false}` (From rule-based FSM)
6. `healthcare/prediction` $\to$ `{"label":"<class>","confidence":<float>,"score":<int>}` (TinyML Classifier output)
7. `healthcare/activityscore` $\to$ Plain integer string (e.g. `85`)
8. `healthcare/alerts` $\to$ Triggered warning payloads:
   - Fall state predicted: `{"alert":"FALL DETECTED","severity":"CRITICAL"}`
   - Inactivity predicted: `{"alert":"INACTIVITY","severity":"WARNING"}`
   - Bed Exit predicted: `{"alert":"BED EXIT","severity":"INFO"}`

---

## 6. How to Deploy the Project

### A. Pre-requisites
1. Install **Arduino IDE 2.x**.
2. Install **ESP32 Arduino Boards Package** (version 3.0.0 or higher is recommended).
3. Select **Tools** $\to$ **Partition Scheme** $\to$ **Huge APP (3MB No OTA/1MB SPIFFS)**.
4. Select **Tools** $\to$ **USB CDC On Boot** $\to$ **Enabled** (if using Native USB port).

### B. Patch the TensorFlow Lite Library
Because ESP32 Core 3.x uses the next-generation unified I2C driver (`driver_ng`), the legacy I2C screen drivers included in the original TensorFlow Lite library will cause compilation errors and runtime conflict loops.
1. Run [ML/download_tflite_zip.py](file:///c:/Users/REVANTH%20VISHNU%20REDDY/Desktop/IOT/ML/download_tflite_zip.py) followed by [ML/patch_and_rebuild_zip.py](file:///c:/Users/REVANTH%20VISHNU%20REDDY/Desktop/IOT/ML/patch_and_rebuild_zip.py) to generate `TensorFlowLite_ESP32.zip`. (This strips the conflicting `src/screen` and `src/bus` drivers and patches C++ compatibility templates).
2. Close your Arduino IDE.
3. Run the [delete_library.bat](file:///c:/Users/REVANTH%20VISHNU%20REDDY/Desktop/IOT/delete_library.bat) script to delete any existing old `Arduino_TensorFlowLite_ESP32-master` folder in your documents libraries path.
4. Reopen Arduino IDE, select **Sketch** $\to$ **Include Library** $\to$ **Add .ZIP Library...** and import the freshly generated ZIP.
5. In Arduino IDE, change **Tools** $\to$ **Core Debug Level** from **None** to **Debug** (this invalidates compile cache and forces a full re-build).
6. Upload to your ESP32-S3!
