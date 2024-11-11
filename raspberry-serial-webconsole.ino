/* ------------------------------------------------- */
#include <WebServer.h>
#include "ESPTelnetStream.h"
#define ESP32_WIFI_TX_POWER WIFI_POWER_5dBm  // not required to have more power, remove if you need more
/* ------------------------------------------------- */


#define SERIAL_SPEED 115200
#define WIFI_SSID "<YOUR-SSID>"
#define WIFI_PASSWORD "<YOUR-WIFI-PASSWORD>"
#define BUFFERSIZE 10000
/* ------------------------------------------------- */
int buff;
ESPTelnetStream telnet;
WebServer server(80);
IPAddress ip;
uint16_t port = 23;
TaskHandle_t Task1;
TaskHandle_t Task2;

/* ------------------------------------------------- */
// Set here your Static IP address in the range of your local network
IPAddress local_IP(192, 168, 1, 5);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

uint8_t serbuffer[BUFFERSIZE];
int buff_cur = 0;
bool overflow = false;


/* ------------------------------------------------- */

bool isConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

void telnetConnected(String ip) {
  Serial.print("Connected, ready to send the buffer: ");
  Serial.println(buff_cur);
  if ((buff_cur > 0) || (overflow)) {
    for (int i = 0; i < buff_cur; i++) {
      telnet.write(serbuffer[i]);
    }
  }
}

void telnetDisconnected(String ip) {
}

void telnetReconnect(String ip) {
}
/* ------------------------------------------------- */

void errorMsg(String error, bool restart = true) {
  //Serial.println(error);
  if (restart) {
    //Serial.println("Rebooting now...");
    delay(2000);
    ESP.restart();
    delay(2000);
  }
}
/* ------------------------------------------------------------- */

bool connectToWiFi(const char* ssid, const char* password, int max_tries = 20, int pause = 500) {
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  }
  WiFi.begin(ssid, password);
  do {
    delay(pause);
    i++;
  } while (!isConnected() && i < max_tries);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  return isConnected();
}
/* --------------- WEB SERVER ---------------------------------- */
void setup_server() {
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");
}

void handleCommand(String command) {
  Serial.print("Received command: ");
  Serial.println(command);
  buff_cur = 0;
  overflow = false;
  if (command == "ps") {
    Serial1.println("ps -ef");
  } else if (command == "dmesg") {
    Serial1.println("dmesg");
  } else if (command == "ifconfig") {
    Serial1.println("ifconfig -a");
  } else if (command == "reboot") {
    Serial1.println("sudo reboot");
  } else if (command == "cputemp") {
    Serial1.println("vcgencmd measure_temp");
  } else if (command == "uptime") {
    Serial1.println("uptime");
  } else if (command == "last") {
    Serial1.println("last");
  }
  delay(2000);
}

void handleRoot() {
  String command = server.arg("command");
  if (command != NULL) {
    handleCommand(command);
  }
  Serial.println("handleRoot");
  String temp;
  temp = "<html><head>\
      <title>Serial Web Console</title>\
      <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: large; Color: #000088; }\
      </style>\
      <meta http-equiv=\"Pragma\" content=\"no-cache\">\
  </head>\
  <body>\
      <h2>Serial Web Console</h2>\
      <hr>\ 
      <table cellspacing='0' cellpadding='0' border='0'>\
      <tr>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=ifconfig';\" value=\"ifconfig -a\" /></form>\</td>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=dmesg';\" value=\"Dmesg\" /></form>\</td>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=ps';\" value=\"ps -ef\" /></form>\</td>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=reboot';\" value=\"Reboot\" /></form>\</td>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=cputemp';\" value=\"CPU Temp\" /></form>\</td>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=uptime';\" value=\"Uptime\" /></form>\</td>\
      <td><form><input type=\"button\" onclick=\"window.location.href='/?command=last';\" value=\"Last access\" /></form>\</td>\
      </tr>\
      </table>\
      <hr>\ 
      <h3>Output from last command</h3>\
      <form name = 'params' method = 'POST' action = 'exec_bash' >\
      <textarea style = 'font-size: medium; width:100%' rows = '50' placeholder = '";
  if ((buff_cur > 0) || (overflow)) {
    if (overflow) {
      for (int i = buff_cur; i < BUFFERSIZE; i++) {
        temp += (char)serbuffer[i];
      }
      for (int i = 0; i < buff_cur; i++) {
        temp += (char)serbuffer[i];
      }
    } else {
      for (int i = 0; i < buff_cur; i++) {
        temp += (char)serbuffer[i];
      }
    }
  }
  temp += "' name = 'bash'></textarea >\
      <br><br><input type = submit style = 'font-size: large' value = 'Execute' /></ form><hr />";
  server.send(200, "text/html", temp.c_str());
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

/* ------------------------------------------------- */

void setupTelnet() {
  telnet.onConnect(telnetConnected);
  telnet.onReconnect(telnetReconnect);
  telnet.onDisconnect(telnetDisconnected);
  if (telnet.begin(port)) {
  } else {
    errorMsg("Will reboot...");
  }
}

/* ------------------ TASKS ------------------------------ */

void send_Serial(void* arg) {
  const TickType_t xDelay = 25 / portTICK_PERIOD_MS;
  while (true) {
    if (Serial1.available()) {

      while (Serial1.available()) {
        buff = Serial1.read();
        if (telnet.isConnected()) {

          telnet.write(buff);
        } else {
          serbuffer[buff_cur++] = buff;
          Serial.write(buff);
          if (buff_cur > BUFFERSIZE - 1) {
            buff_cur = 0;
            overflow = true;
          }
        }
      }
    }

    telnet.loop();
    vTaskDelay(xDelay);
  }
}

void send_Telnet(void* arg) {
  const TickType_t xDelay = 25 / portTICK_PERIOD_MS;
  while (true) {
    if (telnet.available()) {
      while (telnet.available()) {
        Serial1.write(telnet.read());
      };
      telnet.loop();
      vTaskDelay(xDelay);
    }
  }
}

/* ------------------------------------------------ */

void setup() {
  Serial.begin(SERIAL_SPEED);
  Serial1.begin(115200, SERIAL_8N1, 2, 1);
  pinMode(10, INPUT_PULLUP);
  Serial1.setRxBufferSize(1024);
  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  xTaskCreatePinnedToCore(
    send_Serial,   /* Function to implement the task */
    "send_Serial", /* Name of the task */
    50000,         /* Stack size in words */
    NULL,          /* Task input parameter */
    1,             /* Priority of the task */
    &Task1,        /* Task handle. */
    0);            /* Core where the task should run */

  xTaskCreatePinnedToCore(
    send_Telnet,   /* Function to implement the task */
    "send_Telnet", /* Name of the task */
    50000,         /* Stack size in words */
    NULL,          /* Task input parameter */
    1,             /* Priority of the task */
    &Task2,        /* Task handle. */
    0);            /* Core where the task should run */

  if (isConnected()) {
    setupTelnet();
    setup_server();
  } else {
    errorMsg("Error connecting to WiFi");
  }
}

/* ------------------------------------------------- */

void loop() {

  telnet.loop();
  server.handleClient();
  delayMicroseconds(10);
}

/* ------------------------------------------------- */
