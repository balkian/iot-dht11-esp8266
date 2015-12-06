#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>

#include <Ticker.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <DHT.h>
#define DHTTYPE DHT11
#define DHTPIN  2

DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

const char* ssid = "***REMOVED***";
const char* password = "***REMOVED***";
MDNSResponder mdns;

ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27,20,4); 

const int led = 13;
String tempGlobal = "";
float temperature;
float humidity;
int count = 0;

Ticker flipper;

void apagar() {
  for (uint8_t i=0; i<server.args(); i++){
    if(server.argName(i) == "nuevatemperatura"){
       tempGlobal = server.arg(i);
    }
  }
  lcd.setCursor(0,3);
  lcd.print("Temperatura: ");
  lcd.print(tempGlobal);
   for (int i = 0; i < 2; ++i)
  {
      EEPROM.write(i, tempGlobal[i]);
  }
   EEPROM.commit();
  server.send(200, "text/plain", tempGlobal);
}

void json(){
  server.send(200, "application/json", "{\"temperature\": " + tempGlobal + "}");
}

void actualizar(){
  
  temperature = NAN;
//  while (isnan(temperature)){

//
    temperature = (float) dht.readTemperature(false, true);     // Read temperature as Celsius;  
//  }
  
  humidity = NAN;
  humidity = (float) dht.readHumidity(true);
  
  Serial.print("Temperature (°C): "); 
  Serial.println(temperature, 2);
  Serial.print("Humidity: "); 
  Serial.println(humidity, 2);
  lcd.setCursor(0, 0);
  lcd.print("Temperatura: ");
  lcd.print(temperature);
  lcd.setCursor(0, 1);
  lcd.print("Humedad: ");
  lcd.print(humidity);
  Serial.print("Read sensor: ");
  count += 1;
  if(count>100){
     HTTPClient http;
     http.begin("api.thingspeak.com", 80, "/update");
     http.addHeader("X-THINGSPEAKAPIKEY", "P1U0MJLKTAOL3TCI");
     int code = http.POST("field1="+String(temperature,2)+"&field2="+String(humidity,2));
     Serial.print("HTTP code: ");
     Serial.println(code);
     count = 0; 
  }

}

void handleRoot() {
 

//  actualizar();

  
  String message = "<html><body><h1>La temperatura</h1><p>" + String(temperature,0) + "</p><p><h1>La humedad</h1><p>" + String(humidity,2) + "</p> <form action=\"apagar\">   Nueva temperatura:<br> <input type=\"text\" name=\"nuevatemperatura\" value=\""+tempGlobal+"\"> <input type=\"submit\" value=\"Submit\"> </form> </body></html>";
  server.send(200, "text/html", message);
}

void handleNotFound(){
  digitalWrite(led, 1);
//  String message = "File Not Found\n\n";
  String message = "URI: ";
  message += server.uri();
//  message += "\nMethod: ";
//  message += (server.method() == HTTP_GET)?"GET":"POST";
//  message += "\nArguments: ";
//  message += server.args();
//  message += "\n";
//  for (uint8_t i=0; i<server.args(); i++){
//    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//  }
  server.send(404, "text/plain", message);
  lcd.home();

  lcd.print(message);
  for ( uint i=message.length(); i<20; i++){
     lcd.print(" ");
  }
  digitalWrite(led, 0);
}
 
void setup(void){
   EEPROM.begin(512);
     for (int i = 0; i < 2; ++i)
    {
      tempGlobal += char(EEPROM.read(i));
    }
  dht.begin();           // initialize temperature sensor
  lcd.begin();                     
  lcd.backlight();
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(0, 2);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  lcd.setCursor(0, 3);
  lcd.print("Guardada: "+tempGlobal);
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  
  server.on("/", handleRoot);
  server.on("/apagar", apagar);
  server.on("/json", json);
  
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
//   flipper.attach(5, actualizar);
}
 
void loop(void){
  server.handleClient();
  actualizar();
} 