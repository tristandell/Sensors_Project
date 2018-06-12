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

/*** DEBUG CONFIG?
Set this to 1 if initially debugging the code. Otherwise set to 0 and the code will be less computationally intensive as debig steps will be skipped.***/
const debug = 1;

// ****Define some constants****
const int DS18B20_PWR =       5;
const int DHT22_PWR =         6;
const int LED =               9;
const int BATT_ADC =          1;
const int DIP_switch1 =       7;
const int DIP_switch2 =       8;

boolean DHT22_status;
boolean DS18B20_status;
boolean DHT22_slowstart;          //Indicates whether testing showed DHT takes longer than 2 seconds to respond after power on.

#define DHTPIN          18
#define ONE_WIRE_BUS    19
#define DHTTYPE         DHT22

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); //Turn LED on to show programme is running

  //**Read DIP switch positions to assign node ID**
  pinMode(DIP_switch1, INPUT_PULLUP); //INPUT_PULLUP is used to steer the pins to a known state if no input is detected. If switch is engaged, pin is LOW and vice versa. 
  pinMode(DIP_switch2, INPUT_PULLUP);
  

  //**Test DHT is responding and setup serial connection**
  Serial.begin(9600);
  if (debug == 1){
    Serial.println("DHT Test...");

    digitalWrite(DHT22_PWR, HIGH);    //Power up DHT22
    delay(2000);
    dht.begin();

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)){
    
      delay(1500);                        //Wait for further 1.5s to allow DHT22 to warm up
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      DHT22_slowstart = 1

      if (isnan(t) || isnan(h)){
        Serial.println("DHT not identified.");
        DHT22_status = 0;
      }  
    }  
    else{
      DHT22_status = 1;
      DHT22_slowstart = 0;
      Serial.println("DHT detected.");  
    } 
  digitalWrite(DHT22_PWR, LOW);     //Power down DHT22
  }

  //**Test DS18B20 is responding**
  sensors.begin();
  sensors.setWaitForConversion(false); //Turn off automatic teperature conversion to reduce wake time.
  
  if (debug == 1){ 
    Serial.println("DS18B20 Test...");
  
    digitalWrite(DS18B20_PWR, HIGH);
    delay(50);

    numSensors = (sensors.getDeviceCount()); //Counts number of connected sensors
  
    //Search for one wire devices and copy the device addresses to the addrArray.
    byte j=0; //A byte is simply a class storing an nteger between 0-255
    
    while ((j < numSensors) && (oneWire.search(allAddress[j]))) j++; //Searches for location of DS19B20 sensors. If a sensor is found, then addrArray is filled with its location and true is returned.
    digitalWrite(DS18B20_PWR, LOW);  
  
    if (numSensors == 0){
      Serial.println("No DS18B20 devices found.")
      DS18B20_status = 0;
    }
    else{
      DS28B20_status = 1;
      Serial.print("Detected "); Serial.print(numSensors); Serial.print(" DS18B20 sensors.");
      if (DHT22_status = 1) {
        Serial.print("DHT22 and DS18B20 detected, assume DS18B20 is external and will be used for temperature readings.")
      }       
    }
  }

  //Assign input & output pins; set initial on/off states; 
  pinMode(DHT22_PWR, OUTPUT);
  pinMode(DS18B20_PWR, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BATT_ADC, INPUT);
}


void loop() {
  //DHT22 readings take approx. 250ms, DHT readings may have time lag of up to 2 seconds. 

  if ((DHT22_status == 1) && (DS18B20_status == 0)){        //DHT for temp. and hum.
    digitalWrite(DHT_PWR, HIGH);
    delay(2000);
    if (DHT22_slowstart == 1){
      delay(1500);
    }
    float t_int = dht.readTemperature();
    float h = dht.readHumidity();

    Serial.print("Internal temperature: ");
    Serial.print(t_int);
    Serial.print(" ºC");
            
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");

    delay(2000); //Take measurement every 2s  
  }

  else if ((DHT22_status == 1) && (DS18B20_status == 1)){   //DHT for hum. DS18B20 for temp.
    digitalWrite(DHT_PWR, HIGH);
    delay(2000);
    if (DHT22_slowstart == 1){
      delay(1500);
    }
    float h = dht.readHumidity();
    float t_int = dht.readTemperature();
    digitalWrite(DHT_PWR, LOW);
    
    digitalWrite(DS18B20_PWR, HIGH);
    delay(50);
    sensors.requestTemperatures(); //Issues global temp request to all devices on the bus
    float t_ext = sensors.getTemp(ByIndex(0); //In the case of multiple sensors, the index pulls readings from a specific sensor.
    digitalWrite(DS18B20_PWR, LOW);
    
    Serial.print("External temperature: ");
    Serial.print(t_ext);
    Serial.print(" ºC");

    Serial.print("Internal temperature: ");
    Serial.print(t_int);
    Serial.print(" ºC");
            
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");

    delay(2000); //Take measurement every 2s  
  }

  else if ((DHT22_status == 0) && (DS18B20_status == 1)){   //DS12B20 for temp., no hum.
    digitalWrite(DS18B20_PWR, HIGH);
    delay(50);
    sensors.requestTemperatures(); //Issues global temp request to all devices on the bus
    float t_ext = sensors.getTemp(ByIndex(0); //In the case of multiple sensors, the index pulls readings from a specific sensor.
    digitalWrite(DS18B20_PWR, LOW);
    
    Serial.print("External temperature: ");
    Serial.print(t_ext);
    Serial.print(" ºC");

    Serial.print("No DHT, humidity and interal temperature readings not possible."
  }

  else {
    Serial.print("No sensors detected.")
  }

