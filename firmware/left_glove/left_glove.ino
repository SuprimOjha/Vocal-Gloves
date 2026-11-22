
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <math.h>

// ---------------- PINS ----------------
#define BUTTON_PIN 4
#define LED_PIN 3

#define MPU_ADDR 0x68

RF24 radio(9, 10);
const byte address[6] = "VOC01";

// ---------------- PACKET ----------------
struct GesturePacket {
  byte handID;
  byte command;
  byte gestureID;
  byte sequence;
};

GesturePacket packet;

// ---------------- MASTER ----------------
bool masterEnabled = true;
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_TIME = 50;

// ---------------- MPU DATA ----------------
int16_t axRaw, ayRaw, azRaw;
int16_t gxRaw, gyRaw, gzRaw;

float gxOffset = 0;
float pitchOffset = 0;
float rollOffset = 0;

float pitch = 0;
float roll = 0;
float gx = 0;

// ---------------- GESTURE SETTINGS ----------------
const float TILT_THRESHOLD = 25.0;
const float NEUTRAL_THRESHOLD = 12.0;
const float ROTATION_THRESHOLD = 200.0;

const unsigned long TILT_HOLD_TIME = 500;
const unsigned long ROTATION_HOLD_TIME = 80;
const unsigned long NEUTRAL_RESET_TIME = 300;

bool armed = false;

byte candidateGesture = 0;
unsigned long candidateStart = 0;
unsigned long neutralStart = 0;

byte sequenceNumber = 0;

// ---------------- MPU FUNCTIONS ----------------
void writeMPU(byte reg, byte value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)MPU_ADDR, 14, true) != 14) {
    return false;
  }

  axRaw = (Wire.read() << 8) | Wire.read();
  ayRaw = (Wire.read() << 8) | Wire.read();
  azRaw = (Wire.read() << 8) | Wire.read();

  Wire.read();
  Wire.read(); // Temperature

  gxRaw = (Wire.read() << 8) | Wire.read();
  gyRaw = (Wire.read() << 8) | Wire.read();
  gzRaw = (Wire.read() << 8) | Wire.read();

  return true;
}

void initializeMPU() {
  writeMPU(0x6B, 0x00);
  delay(100);

  writeMPU(0x1C, 0x00); // Accelerometer +/-2g
  writeMPU(0x1B, 0x00); // Gyroscope +/-250 degrees/sec
  writeMPU(0x1A, 0x03); // DLPF
  writeMPU(0x19, 0x09); // Sample divider
  delay(100);
}

void calibrateMPU() {
  Serial.println("Keep your LEFT glove completely still...");
  delay(2000);

  float gxSum = 0;
  float pitchSum = 0;
  float rollSum = 0;

  const int samples = 500;

  for (int i = 0; i < samples; i++) {
    if (readMPU()) {
      float ax = axRaw / 16384.0;
      float ay = ayRaw / 16384.0;
      float az = azRaw / 16384.0;

      float p = atan2(ax, sqrt(ay * ay + az * az))
                * 180.0 / PI;

      float r = atan2(ay, az) * 180.0 / PI;

      gxSum += gxRaw / 131.0;
      pitchSum += p;
      rollSum += r;
    }

    delay(3);
  }

  gxOffset = gxSum / samples;
  pitchOffset = pitchSum / samples;
  rollOffset = rollSum / samples;

  Serial.println("Calibration complete!");
}

void updateSensorValues() {
  if (!readMPU()) return;

  float ax = axRaw / 16384.0;
  float ay = ayRaw / 16384.0;
  float az = azRaw / 16384.0;

  float rawPitch = atan2(ax, sqrt(ay * ay + az * az))
                   * 180.0 / PI;

  float rawRoll = atan2(ay, az) * 180.0 / PI;

  pitch = rawPitch - pitchOffset;
  roll = rawRoll - rollOffset;

  gx = (gxRaw / 131.0) - gxOffset;
}

// ---------------- NRF FUNCTIONS ----------------
void sendPacket(byte command, byte gestureID) {
  packet.handID = 1;
  packet.command = command;
  packet.gestureID = gestureID;
  packet.sequence = ++sequenceNumber;

  radio.stopListening();
  bool success = radio.write(&packet, sizeof(packet));
  radio.startListening();

  if (success) {
    Serial.println("Packet delivered.");
  } else {
    Serial.println("Packet delivery failed.");
  }
}

void sendMasterCommand(byte command) {
  for (int i = 0; i < 3; i++) {
    sendPacket(command, 0);
    delay(20);
  }
}

void sendGesture(byte gestureID) {
  sendPacket(1, gestureID);

  Serial.print("LEFT HAND: G");
  Serial.println(gestureID);
}

// ---------------- GESTURE DETECTION ----------------
byte detectGesture() {

  // Rotation gestures have priority.
  if (gx < -ROTATION_THRESHOLD) return 5;
  if (gx > ROTATION_THRESHOLD) return 6;

  // Tilt gestures.
  if (roll > TILT_THRESHOLD) return 1;
  if (roll < -TILT_THRESHOLD) return 2;

  if (pitch < -TILT_THRESHOLD) return 3;
  if (pitch > TILT_THRESHOLD) return 4;

  return 0;
}

bool isNeutral() {
  return abs(pitch) < NEUTRAL_THRESHOLD &&
         abs(roll) < NEUTRAL_THRESHOLD &&
         abs(gx) < 20;
}

void gestureEngine() {
  unsigned long now = millis();

  if (!armed) {
    if (isNeutral()) {
      if (neutralStart == 0) {
        neutralStart = now;
      }

      if (now - neutralStart >= NEUTRAL_RESET_TIME) {
        armed = true;
        neutralStart = 0;
        Serial.println("GESTURE READY");
      }
    } else {
      neutralStart = 0;
    }

    return;
  }

  byte detected = detectGesture();

  if (detected == 0) {
    candidateGesture = 0;
    candidateStart = 0;
    return;
  }

  if (detected != candidateGesture) {
    candidateGesture = detected;
    candidateStart = now;
    return;
  }

  unsigned long holdTime =
      (detected == 5 || detected == 6)
      ? ROTATION_HOLD_TIME
      : TILT_HOLD_TIME;

  if (now - candidateStart >= holdTime) {
    sendGesture(detected);

    armed = false;
    candidateGesture = 0;
    candidateStart = 0;
    neutralStart = 0;
  }
}

// ---------------- MASTER BUTTON ----------------
void checkMasterButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > DEBOUNCE_TIME) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        masterEnabled = !masterEnabled;

        digitalWrite(LED_PIN,
                     masterEnabled ? HIGH : LOW);

        if (masterEnabled) {
          Serial.println("MASTER ENABLED");
          sendMasterCommand(3);
        } else {
          Serial.println("MASTER DISABLED");
          sendMasterCommand(2);
        }

        armed = false;
        candidateGesture = 0;
        candidateStart = 0;
        neutralStart = 0;
      }
    }
  }

  lastButtonReading = reading;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  Wire.begin();
  initializeMPU();

  calibrateMPU();

  if (!radio.begin()) {
    Serial.println("NRF24 NOT FOUND!");
    while (1);
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(100);
  radio.setAutoAck(true);
  radio.setRetries(5, 15);

  radio.openWritingPipe(address);
  radio.stopListening();

  armed = false;

  Serial.println("LEFT GLOVE READY");
  Serial.println("MASTER ENABLED");
  Serial.println("Return to neutral position.");
}

// ---------------- LOOP ----------------
void loop() {
  checkMasterButton();

  if (masterEnabled) {
    updateSensorValues();
    gestureEngine();
  }

  delay(10);
}
