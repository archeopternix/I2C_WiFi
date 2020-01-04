/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Wir laden die uns schon bekannte WiFi Bibliothek
#include <ESP8266WiFi.h>

// Hier geben wir den WLAN Namen (SSID) und den Zugansschlüssel ein
const char* ssid     = "xx";
const char* password = "xx";

#define PULSELENGTH 300 // default for pulselength to avoid burnout of relais

// Wir setzen den Webserver auf Port 80
WiFiServer server(80);

// Eine Variable um den HTTP Request zu speichern
String header;

// Schaltstatus der einzelnen Weichen
byte turnout = 0b00000000;

// Scan I2C Bus when started (in the loop)
bool firstrun = true;

typedef struct relais_t
{
	byte baseaddr;		// I2C baseaddress	
	byte[2] status; 	// for 16 bit I2C device [LOW,HIGH]
	int pulselength; 	// length in ms
	byte bitcount;		// how many bits are active (8 or 16)
};
relais_t[8] Relais;		// max 8 relais can be connected to 1 I2C bus
byte numrelais =0;		// amount of connected relais

struct timer_t {
	byte relais;
	byte pin;
	unsigned long starttime;
	int pulselength; 	// length in ms
};

timer_t Timer;

void setup() {
  Serial.begin(115200);

  while (!Serial);             // wait for serial monitor
 
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
	client.println("<!DOCTYPE html><html>");
	client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
	client.println("<link rel=\"icon\" href=\"data:,\">");
	// Es folgen der CSS-Code um die Ein/Aus Buttons zu gestalten
	// Hier können Sie die Hintergrundfarge (background-color) und Schriftgröße (font-size) anpassen
	client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
	client.println(".button { background-color: #333344; border: none; color: white; padding: 16px 40px;");
	client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
	client.println(".button2 {background-color: #888899;}</style></head>");
	
	// Webseiten-Überschrift
	client.println("<body><h1>ESP8266 Web Server</h1>");
}

void loop(){
  WiFiClient client = server.available();   // Hört auf Anfragen von Clients

  // check the I2C bus and collect relais informations
  if (firstrun) {
    checkI2Cbus();
    firstrun = false;
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

			// iterate through all found I2C relais
			if ((Timer.starttime + Timer.pulselength) > Time()) {
				switchRelais(Timer.relais,Timer.pin,false);
				Timer.starttime=0;
				Timer.pin=255;
				Timer.relais=255;	
				Timer.pulselength=0;				
			} else if (Timer.starttime == 0 ) {
				for (int r=0; r<numrelais; r++) { 
				   for (int i=0; i< Relais[r].bitcount; i++) {           
					  String num = String(i);
					  String rel = String(r);
					  if (header.indexOf("GET /"+ rel + "/" +num + "/on") >= 0) {
						Timer.starttime=Time();
						Timer.pin=i;
						Timer.relais=r;
						Timer.pulselength=Relais[r].pulselength
						switchRelais(Timer.relais,Timer.pin,true);                     
					  } else if (header.indexOf("GET /"+ rel + "/" +num + "/off") >= 0) {
						Timer.starttime=Time();
						Timer.pin=i;
						Timer.relais=r;
						Timer.pulselength=Relais[r].pulselength
						switchRelais(Timer.relais,Timer.pin,false);                              
					  }
				   }
				}
			}
            
			printHTMLhead(client)
            
			// iterate through all found I2C relais
			for (int r=0; r<numrelais; r++) { 
				client.print("<h2>\"Relais :" + rel + "</h2>");
				// Zeige den aktuellen Status, und AN/AUS Buttons  
				for (int i=0; i< Relais[r].bitcount; i++) {            
				  String num = String(i);
				  String rel = String(r);
				  client.print("<p><a href=\"/" + rel + "/"+ num);
				  if (bitRead(turnout,i)==true) {
					  client.println("/off\"><button class=\"button\">"+num+" AUS</button></a></p>");     
				  } else {
					  client.println("/on\"><button class=\"button\">"+num+" EIN</button></a></p>");                         
				  }     
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
  byte error, address;
  numrelais =0;
  
  Serial.print("Scanning I2C bus");
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
		Relais[numrelais].baseaddr = address;
		Relais[numrelais].status[0] = 0b00000000;
		Relais[numrelais].status[1] = 0b00000000;
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

void switchRelais(byte relais,byte pin, bool state) {
	byte error;

	if (state) {
		if (pin > 7) {
			bitClear(Relais[numrelais].status[1], pin - 8);
		} else {
			bitClear(Relais[numrelais].status[0], pin);			
		}
	} else {
		if (pin > 7) {
			bitSet(Relais[numrelais].status[1], pin - 8);
		} else {
			bitSet(Relais[numrelais].status[0], pin);			
		}
	}
	  
	Wire.beginTransmission(Relais[numrelais].baseaddr); 
	Wire.write(Relais[numrelais].status[0]);                  // sends low byte
	Wire.write(Relais[numrelais].status[1]);                 // sends high byte
	error = Wire.endTransmission();       // stop transmitting
	
	switch (error) {
		case 0:
		  Serial.print(F("Success updating relais with address: 0x"));
		  Serial.print(Relais[numrelais].baseaddr,HEX);
		  Serial.print(" - value:");
		  Serial.print(Relais[numrelais].status[0],BIN);        
		  Serial.println(Relais[numrelais].status[1],BIN);        
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


