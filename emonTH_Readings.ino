#include <Adafruit_Sensor.h>

#include <OneWire.h>

/* Sketch to use an emonTH sensor for temperature and humidity readings. 
 * This uses a DHT22 temperature/humidity sensor and a DS18B20 temperature sensor.
   
 - DHT22 Sensor Library  https://github.com/adafruit/DHT-sensor-library - be sure to rename the sketch folder to remove the '-'
 - OneWire library      http://www.pjrc.com/teensy/td_libs_OneWire.html
 - DallasTemperature     http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip  
 
   Written by tristandell. Public domain.*/

// ****Install libraries****
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <JeeLib.h>
#include <avr/sleep.h> //Avr libs are used to power and sleep specific pins.
#include <avr/power.h>

/*** DEBUG CONFIG?
Set this to 1 if initially debugging the code. Otherwise set to 0 and the code will be less computationally intensive as debig steps will be skipped.***/
boolean debug = 1;

// ****Define some constants for the sensors****
const int DS18B20_PWR =       5;
const int DHT22_PWR =         6;
const int LED =               9;
const int BATT_ADC =          1;
const int tempPrecision =     11; //9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resolution of 0.5C, 0.25C, 0.125C and 0.0625C.

boolean DHT22_status;
boolean DS18B20_status;
boolean DHT22_slowstart;          //Indicates whether testing showed DHT takes longer than 2 seconds to respond after power on.
int tick = 0;

#define DHTPIN          18    //Location of DHT pin
#define ONE_WIRE_BUS    19    //This was 17 on early versions of emonTH (<v1.5)
#define DHTTYPE         DHT22

OneWire oneWire(ONE_WIRE_BUS); //Setup oneWire to communicate with any devices connected via the oneWire bus.
DallasTemperature sensors(&oneWire); //Indicates that the sensors variable takes a reference to the actual oneWire variable, not it's absolute value.

int numSensors; //Variable for number of sensors

//Initialise addresses of sensors (max 8), 8 bytes per address
byte allAddress [8][8];

DHT dht(DHTPIN, DHTTYPE);

// ****Define some constants for RF communications****

//Data is sent after [WDT_max_number] pulses, each lasting 80ms. 690x80=55.2secs. This should be roughly 5s shorter than the record interval on the logger.
//Watchdog timer (WDT) is used to wake device from low power mode (https://jeelabs.org/pub/docs/jeelib/classSleepy.html) 
const unsigned long WDT_period = 80;          //Pulse duration as set for watchdog timer*
const unsigned long WDT_max_number = 690;     //Max number of pulses between transmissions*

const unsigned long pulse_max_number = 100;   // Data is sent after this number of pulses*
const unsigned long pulse_max_duration = 50;  //Maximum pulse duration for RF transmission*
//*Unsigned long constants store positive numbers in 32 bit format (between 0 - (2^32-1))

const int DIP_switch1 =       7; //Dip switches will be used to assign network position
const int DIP_switch2 =       8;
const int pulse_countINT =    1;
const int pulse_count_pin =   3;

#define RF69_COMPAT   1           //Set to 1 if using device with RFM69CW, or 0 if using RFM12B

#define RF_freq       RF12_433MHZ //When using the RF12, ensure that the freq matches that of the device. (433/868/915MHz)
int nodeID =          23;         //RF node ID should be unique on the network
const int networkID = 45;         //Network ID must be the same as emonBase and emonGLCD / other devices to be communicated with.


// ****Define the RF payload datastructure and connection checker structure****

typedef struct {
  int temp_int;
  int temp_ext;
  int humidity;
  unsigned long pulsecount;
} RF_payload;
RF_payload emonth;  //NOT SURE THIS IS NECESSARY

typedef struct {
  int con_status;
} connection_check;

volatile unsigned long pulseCount;  //The volatile qualifier indicates that this value may be changed at any moment without direct action from the code.
unsigned long WDT_number;
boolean p;

unsigned long now = 0;

//##################################################################################################################################
void setup() {
  //Assign input & output pins; set initial on/off states; 
  pinMode(DHT22_PWR, OUTPUT);
  pinMode(DS18B20_PWR, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BATT_ADC, INPUT);
  pinMode(pulse_count_pin, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); //Turn LED on to show programme is running
  

  //**Read DIP switch positions to assign node ID and initialise RF chip**
  pinMode(DIP_switch1, INPUT_PULLUP); //INPUT_PULLUP is used to steer the pins to a known state if no input is detected. If switch is engaged, pin is LOW and vice versa. 
  pinMode(DIP_switch2, INPUT_PULLUP);
  boolean DIP1 = digitalRead(DIP_switch1);
  boolean DIP2 = digitalRead(DIP_switch2);

  if ((DIP1 == 1) && (DIP2 == 1)) nodeID = nodeID;
  if ((DIP1 == 0) && (DIP2 == 1)) nodeID = nodeID+1;
  if ((DIP1 == 1) && (DIP2 == 0)) nodeID = nodeID+2;
  if ((DIP1 == 0) && (DIP2 == 0)) nodeID = nodeID+3;
  
  rf12_initialize(nodeID, RF_freq, networkID); //MAY NEED TO CHANGE FOR RF69


  //**Test DHT is responding and setup serial connection**
  Serial.begin(9600);
  
  Serial.print("DHT Test... ");
  digitalWrite(DHT22_PWR, HIGH);    //Power up DHT22
  delay(2000);
  dht.begin();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  digitalWrite(DHT22_PWR, LOW);     //Power down DHT22
  
  /*if (isnan(t) || isnan(h)){
  
    delay(1500);                        //Wait for further 1.5s to allow DHT22 to warm up
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    DHT22_slowstart = 1; Serial.print("slow");*/

  if (isnan(t) || isnan(h)){
    Serial.println("DHT not identified.");
    DHT22_status = 0;
  }  
  //}  
  else{
    DHT22_status = 1;
    DHT22_slowstart = 0;
    Serial.println("DHT detected.");  
  } 
  

  //**Test DS18B20 is responding**
  digitalWrite(DS18B20_PWR, HIGH);
  delay(50);
  sensors.begin();
  sensors.setWaitForConversion(false); //Turn off automatic teperature conversion to reduce wake time.
  

  Serial.print("DS18B20 Test... ");

  

  numSensors = (sensors.getDeviceCount()); //Counts number of connected sensors

  //Search for one wire devices and copy the device addresses to the addrArray.
  byte j=0; //A byte is simply a class storing an nteger between 0-255
  
  while ((j < numSensors) && (oneWire.search(allAddress[j]))) j++; //Searches for location of DS19B20 sensors. If a sensor is found, then addrArray is filled with its location and true is returned.
  digitalWrite(DS18B20_PWR, LOW);  

  if (numSensors == 0){
    Serial.println("No DS18B20 devices found.");
    DS18B20_status = 0;
  }
  else{
    DS18B20_status = 1;
    Serial.println("DS18B20 detected. "); Serial.print(numSensors); Serial.println(" DS18B20 sensors.");
    if (DHT22_status == 1) {
      Serial.println("DHT22 and DS18B20 detected, assume DS18B20 is external and will be used for temperature readings.");
    }       
  }

  //**Set pulse parameters for RF chip**
  emonth.pulsecount = 0;  //Total no. of pulses(?)
  pulseCount = 0;         //Pulses since last transmission(?)
  WDT_number = 720;
  p = 0; //Flag for new pulse

  attachInterrupt(pulse_countINT, onPulse, RISING); //Interrupts code when pulse_countINT goes from LOW to HIGH, running the onPulse function.
}

//Interrupt routine - runs every time a rising edge of a pulse is detected.
void onPulse() {
  p = 1; //Flag identifying new pulse as true
  pulseCount++; //Number of pulses since last RF sent
}

//##################################################################################################################################
void loop() {

  //**Define RF pulses**
  
  if (p) {      // if (p) is met if p != 0
    delay(pulse_max_duration);
    p = 0;
  }

  if (Sleepy::loseSomeTime(WDT_period) == 1) { //Sleepy:loseSomeTime puts the device into low-power mode 
    WDT_number++;
  }

  if (WDT_number > WDT_max_number || pulseCount >= pulse_max_number) {
    noInterrupts();
    emonth.pulsecount = emonth.pulsecount + (unsigned int) pulseCount; //An unsigned int can only hold 0 and +ve numbers, not -ve.
    pulseCount = 0;
    interrupts();
    
  
    
    
    //DHT22 readings take approx. 250ms, DHT readings may have time lag of up to 2 seconds. 
  
    if ((DHT22_status == 1) && (DS18B20_status == 0)){        //DHT for temp. and hum.
      digitalWrite(DHT22_PWR, HIGH);
      delay(2000);
      if (DHT22_slowstart == 1){
        delay(1500);
      }
      float t_int = dht.readTemperature();
      float h = dht.readHumidity();
  
      if (tick == 0){
        Serial.println("");
        tick = 1;
      }
      Serial.print("Internal temperature: ");
      Serial.print(t_int);
      Serial.print(" ºC, ");
              
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %\t ");
  
      delay(2500-2000); //Take measurement every 2.5s (accounts for delay after power on)  
    }
  
    else if ((DHT22_status == 1) && (DS18B20_status == 1)){   //DHT for hum. DS18B20 for temp.
      digitalWrite(DHT22_PWR, HIGH);
      delay(2000);
      if (DHT22_slowstart == 1){
        delay(1500);
      }
      float h = dht.readHumidity();
      float t_int = dht.readTemperature();
      digitalWrite(DHT22_PWR, LOW);
      
      digitalWrite(DS18B20_PWR, HIGH);
      delay(50);
      for (int j=0; j<numSensors; j++) sensors.setResolution(allAddress[j], tempPrecision); //Set sensor resolution
      sensors.requestTemperatures(); //Issues global temp request to all devices on the bus
      float t_ext1 = sensors.getTemp(allAddress[0])/128; //In the case of multiple sensors, the index pulls readings from a specific sensor.
      digitalWrite(DS18B20_PWR, LOW);
  
      if (tick == 0){
        Serial.println("");
        tick = 1;
      }
      Serial.print("External temperature: ");
      Serial.print(t_ext1);
      Serial.print(" ºC, ");
  
      Serial.print("Internal temperature: ");
      Serial.print(t_int);
      Serial.print(" ºC, ");
              
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %\t ");
  
      delay(2000); //Take measurement every 2s  
    }
  
    else if ((DHT22_status == 0) && (DS18B20_status == 1)){   //DS12B20 for temp., no hum.
      digitalWrite(DS18B20_PWR, HIGH);
      delay(50);
      sensors.requestTemperatures(); //Issues global temp request to all devices on the bus
      float t_ext1 = sensors.getTemp(allAddress[0])/128; //In the case of multiple sensors, the index pulls readings from a specific sensor.
      digitalWrite(DS18B20_PWR, LOW);
     
      if (tick == 0){
      Serial.println("No DHT, humidity and interal temperature readings not possible. ");
      Serial.println("");
      tick = 1;
      }
      
      Serial.print("External temperature: ");
      Serial.print(t_ext1);
      Serial.println(" ºC, ");
    }
  
    else {
      if (tick == 0){
      Serial.println("No sensors detected. ");
      tick = 1;
      }      
    }

    //****RF Transmission****
    power_spi_enable();
    rf12_sleep(RF12_WAKEUP);
    delay(100);
    rf12_sendNow(0, &emonth, sizeof emonth);
    rf12_sendWait(2); //Set to 2 if fuses are Arduino default, set to 3 if 258 CK startup fuses are used.
    rf12_sleep(RF12_SLEEP);
    delay(100);
    power_spi_disable();

    WDT_number = 0;
    
    
  }
}
