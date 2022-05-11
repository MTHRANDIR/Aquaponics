#include "heltec.h" 
#include <WiFi.h>            
#include <PubSubClient.h>    
#include <stdio.h>
#include <string.h>

#define BAND 915E6  //you can set band here directly,e.g. 868E6,915E6
#define power 20       //antenna power in db set to máx
#define Node_ID 10

// auxiliary variables****************************************************************

String rssi = "RSSI --";
String packSize = "--";
String packet ;
String outgoing;              // outgoing message

byte localAddress = 0xFD;     // address of this device
byte destination = 0xBB;      // destination to send to
byte msgCount = 0;            // count of outgoing messages

const char ssid[] = "VIVOFIBRA-02C8";  //  your network SSID (name)
const char pass[] = "789387DDF6";       // your network password

WiFiClient espClient; 

PubSubClient MQTT(espClient);        // Instantiates the MQTT Client by passing the object espClient

const char* BROKER_MQTT = "192.168.15.15";     // broker MQTT Local IP
int BROKER_PORT = 1883;                        // default port used by the Broker MQTT  


void keepConnected();  //Garante que as conexoes com WiFi e MQTT Broker se mantenham ativas
void connectWiFi();     //Establishes connection with WiFi
void connectMQTT();     //Establishes connection with MQTT broker
void mqttCallback(char* topic, byte* payload, unsigned int length);
void LoRaData();
void sendMessage();
void onReceive(int packetSize);
bool displayBegin();

void setup() { 
  
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST); //20dB output must via PABOOST
  displayBegin();
  WiFi.disconnect();
  WiFi.mode(WIFI_MODE_STA);
  connectWiFi();
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
  MQTT.setCallback(mqttCallback);  
  connectMQTT();
  LoRa.receive();
}

void loop() {
  onReceive(LoRa.parsePacket());// checks if it received a message from the sensor, if so send it to the Broker
  delay(10);
  MQTT.loop();// checks if it received any command from the server, if yes, forward it to the sensor node
}


void LoRaData(String message,int packetSize){
  packSize = String(packetSize,DEC);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 15 , "Received "+ packSize + " bytes");
  Heltec.display->drawStringMaxWidth(0 , 26 , 128, message);
  Heltec.display->drawString(0, 0, rssi);  
  Heltec.display->display();
  keepConnected(); 
  MQTT.publish("greenhouse_data", message.c_str());
  Serial.println(message);
}

void connectWiFi() {
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 15 , "Connecting to :"+ String(ssid));
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() { 
    while (!MQTT.connected()) {
        Serial.print("Conectando ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(String(Node_ID).c_str())) {
            Serial.println("Conectado ao Broker com sucesso!");
            //digitalWrite(LED_PIN2, HIGH);
            
        } 
        else {
            Serial.println("Noo foi possivel se conectar ao broker.");
            Serial.println("Nova tentatica de conexao em 10s");
            delay(5000);
            
        }
    }
    MQTT.subscribe("control");

}
void keepConnected() {
    /*
    if there is no connection to WiFi, the connection is redone, the same with MQTT
    */
    if((WiFi.status() != WL_CONNECTED)){
      connectWiFi();
    }
    if (!MQTT.connected()) {
      connectMQTT();
    }
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  /*
  mqtt callback, is used to receive external commands and forward them to the end device
  that will make the measurements
  */
  
  Serial.println("mqttCallback");
  char* cleanPayload = (char*)malloc(length+1); 
  String message;
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
  String msg = String(cleanPayload);
  free(cleanPayload);
  String targetStr = String(topic);
  Serial.println("topico :"+String(topic));
  Serial.println("mensagem :"+msg);
  if(targetStr=="control")
  {
      sendMessage(msg.c_str());
  }
  else if(targetStr=="read")
  {
      sendMessage("1");
  }
  
  
}
void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it                          
  msgCount++;                           // increment message ID
}
bool displayBegin()
{
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  delay(1500);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");
  Heltec.display->display();
  
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
 
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  LoRaData(incoming,packetSize);

}
