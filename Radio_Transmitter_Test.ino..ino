/*Sketch to test communications between two RFM69 transceiver modules using a simple data package. 
 * 
 * Written by tristandell. Public domain.*/

//##################################################################################################################################
//LIBRARIES:

#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h> //New version (SPIMemory) is available.

// ****Define some constants / pin assignments****
const int BATT_ADC  = 1;
const int LED       = 9;

int tic = 0;
byte i;

#define RF_freq       RF69_433MHZ
#define MYNODEID      1
#define TONODEID      2
#define NETWORKID     45


/*int nodeID            = 1;
int target_nodeID     = 2;
const int networkID   = 45;*/

RFM69 radio;

//****Define the payload structure to be sent****
/*typedef struct {
  char test_string;
} RF_payload;*/

char RF_payload[3]; //Declare the array

SPIFlash flash(8, 0xEF30); //Use EF40 for 16mbit windbond chip


//##################################################################################################################################
//SETUP:

void setup() {
  
  //****Assign input & output pins; set initial on/off states****
  pinMode(LED, OUTPUT);
  pinMode(BATT_ADC, INPUT);
  digitalWrite(LED, HIGH); //Turn LED on to show programme is running

  //****Setup serial connection****
  Serial.begin(9600);

  //****Intialise RF chip****
  Serial.print("Radio initialising... ");
  radio.initialize(RF_freq, MYNODEID, NETWORKID);
  radio.setHighPower(); //Always use for RF69
  delay(50);
  //radio.promiscuous(true); //Sniff data packets on the network
  Serial.print("Done. \n");  

  tic = 0;
  delay(1000);
}

//##################################################################################################################################
//LOOP:

void loop() {

  //****Ticker for transmission number****
  tic++;

  //****Select random letter to send each cycle****
  for (int i = 0; i <= sizeof(RF_payload); i++){
    RF_payload[i] = 97 + rand() %26;
  }
  Serial.print("Transmission: "); Serial.println(tic);
  Serial.print("Character to be sent: ");
  for (byte i = 0; i <= sizeof(RF_payload); i++) {
    Serial.print(RF_payload[i]);
  }

  //****RF Transmission****
  //radio.receiveDone(); //Wake RF chip
  delay(100);
  send_information();
  //radio.sleep();

  //Delay between transmissions
  delay(500); 
}


//##################################################################################################################################
//FUNCTIONS:

void send_information() {
  Serial.print("\n");
  Serial.print("Data about to be sent... ");
  radio.send(TONODEID, RF_payload, sizeof(RF_payload)+1); //radio.send(ToNodeID, sendBuffer, sendLength)
  Serial.println("Done! \n");
  flash.chipErase();
  delay(100);
}

