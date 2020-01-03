// --------------------------------------
// ESP_i2c_blink
//
// Version 1, January 2018, 
// by Andreas <DOC> Eisner
//    Scanning addresses changed from 0...127 to 1...119,
//    according to the i2c scanner by Nick Gammon
//    http://www.gammon.com.au/forum/?id=10896
//
// This sketch:
// a) tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.
// b) sets all ports on high, waits 2 sec and then to low
     
#include <Wire.h>

#include <ESP8266WiFi.h>


const char* ssid = "xx";
const char* password = "xx";
 

#define BASEADDR 0x20
#define ON 1
#define OFF 0

WiFiServer server(80);     

bool firstrun = true;
bool even = true;

int devices[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
int nDevices = 0;
 
void setup()
{
  Wire.begin();
 
  Serial.begin(9600);
  while (!Serial);             // wait for serial monitor
  Serial.println(F("\nI2C Scanner"));

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(F("WiFi connected"));
 
  // Start the server
  server.begin();
  Serial.println(F("Server started"));
 
  // Print the IP address
  Serial.print(F("Use this URL to connect: "));
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 
}
 

 
void printHTMLHeader(WiFiClient *client) {
    // Return the response
  client->println("HTTP/1.1 200 OK");
  client->println("Content-Type: text/html");
  client->println(""); //  do not forget this one
  client->println("<!DOCTYPE HTML>");
  client->println("<html>");
}


void printHTMLDeviceList(WiFiClient *client) {
  client->println("<h1>List of Devices</h1><br><br>");
  for(int devs = 0; devs < nDevices; devs++ )
  {
    client->print("<a href= \"/id:");
    client->print(devs);
    client->println("\"><button>");
    client->print(devices[devs]);
    client->println("</button></a>");
  }
  client->println("</html>");  
}

void printHTMLConfigPage(WiFiClient *client) {
  client->print("<h1>I2C Configuration</h1>");
 

  client->println("<br><br>");
  client->println("<a href=\"/LED=ON\"\"><button>Turn On </button></a>");
  client->println("<a href=\"/LED=OFF\"\"><button>Turn Off </button></a><br />");  
  client->println("</html>");  
}

void writeI2C(byte device,byte lowbyte,byte highbyte) {
  byte error;
  
  Wire.beginTransmission(device); 
  
  Wire.write(lowbyte);                  // sends low byte
  Wire.write(highbyte);                 // sends high byte

  error = Wire.endTransmission();       // stop transmitting
  switch (error) {
    case 0:
      Serial.print(F("Success updating device with address: 0x"));
      Serial.print(device,HEX);
      Serial.print(" - value:");
      Serial.print(lowbyte,BIN);        
      Serial.println(highbyte,BIN);        
      break;
    case 1:
      Serial.println(F("data too long to fit in transmit buffer"));
      break;
    case 2:
      Serial.println(F("received NACK on transmit of address"));
      break;
    case 3:
      Serial.println(F("received NACK on transmit of data"));
      break;
    default:
      Serial.println("other error ");
  }
}


void checkI2Cbus() {
  byte error, address;
 
  Serial.print("Scanning");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    Serial.print(".");
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    switch (error) {
      case 0:
        Serial.println();
        Serial.print(F("I2C device found at address 0x"));
        if (address<16)
          Serial.print("0");
        Serial.print(address,HEX);
        Serial.println("  !");
        Serial.print("Scanning");
        devices[nDevices]=address;
        nDevices++;
        break;
      case 4:
        Serial.print(F("Unknown error at address 0x"));
        if (address<16)
          Serial.print("0");
        Serial.println(address,HEX);
        break;
    }
  }
  Serial.println();
  if (nDevices == 0)
    Serial.println(F("No I2C devices found\n"));

}


void loop()
{  
  if (firstrun) {
    checkI2Cbus();
   // firstrun = false;
  }
  
    writeI2C(BASEADDR,byte(0b11111111),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b01111111),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00111111),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00011111),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00001111),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00000111),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00000011),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00000001),byte(0x00));
    delay(200);
    writeI2C(BASEADDR,byte(0b00000000),byte(0x00));
    delay(200);
    
  
  delay(2000);
}


