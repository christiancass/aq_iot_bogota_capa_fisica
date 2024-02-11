//VERY IMPORTANT
//to see colors in terminals add this line at the end of platformio.ini
//monitor_flags = --raw
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

String dId = "12345";
String webhook_pass = "GvmPAWgQIe9M6g5r4qIAsKfZaGJBsL10";
String webhook_endpoint = "http://192.168.0.9:3001/api/getdevicecredentials";
const char* mqtt_server = "192.168.0.9";

//PINS 
#define led 2

//WiFi
const char *wifi_ssid = "CASS";
const char *wifi_password = "749498-I-D-C";

//Functions definitions
bool get_mqtt_credentials();
void check_mqtt_connection();
bool reconnect();
void clear();
void process_sensors();
void send_data_to_broker();

//Global Vars
WiFiClient espclient;
PubSubClient client(espclient);
long lastReconnectAttemp = 0;

DynamicJsonDocument mqtt_data_doc(2048);

void setup()
{

  Serial.begin(921600);
  pinMode(led, OUTPUT);
  clear();

  Serial.println("WiFi Connection in Progress");

  WiFi.begin(wifi_ssid, wifi_password);

  int counter = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;

    if (counter > 10)
    {
      Serial.print("Ups WiFi Connection Failed :( ");
      Serial.println(" -> Restarting...");
      delay(2000);
      ESP.restart();
    }
  }

  //Printing local ip
  Serial.println("WiFi Connection -> SUCCESS :)");
  Serial.print("Local IP -> ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  check_mqtt_connection();
  send_data_to_broker();
  process_sensors();
  delay(5000);
  serializeJsonPretty(mqtt_data_doc, Serial);
}

int prev_temp = 0;
int prev_hum = 0;


void process_sensors(){


  //get temp simulation
  int temp = random(1, 100);
  mqtt_data_doc["variables"][0]["last"]["value"] = temp;
  mqtt_data_doc["variables"][0]["last"]["save"] = 1;

}


//TEMPLATE ⤵

long varsLastSend[20];

void send_data_to_broker(){
  long now = millis();

  for(int i = 0; i < mqtt_data_doc["variables"].size(); i++){
    if (mqtt_data_doc["variables"][i]["variableType"] == "output"){
      continue;
    }
    int freq = mqtt_data_doc["variables"][i]["variableSendFreq"];
    if (now - varsLastSend[i] > freq * 1000){
      varsLastSend[i] = millis();
      String str_root_topic = mqtt_data_doc["topic"];
      String str_variable = mqtt_data_doc["variables"][i]["variable"];
      String topic = str_root_topic + str_variable + "/sdata";
      String toSend = "";
      serializeJson(mqtt_data_doc["variables"][i]["last"], toSend);
      client.publish(topic.c_str(), toSend.c_str());
    }
  }
}


bool reconnect()
{

  if (!get_mqtt_credentials())
  {
    Serial.println("Error getting mqtt credentials :( RESTARTING IN 10 SECONDS");
    Serial.println("RESTARTING IN 10 SECONDS");
    delay(10000);
    ESP.restart();
  }

  //Setting up Mqtt Server
  client.setServer(mqtt_server, 1883);

  Serial.println("Trying MQTT Connection");

  String str_client_id = "device_" + dId + "_" + random(1, 9999);
  const char *username = mqtt_data_doc["username"];
  const char *password = mqtt_data_doc["password"];
  String str_topic = mqtt_data_doc["topic"];

  if (client.connect(str_client_id.c_str(), username, password))
  {
    Serial.print("Mqtt Client Connected :)" );
    delay(2000);

    client.subscribe((str_topic + "+/actdata").c_str());
    return true;  // Agregar esta línea
  }
  else
  {
    Serial.println("Mqtt Client Connection Failed :( ");
  }
}

void check_mqtt_connection()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Ups WiFi Connection Failed :( ");
    Serial.println(" -> Restarting...");
    delay(15000);
    ESP.restart();
  }

  if (!client.connected())
  {

    long now = millis();

    if (now - lastReconnectAttemp > 5000)
    {
      lastReconnectAttemp = millis();
      if (reconnect())
      {
        lastReconnectAttemp = 0;
      }
    }
  }
  else
  {
    client.loop();
  }
}

bool get_mqtt_credentials()
{

  Serial.println("Getting MQTT Credentials from WebHook");
  delay(1000);

  String toSend = "dId=" + dId + "&password=" + webhook_pass;

  HTTPClient http;
  http.begin(webhook_endpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int response_code = http.POST(toSend);

  if (response_code < 0)
  {
    Serial.println("Error Sending Post Request :( ");
    http.end();
    return false;
  }

  if (response_code != 200)
  {
    Serial.println("Error in response :(   e-> ");
    http.end();
    return false;
  }

  if (response_code == 200)
  {
    String responseBody = http.getString();

    Serial.println("Mqtt Credentials Obtained Successfully :) ");

    deserializeJson(mqtt_data_doc, responseBody);
    http.end();
    delay(1000);
  }

  return true;
}

void clear()
{
  Serial.write(27);    // ESC command
  Serial.print("[2J"); // clear screen command
  Serial.write(27);
  Serial.print("[H"); // cursor to home command
}