#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_SHT31.h>
#include "PMS.h"
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include "SensorKalman.h"
#include <ArduinoJson.h>

TinyGPS gps;
SoftwareSerial ss(13, 12); //conectar pin 13 a Tx y pin 12 a RX
SoftwareSerial trasmision(2, 4); //conectar pin 2 a Tx1 y pin 12 a RX1

PMS pms(Serial1);
PMS::DATA data;
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

SensorKalman fpms10;
SensorKalman fpms1;
SensorKalman fpms25;
SensorKalman ftemp;
SensorKalman fhumd;
SensorKalman fpress;

double filterpms10,filterpms1,filterpms25,filtertemp,filterhumd,filterpress;
uint32_t timer=0;
double dt=0;

boolean estado=true;
bool newData;
unsigned long chars;
unsigned short sentences, failed;


float presion,temperatura,humedad, flat, flon, pm1,pm25,pm10;
unsigned long age;

void setup(void) 
{
  Serial.begin(115200);  
  Serial1.begin(9600, SERIAL_8N1, 16, 17);  // RX, TX
  delay(4000);
  ss.begin(9600);
  trasmision.begin(9600);
  Serial.print("Simple TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println("by Mikal Hart");
  Serial.println();
  if(!bmp.begin())
  {
    Serial.print("Sensor BMP180 no detectado");
    while(1);
  }
  Serial.println("Sensor BMP180 encontrado!");

  if (!sht31.begin(0x44)) {
    Serial.println("No se pudo encontrar el sensor SHT31");
    while (1);
  }
  Serial.println("Sensor SHT31 encontrado!");
  
  setSensores();
  timer=micros();
}

void loop() 
{
  
  gpsNeo6m();
  sensorbmp180();
  sensorsht31();
  sensorpms5003();
  
  while(estado)
  {
    sensorpms5003();
  }
  filtroSensor();
  estado=true;
  viewData();
}

void viewData()
{
  StaticJsonDocument<200> jsonDocument;
  char key1[] = "dato1";
  char key2[] = "dato2";
  char key3[] = "dato3";
  char key4[] = "dato4";
  char key5[] = "dato5";
  char key6[] = "dato6";
  char key7[] = "dato7";
  char key8[] = "dato8";
  jsonDocument[key1] = filterpms1;
  jsonDocument[key2] = filterpms25;
  jsonDocument[key3] = filterpms10;
  jsonDocument[key4] = filtertemp;
  jsonDocument[key5] = filterhumd;
  jsonDocument[key6] = filterpress;
  jsonDocument[key7] = flat;
  jsonDocument[key8] = flon;

// Serializa el objeto JSON
  char buffer[256]; // Ajusta el tamaño según sea necesario
  size_t n = serializeJson(jsonDocument, buffer);
  
  // Envía los datos en formato JSON
  trasmision.write(buffer, n);
  trasmision.println(); // Agrega un salto de línea al final de cada conjunto de datos
}

void setSensores()
{
  gpsNeo6m();
  sensorbmp180();
  sensorsht31();
  sensorpms5003();
  while(estado)
  {
    sensorpms5003();
  }

  fpms1.setDistance(pm1);
  fpms25.setDistance(pm25);
  fpms10.setDistance(pm10);
  ftemp.setDistance(temperatura);
  fhumd.setDistance(humedad);
  fpress.setDistance(presion);
}

void filtroSensor()
{
  dt=(double)(micros()-timer)/1000000;
  timer = micros();
  filterpms1=fpms1.getDistance(pm1,dt);
  filterpms25=fpms25.getDistance(pm25,dt);
  filterpms10=fpms10.getDistance(pm10,dt);
  filtertemp=ftemp.getDistance(temperatura,dt);
  filterhumd=fhumd.getDistance(humedad,dt);
  filterpress=fpress.getDistance(presion,dt);
}

void sensorsht31()
{
  humedad = sht31.readHumidity();
}

void sensorbmp180()
{
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure)
  {  
    presion=event.pressure;
    bmp.getTemperature(&temperatura);
  }
}


void gpsNeo6m()
{
  newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    gps.f_get_position(&flat, &flon, &age);
    /*Serial.print("LAT=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" LON=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);*/
  }
  
  /*gps.stats(&chars, &sentences, &failed);
  Serial.print(" CHARS=");
  Serial.print(chars);
  Serial.print(" SENTENCES=");
  Serial.print(sentences);
  Serial.print(" CSUM ERR=");
  Serial.println(failed);
  if (chars == 0)
    Serial.println("** No characters received from GPS: check wiring **");*/
}

void sensorpms5003()
{

  if (pms.read(data))
  {
    pm1=data.PM_AE_UG_1_0;
    pm25=data.PM_AE_UG_2_5;
    pm10=data.PM_AE_UG_10_0;
    estado=false;
  }
}