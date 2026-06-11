#include <SPI.h>
#include <RF24.h>

// ============================================================================
// SYSTEM STATES (Matches Patient Node)
// ============================================================================
enum PatientState : uint8_t {
    STATE_IDLE = 0,
    STATE_ACTIVE = 1,
    STATE_WALKING = 2,
    STATE_STANDING = 3,
    STATE_SITTING = 4,
    STATE_SLEEPING = 5,
    STATE_BED_OCCUPIED = 6,
    STATE_BED_EXIT = 7,
    STATE_IMMOBILE = 8,
    STATE_FALL_DETECTED = 9,
    STATE_EMERGENCY = 10
};

// Convert state to human-readable string
const char* stateToString(uint8_t state) {
    switch (state) {
        case STATE_IDLE:          return "IDLE";
        case STATE_ACTIVE:        return "ACTIVE";
        case STATE_WALKING:       return "WALKING";
        case STATE_STANDING:      return "STANDING";
        case STATE_SITTING:       return "SITTING";
        case STATE_SLEEPING:      return "SLEEPING";
        case STATE_BED_OCCUPIED:  return "BED OCCUPIED";
        case STATE_BED_EXIT:      return "BED EXIT";
        case STATE_IMMOBILE:      return "IMMOBILE";
        case STATE_FALL_DETECTED: return "FALL DETECTED";
        case STATE_EMERGENCY:     return "EMERGENCY";
        default:                  return "UNKNOWN";
    }
}

// ============================================================================
// NRF24L01 CONFIGURATION (Arduino Uno SPI Pins: MISO=12, MOSI=11, SCK=13)
// ============================================================================
#define CE_PIN   9
#define CSN_PIN  10

RF24 radio(CE_PIN, CSN_PIN);
const uint8_t RX_ADDRESS[6] = "00001";

// ============================================================================
// PACKET STRUCTURE (Must be byte-for-byte identical to ESP32-S3)
// ============================================================================
struct __attribute__((__packed__)) PatientPacket {
    uint8_t patientId;          // Unique node ID (1 byte)
    uint8_t currentState;       // FSM State integer (1 byte)
    float distance;             // Distance in cm (4 bytes)
    float svm;                  // Signal Vector Magnitude (4 bytes)
    uint8_t pirStatus;          // PIR status (1 byte)
    uint8_t irStatus;           // IR status (1 byte)
    uint32_t timestamp;         // Milliseconds since boot (4 bytes)
    float batteryLevel;         // Battery voltage (4 bytes)
    uint8_t alertCode;          // 0 = Info, 1 = Warning, 2 = Critical Alert (1 byte)
    uint32_t txCounter;         // Packet index count (4 bytes)
}; // Total size = 25 bytes

void setup() {
    // Start serial console at standard speed
    Serial.begin(115200);
    while (!Serial) {
        // Wait for serial monitor
    }

    Serial.println(F("=================================================="));
    Serial.println(F("AAL GATEWAY NODE RECEIVER - INITIALIZING"));
    Serial.println(F("=================================================="));

    // Initialize RF24
    if (!radio.begin()) {
        Serial.println(F("ERROR: NRF24L01 hardware did not respond!"));
        while (1) {
            // Halt if radio initialization fails
        }
    }

    // Configure Radio Parameters (Must match Transmitter)
    radio.setPALevel(RF24_PA_LOW);      // Use LOW for workbench testing
    radio.setDataRate(RF24_250KBPS);    // Matches Transmitter
    radio.setChannel(76);               // Matches channel 76
    radio.setAutoAck(true);             // Hardware ACKs enabled

    // Open Rx pipe to match address
    radio.openReadingPipe(1, RX_ADDRESS);
    radio.startListening();             // Put radio in RX Mode

    Serial.println(F("[Gateway] Receiver online, listening on address: 00001"));
    Serial.println(F("=================================================="));
}

void loop() {
    if (radio.available()) {
        PatientPacket rxPacket;
        
        // Read incoming payload
        radio.read(&rxPacket, sizeof(PatientPacket));

        // Get matching state string
        const char* stateStr = stateToString(rxPacket.currentState);

        // 1. Human-Readable Console Printout
        printStructuredConsole(rxPacket, stateStr);

        // 2. Machine-Readable JSON Output (for Node-Red, Python, or Cloud ingestion)
        printMachineJSON(rxPacket, stateStr);
    }
}

void printStructuredConsole(const PatientPacket &packet, const char* stateStr) {
    Serial.println(F("\n--- NEW TELEMETRY RECEIVED ---"));
    Serial.print(F("Patient ID : 0x"));
    if (packet.patientId < 0x10) Serial.print('0');
    Serial.println(packet.patientId, HEX);
    
    Serial.print(F("Tx Counter : "));
    Serial.println(packet.txCounter);
    
    Serial.print(F("Patient FSM: STATE_"));
    Serial.println(stateStr);
    
    Serial.print(F("Distance   : "));
    Serial.print(packet.distance, 1);
    Serial.println(F(" cm"));

    Serial.print(F("Accel SVM  : "));
    Serial.print(packet.svm, 3);
    Serial.println(F(" g"));

    Serial.print(F("PIR Status : "));
    Serial.println(packet.pirStatus == 1 ? F("MOTION") : F("NO MOTION"));

    Serial.print(F("Bed Status : "));
    Serial.println(packet.irStatus == 0 ? F("OCCUPIED") : F("VACANT"));

    Serial.print(F("Node Uptime: "));
    Serial.print((float)packet.timestamp / 1000.0f, 2);
    Serial.println(F(" seconds"));

    Serial.print(F("Battery Lvl: "));
    Serial.print(packet.batteryLevel, 2);
    Serial.println(F(" V"));

    Serial.print(F("Alert Level: "));
    if (packet.alertCode == 2) {
        Serial.println(F("CRITICAL (!!! EMERGENCY !!!)"));
    } else if (packet.alertCode == 1) {
        Serial.println(F("WARNING (BED EXIT / IMMOBILITY)"));
    } else {
        Serial.println(F("NORMAL"));
    }
    Serial.println(F("------------------------------"));
}

void printMachineJSON(const PatientPacket &packet, const char* stateStr) {
    // Print prefix to differentiate JSON from terminal logs
    Serial.print(F("DATA_JSON:"));
    
    Serial.print(F("{\"patientId\":"));
    Serial.print(packet.patientId);
    Serial.print(F(",\"txCounter\":"));
    Serial.print(packet.txCounter);
    Serial.print(F(",\"state\":\""));
    Serial.print(stateStr);
    Serial.print(F("\",\"distance\":"));
    Serial.print(packet.distance, 2);
    Serial.print(F(",\"svm\":"));
    Serial.print(packet.svm, 4);
    Serial.print(F(",\"pirStatus\":"));
    Serial.print(packet.pirStatus);
    Serial.print(F(",\"irStatus\":"));
    Serial.print(packet.irStatus);
    Serial.print(F(",\"timestamp\":"));
    Serial.print(packet.timestamp);
    Serial.print(F(",\"batteryLevel\":"));
    Serial.print(packet.batteryLevel, 2);
    Serial.print(F(",\"alertCode\":"));
    Serial.print(packet.alertCode);
    Serial.println(F("}"));
}
