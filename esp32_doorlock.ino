#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Venom Insight";
const char* password = "saumyasish";

// FREE PUBLIC MQTT BROKER
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT Topics
const char* subTopic = "soumyasish/door";      // Subscriber topic



// ====================== SUBSCRIBER CALLBACK =======================
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message received: ");
  //Serial.print(topic);


  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  
  // CRITICAL: Remove any hidden whitespace/newlines
  msg.trim(); 
  
  Serial.println(msg);

  // Logic: If Open, Turn PIN 2 ON and PIN 16 OFF
  if (msg == "open") {
    Serial.println("Status: Door OPEN");
    digitalWrite(2, HIGH);
    digitalWrite(16, LOW);
  }
  // Logic: If Close, Turn PIN 2 OFF and PIN 16 ON
  else if (msg == "close") {
    Serial.println("Status: Door CLOSED");
    digitalWrite(2, LOW);
    digitalWrite(16, HIGH);
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
    Serial.print("Attempting MQTT connection... ");
    
    // Create a random client ID to avoid conflicts on public broker
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(subTopic);
      Serial.println("Subscribed to: " + String(subTopic));
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
  
  // Pin Setup
  pinMode(ledOpen, OUTPUT);
  pinMode(ledClose, OUTPUT);
  
  // Initialize LEDs to OFF
  digitalWrite(ledOpen, LOW);
  digitalWrite(ledClose, LOW);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ====================== MAIN LOOP =======================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // This must run frequently to listen for messages
  
  // DO NOT ADD LONG DELAYS HERE
}