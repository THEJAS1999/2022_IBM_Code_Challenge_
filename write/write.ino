/*This program sends the CAN frame.
MCP2515 LIBRARY:
Download the ZIP file fromhttps://github.com/autowp/arduino-mcp2515/archive/master.zip .
From the Arduino IDE: Sketch -> Include Library... -> Add .ZIP Library.
Restart the Arduino IDE to see the new "mcp2515" library with examples.
*/
#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg1;
struct can_frame canMsg2;
struct can_frame canMsg;

MCP2515 mcp2515(10);

static int i=16,j=1,k=1;

void setup() {
  while (!Serial);
  Serial.begin(115200);
  Serial.println("Example: Write to CAN");
}

void loop() {

  canMsg2.can_id  = 0x4E; //frame used to send the Vehicle speed engine speed and gear
  canMsg2.can_dlc = 8;
  canMsg2.data[0] = j; //1st byte is circular_counter
  canMsg2.data[1] = i; //2nd byte is Vehicle speed
  canMsg2.data[2] = i; //3rd byte is Engine speed
  canMsg2.data[3] = i; //4th byte is gear
  canMsg2.data[4] = 0x00;
  canMsg2.data[5] = 0x00;
  canMsg2.data[6] = 0x00;
  canMsg2.data[7] = 0x00;

  canMsg1.can_id  = 0x2D; //frame used to send the request for Ambient temperature,Ambient humidity or Both.
  canMsg1.can_dlc = 8;
  canMsg1.data[0] = k++;
  canMsg1.data[1] = 0x00;
  canMsg1.data[2] = 0x00;
  canMsg1.data[3] = 0x00;
  canMsg1.data[4] = 0x00;
  canMsg1.data[5] = 0x00;
  canMsg1.data[6] = 0x00;
  canMsg1.data[7] = 0x00;


  mcp2515.reset(); //reset MCP2515
  mcp2515.setBitrate(CAN_125KBPS); //Baud rate is 125KBPS
  mcp2515.setNormalMode();

  mcp2515.sendMessage(&canMsg2); //send 2nd frame
  delay(10000);
  mcp2515.sendMessage(&canMsg1); //send 1st frame
  delay(10000);

  Serial.print("Messages sent:");
  Serial.println(canMsg2.data[1]); //print the data
   if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("Received from Cloud ");
   }
  i=i+16; // Vehicle speed,engine speed and gear is incremented by 16 each time
  j=j+2; //circular count is incremented by 2
  if(k==4)
    k=1; //start from 0x01
  if(i==256)
  {
   i=16; //reassign to 16
  }
  if(j==256)
  {
    j=1; //reassign to 1
  }
}
