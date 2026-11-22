#include <SPI.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ---------------- NRF24 ----------------
RF24 radio(9, 10);
const byte address[6] = "VOC01";

// ---------------- DFPLAYER ----------------
// Arduino RX = D2, Arduino TX = D3
SoftwareSerial dfSerial(2, 3);
DFRobotDFPlayerMini player;

// ---------------- PACKET ----------------
struct GesturePacket {
  byte handID;
  byte command;
  byte gestureID;
  byte sequence;
};

GesturePacket packet;

// ---------------- SYSTEM SETTINGS ----------------
bool gesturesEnabled = true;

// 1 = Nepali
// 2 = English
byte languageFolder = 1;

// ---------------- AUDIO ----------------
void playGestureAudio(byte gestureID) {
  if (gestureID < 1 || gestureID > 12) {
    Serial.println("INVALID GESTURE ID!");
    return;
  }

  Serial.println("-----------------------");

  Serial.print("GESTURE: G");
  Serial.println(gestureID);

  Serial.print("LANGUAGE FOLDER: ");
  Serial.println(languageFolder);

  Serial.print("TRACK: ");
  Serial.println(gestureID);

  player.playFolder(languageFolder, gestureID);

  Serial.println("AUDIO PLAY COMMAND SENT");
  Serial.println("-----------------------");
}

// ---------------- PACKET PROCESSING ----------------
void processPacket() {
  while (radio.available()) {
    radio.read(&packet, sizeof(packet));

    // Check valid hand ID.
    if (packet.handID != 1 && packet.handID != 2) {
      continue;
    }

    // COMMAND 2: MASTER DISABLED
    if (packet.handID == 1 && packet.command == 2) {
      if (gesturesEnabled) {
        gesturesEnabled = false;
        player.stop();

        Serial.println("MASTER DISABLED");
        Serial.println("BOTH GLOVES LOCKED");
      }

      continue;
    }

    // COMMAND 3: MASTER ENABLED
    if (packet.handID == 1 && packet.command == 3) {
      if (!gesturesEnabled) {
        gesturesEnabled = true;

        Serial.println("MASTER ENABLED");
        Serial.println("BOTH GLOVES ACTIVE");
      }

      continue;
    }

    // COMMAND 4: LANGUAGE FOLDER SELECTION
    if (packet.handID == 2 && packet.command == 4) {
      if (packet.gestureID == 1 || packet.gestureID == 2) {
        languageFolder = packet.gestureID;

        if (languageFolder == 1) {
          Serial.println("LANGUAGE CHANGED: NEPALI");
        } else {
          Serial.println("LANGUAGE CHANGED: ENGLISH");
        }
      }

      continue;
    }

    // Ignore gestures when master is disabled.
    if (!gesturesEnabled) {
      continue;
    }

    // COMMAND 1: NORMAL GESTURE
    if (packet.command == 1) {

      // Left glove must send G01-G06.
      if (packet.handID == 1 &&
          packet.gestureID >= 1 &&
          packet.gestureID <= 6) {

        Serial.println("LEFT HAND GESTURE");
        playGestureAudio(packet.gestureID);
      }

      // Right glove must send G07-G12.
      else if (packet.handID == 2 &&
               packet.gestureID >= 7 &&
               packet.gestureID <= 12) {

        Serial.println("RIGHT HAND GESTURE");
        playGestureAudio(packet.gestureID);
      }
    }
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  Serial.println("============================");
  Serial.println("VOCAL GLOVES RECEIVER");
  Serial.println("============================");

  // Initialize DFPlayer.
  Serial.println("Initializing DFPlayer...");

  dfSerial.begin(9600);
  delay(1000);

  if (!player.begin(dfSerial, true, true)) {
    Serial.println("DFPLAYER NOT DETECTED!");
    Serial.println("Check wiring, power and SD card.");

    while (1) {
      delay(100);
    }
  }

  player.volume(20);
  player.EQ(DFPLAYER_EQ_NORMAL);

  Serial.println("DFPLAYER READY!");

  delay(500);

  // Initialize NRF24.
  Serial.println("Initializing NRF24...");

  if (!radio.begin()) {
    Serial.println("NRF24 NOT FOUND!");

    while (1) {
      delay(100);
    }
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(100);
  radio.setAutoAck(true);
  radio.setRetries(5, 15);

  radio.openReadingPipe(1, address);
  radio.startListening();

  Serial.println("NRF24 READY!");
  Serial.println("MASTER: ENABLED");
  Serial.println("LANGUAGE: NEPALI");
  Serial.println("RECEIVER READY!");
}

// ---------------- LOOP ----------------
void loop() {
  processPacket();
}
