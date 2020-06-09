#include "Arduino.h"
#include "heltec.h"
#include "WiFi.h"
#include "images.h"
#include <PubSubClient.h>

#define BAND    915E6

String rssi;
String packSize = "--";
String packet;
String lat;
String lng;
String equip;
String counter = "";
const char* mqtt_server = "test.mosquitto.org";
String valor;
const char* fila = "teste-lora-tcs";

bool receiveflag = false;

long lastSendTime = 0;
int interval = 1000;
uint64_t chipid;

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, espClient);

void logo(){
  Heltec.display -> clear();
  Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
  Heltec.display -> display();
}

void WIFISetUp(void) {
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin("Mara","pipocapacoca");
  
  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && count < 10){
    count ++;
    delay(200);
    Heltec.display -> drawString(0, 0, "Connecting...");
    Heltec.display -> display();
  }

  Heltec.display -> clear();
  if(WiFi.status() == WL_CONNECTED){
    Heltec.display -> drawString(0, 0, "Connecting...OK.");
    Heltec.display -> display();
  } else {
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, "Connecting...Failed");
    Heltec.display -> display();
  }
  Heltec.display -> drawString(0, 10, "WIFI Setup done");
  Heltec.display -> display();

  Heltec.display -> clear();
  if (client.connect(fila)) {
    Heltec.display -> drawString(0, 30, "mqtt.. OK");
    Heltec.display -> display();
  } else {
    Heltec.display -> drawString(0, 30, "mqtt.. Failed");
    Heltec.display -> display();
  }
  delay(400);
}

bool resendflag=false;
bool deepsleepflag=false;
void interrupt_GPIO0(){
  delay(10);
  if(digitalRead(0)==0){
    if(digitalRead(LED)==LOW) {
      resendflag=true;
    }
    else
    {
      deepsleepflag=true;
    }     
  }
}

void setup(){
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Enable*/, true /*Serial Enable*/, true /*LoRa use PABOOST*/, BAND /*LoRa RF working band*/);
  
  logo();
  Heltec.display -> clear();
  
  WIFISetUp();
  
  attachInterrupt(0,interrupt_GPIO0,FALLING);
  LoRa.onReceive(onReceive);
  LoRa.receive();
  displaySendReceive();
}

void loop() {
  if(deepsleepflag) {
    LoRa.end();
    LoRa.sleep();
    delay(100);
    pinMode(4,INPUT);
    pinMode(5,INPUT);
    pinMode(14,INPUT);
    pinMode(15,INPUT);
    pinMode(16,INPUT);
    pinMode(17,INPUT);
    pinMode(18,INPUT);
    pinMode(19,INPUT);
    pinMode(26,INPUT);
    pinMode(27,INPUT);
    digitalWrite(Vext,HIGH);
    delay(2);
    esp_deep_sleep_start();
  }
  if(resendflag){
    resendflag=false;
    send();      
    LoRa.receive();
    displaySendReceive();
    client.publish(fila, (char*) valor.c_str());  
  }
  if(receiveflag){
    digitalWrite(25,HIGH);
    displaySendReceive();
    delay(1000);
    receiveflag = false;  
    send();
    LoRa.receive();
    displaySendReceive();
    if (equip.length() > 0)
    client.publish(fila, (char*) valor.c_str());  
  }

  digitalWrite(LED, HIGH);
  digitalWrite(LED, LOW);
}

void send(){
  LoRa.beginPacket();
  LoRa.print(counter);
  LoRa.endPacket();
}
void displaySendReceive()
{
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 10, packet);
  Heltec.display -> drawString(0, 20, "ID   " + equip);
  Heltec.display -> drawString(0, 30, "Lat   " + lat);
  Heltec.display -> drawString(0, 40, "Long   " + lng);
  Heltec.display -> drawString(0, 50, rssi);
  Heltec.display -> display();    
}

void onReceive(int packetSize){
  packet = "";
  packSize = String(packetSize,DEC);

  while (LoRa.available()){
    packet += (char) LoRa.read();
  }

  if (packet.startsWith("m")) {
    equip = packet.substring(0, packet.indexOf(","));
    lat = packet.substring(packet.indexOf(",") + 1, packet.indexOf("/")- 1);
    lng = packet.substring(packet.indexOf("/") + 1);
  } else {
    counter = packet;
  }
  valor = "{\"id\" : \"" + equip +"\",\"position\":{\"lat\":" + lat +  ",\"lng\": " + lng + "}}";
  
  rssi = "RSSI: " + String(LoRa.packetRssi(), DEC);    
  receiveflag = true;    
}
