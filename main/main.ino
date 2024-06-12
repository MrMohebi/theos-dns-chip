#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <String.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include "./DNSServer.h"
#include "coap-simple.h"

WiFiUDP udp;
Coap coap(udp);

const char *AP_ssid = "Theos_DNS";
const char *AP_password = "123654789";

bool IS_CONNECTED_TO_WIFI = false;

struct {
  char WIFI_ssid[32] = "";
  char WIFI_password[32] = "";
  char token[32] = "";
  char server_ip[32] = "";  // with port of https dns server, example: 10.10.10.10:1443
  char are_data_ok[3] = "ok";
} _settings;

IPAddress _serverIp;

DNSServer _dnsServer;

const char *_serverAuthPort = "8005";
const char *_serverIpAskPort = "1234";

IPAddress _lastClinetIp;


WebServer _server(80);

void handle_index();

void handle_saveSettings();

void handle_notFound();


bool has_wifi_or_connect();


char *prepareServerName(const char *port);

IPAddress localIp();

String announceServerMyIp();

void registerMyIp( void * pvParameters );

TaskHandle_t registerMyIp_task;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n\n");
  delay(100);

  EEPROM.begin(sizeof(_settings));
  unsigned int addr = 0;
  EEPROM.get(addr, _settings);

  Serial.println(_settings.server_ip);

  if (!has_wifi_or_connect()) {

    IPAddress local_ip(10, 90, 90, 10);
    IPAddress gateway(10, 90, 90, 10);
    IPAddress subnet(255, 255, 255, 0);

    Serial.println("Setting AP (Access Point)…");
    WiFi.softAP(AP_ssid, AP_password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

    _server.on("/", HTTP_GET, handle_index);
    _server.on("/saveSettings", HTTP_POST, handle_saveSettings);

    _server.onNotFound(handle_notFound);

    _server.begin();
    Serial.println("HTTP server started");

  } else {
    Serial.println("wifi founded");

    String serverDnsAddress = String(_settings.server_ip);
    String serverIp = serverDnsAddress.substring(0, serverDnsAddress.indexOf(":"));
    _serverIp.fromString(serverIp);

    Serial.println("-------------------------");
    if (_dnsServer.start((const byte)53, _serverIp)) {
      Serial.println("DNS server started");
    }
    Serial.println("-------------------------");

    _dnsServer.setCOAP(&coap);
  }


    xTaskCreatePinnedToCore(
                    registerMyIp,        /* Task function. */
                    "registerMyIp",      /* name of task. */
                    10000,               /* Stack size of task */
                    NULL,                /* parameter of the task */
                    1,                   /* priority of the task */
                    &registerMyIp_task,  /* Task handle to keep track of created task */
                    0);                  /* pin task to core 0 */   

}

void loop() {

  _dnsServer.checkToResponse();

  if (!has_wifi_or_connect()) {
    _server.handleClient();
  }

  // sleepms(1);
  // Serial.print("getFreeHeap: ");
  // Serial.println(ESP.getFreeHeap());
  // Serial.print("getMinFreeHeap: ");
  // Serial.println(ESP.getMinFreeHeap());
}

void registerMyIp( void * pvParameters ){
  Serial.print("registerMyIp loop core =>");
  Serial.println(xPortGetCoreID());
  for(;;){
    if(has_wifi_or_connect()){
      IPAddress askedIp;
      askedIp = localIp();

      if (_lastClinetIp != askedIp) {
        String res = announceServerMyIp(askedIp);
        if (res == "already added" || res == "added") {
          for(int i = 0; i < 4; i++){
            _lastClinetIp[i] = askedIp[i];
          }
          Serial.println("my ip is Saved and is authorized");
        }
      }
    }
    delay(30000);
  } 
}

char *prepareServerName(const char *port) {
  char *serverName;
  serverName = (char *)malloc(
    strlen((char *)"http://") + strlen(_serverIp.toString().c_str()) + strlen((char *)":") + strlen(port) + 1);
  strcpy(serverName, (char *)"http://");
  strcat(serverName, _serverIp.toString().c_str());
  strcat(serverName, (char *)":");
  strcat(serverName, port);

  return serverName;
}


String announceServerMyIp(IPAddress &newIp) {
  char *baseServerName = prepareServerName(_serverAuthPort);
  String pathAndParams = "/tap-in?token=" + String(_settings.token) + "&ip=" + newIp.toString();
  String serverName = String(baseServerName) + pathAndParams;

  HTTPClient http;
  http.begin(serverName.c_str());

  int httpResponseCode = http.GET();

  String payload = "";
  if (httpResponseCode > 0) {
    payload = http.getString();
  } else {
    Serial.print("Error code to announce new ip: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  free(baseServerName);

  return payload;
}


IPAddress localIp() {
  char *serverName = prepareServerName(_serverIpAskPort);
  IPAddress ip ;

  HTTPClient http;
  http.begin(serverName);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    ip.fromString(http.getString());
  } else {
    Serial.print("Error code to get local ip: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  free(serverName);

  return ip;
}


// ---------------  web server stuff ----------------
void handle_notFound() {
  _server.send(404, "text/plain", "Not found");
}

void handle_index() {
  String index = "<!DOCTYPE html> <html lang='en'> <head> <meta charset='UTF-8'> <title>Theos DNS</title> <style> html { font-family: Helvetica, sans-serif; display: inline-block; margin: 0 auto; text-align: center; color: #a9a9a9; } body { font-size: 3rem; display: flex; flex-direction: column; align-items: center; background-color: #222222; overflow-x: hidden; } h1 { color: #a9a9a9; margin: 50px auto 30px; } h3 { color: #a9a9a9; margin-bottom: 50px; } p { font-size: 14px; color: #888; margin-bottom: 10px; } .button { display: block; width: 200px; background-color: #1abc9c; border: none; color: white; padding: 13px 30px; text-decoration: none; font-size: 2rem; margin: 0 auto 35px; cursor: pointer; border-radius: 4px; } input[type=text] { border: 2px solid #aaa; border-radius: 4px; margin: 8px 0; outline: none; padding: 8px; box-sizing: border-box; transition: .3s; background: #222222; color: #a9a9a9; font-size: 2.5rem; max-width: 500px; width: 500px; } input[type=text]:focus { border-color: #1abc9c; box-shadow: 0 0 8px 0 #1abc9c; } .content-center-row { display: flex; flex-direction: row; align-content: center; justify-content: center; } label { white-space: nowrap; text-align: center; display: flex; align-items: center; width: 100%; font-size: 2rem; } .space-x-4 > * { margin-right: 1rem; } a{ color: #1abc9c; } </style> </head> <body> <h1>Theos DNS</h1> <h3> <span>Support: </span> <a target='_blank' href='https://t.me/theos_dns'>@theos_dns</a> </h3> <div style='width: 90%;margin-bottom: 100px'> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='server_ip'>Server IP:</label> <input id='server_ip' type='text' placeholder='111.111.111.111' required> </div> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='token'>Token:</label> <input id='token' type='text' placeholder='xxxxxxxxxxxxxx' required> </div> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='wifi_ssid'>Wifi ssid:</label> <input id='wifi_ssid' type='text' placeholder='my wifi' required> </div> <div class='content-center-row space-x-4' style=''> <label for='wifi_password'>Wifi Password:</label> <input id='wifi_password' type='text' placeholder='p@ssw0rd' required> </div> </div> <button class='button' onclick='onSubmit()'>Submit</button> <script> function onSubmit() { let formData = new FormData(); formData.append('serverIp', document.getElementById('server_ip').value); formData.append('token', document.getElementById('token').value); formData.append('ssid', document.getElementById('wifi_ssid').value); formData.append('password', document.getElementById('wifi_password').value); fetch('/saveSettings', { method: 'POST', body: formData }).then((res)=>{ res.text().then(result=>{ alert(result); if(result === 'ok'){ document.body.innerHTML = 'Ok'; } }) }); } </script> </body> </html>";
  _server.send(200, "text/html", index);
}

void handle_saveSettings() {
  if (
    !_server.hasArg("token") || !_server.hasArg("serverIp") || !_server.hasArg("ssid") || !_server.hasArg("ssid") || _server.arg("token") == NULL || _server.arg("serverIp") == NULL || _server.arg("ssid") == NULL || _server.arg("password") == NULL || sizeof(_server.arg("token")) >= 32 || sizeof(_server.arg("serverIp")) >= 32 || sizeof(_server.arg("ssid")) >= 32 || sizeof(_server.arg("password")) >= 32) {
    _server.send(400, "text/plain", "400: Invalid Request");
    return;
  }
  strcpy(_settings.server_ip, _server.arg("serverIp").c_str());
  strcpy(_settings.token, _server.arg("token").c_str());
  strcpy(_settings.WIFI_ssid, _server.arg("ssid").c_str());
  strcpy(_settings.WIFI_password, _server.arg("password").c_str());

  strncpy(_settings.are_data_ok, "ok", sizeof("ok"));

  unsigned int addr = 0;
  EEPROM.put(addr, _settings);
  EEPROM.commit();
  Serial.println("settings has been saved");

  _server.send(200, "text/plain", "ok");
  delay(1000);

  ESP.restart();
}
// --------------------------------------------------------


bool has_wifi_or_connect() {
  if (!(_settings.are_data_ok[0] == 'o' && _settings.are_data_ok[1] == 'k')) {
    return false;
  }
  if (IS_CONNECTED_TO_WIFI) {
    return true;
  }

  WiFi.begin(_settings.WIFI_ssid, _settings.WIFI_password);
  Serial.print("Connecting to ");
  Serial.print(_settings.WIFI_ssid);
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 15) {
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println('\n');
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    IS_CONNECTED_TO_WIFI = true;
    return true;
  } else {
    Serial.println('\n');
    Serial.println("Couldn't establish connection!");

    strncpy(_settings.are_data_ok, "no", sizeof("no"));
  }

  return false;
}
