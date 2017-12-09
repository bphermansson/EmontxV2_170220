#include <Arduino.h>

// This version is in use as of 20170302

// Uncomment this for serial output:
//#define DEBUG

// https://github.com/openenergymonitor/emontx3/blob/master/firmware/src/src.ino
#define RF69_COMPAT 1 // set to 1 to use RFM69CW

#include <JeeLib.h> //https://github.com/jcw/jeelib - Tested with JeeLib 3/11/14
#include <EmonLib.h>   // make sure V12 (latest) is used if using RFM69CW

// BMP180
#include <SFE_BMP180.h>
#include <Wire.h>
#define ALTITUDE 54.0 // Altitude of Såtenäs (my location) in meters
//Create an instance of the object
SFE_BMP180 pressure;

// HTU21D
#include "SparkFunHTU21D.h"
//Create an instance of the object
HTU21D htu21d;

//EnergyMonitor emon1;                   // Create an instance

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption

//-----------------------RFM12B / RFM69CW SETTINGS----------------------------------------------------------------------------------------------------
byte RF_freq=RF12_433MHZ;                                           // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID = 27;                                                    // emonTx RFM12B node ID
int networkGroup = 210;
boolean RF_STATUS = 1;                                              // Enable RF

// Note: Please update emonhub configuration guide on OEM wide packet structure change:
// https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md

// Number of milliseconds to sleep, in range 0..65535.
//const byte TIME_BETWEEN_READINGS = 65535;            //Time between readings

// Emontx payload
// Datacodes: https://community.openenergymonitor.org/t/emonhub-receiving-weird-data-where-is-it-coming-from/839/5
typedef struct {
  int battery;
  int pressure;
  int temp;
  int uvraw;
  int hum;
  int solarv;
  int light;
  //int power=0;
} PayloadTX;     // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;

unsigned long start=0;
const char compile_date[] = __DATE__ " " __TIME__;

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
    delay(100);
    Serial.println(F("PH emonTX v2"));
    Serial.println(compile_date);
    Serial.println(F("OpenEnergyMonitor.org"));
    Serial.print(F("Node: "));
    Serial.print(nodeID);
    Serial.print(" Freq: ");
    if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
    if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
    if (RF_freq == RF12_915MHZ) Serial.print("915Mhz");
    Serial.print(F(" Network: "));
    Serial.println(networkGroup);
    delay(10);
  #endif

  rf12_initialize(nodeID, RF_freq, networkGroup);  // initialize RFM12B
  rf12_control(0xC040);                            // set low-battery level to 2.2V i.s.o. 3.1V
  delay(100);
  rf12_sendWait(2);

  // Initialize the sensor (it is important to get calibration values stored on the device).
  if (pressure.begin()) {
    #ifdef DEBUG
      Serial.println("BMP180 init success");
    #endif
  }
  else {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    #ifdef DEBUG
      Serial.println("BMP180 init error");
    #endif
    while(1); // Pause forever.
  }

    // Init humidity sensor
    #ifdef DEBUG
      Serial.println(F("HTU21 init"));
    #endif
    htu21d.begin();
    /*
    #ifdef DEBUG
      Serial.println(F("Current sensor init"));
    #endif
    emon1.current(2, 90.9);             // Current: input pin, calibration.
    */
    #ifdef DEBUG
      Serial.println (F("Setup done"));
    #endif
   //Serial.end();
}

void loop() {

  // Read battery voltage
  emontx.battery = readVcc();
  #ifdef DEBUG
    Serial.print("Battery: ");
    Serial.println(emontx.battery);
  #endif

  // Read solar cell voltage
  // Read A0 and calculate value
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (emontx.battery / 1023.0);
  emontx.solarv = int(voltage);

  #ifdef DEBUG
    Serial.print("Solar volt: ");
    Serial.println(emontx.solarv);
  #endif

  // Read BMP180 Temp/pressure sensor
  readbmp();
  #ifdef DEBUG
    Serial.print("Pressure: ");
    Serial.println(emontx.pressure);
  #endif

  // UV sensor
  emontx.uvraw = analogRead(A1);
  #ifdef DEBUG
    Serial.print("UV: ");
    Serial.println(emontx.uvraw);
  #endif

  delay(100);

  // Light sensor
  emontx.light = analogRead(A3);
  #ifdef DEBUG
    Serial.print("Light: ");
    Serial.println(emontx.light);
  #endif

  // Humidity & Temp
  float fhum = htu21d.readHumidity();
  float ftemp = htu21d.readTemperature();
  emontx.hum = fhum*1000;
  emontx.temp = ftemp*1000;
  #ifdef DEBUG
    Serial.println("Hum & temp");
    Serial.println(emontx.hum);
    Serial.println(emontx.temp);
  #endif

  // Power
  //double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  //emontx.power = Irms*230.0;
  //Serial.print(Irms*230.0);         // Apparent power
  //Serial.print(" ");
  //Serial.println(Irms); // Irms
  delay(100);
  #ifdef DEBUG
    Serial.println(millis());
    Serial.print("Battery: ");
    Serial.println(emontx.battery);
    Serial.print("Solarv: ");
    Serial.println(emontx.solarv);
    delay(100);
    //Serial.println(emontx.BMPtemp);
    Serial.print("Uvraw: ");
    Serial.println(emontx.uvraw);
    Serial.print("Pressure: ");
    Serial.println(emontx.pressure);
    Serial.print("Humidity: ");
    Serial.print(emontx.hum);
    delay(100);
    //Serial.print("Power: ");
    //Serial.println(emontx.power);
    Serial.println(F("Send to emonbase"));
  #endif
  if (emontx.battery<2500){
    Sleepy::powerDown();
  }
  send_rf_data();                                                           // *SEND RF DATA* - see emontx_lib
  delay(100);

  /* Sleep
   From above:
   const byte TIME_BETWEEN_READINGS = 70;            //Time between readings
   unsigned long start=0;

  */
  //unsigned long runtime = millis() - start;
  //unsigned long sleeptime = (TIME_BETWEEN_READINGS*1000) - runtime - 100;  // byte Sleepy::loseSomeTime (word msecs)
  //Sleepy::loseSomeTime(sleeptime-500);                                    // sleep or delay in milliseconds

  // 4000000 = 66min. 600000=10min
  #ifdef DEBUG
    unsigned long ms = millis()/1000;
    Serial.println(ms);
  #endif

  // http://jeelabs.org/pub/docs/jeelib/classSleepy.html
  // Number of milliseconds to sleep, in range 0..65535.

 for (int i=0; i <= 5; i++){
    Sleepy::loseSomeTime(65535);   // sleep or delay in milliseconds
 }
  // Sleep some more
  //Sleepy::loseSomeTime(65535);
  //Sleepy::loseSomeTime(65535);

}

//-------------------------------------------------------------------------------------------------------------------------------------------
void send_rf_data()
{
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &emontx, sizeof emontx);   //send data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);   //if powered by battery then put the RF module into sleep inbetween readings
}
