/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-mesh-esp32-esp8266-painlessmesh/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <json.h>
#include <EEPROM.h>

// MESH Details
#define MESH_PREFIX "AlphaMesh"      // name for MESH
#define MESH_PASSWORD "MESHpassword" // password for MESH
#define WIFI_CHANNEL 6               // WiFi Channel
#define MESH_PORT 5555               // default port
#define DHTPIN 5                     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11                // DHT 11

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

int value;
int address = 0;
const int VhumMax = 405;
const int VhumMin = 767;
const int AnalogPin = A0;
// long output;

void readEEPROM()
{
  // read data from eeprom

  value = EEPROM.read(address);
  Serial.print("Read Id = ");
  Serial.print(value, DEC);
}

int soilSensor(int VhumMin, int VhumMax)
{
  // put your main code here, to run repeatedly:
  int sensorValue = analogRead(A0);
  int val = map(sensorValue, VhumMin, VhumMax, 0, 100);
  if (val > 100)
  {
    val = 100;
  }
  else if (val < 0)
  {
    val = 0;
  };
  return val;
  // delay(100);
}

void receivedCallback(uint32_t from, String &msg);
Scheduler userScheduler;
painlessMesh mesh;
void myLoggingTaskCB();
size_t logServerId = 0;
int counter = 0;
// Send message to the logServer every 10 seconds
Task myLoggingTask(1 * TASK_HOUR, TASK_FOREVER, &myLoggingTaskCB, &userScheduler);
void myLoggingTaskCB() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilHumidity = soilSensor(VhumMin, VhumMax);
  DynamicJsonDocument msg(1024);
  DynamicJsonDocument test(1024);
  // JsonObject& msg = jsonBuffer.createObject();
  // msg["nodename"] = "A2";  //change for identify for the node that send data mcu-t1 to mcu-t3
  msg["idNode"] = value;
  msg["airTemp"] = serialized(String(t,2));
  msg["airHum"] = serialized(String(h,2));
  msg["soilHum"] = soilHumidity;
  msg["msgId"] = counter;
  test["lat"] = 12.345;
  test["long"] = -34.789;
  msg["gps"] = test;
  String str;
  serializeJson(msg, str);
  if (logServerId == 0){
     // If we don't know the logServer yet
    mesh.sendBroadcast(str);
    counter++;
  }
  else
  {
    mesh.sendSingle(logServerId, str);
    counter++;
  }
  // log to serial
  serializeJson(msg, Serial);
  Serial.printf("\n"); 
  }

void setup()
{
  Serial.begin(115200);
  Serial.println("Begin DHT22 Mesh Network test!");
  dht.begin();
  EEPROM.begin(512);
  readEEPROM();
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION); // set before init() so that you can see startup messages
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, WIFI_CHANNEL, PHY_MODE_11G);
  mesh.onReceive(&receivedCallback);
  // Add the task to the mesh scheduler
  userScheduler.addTask(myLoggingTask);
  myLoggingTask.enable();
}

void loop()
{
  // put your main code here, to run repeatedly:
  mesh.update();
}
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("logClient: Received from %u msg=%s\n", from, msg.c_str());
  // Saving logServer
  DynamicJsonDocument root(1024);
  deserializeJson(root, msg);
  if (root.containsKey("topic"))
  {
    if (String("logServer").equals(root["topic"].as<String>()))
    {
      // check for on: true or false
      logServerId = root["nodeId"];
      Serial.printf("logServer detected!!!\n");
    }
    Serial.printf("Handled from %u msg=%s\n", from, msg.c_str());
  }
}