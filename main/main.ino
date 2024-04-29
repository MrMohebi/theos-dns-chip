#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <String.h>
#include <EEPROM.h>


const char* AP_ssid     = "Theos_DNS";
const char* AP_password = "123654789";

bool IS_CONNECTED_TO_WIFI = false;

struct { 
    char WIFI_ssid[32] = "";
    char WIFI_password[32] = "";
    char token[32] = "";
    char server_ip[16] = "";
    char are_data_ok[3] = "ok";
} settings;


ESP8266WebServer server(80);


void handle_index();
void handle_saveSettings();
void handle_notFound();


bool has_wifi_or_connect();



void setup() {
  Serial.begin(115200);
  Serial.println("\n\n\n");
  delay(100);

  EEPROM.begin(sizeof(settings));
  unsigned int addr = 0;
  EEPROM.get(addr, settings);


  Serial.println(settings.are_data_ok[0]);
  Serial.println(settings.are_data_ok[1]);
  Serial.println(settings.server_ip);

  if(!has_wifi_or_connect()){

    IPAddress local_ip(192,168,1,1);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);

    Serial.println("Setting AP (Access Point)…");
    WiFi.softAP(AP_ssid, AP_password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

    server.on("/",HTTP_GET, handle_index);
    server.on("/saveSettings",HTTP_POST, handle_saveSettings);

    server.onNotFound(handle_notFound);

    server.begin();
    Serial.println("HTTP server started");

  }else{
    Serial.println("wifi founded");
  }
}

void loop() {
  if(!has_wifi_or_connect()){
    server.handleClient();
  }

}




// ---------------  web server stuff ----------------
void handle_notFound(){
  server.send(404, "text/plain", "Not found");
}

void handle_index() {
  String index = "<!DOCTYPE html> <html lang='en'> <head> <meta charset='UTF-8'> <title>Theos DNS</title> <style> html { font-family: Helvetica, sans-serif; display: inline-block; margin: 0 auto; text-align: center; color: #a9a9a9; } body { font-size: 3rem; display: flex; flex-direction: column; align-items: center; background-color: #222222; overflow-x: hidden; } h1 { color: #a9a9a9; margin: 50px auto 30px; } h3 { color: #a9a9a9; margin-bottom: 50px; } p { font-size: 14px; color: #888; margin-bottom: 10px; } .button { display: block; width: 200px; background-color: #1abc9c; border: none; color: white; padding: 13px 30px; text-decoration: none; font-size: 2rem; margin: 0 auto 35px; cursor: pointer; border-radius: 4px; } input[type=text] { border: 2px solid #aaa; border-radius: 4px; margin: 8px 0; outline: none; padding: 8px; box-sizing: border-box; transition: .3s; background: #222222; color: #a9a9a9; font-size: 2.5rem; max-width: 500px; width: 500px; } input[type=text]:focus { border-color: #1abc9c; box-shadow: 0 0 8px 0 #1abc9c; } .content-center-row { display: flex; flex-direction: row; align-content: center; justify-content: center; } label { white-space: nowrap; text-align: center; display: flex; align-items: center; width: 100%; font-size: 2rem; } .space-x-4 > * { margin-right: 1rem; } a{ color: #1abc9c; } </style> </head> <body> <h1>Theos DNS</h1> <h3> <span>Support: </span> <a target='_blank' href='https://t.me/theos_dns'>@theos_dns</a> </h3> <div style='width: 90%;margin-bottom: 100px'> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='server_ip'>Server IP:</label> <input id='server_ip' type='text' placeholder='111.111.111.111' required> </div> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='token'>Token:</label> <input id='token' type='text' placeholder='xxxxxxxxxxxxxx' required> </div> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='wifi_ssid'>Wifi ssid:</label> <input id='wifi_ssid' type='text' placeholder='my wifi' required> </div> <div class='content-center-row space-x-4' style=''> <label for='wifi_password'>Wifi Password:</label> <input id='wifi_password' type='text' placeholder='p@ssw0rd' required> </div> </div> <button class='button' onclick='onSubmit()'>Submit</button> <script> function onSubmit() { let formData = new FormData(); formData.append('serverIp', document.getElementById('server_ip').value); formData.append('token', document.getElementById('token').value); formData.append('ssid', document.getElementById('wifi_ssid').value); formData.append('password', document.getElementById('wifi_password').value); fetch('/saveSettings', { method: 'POST', body: formData }).then((res)=>{ res.text().then(result=>{ alert(result); if(result === 'ok'){ document.body.innerHTML = 'Ok'; } }) }); } </script> </body> </html>";
  server.send(200, "text/html", index); 
}

void handle_saveSettings() {
  if( 
    !server.hasArg("token") || !server.hasArg("serverIp") || !server.hasArg("ssid") || !server.hasArg("ssid") ||
     server.arg("token") == NULL || server.arg("serverIp") == NULL || server.arg("ssid") == NULL || server.arg("password") == NULL ||
     sizeof(server.arg("token")) >= 32 || sizeof(server.arg("serverIp")) >= 32 || sizeof(server.arg("ssid")) >= 32 || sizeof(server.arg("password")) >= 32
      ) { 
    server.send(400, "text/plain", "400: Invalid Request");
    return;
  }
  strcpy(settings.server_ip, server.arg("serverIp").c_str());
  strcpy(settings.token, server.arg("token").c_str());
  strcpy(settings.WIFI_ssid, server.arg("ssid").c_str());
  strcpy(settings.WIFI_password, server.arg("password").c_str());

  strncpy(settings.are_data_ok, "ok", sizeof("ok"));

  unsigned int addr = 0;
  EEPROM.put(addr, settings); 
  EEPROM.commit();
  Serial.println("settings has been saved");

  server.send(200, "text/plain","ok");
  delay(1000);

  ESP.restart();
}
// --------------------------------------------------------


bool has_wifi_or_connect(){
  if(!(settings.are_data_ok[0] == 'o' && settings.are_data_ok[1] == 'k')){
    return false;
  }
  if(IS_CONNECTED_TO_WIFI){
    return true;
  }

  WiFi.begin(settings.WIFI_ssid, settings.WIFI_password);           
  Serial.print("Connecting to ");
  Serial.print(settings.WIFI_ssid); Serial.println(" ...");
  
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 15) { 
    delay(1000);
    Serial.print(++i); Serial.print(' '); 
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println('\n');
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());  
    IS_CONNECTED_TO_WIFI = true;
    return true;
  }else{
    Serial.println('\n');
    Serial.println("Couldn't establish connection!");

    strncpy(settings.are_data_ok, "no", sizeof("no"));
  }

  return false;
 
}


