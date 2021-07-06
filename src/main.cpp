#include <Arduino.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
//ESP Web Server Library to host a web page
#include <ESP8266WebServer.h>

#define LED 2
#define SWITCH 12
#define RELAY 13

char ssid[20] = "WBML_229_2.4G";
char pass[20] = "wbml0229";
char host[20] = "192.168.0.9";
char dev_name[20] = "Relay_01";

// char ssid[20] = "WBML_229_2.4G";
// char pass[20] = "wbml0229";
// char host[20] = "192.168.0.9";
// char dev_name[20] = "Relay_01";

const char PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<center>
<h1>WiFi Relay Module Config Page</h1><br>
<form method="post" action="/config">
SSID <input type="text" name="ssid"><br>
Password <input type="password" name="password"><br>
Gateway IP <input type="text" name="gateway_ip"><br>
Device Name <input type="text" name="device_name"><br>
<button type="submit">SUBMIT</button>
</form>
</center>
</body>
</html>
)=====";

ESP8266WebServer server(80); //Server on port 80

int sw_state = LOW;

String eeprom_read_line(int position) { // line 20byte
  char eeprom_data[20];
  for (int i = 0; i < 20; i++) {
    eeprom_data[i] = EEPROM.read(position + i);
    Serial.print(eeprom_data[i]);
  }
  return eeprom_data;
}

void eeprom_write_line(int position, String data_str) { // line 20byte
  char eeprom_data[20];
  strcpy(eeprom_data, data_str.c_str());
  for (int i = 0; i < 20; i++) {
    EEPROM.write(position + i, eeprom_data[i]);
    EEPROM.commit();
  }
}

void handleRoot() {
  String page = PAGE;
  server.send(200, "text/html", page);
}

void handleConfig() {
  String input_ssid = server.arg("ssid");
  String input_pass = server.arg("password");
  String input_gateway_ip = server.arg("gateway_ip");
  String input_device_name = server.arg("device_name");

  eeprom_write_line(100, input_ssid);
  eeprom_write_line(200, input_pass);
  eeprom_write_line(300, input_gateway_ip);
  eeprom_write_line(400, input_device_name);
  
  server.send(200, "text/html", "Config OK\nSSID : " + input_ssid + "\n" + 
                    "PASS : " + input_pass + "\n" + 
                    "HOST : " + input_gateway_ip + "\n" + 
                    "DEVICE : " + input_device_name);
}

void setup() {
  Serial.begin(115200);
  Serial.println("WiFi_Relay_Board");

  EEPROM.begin(512);
  
  if (EEPROM.read(100) != 0) { // Data confirm
    String ssid_str = eeprom_read_line(100);
    String pass_str = eeprom_read_line(200);
    String ip_str = eeprom_read_line(300);
    String device_str = eeprom_read_line(400);

    strcpy(ssid, ssid_str.c_str());
    strcpy(pass, pass_str.c_str());
    strcpy(host, ip_str.c_str());
    strcpy(dev_name, device_str.c_str());
  }
  
  pinMode(SWITCH, INPUT);
  sw_state = digitalRead(SWITCH);
  Serial.println(sw_state);
  
  if (sw_state) {
    Serial.print("Connecting to " + String(ssid));
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    const char* ssid_ap = "iTACT_Module_01";
    const char* pass_ap = "";
    WiFi.softAP(ssid_ap, pass_ap);
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.begin(); // Start server
    Serial.print("AP Start");

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP()); // AP IP
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
                  "Device: " + dev_name + "\r\n");

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
    server.handleClient(); //Handle client requests
  } 
}