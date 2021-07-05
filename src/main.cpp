#include <Arduino.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
//ESP Web Server Library to host a web page
#include <ESP8266WebServer.h>

#define LED 2
#define SWITCH 12
#define RELAY 13

char ssid[21] = "WBML_229_2.4G";
char pass[21] = "wbml0229";

char* host = "192.168.0.9";
char* device = "Relay_01";

const char CONFIG_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<center>
<style>
.c{text-align:center;}div,
input{padding:5px;font-size:1em;}input{width:60%;}
body{text-align:center;font-family:verdana;}
button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:60%}
</style>
<h1>WiFi LED on off demo: 1</h1><br>
Ciclk to turn <a href="ledOn" target="myIframe">LED ON</a><br>
Ciclk to turn <a href="ledOff" target="myIframe">LED OFF</a><br>
<input id='s'name='s'length=20 placeholder='SSID'><br/>
<input id='p'name='p'length=20 type='password'placeholder='password'><br/>
<p>Button A</p>
<a href=\"/A/0\">
<button style=\"background-color:#12cb3d;\">ON</button></a></p>
LED State:<iframe name="myIframe" width="100" height="25" frameBorder="0"><br>
</center>

</body>
</html>
)=====";

//Declare a global object variable from the ESP8266WebServer class.
ESP8266WebServer server(80); //Server on port 80

int sw_state = LOW;

void handleRoot() {
  Serial.println("You called root page");
  String s = CONFIG_PAGE; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

String toStringIp(IPAddress ip) {  // 32비트 ip 주소를 스트링으로 변환하는 함수 
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

void setup() {
  Serial.begin(115200);
  Serial.println("WiFi_Relay_Board");

  if(EEPROM.read(1023) != 1) {
    EEPROM.write(1023, 1); // eeprom 저장 플래그 

  } else { 

  }


  pinMode(SWITCH, INPUT);
  sw_state = digitalRead(SWITCH);
  
  if (sw_state) {
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    //If connection successful show IP address in serial monitor
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  } else {
    const char* ssid_ap = "iTACT_Module_01";
    const char* pass_ap = "";
    WiFi.softAP(ssid_ap, pass_ap);
    server.on("/", handleRoot);
    server.begin();                  //Start server
    Serial.print("AP Start");

    //If connection successful show IP address in serial monitor
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());  //IP address assigned to your ESP
  }
}

void loop() {
  if (sw_state) {
    WiFiClient client;

    Serial.printf("\n[Connecting to %s ... ", host);
    if (client.connect(host, 3333)) {
      Serial.println("connected]");

      Serial.println("[Sending a request]");
      client.print(String("GET /") + " data " + "\r\n" +
                  "Host: " + host + "\r\n" +
                  "Device: " + device + "\r\n");

      Serial.println("[Response:]");
      while (client.connected() || client.available()) {
        if (client.available()) {
          String line = client.readStringUntil('\n');
          Serial.println(line);
        }
      }
      client.stop();
      Serial.println("\n[Disconnected]");
    }
    else {
      Serial.println("connection failed!]");
      client.stop();
    }
    delay(5000);
  } else {
    server.handleClient();          //Handle client requests
  } 
}