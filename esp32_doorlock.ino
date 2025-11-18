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

// ================= PIN DEFINITIONS =================
// Relay Pins (Connect to IN1 and IN2 on Relay Module)
const int relay1 = 18; 
const int relay2 = 19;
// LED PIN
const int green = 2;
const int red = 16;


// ================= STATE TRACKING =================
// We start as "unknown" so the first command always works to sync the system
String currentDoorState = "unknown";



//blink led
void blinkled(){
  int n = 4;
  while(n){
    digitalWrite(17, HIGH);
    delay(500);
    digitalWrite(17, LOW);
    delay(500);
    n=n-1;
  }  
}


// ================= MOTOR CONTROL FUNCTION =================
//function to open door
void opendoor(){
  //Rotate Clockwise (IN1 ON, IN2 OFF)
  digitalWrite(relay1, LOW);  // Relay 1 ON
  digitalWrite(relay2, HIGH); // Relay 2 OFF
  //delay(4000); // Run for 2 seconds

  int n = 4;
  while(n){
    digitalWrite(green, HIGH);
    delay(500);
    digitalWrite(green, LOW);
    delay(500);
    n=n-1;
  }
}

//function to close door
void closedoor(){
  //Rotate Counter-Clockwise (IN1 OFF, IN2 ON)
  digitalWrite(relay1, HIGH); // Relay 1 OFF
  digitalWrite(relay2, LOW);  // Relay 2 ON
  //delay(4000); // Run for 2 seconds
  int n = 4;
  while(n){
    digitalWrite(red, HIGH);
    delay(500);
    digitalWrite(red, LOW);
    delay(500);
    n=n-1;
  }
}


void doorstop(){
  //Stop motor
  digitalWrite(relay1, HIGH); 
  digitalWrite(relay2, HIGH); 
}



// ====================== SUBSCRIBER CALLBACK =======================
void callback(char* topic, byte* message, unsigned int length) {
  //Serial.print("Message received: ");
  //Serial.print(topic);


  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  
  // CRITICAL: Remove any hidden whitespace/newlines
  msg.trim(); 
  
  //Serial.println(msg);

  // Logic: If Open, Turn PIN 2 ON and PIN 16 OFF
  if (msg == "open"){
    // CHECK: Is it already open?
    if (currentDoorState == "open") {
      Serial.println("Status: Door already OPENED");
    } 
    else {
      // It is not open, so let's open it
      Serial.println("Status: Door OPENING");
      digitalWrite(red, LOW);
      opendoor();
      digitalWrite(green, HIGH);
      doorstop();
      //Update State Variable
      currentDoorState = "open";
      Serial.println("Status: Door OPENED");
    }
  }
  // Logic: If Close, Turn PIN 2 OFF and PIN 16 ON
  else if (msg == "close") {
    // CHECK: Is it already closed?
    if (currentDoorState == "close") {
      Serial.println("Status: Door already CLOSED");
    } 
    else {
      // It is not closed, so let's close it
      Serial.println("Status: Door CLOSING");
      digitalWrite(green, LOW);
      closedoor();
      digitalWrite(red, HIGH);
      doorstop();
      //Update State Variable
      currentDoorState = "close";
      Serial.println("Status: Door CLOSED");
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
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(17, OUTPUT);    //TESTING


  // Initialize Relays, LEDs to OFF
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(green, LOW);
  digitalWrite(red, LOW);
  digitalWrite(17,LOW);   //TESTING

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
