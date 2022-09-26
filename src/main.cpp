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

// MESH Details
#define MESH_PREFIX "RNTMESH"        // name for your MESH
#define MESH_PASSWORD "MESHpassword" // password for your MESH
#define MESH_PORT 5555               // default port
#define DHTPIN 5                     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11                // DHT 11

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

// Number for this node
int nodeNumber = 1;

// String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// Init dht
void initdht()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
}

// User stub
void sendMessage();   // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

// Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);

String getReadings()
{
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = dht.readTemperature();
  jsonReadings["hum"] = dht.readHumidity();
  readings = JSON.stringify(jsonReadings);
  return readings;
}

void sendMessage()
{
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
  Serial.println(msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());
  // int node = myObject["node"];
  // double temp = myObject["temp"];
  // double hum = myObject["hum"];
  // Serial.print("Node: ");
  // Serial.println(node);
  // Serial.print("Temperature: ");
  // Serial.print(temp);
  // Serial.println(" C");
  // Serial.print("Humidity: ");
  // Serial.print(hum);
  // Serial.println(" %");
}

void newConnectionCallback(uint32_t nodeId)
{
  // Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  // Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  initdht();

  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop()
{
  // it will run the user scheduler as well
  mesh.update();
}
