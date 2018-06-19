/*Sketch to test communications between two RFM69 transceiver modules using a simple data package. 
 * 
 * Written by tristandell. Public domain.*/

//##################################################################################################################################
//LIBRARIES:

#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h> //New version (SPIMemory) is available.

// ****Define some constants / pin assignments****
const int BATT_ADC = 1;
const int LED = 9;

int tick = 0;

#define RF_freq       RF69_433MHZ
#define MYNODEID      1
#define TONODEID      2
#define NETWORKID     45

/*int nodeID            = 1;
int target_nodeID     = 2;
const int networkID   = 45;*/

RFM69 radio;
#define SS_pin 7    //Radio hand shake pin to activate slave

SPIFlash flash(8, 0xEF30); //Use EF40 for 16mbit windbond chip


//##################################################################################################################################
//SETUP:

void setup() {
  
  //****Assign input & output pins; set initial on/off states****
  pinMode(LED, OUTPUT);
  pinMode(BATT_ADC, INPUT);
  digitalWrite(LED, HIGH); //Turn LED on to show programme is running

  //****Setup serial connection***
  Serial.begin(9600);

  //****Intialise RF chip****
  Serial.print("Radio initialising... ");
  //radio.setCS(SS_pin);
  radio.initialize(RF_freq, MYNODEID, NETWORKID);
  radio.setHighPower();    //Always use with RFM69
  delay(1000);
  radio.promiscuous(true); //Do or do not sniff data packets on the network
  Serial.print("Done. \n");
  
}

//##################################################################################################################################
//LOOP:

void loop() {

  //****RF Receive****
  if (radio.receiveDone()); { //Wake RF chip
    delay(50);
    //node_connection(); //Receive data
    
    if (radio.SENDERID > 0) {

      //****Ticker for transmission number****
      tick++;
  
      Serial.print("Received from node: "); Serial.println(radio.SENDERID);
      Serial.print("Receive cycle: "); Serial.println(tick);
      Serial.print("Data received: "); 
      for (byte i = 0; i <= radio.DATALEN; i++) {
        Serial.print((char) radio.DATA[i]);        
      }
      Serial.print("\n");
      //Serial.println(data);
      
    
      //Receive signal strength indicator (RSSI), low is good.
      //Could use this to vary the transmit strength to avoid unnecessary power usage.
      Serial.print("RSSI = "); Serial.println(radio.RSSI);
      Serial.print("\n");
       
      //radio.sleep();
    }
  }
}

