#include <Arduino.h>
#include <pitches.h>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define LED 2
#define BTN A0
#define RELAY_1 5
#define RELAY_2 12
#define RELAY_3 13
#define RELAY_4 14

int connect_cnt = 0;

char ssid[20] = "SSID";
char pass[20] = "PASSWORD";
char host[20] = "HOST IP";
char dev_name[20] = "DEVICE NAME";

char PAGE_TEST_1[] = "Hi";
char PAGE_TEST_2[] = "Bye";

const char PAGE_CONFIG[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no">
    <title>WiFi Relay Module Config Page</title>
</head>
    
<style>

    html,body{margin: 0; padding: 0; border: 0; font-size: 100%; font: inherit; vertical-align: baseline; width: 100%; height: 100%;}
    .wrap{position: absolute; top: 50%; left: 50%; width: 300px; height:470px; margin-left: -150px; margin-top: -235px;}
    h1{text-align: center; margin-bottom: 30px;}
    table{width: 300px; margin-bottom: 30px;}
    tbody tr{height: 50px;}
    tbody tr td{text-align: left; font-weight: 800;}
    input{width: 166px; height: 20px; margin-left: 20px;}
    button{width: 300px; height: 30px; margin-bottom: 30px; font-weight: 800;}
    p{font-size: 12px; text-align: center;}
</style>    

<body>
    
    <div class="wrap">
        <h1>WiFi Relay Module Config Page</h1>
        <form method="post" action="/confirm">
        
            <table>
            
                <tbody>
                
                    <tr><td>SSID</td><td><input type="text" name="ssid"></td></tr>
                    <tr><td>Password</td><td><input type="password" name="password"></td></tr>
                    <tr><td>Gateway IP</td><td><input type="text" name="gateway_ip"></td></tr>
                    <tr><td>Device Name</td><td><input type="text" name="device_name"></td></tr>
                
                </tbody>
                
            </table>
            
            <button type="submit">SUBMIT</button>
            
            <p>Copyright © 2021 iTACT inc. All rights reserved.</p>
        </form>
    
    </div>
    
</body>
</html>
)=====";

const char PAGE_COMPLETE[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no">
    <title>WiFi Relay Module Config Page</title>
</head>
    
<style>

    html,body{margin: 0; padding: 0; border: 0; font-size: 100%; font: inherit; vertical-align: baseline; width: 100%; height: 100%;}
    .wrap{position: absolute; top: 50%; left: 50%; width: 300px; height:470px; margin-left: -150px; margin-top: -235px;}
    h1{text-align: center; margin-bottom: 30px;}
    h2{text-align: center; font-size:16px;}
    table{width: 300px; margin-bottom: 30px;}
    tbody tr{height: 50px;}
    tbody tr td{text-align: left; font-weight: 800;}
    input{width: 166px; height: 20px; margin-left: 20px;}
    button{width: 300px; height: 30px; margin-bottom: 30px; font-weight: 800;}
    p{font-size: 12px; text-align: center;}
</style>    

<body>
    
    <div class="wrap">
        <h1>Module Restart</h1>

        <h2>모듈이 자동으로 재시작됩니다.</h2>
          
        <p>Copyright © 2021 iTACT inc. All rights reserved.</p>
    
    </div>
    
</body>
</html>
)=====";


ESP8266WebServer server(80); //Server on port 80
WiFiClient client;

int sw_state = LOW;
boolean connect_flag = true;

String input_ssid;
String input_pass;
String input_gateway_ip;
String input_device_name;

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

void ledBlink(int cnt, int delay_time) {
  for (int i = 0; i < cnt; i++) {
    digitalWrite(LED, LOW);
    delay(delay_time);
    digitalWrite(LED, HIGH);
    delay(delay_time);
  }
}

void deviceReset() {
    delay(1000);
    ledBlink(10, 50);
    delay(1000);
    ESP.reset();  // ESP Reset
}

void eeprom_reset() {
  delay(500);
  for (int i = 0; i < int(EEPROM.length()); i++) {
    EEPROM.write(i, 0);
    if (i % 2 == 0) ledBlink(1, 20);
  }
  EEPROM.commit();
  Serial.println("EEPROM RESET");
  delay(500);
}

void handleRoot() {
  String page = PAGE_CONFIG;
  server.send(200, "text/html", page);
}

void handleConfirm() {
  input_ssid = server.arg("ssid");
  input_pass = server.arg("password");
  input_gateway_ip = server.arg("gateway_ip");
  input_device_name = server.arg("device_name");
  
  String page;
  page += R"=====(
    <!DOCTYPE HTML>
    <html lang="ko">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no">
        <title>WiFi Relay Module Config Page</title>
    </head>
        
    <style>

        html,body{margin: 0; padding: 0; border: 0; font-size: 100%; font: inherit; vertical-align: baseline; width: 100%; height: 100%;}
        .wrap{position: absolute; top: 50%; left: 50%; width: 300px; height:470px; margin-left: -150px; margin-top: -235px;}
        h1{text-align: center; margin-bottom: 30px;}
        table{width: 300px; margin-bottom: 30px;}
        tbody tr{height: 50px;}
        tbody tr td{text-align: left; font-weight: 800;}
        input{width: 166px; height: 20px; margin-left: 20px;}
        button{width: 300px; height: 30px; margin-bottom: 30px; font-weight: 800;}
        p{font-size: 12px; text-align: center;}
    </style>

    <body>
        
        <div class="wrap">
            <h1>Confirm Infomation</h1>
            
            
            <table>

                <tbody>)=====";

  page += "<tr><td>SSID</td><td>" + input_ssid + "</td></tr>";
  page += "<tr><td>Password</td><td>" + input_pass + "</td></tr>";
  page += "<tr><td>Gateway IP</td><td>" + input_gateway_ip + "</td></tr>";
  page += "<tr><td>Device Name</td><td>" + input_device_name + "</td></tr>";
  page += R"=====(
                </tbody>

            </table>

            <button onclick="location.href='/complete'">OK</button>
            <button onclick="location.href='/'">BACK</button>

            <p>Copyright © 2021 iTACT inc. All rights reserved.</p>
        
        </div>
        
    </body>
    </html>)=====";
  
  server.send(200, "text/html", page);
}

void handleComplete() {
  eeprom_write_line(100, input_ssid);
  eeprom_write_line(200, input_pass);
  eeprom_write_line(300, input_gateway_ip);
  eeprom_write_line(400, input_device_name);

  String page = PAGE_COMPLETE;
  server.send(200, "text/html", page);
  delay(1000);
  ledBlink(10, 50);
  delay(1000);
  ESP.reset();  // ESP Reset
}

void setup() {

  Serial.begin(115200);
  Serial.println("WiFi_Relay_Board");

  EEPROM.begin(512);
  delay(200);

  int rs = ESP.random();
  randomSeed(rs);
  Serial.printf("Random Seed : %d\n", rs);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pinMode(BTN, INPUT);
  pinMode(RELAY_1, OUTPUT);
  digitalWrite(RELAY_1, HIGH);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_2, HIGH);
  pinMode(RELAY_3, OUTPUT);
  digitalWrite(RELAY_3, HIGH);
  pinMode(RELAY_4, OUTPUT);
  digitalWrite(RELAY_4, HIGH);

  // Serial.printf("RELAY_1 OFF");
  // digitalWrite(RELAY_1, HIGH);
  // delay(1000);
  // Serial.printf("RELAY_1 ON");
  // digitalWrite(RELAY_1, LOW);
  // delay(1000);
  // Serial.printf("RELAY_1 OFF");
  // digitalWrite(RELAY_1, HIGH);
  // delay(1000);

  ledBlink(10, 50);

  sw_state = analogRead(BTN);
  delay(200);

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
    server.on("/confirm", handleConfirm);
    server.on("/complete", handleComplete);
    server.begin(); // Start server
    Serial.println("\nAP Start");
    Serial.print("AP IP address: ");
    Serial.println(ssid_ap);
    // Serial.println(pass_ap);
    Serial.println(WiFi.softAPIP()); // AP IP
    digitalWrite(LED, LOW);
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
    deviceReset();
  }
}

void loop() {
  if (connect_flag) {
    Serial.printf("\n[Connecting to %s ... ", host);
    int rt = random(0, 5000);
    delay(rt);
    if (client.connect(host, 3333)) {
      connect_cnt = 0;
      digitalWrite(LED, LOW);
      Serial.println("connected]");
      Serial.printf("Random Delay : %dms\n", rt);

      Serial.println("[Sending a request]");
      client.print("Host: " + String(host) + "\r\n" +
                   "Device: " + String(dev_name) + "\r\n");

      while (client.connected() || client.available()) {
        if (client.available()) {
          ledBlink(2, 50);
          String line = client.readStringUntil('\n');
          Serial.print("[Response] : ");
          Serial.println(line);
          cmd_split(line);
          digitalWrite(LED, LOW);
        }
      }
      client.stop();
      Serial.println("\n[Disconnected]");
      digitalWrite(LED, HIGH);
    } else {
      ledBlink(10, 25);
      Serial.println("connection failed!]");
      Serial.printf("Random Delay : %dms\n", rt);
      client.stop();
      connect_cnt++;
      if (connect_cnt > 10) {
        Serial.println("The connection to the server has been lost.");
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, HIGH);
      }
    }
    delay(5000);
  } else {
    server.handleClient(); //Handle client requests
  } 
}