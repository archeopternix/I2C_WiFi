/*********
  D.O.CRui
  based on the sample of Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// WiFi library
#include <ESP8266WiFi.h>
#include <Wire.h>

// Hier geben wir den WLAN Namen (SSID) und den Zugansschlüssel ein
//const char* ssid     = "xx";
//const char* password = "xx";
const char* ssid = "Amaranth";
const char* password = "2256654690724849";

#define PULSELENGTH 1000 // default for pulselength to avoid burnout of relais

// Port of webserver: 80
WiFiServer server(80);

// stores the HTTP Request 
String header;


// Scan I2C Bus when started (in the loop)
bool firstrun = true;

typedef struct relais_t
{
	byte baseaddr;		// I2C baseaddress	
	word status; 		// for 16 bit I2C device [LOW,HIGH]
	int pulselength; 	// length in ms
	byte bitcount;		// how many bits are active (8 or 16)
};
relais_t Relais[8];		// max 8 relais can be connected to 1 I2C bus

byte numrelais =0;		// amount of connected relais

// is used to send PULSELENGTH pulses to the relais
typedef struct timer_typ {
	byte relais;
	byte pin;
	unsigned long starttime;
	int pulselength; 	// length in ms
};
timer_typ Timer;

void setup() {
	Serial.begin(115200);

	while (!Serial);        // wait for serial monitor !!remove for production!!

	Wire.begin();			// Initialize the I2C library

	Timer.starttime=0;
	Timer.pin=255;
	Timer.relais=255;
	 
	// Per WLAN mit dem Netzwerk verbinden
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	// Die IP vom Webserver auf dem seriellen Monitor ausgeben
	Serial.println("");
	Serial.println("WLAN verbunden.");
	Serial.println("IP Adresse: ");
	Serial.println(WiFi.localIP());
	
	server.begin();
}

void printHTMLhead(WiFiClient *client) {
	// Hier wird nun die HTML Seite angezeigt:
	client->println("<!DOCTYPE html><html>");
	client->println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
	client->println("<link rel=\"icon\" href=\"data:,\">");
	// Es folgen der CSS-Code um die Ein/Aus Buttons zu gestalten
	// Hier können Sie die Hintergrundfarge (background-color) und Schriftgröße (font-size) anpassen
	client->println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
	client->println(".button { background-color: #333344; border: none; color: white; padding: 16px 40px;");
	client->println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
	client->println(".button2 {background-color: #888899;}</style></head>");
	
	// Webseiten-Überschrift
	client->println("<body><h1>ESP8266 Web Server</h1>");
}

void loop(){
  WiFiClient client = server.available();   // Hört auf Anfragen von Clients

  // check the I2C bus and collect relais informations
  if (firstrun) {
    checkI2Cbus();
    firstrun = false;
  }

  // iterate through all found I2C relais
  if ((Timer.starttime > 0) && (millis() > (Timer.starttime + Timer.pulselength) )) {
    switchRelais(Timer.relais,Timer.pin);
    Timer.starttime=0;     
  }

  if (client) {                             // Falls sich ein neuer Client verbindet,
    Serial.println("Neuer Client.");          // Ausgabe auf den seriellen Monitor
    String currentLine = "";                // erstelle einen String mit den eingehenden Daten vom Client
    while (client.connected()) {            // wiederholen so lange der Client verbunden ist
      if (client.available()) {             // Fall ein Byte zum lesen da ist,
        char c = client.read();             // lese das Byte, und dann
        Serial.write(c);                    // gebe es auf dem seriellen Monitor aus
        header += c;
        if (c == '\n') {                    // wenn das Byte eine Neue-Zeile Char ist
          // wenn die aktuelle Zeile leer ist, kamen 2 in folge.
          // dies ist das Ende der HTTP-Anfrage vom Client, also senden wir eine Antwort:
          if (currentLine.length() == 0) {
            // HTTP-Header fangen immer mit einem Response-Code an (z.B. HTTP/1.1 200 OK)
            // gefolgt vom Content-Type damit der Client weiss was folgt, gefolgt von einer Leerzeile:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

			// iterate through all found I2C relais when no timer is set
			if (Timer.starttime < 1) {
				for (int r=0; r<numrelais; r++) { 
				   for (int i=0; i< Relais[r].bitcount; i++) {           
					  String num = String(i);
					  String rel = String(r);
					  if (header.indexOf("GET /"+ rel + "/" +num + "/on") >= 0) {
  						Timer.starttime=millis();
  						Timer.pin=i;
  						Timer.relais=r;
  						Timer.pulselength=Relais[r].pulselength;
  						switchRelais(r,i);                   
					  }
				   }
				}
			}
            
			printHTMLhead(&client);
            
			// iterate through all found I2C relais
			for (int r = 0; r < numrelais; r++) { 
				String rel = String(r);
				client.print("<h2>\"Relais :" + rel + "</h2>");
       
				// Zeige den aktuellen Status, und AN Buttons  
				for (int i=0; i< Relais[r].bitcount; i++) {            
				  String num = String(i);         
				  client.print("<p><a href=\"/" + rel + "/"+ num + "/on\"><button class=\"button\">"+num+" EIN</button></a></p>");                         
				}
            }
			
            // Die HTTP-Antwort wird mit einer Leerzeile beendet
            client.println();
            // und wir verlassen mit einem break die Schleife
            break;
          } else { // falls eine neue Zeile kommt, lösche die aktuelle Zeile
            currentLine = "";
          }
        } else if (c != '\r') {  // wenn etwas kommt was kein Zeilenumbruch ist,
          currentLine += c;      // füge es am Ende von currentLine an
        }
      }
    }
    // Die Header-Variable für den nächsten Durchlauf löschen
    header = "";
    // Die Verbindung schließen
    client.stop();
    Serial.println("Client getrennt.");
    Serial.println("");
  }
}

void checkI2Cbus() {
  byte error;
  
  Serial.print("Scanning I2C bus");
  for(byte address = 1; address < 127; address++ )
  {
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
    		Relais[numrelais].baseaddr = address;
    		Relais[numrelais].status = 0b1111111111111111;
    		Relais[numrelais].pulselength = PULSELENGTH;
    		Relais[numrelais].bitcount = 8;		
        numrelais++;
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
  if (numrelais == 0)
    Serial.println(F("No I2C devices found\n"));
}

void switchRelais(byte relais,byte pin) {
	byte error;
	
	if (bitRead(Relais[relais].status, pin)) {
		bitClear(Relais[relais].status, pin);
	} else {
		bitSet(Relais[relais].status, pin );
	}
	  
	Wire.beginTransmission(Relais[relais].baseaddr); 
	Wire.write(lowByte(Relais[relais].status));                  // sends low byte
	Wire.write(highByte(Relais[relais].status));                 // sends high byte
	error = Wire.endTransmission();       // stop transmitting

	Serial.print(F("relais with address: 0x"));
	Serial.print(Relais[relais].baseaddr,HEX);
	Serial.print(" - value:");
	Serial.print(Relais[relais].status,BIN);        
  
	switch (error) {
		case 0:
		  Serial.println(F("Success updating relais "));       
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


