#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WebServer.h>
#include "HTTPSRedirect.h"
#include <Servo.h>


const char* ssid = "";
const char* password = "";
const char *GScriptId = "";

const char* twilioAccountSID = "";
const char* twilioAuthToken = "";
const char* twilioPhoneNumber = "";
const char* targetWhatsAppNumber = "";



const long utcOffsetInSeconds = 19800;
WiFiUDP ntpUDP;
WiFiClientSecure client;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
const char* host = "script.google.com";
const int httpsPort = 443;


const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client1 = nullptr;

const int relayPin = D2;
const int relay2 = D1;
int feederServoPin = D3;
Servo feederServo;

// const int relay3 = D3;
// const int relay4 = D4;
String  value0 = String(0);
String  value1 = String(0);
String  value2 = String(0);
int fishflag = 0;
int flag1=0;
int flag2=0;
bool relayState = true;
bool switchstate = false;

ESP8266WebServer server(80);
String html;
void handleRoot() {
  if (relayState) {
    digitalWrite(relay2, HIGH);
  } else {
    digitalWrite(relay2, LOW);
  }
  html = "<html><body><h1 align='center'>Aquarium Control</h1>";
  //html += "<p>Aquarium Light: " + String(relayState ? "On" : "Off") + "</p>";
  // html += "<p>Aquarium Light: ";
  html += "<form action='/on' method='get'><label>Aquarium Light:</label><label><input type='radio' name='relay' value='on' " + String(relayState && switchstate ? "checked" : "") + " onchange='this.form.submit()'>On</label>";
  html += "<label><input type='radio' name='relay' value='off' " + String(relayState && switchstate ? "" : "checked") + " onchange='this.form.submit()'>Off</label>";
  html += "<label><input type='radio' name='relay' value='auto' " + String(!switchstate ? "checked" : "") + " onchange='this.form.submit()'>Auto</label><br><br>";
 // html += "<input type='submit' value='Submit'></form>";
 html += "</form>";
 html += "<form action='/feed' method='get'><button type='submit'>Feed Fish</button></form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleOn() {
   switchstate = true;
  // Serial.println(server.arg("relay"));
  if (server.arg("relay") == "on") {
    relayState = true;
  sendData("Light On", "Manual");
    digitalWrite(relay2, HIGH);
  } else if (server.arg("relay") == "off") {
    relayState = false;
    digitalWrite(relay2, LOW);
    sendData("Light Off", "manual");
  }  else if (server.arg("relay") == "auto") {
    sendData("Light set auto", "Manual");
    switchstate = false;
  }
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}
void handleFeed(){
  sendData("Feeder On", "Manual");
  feedFish();
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(relay2, OUTPUT);
   feederServo.attach(feederServoPin);
  // pinMode(relay3, OUTPUT);
  // pinMode(relay4, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Serial.println("Connected to WiFi");
  // Serial.println(WiFi.localIP());

  client1 = new HTTPSRedirect(httpsPort);
  client1->setInsecure();
  client1->setPrintResponseBody(true);
  client1->setContentTypeHeader("application/json");
    bool flag = false;
  for (int i=0; i<5; i++){ 
    int retval = client1->connect(host, httpsPort);
    if (retval == 1){
       flag = true;
       Serial.println("Connected");
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    return;
  }
  delete client1;    // delete HTTPSRedirect object
  client1 = nullptr; // delete HTTPSRedirect object

  timeClient.begin();
  timeClient.update();

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/feed", handleFeed);

  server.begin();
}

void loop() {
  // sendData("Check", "Auto");
  // Serial.println("here");
  Serial.println(WiFi.localIP());
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
// Serial.println(currentHour);

  if(currentHour == 18 && flag1 == 0){
    relayState = true;
    sendData("Time is "+String(currentHour)+"pm", "Auto");
    sendData("switchstate = " + String(switchstate), "Auto");
    sendData("relayState = " + String(relayState), "Auto");
  }else if(currentHour != 18){
    flag1=0;
  }
    if(currentHour == 22 && flag2==0){
      relayState = false;
    sendData("Time is "+String(currentHour)+"pm", "Auto");
    sendData("switchstate = " + String(switchstate), "Auto");
    sendData("relayState = " + String(relayState), "Auto");
    
  }else if(currentHour != 22){
    flag2=0;
  }
if(!switchstate){
  if(relayState==true){
     if ( (currentHour >=18  && currentHour < 22  ) ) {

      if(flag1==0 && currentHour == 18){
        sendData("Light is On", "Auto");
        flag1=1;
      }
        //  if ( (currentMinute >=07  && currentMinute < 10  ) ) {
      Serial.println("in");
      // sendData("Light On", "Auto");
      digitalWrite(relay2, HIGH); // Turn on the light
      relayState = true;
    } else {
      if(flag2==0 && currentHour == 22){
        sendData("Light is Off", "Auto");
        flag2=1;
      }
      // Serial.println("out");
      digitalWrite(relay2, LOW); // Turn off the light
      relayState = false;
      // sendData("Light Off", "Auto");
    }
  }else{
     if(flag2==0 && currentHour == 22){
        sendData("Light is Off", "Auto");
        flag2=1;
      }
    digitalWrite(relay2, LOW); // Turn off the light
      relayState = false;
      // sendData("Light Off on relay off", "Auto");
  }
  switchstate =false;
}

  if((currentHour == 9 || currentHour == 21) && fishflag == 0){
    sendData("Feeder call", "Auto");
    feedFish();
    fishflag =1;
  }
  if(currentHour == 10 || currentHour == 22){
    fishflag =0;
  }


  server.handleClient();
}
void feedFish() {
  // sendData("Feeder On", "Auto");
  // for (int i = 0; i <1; i++) {
     sendData("Feeder On ", "Auto");
     feederServo.write(0);
     delay(1000);
    feederServo.write(180);  // Move the servo to a specific position (adjust as needed)
    delay(1000);            // Delay to allow the servo to reach the desired position
    feederServo.write(0);   // Move the servo back to the initial position (adjust as needed)
  // }
  sendData("Feeder Off", "Auto");
}
// Subroutine for sending data to Google Sheets
void sendData(const String& msg, const String& type) {
 // create some fake data to publish
  value0 = type;
  value1 = msg;
  // value2 = random(0,100000);


  static bool flag = false;
  if (!flag){
    client1 = new HTTPSRedirect(httpsPort);
    client1->setInsecure();
    flag = true;
    client1->setPrintResponseBody(true);
    client1->setContentTypeHeader("application/json");
  }
  if (client1 != nullptr){
    if (!client1->connected()){
      client1->connect(host, httpsPort);
    }
  }
  else{
    Serial.println("Error creating client object!");
  }
  
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + value0 + "," + value1 +  "\"}"; //"," + value2 +
  
  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if(client1->POST(url, host, payload)){ 
    // do stuff here if publish was successful
  }
  else{
    // do stuff here if publish was not successful
    Serial.println("Error while connecting");
  }

  // a delay of several seconds is required before publishing again    
  delay(5000);
}
