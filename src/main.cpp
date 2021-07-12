#include <Arduino.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define LED 2
#define SWITCH 4
#define RELAY_1 5
#define RELAY_2 12
#define RELAY_3 13
#define RELAY_4 14
// or 10pin

char ssid[20] = "SSID";
char pass[20] = "PASSWORD";
char host[20] = "HOST IP";
char dev_name[20] = "DEVICE NAME";

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
<button type="button" action="/reset>RESET</button>
</center>
</body>
</html>
)=====";

ESP8266WebServer server(80); //Server on port 80
WiFiClient client;

int sw_state = LOW;
boolean connect_flag = true;

String eeprom_read_line(int position) { // line 20byte
  char eeprom_data[20];
  for (int i = 0; i < 20; i++) {
    eeprom_data[i] = EEPROM.read(position + i);
  }
  Serial.println(eeprom_data);
  return eeprom_data;
}

void eeprom_write_line(int position, String data_str) { // line 20byte
  char eeprom_data[20];
  strcpy(eeprom_data, data_str.c_str());
  for (int i = 0; i < 20; i++) {
    EEPROM.write(position + i, eeprom_data[i]);
  }
  EEPROM.commit();
}

void eeprom_reset() {
  for (int i = 0; i < int(EEPROM.length()); i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("EEPROM RESET");
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

void handleReset() {
  eeprom_reset();
  server.send(200, "text/html", "Reset OK\n");
}

void setup() {
  Serial.begin(115200);
  Serial.println("WiFi_Relay_Board");

  EEPROM.begin(512);
  delay(200);

  pinMode(SWITCH, INPUT);
  pinMode(RELAY_1, OUTPUT);
  digitalWrite(RELAY_1, HIGH);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_2, HIGH);
  pinMode(RELAY_3, OUTPUT);
  digitalWrite(RELAY_3, HIGH);
  pinMode(RELAY_4, OUTPUT);
  digitalWrite(RELAY_4, HIGH);

  sw_state = digitalRead(SWITCH);
  Serial.println(sw_state);

  if (sw_state) {
    handleReset();
  }

  delay(200);
  
  if (EEPROM.read(100) != 0) { // Data confirm
    String ssid_str = eeprom_read_line(100);
    String pass_str = eeprom_read_line(200);
    String gwip_str = eeprom_read_line(300);
    String device_str = eeprom_read_line(400);

    strcpy(ssid, ssid_str.c_str());
    strcpy(pass, pass_str.c_str());
    strcpy(host, gwip_str.c_str());
    strcpy(dev_name, device_str.c_str());
  }

  
  delay(200);
  int connect_cnt = 0;
  Serial.print("Connecting to " + String(ssid));
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    connect_cnt++;
    if (connect_cnt == 10) {
      connect_flag = false;
      break;
    }
  }
  
  if (connect_flag) {
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
    server.on("/reset", handleConfig);
    server.begin(); // Start server
    Serial.print("AP Start");

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP()); // AP IP
  }
}

void control_func(String split_data[], int split_cnt) {
  int oper = 255;
  if (split_data[1].indexOf("ON") > -1) {
    oper = LOW;
  } else if (split_data[1].indexOf("OFF") > -1) {
    oper = HIGH;
  }

  int num = split_data[2].toInt();
  if (num == 1) {
    digitalWrite(RELAY_1, oper);
    // Serial.println("RELAY_1 : " + split_data[1]);
  } else if (num == 2) {
    digitalWrite(RELAY_2, oper);
    // Serial.println("RELAY_2 : " + split_data[1]);
  } else if (num == 3) {
    digitalWrite(RELAY_3, oper);
    // Serial.println("RELAY_3 : " + split_data[1]);
  } else if (num == 4) {
    digitalWrite(RELAY_4, oper);
    // Serial.println("RELAY_4 : " + split_data[1]);
  }
}

void cmd_split(String line) {
  int split_cnt = 0;
  char line_data[20];
  String split_data[10];

  strcpy(line_data, line.c_str());
  char* ptr = strtok(line_data, "_");
  split_data[split_cnt] = ptr;
  while (ptr != NULL) {
    // Serial.println(split_data[split_cnt]);
    ptr = strtok(NULL, "_");
    split_cnt++;
    split_data[split_cnt] = ptr;
  }

  String cmd = split_data[0];
  if (cmd.indexOf("CONTROL") > -1) {
    control_func(split_data, split_cnt);
  } else if (cmd.indexOf("RESET") > -1) {

  }
}

void loop() {
  if (connect_flag) {
    Serial.printf("\n[Connecting to %s ... ", host);
    if (client.connect(host, 3333)) {
      Serial.println("connected]");

      Serial.println("[Sending a request]");
      client.print("Host: " + String(host) + "\r\n" +
                   "Device: " + String(dev_name) + "\r\n");

      while (client.connected() || client.available()) {
        if (client.available()) {
          String line = client.readStringUntil('\n');
          Serial.print("[Response] : ");
          Serial.println(line);
          cmd_split(line);
        }
      }
      client.stop();
      Serial.println("\n[Disconnected]");
    } else {
      Serial.println("connection failed!]");
      client.stop();
    }
    delay(5000);
  } else {
    server.handleClient(); //Handle client requests
  } 
}
