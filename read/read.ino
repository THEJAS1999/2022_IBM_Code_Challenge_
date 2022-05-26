/*
 This program receives the CAN frames and sends the data to cloud.
 Also,receives the data from Cloud.
 ESP8266 LIBRARY:
 By Bruno Portaluri version 2.2.2
 Arduino WiFi library for ESP8266 .Works only with SDK version 1.1.1 and above (AT version 0.25 and above.
 GitHub - bportaluri/WiFiEsp: Arduino WiFi library for ESP8266 modules

MCP2515 LIBRARY:
Download the ZIP file fromhttps://github.com/autowp/arduino-mcp2515/archive/master.zip .
From the Arduino IDE: Sketch -> Include Library... -> Add .ZIP Library.
Restart the Arduino IDE to see the new "mcp2515" library with examples.

THINGSPEAK LIBRARY:
 By Mathworks version 2.0.1.
 Thingspeak Communication library for Arduino,ESP8266 and EPS32 Thingspeak ( https://www.thingspeak.com) is an analytic IoT platform service that allows you to aggregate,visualize and analyze live data streams in the cloud.

QLIST LIBRARY:
By Martin Dagarin Version 0.6.7
Library implements linked lists.It enables to create list of items in order like queue or stack or vector.
GitHub - SloCompTech/QList: Linked list library for Arduino

*/

#include <SPI.h>
#include <mcp2515.h>
#include "QList.h"

#include "WiFiEsp.h"
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

char ssid[] = "POCO M3";   // your network SSID (name)
char pass[] = "priya123@";   // your network password

WiFiEspClient  client;

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"

SoftwareSerial Serial1(6, 7); // RX, TX

#define ESP_BAUDRATE  19200
#else
#define ESP_BAUDRATE  115200
#endif

struct can_frame canMsg;
MCP2515 mcp2515(10);
static int cyclic_count=0;

#define Ambient_temperature 1 //Macro for Ambient temperature
#define Ambient_humidity 2  //Macro for Ambient pressure
#define both 3  //Macro for both Ambient temperature and Ambient pressure

unsigned long ChannelNumber = 1748729;    //channel number of my channel
unsigned long readChannelNumber =1359457 ; //channel number of public channel
const char * myWriteAPIKey = "LGS0EUVXQ5ET6BBL"; //Write API KEY of my channel
QList<int> q;
void setup() {
  Serial.begin(115200); //Baud rate of esp8266
  while(!Serial){
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }

  // initialize serial for ESP module
  setEspBaudRate(ESP_BAUDRATE);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }

  Serial.print("Searching for ESP8266...");
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
  Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }
  Serial.println("found it!");
  mcp2515.reset(); //reset MCP2515
  mcp2515.setBitrate(CAN_125KBPS);//Baud rate of MCP2515 is 125KBPS
  mcp2515.setNormalMode();


}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
  if(canMsg.can_id==78) //0x4E=78 in decimal
  {
    Serial.println("Writing to cloud");
    write_data_to_cloud();
    delay(2000); //delay of 2 sec
  }
 if(canMsg.can_id==45) //0x2D=45 in decimal
  {
    Serial.println("Reading from cloud");
    read_data_from_cloud();
    delay(2000);
  }
  }
}

void write_data_to_cloud() {
  int i, j, k;
  if (cyclic_count == 0) //first frame
  {
    thingspeak(canMsg.data[1],canMsg.data[2],canMsg.data[3]); //send the first frame to thingspeak
    cyclic_count = canMsg.data[0]; //cyclic_count is assigned the counter value.
    delay(1000);
  }
  else
  {
    for (i = cyclic_count + 1; i < canMsg.data[0]; i++) //cyclic counter
    {
      thingspeak(0xff,0xff,0xff); //fill the missing frame with 0xff
      delay(1000);
    }
    thingspeak(canMsg.data[1],canMsg.data[2],canMsg.data[3]); //send the actual vehicle speed,engine speed and gear.
    Serial.print("incremented counter value:");
    Serial.println(canMsg.data[0]-cyclic_count); //Difference in the circular counter
    cyclic_count = canMsg.data[0];
  }
}

void send_data_to_thingspeak(){
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  boolean value=internet();

  if(value==false) //if WiFi is not connected
  {
     send_data_to_queue();
  }
  else
  {
    if(q.size()!=0)
    {
        for(int i=0;i<q.size();i=i+3)
        thingspeak(q.at(i),q.at(i+1),q.at(i+2));
        delay(1000);
        q.clear();//clear the queue
    }
    thingspeak(canMsg.data[1],canMsg.data[2],canMsg.data[3]);//if queue is empty then send vehicle speed engine speed and gear.
  }
}
void send_data_to_queue()
{
    q.push_front(canMsg.data[1]);//push vehicle speed to queue
    q.push_front(canMsg.data[2]);//push engine speed to queue
    q.push_front(canMsg.data[3]);//push gear to queue
    delay(1000);
}

void read_data_from_cloud(){
  struct can_frame canSendMsg; //created a frame can_sendMsg
  canSendMsg.can_id = 0x4B; //CAN_ID is 0x4B
  canSendMsg.can_dlc=8;
  if (canMsg.data[0] == Ambient_temperature) //if requested data is 0x01:Ambient_temperature
  {
    canSendMsg.data[1] = get_data_from_Thingspeak(1); //get data from thingspeak
  }
  else if (canMsg.data[0] == Ambient_humidity) //if requested data is 0x02:Ambient_humidity
  {
    canSendMsg.data[2] = get_data_from_Thingspeak(2);
  }
  else if (canMsg.data[0] == both) //if requested data is 0x0F:both
  {
    canSendMsg.data[1] = get_data_from_Thingspeak(1);
    canSendMsg.data[2] = get_data_from_Thingspeak(2);
  }
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  mcp2515.sendMessage(&canSendMsg);
  delay(1000);//send the requested data(Ambient_temperature or Ambient_pressure or both) to ECU
}

float get_data_from_Thingspeak(const int Field_Number)
{

  ThingSpeak.begin(client);  //Intialize thingspeak to connect with the internet.
  internet();
 float data= ThingSpeak.readFloatField(readChannelNumber,Field_Number);
   int statusCode = ThingSpeak.getLastReadStatus();
  if (statusCode == 200) {
    //OK / Success
    Serial.print("Read data:");
    Serial.println(data);
      return data;
  }
  else
  {
    Serial.println("Unable to read channel / No internet connection");
    return 0;
  }

}

boolean internet()
{
  if (WiFi.status() != WL_CONNECTED)
  {
   int i=1;
    while (WiFi.status() != WL_CONNECTED && i!=10)//check for 10 times.
    {
      WiFi.begin(ssid, pass);
      i++;
      delay(1000);
    }
  return false;
  }
  return true;
}


void setEspBaudRate(unsigned long baudrate){
  long rates[6] = {115200,74880,57600,38400,19200,9600};

  Serial.print("Setting ESP8266 baudrate to ");
  Serial.print(baudrate);
  Serial.println("...");

  for(int i = 0; i < 6; i++){
    Serial1.begin(rates[i]);
    delay(100);
    Serial1.print("AT+UART_DEF=");
    Serial1.print(baudrate);
    Serial1.print(",8,1,0,0\r\n");
    delay(100);
  }

  Serial1.begin(baudrate);
}
void thingspeak(int data1,int data2,int data3)
 {
  ThingSpeak.begin(client);  // Initialize ThingSpeak
       boolean value=internet();
       ThingSpeak.setField(1, data1);
       ThingSpeak.setField(2, data2);
       ThingSpeak.setField(3, data3);

       int x=ThingSpeak.writeFields(ChannelNumber, myWriteAPIKey);
       if(x == 200){
      Serial.println("Channel updated with successful Values.");
      }
  else{
    Serial.println("Problem updating channel with value. HTTP error code " + String(x));
  }
 }
