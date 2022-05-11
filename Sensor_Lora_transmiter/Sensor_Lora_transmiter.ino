#include "heltec.h"
#include "DHTesp.h"
#include <OneWire.h>
#include <DS18B20.h>
     
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6
#define power 20       //antenna power in db set to máx

// Pinos do lora (sensors spi communication)
#define DHTpin 23
#define ONE_WIRE_BUS 19//19

DHTesp dht;
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);         // Pass our oneWire reference to Dallas Temperature if using mutiples sensors. 
DeviceAddress insideThermometer;  // arrays to hold device address

String outgoing;               // outgoing message
byte localAddress = 0xBB;      // address of this device
byte destination = 0xFD;       // destination to send to
byte msgCount = 0;             // count of outgoing messages
long lastSendTime = 0;         // last send time
int interval = 10000;//720000;          //720000 interval between sends

void setup()
{
  
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST); //20dB output must via PABOOST
  displayBegin();                    //start oled display
  dht.setup(DHTpin, DHTesp::DHT11);  //air temp and humidity sensor
  sensor.begin();                    // water temp sensor
 
}

void loop()
{
  //send messages every time interval
  if (millis() - lastSendTime > interval){
    String message = readSensors();
    sendMessage(message);
    displayMeasures(message);
    lastSendTime = millis();
  }
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}



String assembleMessage(float Air_temp,float Water_temp, float humidity){
  /*Assembles message in Json format, library in python*/
  String message = "{" ; 
  message+= '"';
  message+= "Air_temp";
  message+= '"';
  message+= ':';
  message+= String(Air_temp).c_str();
  message+= ';';
  message+= '"';
  message+= "Water_temp";
  message+= '"';
  message+= ':';
  message+= String(Water_temp).c_str();
  message+= ';';
  message+= '"';
  message+= "humidity";
  message+= '"';
  message+= ':';
  message+= String(humidity).c_str();
  message+= '}';
  return message;
}
void sendMessage(String message)
{
  Serial.println("send message :");
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(message.length());        // add payload length
  LoRa.print(message);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize)
{
  /*function to receive messages from the gateway, messages will be used to request measurements 
   or change the measurement cadence*/
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
  if(incoming.toInt()==1){ //if the message is 1 then take a measurement and send it to the gateway
     sendMessage(readSensors());
     Serial.println("Medida solicitada enviada ");
     
  }else{//if the message is a number other than 1, then change the measurement interval to the number received in seconds
     interval=1000*incoming.toInt();
  }
  
}
String readSensors(){
  /*reads both sensors and assembles the message using the function "assembleMessage()" */
    float temperature = dht.getTemperature();
    float humidity = dht.getHumidity();
    sensor.requestTemperatures();
    while (!sensor.isConversionComplete());  // wait until sensor is ready
    float water_temperature = sensor.getTempC();
    displayMeasures(assembleMessage(temperature,water_temperature,humidity));
    return assembleMessage(temperature,water_temperature,humidity);
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
void displayMeasures(String message){
    /*shows on the screen the message sent, with sensor readings*/
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawStringMaxWidth(10 , 0 , 128, message);
    Heltec.display->display();
    Serial.println("Sending " + message);
}
