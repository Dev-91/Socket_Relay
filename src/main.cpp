#include <Arduino.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define LED 2
#define BTN A0
#define RELAY_1 5
#define RELAY_2 12
#define RELAY_3 13
#define RELAY_4 14


char ssid[20] = "SSID";
char pass[20] = "PASSWORD";
char host[20] = "HOST IP";
char dev_name[20] = "DEVICE NAME";

char PAGE_TEST_1[] = "Hi";
char PAGE_TEST_2[] = "Bye";

const char PAGE_HEAD[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no">
    <title>WiFi Relay Module Config Page</title>
</head>
)=====";
    
const char PAGE_STYLE[] PROGMEM = R"=====(
<style>

    html,body{margin: 0; padding: 0; border: 0; font-size: 100%; font: inherit; vertical-align: baseline; width: 100%; height: 100%;}
    .wrap{position: absolute; top: 50%; left: 50%; width: 300px; margin-left: -150px; margin-top: -150px;}
    h1{text-align: center; margin-bottom: 30px;}
    table{width: 300px; margin-bottom: 30px;}
    tbody tr{height: 50px;}
    tbody tr td{text-align: left; font-weight: 800;}
    input{width: 166px; height: 20px; margin-left: 20px;}
    button{width: 300px; height: 30px; margin-bottom: 30px; font-weight: 800;}
    p{font-size: 12px; text-align: center;}
</style>
)=====";

const char PAGE_BODY_TOP[] PROGMEM = R"=====(
<body>
    
    <div class="wrap">
)=====";

const char PAGE_BODY_BOTTOM[] PROGMEM = R"=====(
            <p>Copyright © 2021 iTACT inc. All rights reserved.</p>
        </form>
    
    </div>
    
</body>
</html>
)=====";


const char PAGE_BODY_CONFIG[] PROGMEM = R"=====(
        <h1>WiFi Relay Module Config Page</h1>
        <form method="post" action="/config">
        
            <table>
            
                <tbody>
                
                    <tr><td>SSID</td><td><input type="text" name="ssid"></td></tr>
                    <tr><td>Password</td><td><input type="password" name="password"></td></tr>
                    <tr><td>Gateway IP</td><td><input type="text" name="gateway_ip"></td></tr>
                    <tr><td>Device Name</td><td><input type="text" name="device_name"></td></tr>
                
                </tbody>
                
            </table>
            
            <button type="submit">SUBMIT</button>
)=====";

const char PAGE_BODY_CONFIRM_TOP[] PROGMEM = R"=====(
<h1>Confirm Infomation</h1>
        
        
        <table>

            <tbody>

)=====";

const char PAGE_BODY_CONFIRM_BOTTOM[] PROGMEM = R"=====(
            </tbody>

        </table>

        <button onclick="location.href='/config'">BACK</button>
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
  delay(500);
  for (int i = 0; i < int(EEPROM.length()); i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("EEPROM RESET");
  delay(500);
}

void handleRoot() {
  String page;
  page += PAGE_TEST_1;
  page += PAGE_TEST_2;
  // page += PAGE_HEAD;
  // page += PAGE_STYLE;
  // page += PAGE_BODY_TOP;
  // page += PAGE_BODY_CONFIG;
  // page += PAGE_BODY_BOTTOM;

  server.send(200, "text/html", page);
}

void handleConfirm() {

  String page;
  String input_ssid = server.arg("ssid");
  String input_pass = server.arg("password");
  String input_gateway_ip = server.arg("gateway_ip");
  String input_device_name = server.arg("device_name");

  eeprom_write_line(100, input_ssid);
  eeprom_write_line(200, input_pass);
  eeprom_write_line(300, input_gateway_ip);
  eeprom_write_line(400, input_device_name);

  // page += PAGE_HEAD;
  // page += PAGE_STYLE;
  // page += PAGE_BODY_TOP;
  // page += PAGE_BODY_CONFIRM_TOP;
  // page += "<tr><td>SSID</td><td>" + input_ssid + "</td></tr>";
  // page += "<tr><td>Password</td><td>" + input_pass + "</td></tr>";
  // page += "<tr><td>Gateway IP</td><td>" + input_gateway_ip + "</td></tr>";
  // page += "<tr><td>Device Name</td><td>" + input_device_name + "</td></tr>";
  // page += PAGE_BODY_CONFIRM_BOTTOM;
  // page += PAGE_BODY_BOTTOM;
  
  server.send(200, "text/html", page);
}

void ledBlink(int cnt) {
  for (int i = 0; i < cnt; i++) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }
}

void setup() {
  ledBlink(5);

  Serial.begin(115200);
  Serial.println("WiFi_Relay_Board");

  EEPROM.begin(512);
  delay(200);

  pinMode(BTN, INPUT);
  pinMode(RELAY_1, OUTPUT);
  digitalWrite(RELAY_1, HIGH);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_2, HIGH);
  pinMode(RELAY_3, OUTPUT);
  digitalWrite(RELAY_3, HIGH);
  pinMode(RELAY_4, OUTPUT);
  digitalWrite(RELAY_4, HIGH);

  sw_state = analogRead(BTN);
  delay(100);

  if (sw_state > 900) {
    eeprom_reset();
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
    server.on("/config", handleConfirm);
    server.begin(); // Start server
    Serial.println("\nAP Start");
    Serial.print("AP IP address: ");
    Serial.println(ssid_ap);
    Serial.println(pass_ap);
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
