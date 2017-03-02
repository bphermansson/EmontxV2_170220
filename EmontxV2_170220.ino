// This version is in use as of 20170302

// https://github.com/openenergymonitor/emontx3/blob/master/firmware/src/src.ino
#define RF69_COMPAT 1 // set to 1 to use RFM69CW 

#include <JeeLib.h> //https://github.com/jcw/jeelib - Tested with JeeLib 3/11/14
#include <EmonLib.h>   // make sure V12 (latest) is used if using RFM69CW

// BMP180
#include <SFE_BMP180.h>
#include <Wire.h>
#define ALTITUDE 54.0 // Altitude of Såtenäs (my location) in meters
SFE_BMP180 pressure;

// HTU21D
#include "SparkFunHTU21D.h"
//Create an instance of the object
HTU21D htu21d;

EnergyMonitor emon1;                   // Create an instance

ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption

//-----------------------RFM12B / RFM69CW SETTINGS----------------------------------------------------------------------------------------------------
byte RF_freq=RF12_433MHZ;                                           // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID = 27;                                                    // emonTx RFM12B node ID
int networkGroup = 210;
boolean RF_STATUS = 1;                                              // Enable RF

// Note: Please update emonhub configuration guide on OEM wide packet structure change:
// https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md

const byte TIME_BETWEEN_READINGS = 70;            //Time between readings

// Emontx payload
// Datacodes: https://community.openenergymonitor.org/t/emonhub-receiving-weird-data-where-is-it-coming-from/839/5
typedef struct {
  int battery;
  double pressure;
  double BMPtemp;
  int uvraw;
  int hum;
  double solarv;
  double power;
} PayloadTX;     // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;

unsigned long start=0;

void setup() {
  //Serial.begin(115200);
  //Serial.println(F("PH emonTX v2")); 

  delay(10);
    
  rf12_initialize(nodeID, RF_freq, networkGroup);                          // initialize RFM12B
  rf12_control(0xC040);                                                 // set low-battery level to 2.2V i.s.o. 3.1V
  delay(100);
  rf12_sendWait(2);

  // Initialize the sensor (it is important to get calibration values stored on the device).
  if (pressure.begin()) {
    //Serial.println("BMP180 init success");
  } 
  else {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    //Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }

    // Init humidity sensor
    #ifdef DEBUG
      //Serial.println(F("HTU21 init"));
    #endif  
    htu21d.begin();

    emon1.current(2, 90.9);             // Current: input pin, calibration.

    //Serial.println (F("Setup done"));
   //Serial.end();
}

void loop() {

  // Read battery voltage
  emontx.battery = readVcc();
  
  // Read solar cell voltage
  // Read A0 and calculate value
  //Serial.println(analogRead(A0));
  //Serial.println(emontx.battery);
  
  emontx.solarv = (analogRead(A0) * (emontx.battery/1000)) / 1024.0;
  
  // Read BMP180 Temp/pressure sensor
  //Serial.println("Read temp/pres");
  readbmp();

  // UV sensor
  emontx.uvraw = analogRead(A1);

  // Humidity
  htu21();

  // Power
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  emontx.power = Irms*230.0;
  //Serial.print(Irms*230.0);         // Apparent power
  //Serial.print(" ");
  //Serial.println(Irms); // Irms
  
  //Serial.println(F("Send to emonbase"));
  send_rf_data();                                                           // *SEND RF DATA* - see emontx_lib


  // Sleep
  unsigned long runtime = millis() - start;
  unsigned long sleeptime = (TIME_BETWEEN_READINGS*1000) - runtime - 100;  // byte Sleepy::loseSomeTime (word msecs)  
  Sleepy::loseSomeTime(sleeptime-500);                                    // sleep or delay in milliseconds
                                                                            // Number of milliseconds to sleep, in range 0..65535.
  // Sleep some more
  Sleepy::loseSomeTime(sleeptime-500);
  Sleepy::loseSomeTime(sleeptime-500);

}

//-------------------------------------------------------------------------------------------------------------------------------------------
void send_rf_data()
{
  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &emontx, sizeof emontx);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);                             //if powred by battery then put the RF module into sleep inbetween readings
}
