/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Wir laden die uns schon bekannte WiFi Bibliothek
#include <ESP8266WiFi.h>

// Hier geben wir den WLAN Namen (SSID) und den Zugansschlüssel ein
const char* ssid     = "Amaranth";
const char* password = "2256654690724849";

// Wir setzen den Webserver auf Port 80
WiFiServer server(80);

// Eine Variable um den HTTP Request zu speichern
String header;

byte turnout = 0b00000000;


// Die verwendeted GPIO Pins
// D1 = GPIO5 und D2 = GPIO4 - einfach bei Google nach "Amica Pinout" suchen  
const int led13 = 5;

void setup() {
  Serial.begin(115200);
  // Die definierten GPIO Pins als output definieren ...
  pinMode(led13, OUTPUT);
  // ... und erstmal auf LOW setzen
  digitalWrite(led13, LOW);


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

void loop(){
  WiFiClient client = server.available();   // Hört auf Anfragen von Clients

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

           for (int i=0; i<8; i++) {            
              String num = String(i);
              if (header.indexOf("GET /"+num + "/on") >= 0) {
                Serial.println(num + " on");
                bitWrite(turnout,i,true);                                   
                digitalWrite(led13, HIGH);
              } else if (header.indexOf("GET /"+num + "/off") >= 0) {
                Serial.println(num + " off");
                bitWrite(turnout,i,false);                                   
                digitalWrite(led13, LOW);
              }
           }
            
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
            
            // Zeige den aktuellen Status, und AN/AUS Buttons for GPIO 5  
    
            
            for (int i=0; i<8; i++) {            
              String num = String(i);
              client.print("<p><a href=\"/" + num);
              if (bitRead(turnout,i)==true) {
                  client.println("/off\"><button class=\"button\">"+num+" AUS</button></a></p>");     
              } else {
                  client.println("/on\"><button class=\"button\">"+num+" EIN</button></a></p>");                         
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


