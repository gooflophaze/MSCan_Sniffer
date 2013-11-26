/*
MSCan_Sniffer - Program to sniff all CAN traffic coming out of 
a Megasquirt ECU. Megasquirt does not send any CAN data by 
default - broadcasting must be enabled through tunerstudio.

ANSI terminal required for full viewing pleasure.

Created by David Will, November 25 2013.

Uses Rechargecar's MCP2515 library found at
https://github.com/rechargecar/mcp2515

*/

#include <SPI.h>
#include <MCP2515.h>

// Pin definitions specific to how the MCP2515 is wired up.
// Pin 10 for sparkfun canbus shield
#define CS_PIN    10
#define RESET_PIN  99 //this does nothing
#define INT_PIN    2 // INTERRUPT != PIN. 



// Create CAN object with pins as defined
MCP2515 CAN(CS_PIN, RESET_PIN, INT_PIN);


void CANHandler() {
	CAN.intHandler();
}


void setup() {                
  Serial.begin(115200);
  Serial.println("Initializing ...");

//CanShield Setup
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.begin();
	
	// Initialize MCP2515 CAN controller at the specified speed and clock frequency
	// (Note:  This is the oscillator attached to the MCP2515, not the Arduino oscillator)
	//speed in KHz, clock in MHz
	if(CAN.Init(500,16))
	{
		Serial.println("MCP2515 Init OK ...");
	} else {
		Serial.println("MCP2515 Init Failed ...");
	}
	
	attachInterrupt(6, CANHandler, FALLING);
	CAN.InitFilters(true);

//ALLOW All the bits to be received!
        CAN.Write(RXB0CTRL, B01101000);
        CAN.Write(RXB1CTRL, B01101000);
	Serial.println("Ready ...");

}


// CAN message frame (actually just the parts that are exposed by the MCP2515 RX/TX buffers)
Frame message;

void loop() {
  byte canintf, canstat, canctrl, cnf1, cnf2, cnf3;
  boolean rxstatus;
  canintf=CAN.Read(CANINTF);
    
  rxstatus=bitRead(canintf,0);
   if (rxstatus) {
    Serial.println("RXB0----------------------------");
    MSrx(0x00);
  }
    rxstatus=bitRead(canintf,1);
  if (rxstatus) {
    Serial.println("RXB1---------------------------");
    MSrx(0x01);
  }
  delay(10);
  CAN.Write(CANINTF,0x00);
/*  canintf=CAN.Read(RXB0CTRL);
  bytePrint(canintf);
  Serial.println("");
  delay(200);
  */
}

void MSrx(byte buffer) {
 byte SIDH, SIDL, EID8, EID0, DLC;
 byte databuffer[7];
 unsigned int data;
 
 if (buffer == 0) {
 SIDH=CAN.Read(RXB0SIDH);
 SIDL=CAN.Read(RXB0SIDL);
 EID8=CAN.Read(RXB0EID8);
 EID0=CAN.Read(RXB0EID0);
 DLC=CAN.Read(RXB0DLC);
 
 databuffer[0]=CAN.Read(RXB0D0);
 databuffer[1]=CAN.Read(RXB0D1);
 databuffer[2]=CAN.Read(RXB0D2);
 databuffer[3]=CAN.Read(RXB0D3);
 databuffer[4]=CAN.Read(RXB0D4);
 databuffer[5]=CAN.Read(RXB0D5);
 databuffer[6]=CAN.Read(RXB0D6);
 databuffer[7]=CAN.Read(RXB0D7);
 }
 else if (buffer == 1) {
 SIDH=CAN.Read(RXB1SIDH);
 SIDL=CAN.Read(RXB1SIDL);
 EID8=CAN.Read(RXB1EID8);
 EID0=CAN.Read(RXB1EID0);
 DLC=CAN.Read(RXB1DLC);
 
 databuffer[0]=CAN.Read(RXB1D0);
 databuffer[1]=CAN.Read(RXB1D1);
 databuffer[2]=CAN.Read(RXB1D2);
 databuffer[3]=CAN.Read(RXB1D3);
 databuffer[4]=CAN.Read(RXB1D4);
 databuffer[5]=CAN.Read(RXB1D5);
 databuffer[6]=CAN.Read(RXB1D6);
 databuffer[7]=CAN.Read(RXB1D7);   
 }
   
bytePrintColor(SIDH, SIDL, EID8, EID0, DLC);

Serial.print("Data: ");
data=databuffer[0];
data=((data << 8) | databuffer[1]);
Serial.println(data);

byte datalength=(DLC & 0x0F);
for (byte x=0 ; x < datalength ; x++) {
  bytePrint(databuffer[x]);
}
  Serial.println("");

byte temp;
int var_offset;
var_offset=SIDH;
temp=((SIDL & B11100000) >> 5);
var_offset=((var_offset << 3 )| temp);
Serial.print("Var_Offset: ");
Serial.println(var_offset);

int msg_type;
msg_type=((B00000011 & SIDL) << 1);
temp=0x00;
temp=((B10000000 & EID8) >> 7);
msg_type=msg_type | temp;
Serial.print("msg_type: ");
Serial.println(msg_type);

byte from_id, to_id;
from_id=((B01111000 & EID8) >> 3);
to_id=((B00000111 & EID8) << 1);
temp=0x00;
temp=((B10000000 & EID0) >> 7);
to_id=to_id | temp;
Serial.print("from_id: ");
Serial.print(from_id);
Serial.print(" to_id: ");
Serial.println(to_id);

int var_block;
var_block=((B01111000 & EID0) >> 3);
temp=0x00;
temp=((B00000100 & EID0) << 3);
var_block=var_block | temp;
Serial.print("var_block: ");
Serial.println(var_block);

Serial.print("Oneliner_f:");
Serial.print(from_id);
Serial.print("_t:");
Serial.print(to_id);
Serial.print("_b:");
Serial.print(var_block);
Serial.print("_offset:");
Serial.print(var_offset);
Serial.print("_data:");
Serial.println(data);
}

void bytePrint(byte victim) {
  boolean temp;
  Serial.print("b");
  for (int x = 7; x >=0; x--) {
    temp=bitRead(victim,x);
    Serial.print(temp,BIN);
  }
}



void bytePrintColor(byte SIDH, byte SIDL, byte EID8, byte EID0, byte DLC) {
#define BRACE 0x5B
#define ESCAPE 0x1B
byte temp;

Serial.write(ESCAPE); //SIDH - all var offset
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.print("SIDH: b");
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;46m");
for (int x = 7; x >=0; x--) {
    temp=bitRead(SIDH,x);
    Serial.print(temp,BIN);
  }
Serial.write(ESCAPE); // SIDL - var_offset, control bits, msg_type
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.println("   // var_offset"); // end SIDH


Serial.print("SIDL: b"); //begin SIDL
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;46m");
Serial.print(bitRead(SIDL,7),BIN); //var_offset
Serial.print(bitRead(SIDL,6),BIN); //var_offset
Serial.print(bitRead(SIDL,5),BIN); //var_offset
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("1;36;49m");
//control bits - EXIDE, etc
Serial.print(bitRead(SIDL,4),BIN); //SRR
Serial.print(bitRead(SIDL,3),BIN); //IDE
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;37;47m");
Serial.print(bitRead(SIDL,2),BIN); //not used
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;30;43m");
Serial.print(bitRead(SIDL,1),BIN); //msg_type
Serial.print(bitRead(SIDL,0),BIN); //msg_type
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.println("   // var_offset, SRR + IDE, NA, msg_type"); //end SIDL


Serial.print("EID8: b");
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;30;43m");
Serial.print(bitRead(EID8,7),BIN); //msg_type
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;42m");
Serial.print(bitRead(EID8,6),BIN); //from_id
Serial.print(bitRead(EID8,5),BIN); //from_id
Serial.print(bitRead(EID8,4),BIN); //from_id
Serial.print(bitRead(EID8,3),BIN); //from_id
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;41m");
Serial.print(bitRead(EID8,2),BIN); //to_id
Serial.print(bitRead(EID8,1),BIN); //to_id
Serial.print(bitRead(EID8,0),BIN); //to_id
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;49m");
Serial.println("   // msg_type, from_id, to_id");


Serial.print("EID0: b");
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;41m");
Serial.print(bitRead(EID0,7),BIN); //to_id
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;44m"); 
Serial.print(bitRead(EID0,6),BIN); //var_block
Serial.print(bitRead(EID0,5),BIN); //var_block
Serial.print(bitRead(EID0,4),BIN); //var_block
Serial.print(bitRead(EID0,3),BIN); //var_block
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("1;34;46m"); 
Serial.print(bitRead(EID0,2),BIN); //var_block - bit 5
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;45m"); 
Serial.print(bitRead(EID0,1),BIN); //spare
Serial.print(bitRead(EID0,0),BIN); //spare
Serial.write(ESCAPE);
Serial.write(BRACE);
Serial.print("0;39;49m"); 
Serial.println("   // to_id, var_block 3:0, var_block 4:, spare");

Serial.print("DLC: ");
bytePrint(DLC);
Serial.println("");
}

