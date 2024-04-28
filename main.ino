#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <String.h>


const char* AP_ssid     = "Theos_DNS";
const char* AP_password = "123654789";

const char* WIFI_ssid = "" ;
const char* WIFI_password = "";

ESP8266WebServer server(80);


String SendHTML(){

  String ptr = "<!DOCTYPE html> <html lang='en'> <head> <meta charset='UTF-8'> <title>Theos DNS</title> <style> html { font-family: Helvetica, sans-serif; display: inline-block; margin: 0 auto; text-align: center; color: #a9a9a9; } body { font-size: 3rem; display: flex; flex-direction: column; align-items: center; background-color: #222222; overflow: hidden; } h1 { color: #a9a9a9; margin: 50px auto 30px; } h3 { color: #a9a9a9; margin-bottom: 50px; } p { font-size: 14px; color: #888; margin-bottom: 10px; } .button { display: block; width: 200px; background-color: #1abc9c; border: none; color: white; padding: 13px 30px; text-decoration: none; font-size: 2rem; margin: 0 auto 35px; cursor: pointer; border-radius: 4px; } input[type=text] { border: 2px solid #aaa; border-radius: 4px; margin: 8px 0; outline: none; padding: 8px; box-sizing: border-box; transition: .3s; background: #222222; color: #a9a9a9; font-size: 2.5rem; max-width: 500px; width: 500px; } input[type=text]:focus { border-color: #1abc9c; box-shadow: 0 0 8px 0 #1abc9c; } .content-center-row { display: flex; flex-direction: row; align-content: center; justify-content: center; } label { white-space: nowrap; text-align: center; display: flex; align-items: center; width: 100%; font-size: 2rem; } .space-x-4 > * { margin-right: 1rem; } a{ color: #1abc9c; } </style> </head> <body> <h1>Theos DNS</h1> <h3> <span>Support: </span> <a target='_blank' href='https://t.me/theos_dns'>@theos_dns</a> </h3> <div style='width: 90%;margin-bottom: 100px'> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='server_ip'>Server IP:</label> <input id='server_ip' type='text' placeholder='111.111.111.111' required> </div> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='token'>Token:</label> <input id='token' type='text' placeholder='xxxxxxxxxxxxxx' required> </div> <div class='content-center-row space-x-4' style='margin-bottom: 10px'> <label for='wifi_ssid'>Wifi ssid:</label> <input id='wifi_ssid' type='text' placeholder='my wifi' required> </div> <div class='content-center-row space-x-4' style=''> <label for='wifi_password'>Wifi Password:</label> <input id='wifi_password' type='text' placeholder='p@ssw0rd' required> </div> </div> <a class='button' href='\''>Submit</a> </body> </html>";
  return ptr;
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void handle_Index() {
  Serial.println("Index opening...");
  server.send(200, "text/html", SendHTML()); 
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n\n");
  delay(100);



  if(strlen(WIFI_ssid) <1){

    IPAddress local_ip(192,168,1,1);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);

    Serial.println("Setting AP (Access Point)…");
    WiFi.softAP(AP_ssid, AP_password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

    server.on("/", handle_Index);

    server.onNotFound(handle_NotFound);

    server.begin();
    Serial.println("HTTP server started");

  }else{
    Serial.println("wifi founded");
  }
}

void loop() {
  if(strlen(WIFI_ssid) <1){
    server.handleClient();
  }

}
