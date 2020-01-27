/*
  Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <M5StickC.h>

const int speaker_pin = 26;
int freq = 50;
int ledChannel = 0;
int resolution = 10;
int startup_music[4] = {500, 400, 300, 500};

unsigned long startMillis;
unsigned long currentMillis;

// Publish messages every 10 seconds
const unsigned long publish_interval = 10000;

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");
  M5.Lcd.println("Attempting\nto connect");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  Serial.println("AWS IoT Connected!");
  M5.Lcd.println("AWS IoT\nConnected!");

  playMusic(startup_music, sizeof(startup_music), 250);
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(0);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // very ghetto but allows the string to be parsed as JSON
  for(int i = 0; i < payload.length(); i++) {
    if(payload[i] == '"' || payload[i] == '\\') {
      payload.remove(i, 1);
    }
  }

  const char* json = payload.c_str();

  StaticJsonDocument<500> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("Error parsing json string: (");
    Serial.println(payload);
    Serial.println(")");
    Serial.println("Error:");
    Serial.println(error.c_str());
    return;
  }

  int noteDelay = doc["delay"];
  JsonArray notes = doc["notes"];

  int notesArray[notes.size()] = {};
  int noteCount = 0;
  for(JsonVariant v : notes) {
    notesArray[noteCount] = v.as<int>();
    noteCount++;
  }

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextFont(1);
  M5.Lcd.println(payload);

  playMusic(notesArray, notes.size(), noteDelay);
}

void playMusic(int music_data[], int music_length, int noteDelay) {
  for(int i = 0; i < music_length; i++) {
    ledcWriteTone(ledChannel, music_data[i]);
    delay(noteDelay);
  }

  delay(noteDelay);
  ledcWriteTone(ledChannel, 0);
}

void setup() {
  //initial start time
  startMillis = millis();

  M5.begin();

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.setTextSize(2);

  // setup speaker HAT
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(speaker_pin, ledChannel);

  Serial.begin(9600);
  connectAWS();
}

void loop() {
  currentMillis = millis();

  if (currentMillis - startMillis >= publish_interval)
  {
    publishMessage();
    startMillis = currentMillis;
  }

  client.loop();
}
