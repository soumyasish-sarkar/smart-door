#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>  // LIBRARY FOR SAVING DATA TO FLASH MEMORY
#include <SPI.h>
#include <MFRC522.h>

const char* ssid = "Venom Insight";
const char* password = "saumyasish";



// FREE PUBLIC MQTT BROKER
const char* mqtt_server = "broker.hivemq.com";

// ====== ADDED: MQTT AUTH CREDENTIALS & MESSAGE TOKEN ======
const char* mqtt_user = "smartdoor";  // <-- set this
const char* mqtt_password = "door";   // <-- set this
// NOTE: token removed as per request - MQTT will rely on broker username/password
// =========================================================

// ================= PIN DEFINITIONS =================
// Relay Pins (Connect to IN1 and IN2 on Relay Module)
const int relay1 = 18;
const int relay2 = 19;
// LED PIN
const int green = 2;
const int red = 16;

//RC522 pins
#define RST_PIN 27
#define SS_PIN 5
#define SCK_PIN 14
#define MISO_PIN 12
#define MOSI_PIN 13

unsigned long ignoreCommandsUntil = 0;



WiFiClient espClient;
PubSubClient client(espClient);
Preferences preferences;  // Create preferences object

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// MQTT Topics
const char* subTopic = "soumyasish/door/operations";  // Subscriber to door operation
const char* subTopic2 = "soumyasish/door/ipcam";      //subscribed to the webcam operation from laptop
const char* pubTopic = "soumyasish/door/logs";

// ====== ADDED: secure publish helper (no token) ======
void securePublish(const char* topic, const char* message) {
  // Previously this wrapped messages with a token. Token auth removed:
  client.publish(topic, message);
}
// ==========================================================

// ===== Pre-saved Allowed IDs (from Block 2) =====
String allowedList[] = {
  "CSB22054",
  "CSB22082",
  "soumyasish"
};
int allowedCount = sizeof(allowedList) / sizeof(allowedList[0]);

// Convert byte array to ASCII
String byteArrayToAscii(byte* buffer, byte bufferSize) {
  String s = "";
  for (byte i = 0; i < bufferSize; i++) {
    char c = (char)buffer[i];
    if (c >= 32 && c <= 126) s += c;
  }
  return s;
}

// Read a single block
String readBlock(byte blockAddr) {
  byte buffer[18];
  byte size = sizeof(buffer);

  MFRC522::StatusCode status;

  // Authenticate
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) return "";

  // Read
  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) return "";

  return byteArrayToAscii(buffer, 16);
}





// ================= STATE TRACKING =================
// We will load this from memory in setup()
String currentDoorState = "";



// ================= MOTOR CONTROL FUNCTION =================
//function to open door
void opendoor() {
  //Rotate Clockwise (IN1 ON, IN2 OFF)
  digitalWrite(relay1, LOW);   // Relay 1 ON
  digitalWrite(relay2, HIGH);  // Relay 2 OFF
  //delay(4000); // Run for 2 seconds

  //blink green LED while opening
  int n = 4;
  while (n) {
    digitalWrite(green, HIGH);
    delay(500);
    digitalWrite(green, LOW);
    delay(500);
    n = n - 1;
  }
}

//function to close door
void closedoor() {
  //Rotate Counter-Clockwise (IN1 OFF, IN2 ON)
  digitalWrite(relay1, HIGH);  // Relay 1 OFF
  digitalWrite(relay2, LOW);   // Relay 2 ON
  //delay(4000); // Run for 2 seconds

  //Blink Red LED while closing
  int n = 4;
  while (n) {
    digitalWrite(red, HIGH);
    delay(500);
    digitalWrite(red, LOW);
    delay(500);
    n = n - 1;
  }
}


void doorstop() {
  //Stop motor
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
}



// ====================== SUBSCRIBER CALLBACK =======================
void callback(char* topic, byte* message, unsigned int length) {
  //Serial.print("Message received: ");
  //Serial.print(topic);

  // ========= Ignore MQTT commands for first 5 seconds =========

  if (millis() < ignoreCommandsUntil) {
    return;
  }


  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }

  // CRITICAL: Remove any hidden whitespace/newlines
  msg.trim();

  // ====== REMOVED: Message-level authorization check ======
  // Token-based incoming message check removed. Broker-level auth (username/password)
  // is used now. Incoming payloads are expected to be plain commands like "open" or "close".
  // ======================================================

  //Serial.println(msg);


  // Logic: If Open, Turn PIN 2 ON and PIN 16 OFF
  if (msg == "open") {
    // CHECK: Is it already open?
    if (currentDoorState == "open") {
      Serial.println("Status: Door already OPENED");
      securePublish(pubTopic, "Status: Door already OPENED");
    } else {
      // It is not open, so let's open it
      if (String(topic) == subTopic) {
        Serial.println("Status: Door OPENING --MQTT");
        securePublish(pubTopic, "Status: Door OPENING --MQTT");
      } else if (String(topic) == subTopic2) {
        Serial.println("Status: Door OPENING --WebCam");
        securePublish(pubTopic, "Status: Door OPENING --WebCam");
      }
      digitalWrite(red, LOW);
      opendoor();
      digitalWrite(green, HIGH);
      doorstop();
      //Update State Variable
      currentDoorState = "open";
      preferences.putString("state", "open");
      Serial.println("Status: Door OPENED");
      securePublish(pubTopic, "Status: Door OPENED");
    }
  }
  // Logic: If Close, Turn PIN 2 OFF and PIN 16 ON
  else if (msg == "close") {
    // CHECK: Is it already closed?
    if (currentDoorState == "close") {
      Serial.println("Status: Door already CLOSED");
      securePublish(pubTopic, "Status: Door already CLOSED");
    } else {
      // It is not closed, so let's close it
      if (String(topic) == subTopic) {
        Serial.println("Status: Door CLOSING --MQTT");
        securePublish(pubTopic, "Status: Door CLOSING --MQTT");
      } else if (String(topic) == subTopic2) {
        Serial.println("Status: Door CLOSING --WebCam");
        securePublish(pubTopic, "Status: Door CLOSING --WebCam");
      }
      digitalWrite(green, LOW);
      closedoor();
      digitalWrite(red, HIGH);
      doorstop();
      //Update State Variable
      currentDoorState = "close";
      preferences.putString("state", "close");
      Serial.println("Status: Door CLOSED");
      securePublish(pubTopic, "Status: Door CLOSED");
    }
  }
}

// ====================== WIFI CONNECT =======================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ====================== MQTT CONNECT =======================
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (millis() < ignoreCommandsUntil) {
      return;
    }
    Serial.print("Attempting MQTT connection... ");

    // Create a random client ID to avoid conflicts on public broker
    String clientId = "ESP32 Door Controller";

    // ====== CHANGED: use username/password for broker authentication ======
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(subTopic);
      Serial.println("Subscribed to: " + String(subTopic));
      client.subscribe(subTopic2);
      Serial.println("Subscribed to: " + String(subTopic2));
      // Publish boot message (uses secure publish)
      securePublish(pubTopic, "ESP32 Connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ====================== SETUP =======================
void setup() {
  Serial.begin(115200);

  // Block MQTT commands for first 5 seconds
  ignoreCommandsUntil = millis() + 5000;

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  delay(200);

  // Pin Setup
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);


  // Initialize Relays, LEDs to OFF
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);


  //Memory load
  // Open a storage namespace called "door_app"
  preferences.begin("door_app", false);

  // Get the last known state. If empty (first time ever), default to "close".
  currentDoorState = preferences.getString("state", "close");

  Serial.print("Status: SYSTEM REBOOTED. Previous State Loaded -> ");
  Serial.println(currentDoorState);

  //Motor always stopped at boot
  doorstop();

  // ========= ENSURE DOOR STARTS CLOSED ONLY ON FIRST-EVER USE (NOT ON CODE UPLOAD) =========
  if (!preferences.isKey("state")) {
    // First time ever only
    currentDoorState = "close";
    preferences.putString("state", "close");
  }


  // Restore LED status based on memory
  if (currentDoorState == "open") {
    digitalWrite(green, HIGH);
    digitalWrite(red, LOW);
  } else {
    digitalWrite(green, LOW);
    digitalWrite(red, HIGH);
  }



  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //Serial.println("RC522 Ready. Tap card...");
}

// ====================== MAIN LOOP =======================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // This must run frequently to listen for messages


  // ================= RFID CHECK =================
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {

    String block2 = readBlock(2);

    if (block2.length() == 0) {
      Serial.println("Error: Failed to read block 2 or authentication error.");
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return;  // do NOT publish any 'Authorized/Unauthorized' message
    }

    bool match = false;
    for (int i = 0; i < allowedCount; i++) {
      if (block2.indexOf(allowedList[i]) >= 0) {
        match = true;
        break;
      }
    }

    if (match) {
      // ====== RFID TOGGLE LOGIC ======
      if (currentDoorState == "open") {

        // CLOSE DOOR
        Serial.printf("Status: Door CLOSING -- %s (RFID)\n", block2.c_str());
        String msg = "Status: Door CLOSING -- " + block2 + " (RFID)";
        securePublish(pubTopic, msg.c_str());
        digitalWrite(green, LOW);
        closedoor();
        digitalWrite(red, HIGH);
        doorstop();

        currentDoorState = "close";
        preferences.putString("state", "close");
        Serial.println("Status: Door CLOSED");
        securePublish(pubTopic, "Status: Door CLOSED");
      } else if (currentDoorState == "close") {

        // OPEN DOOR
        Serial.printf("Status: Door OPENING -- %s (RFID)\n", block2.c_str());
        String msg = "Status: Door OPENING -- " + block2 + " (RFID)";
        securePublish(pubTopic, msg.c_str());
        digitalWrite(red, LOW);
        opendoor();
        digitalWrite(green, HIGH);
        doorstop();

        currentDoorState = "open";
        preferences.putString("state", "open");
        Serial.println("Status: Door OPENED");
        securePublish(pubTopic, "Status: Door OPENED");
      } else {
        Serial.println("Status: INVALID SIGNAL");
        securePublish(pubTopic, "Status: INVALID SIGNAL");
      }
    } else {
      Serial.printf("\nUnauthorized CARD Access: %s\n", block2.c_str());
      String msg2 = "Unauthorized CARD Access: " + block2;
      securePublish(pubTopic, msg2.c_str());
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}
