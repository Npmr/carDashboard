/**************************************************************************
  
    ----> https://www.adafruit.com/product/2088
  The 1.14" TFT breakout
  ----> https://www.adafruit.com/product/4383
  The 1.3" TFT breakout
  ----> https://www.adafruit.com/product/4313
  The 1.54" TFT breakout
    ----> https://www.adafruit.com/product/3787
  The 1.69" TFT breakout
    ----> https://www.adafruit.com/product/5206
  The 2.0" TFT breakout
    ----> https://www.adafruit.com/product/4311
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618
 
 **************************************************************************/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#define TFT_CS         15
#define TFT_RST        4                                            
#define TFT_DC         0
// For 1.14", 1.3", 1.54", 1.69", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//#define TFT_MOSI 13  // Data out
//#define TFT_SCLK 14  // Clock out
// OR for the ST7789-based displays, we will use this call
//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
const char* ssid = "YOUR SSID";
const char* password =  "YOUR PASSWORD";
const char* mqttServer = "YOUR MQTT ADDRESS";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
WiFiClient espClient;
PubSubClient client(espClient);

const int buttonPin = D6;     // the number of the pushbutton pin
// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int count_value =0;
int prestate =0;

const int handbremse = D1;     // the number of the pushbutton pin
int handbremseState = 0;
int handbremseLose = 0;
int runHandBremseOnce = 1;

String stringOne = "Menu: ";
String stringThree = "";
String result = "";

//mqtt messages that should be displyed
String recievedText= "";

String results[11] = { "Assistent", "Wetter", "Verbrauch","Uhrzeit","Fahrzeit","Batterie","Kompass","Koordinaten","G-Sensor","Neigung","Dunkel"};
String zwischenSpeicherTabelle[11] = { "", "", "","","","", "", "", "", "",""};
String mqttUpdatedTable[11] =  { "", "", "","","","", "", "", "", "",""};
String assistentFaces[7] = {"^_^", "^.^", "=_=","=.=", ">.>","<.<","^~^"};

float p = 3.1415926;

void setup(void) {
  Serial.begin(115200);

  tft.init(135,240);           // Init ST7789 240x135
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setRotation(3);
  tft.setCursor((tft.width()/2)-40, (tft.height()/2)-10);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Wilkommen");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  count_value = 9;
  
  pinMode(buttonPin, INPUT);
  pinMode(handbremse, INPUT);

  client.subscribe("dashboard/weather");
  client.subscribe("dashboard/consumption");
  client.subscribe("dashboard/voltage");
  client.subscribe("dashboard/time");
  client.subscribe("dashboard/orientation");
  client.subscribe("dashboard/coordinates");
  client.subscribe("dashboard/acceleration");
  client.subscribe("dashboard/pitch");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  handbremseState = digitalRead(handbremse);
  buttonState = digitalRead(buttonPin);

  calcStartTime();

  if (handbremseState==1){
    if(runHandBremseOnce == 1){
      handBreakActive();
      runHandBremseOnce = 0;
    }
    handbremseLose = 1;
  }
  else{
    if(handbremseLose == 1){
      Serial.println("hand break not active");
      Serial.println(stringThree);
      Serial.println(results[count_value]);
      displayText(stringThree, results[count_value]);
      handbremseLose = 0;
      runHandBremseOnce = 1;
    }
    
    // check if the pushbutton is pressed. If it is, then the buttonState is HIGH:
    if (buttonState == HIGH && prestate == 0) {
      if(count_value <=9){
        count_value++;
      }
      else if(count_value == 10){
        count_value = 0;
      }
      stringThree = stringOne + count_value;
      result = results[count_value]; 
      displayText(stringThree, results[count_value]);
      delay(500);
      showsecondMenu(results[count_value], zwischenSpeicherTabelle[count_value]);
      if(count_value == 0){
        showsecondMenu(results[count_value], assistentFaces[random(6)]);
      }
      Serial.println(count_value);  
      prestate = 1;
    } else if(buttonState == LOW) {
      prestate = 0;
    }

        //choose random face from ascii face for assitent
      // TODO rewrite face expression function to work properly
//      for (int i = 0; i<5; i=i+1){
//        if(zwischenSpeicherTabelle[i] != mqttUpdatedTable[i] && count_value == 0){
//          showsecondMenu(results[count_value], assistentFaces[random(6)]);
//        }
//      }
      
    if (zwischenSpeicherTabelle[count_value] != mqttUpdatedTable[count_value]){
      zwischenSpeicherTabelle[count_value] = mqttUpdatedTable[count_value]; 
      Serial.print("element ");  Serial.print (count_value); Serial.println (" does not match");
      Serial.print(results[count_value]); Serial.print ("---------");Serial.print (zwischenSpeicherTabelle[count_value]);
      showsecondMenu(results[count_value], zwischenSpeicherTabelle[count_value]);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("connected");  
      client.subscribe("dashboard/weather");
      client.subscribe("dashboard/consumption");
      client.subscribe("dashboard/voltage");
      client.subscribe("dashboard/time");
      client.subscribe("dashboard/orientation");
      client.subscribe("dashboard/coordinates");
      client.subscribe("dashboard/acceleration");
      client.subscribe("dashboard/pitch");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  recievedText ="";

  if (strcmp(topic,"dashboard/weather")==0){
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[1]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/consumption")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[2]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/voltage")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[5]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/time")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[3]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/orientation")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[6]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/coordinates")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[7]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/acceleration")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[8]=recievedText;
    }
  }
  if (strcmp(topic,"dashboard/pitch")==0) {
    for (int i = 0; i < length; i++) {
      recievedText = recievedText +(char)payload[i];
      mqttUpdatedTable[9]=recievedText;
    }
  }
}

void calcStartTime(){
  long millisecs = millis();
  String StringMinuten = String((millisecs / (1000 * 60)) % 60);
  String StringStunden = String((millisecs / (1000 * 60 * 60)) % 24);
  String gesamteFahrZeit = StringStunden +":"+ StringMinuten;
  if(gesamteFahrZeit != mqttUpdatedTable[4]){
    mqttUpdatedTable[4] = gesamteFahrZeit;  
  }
}

void handBreakActive(){
  tft.fillScreen(ST77XX_RED);
  for (int16_t y=0; y < tft.height(); y+=15) {
    tft.drawFastHLine(0, y, tft.width(), 0xA820);
  }
  for (int16_t x=0; x < tft.width(); x+=15) {
    tft.drawFastVLine(x, 0, tft.height(), 0xD124);
  }
}

void displayText(String menu, String menuTitle){
    if(menu.equals("Menu: 10")){
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextWrap(true);
      tft.setTextColor(ST77XX_BLACK);
      tft.setCursor(20, 20);
      tft.setTextSize(3);
      tft.print(menu);
      tft.setCursor(20, 60);
      tft.setTextSize(4);
      tft.print(menuTitle);
    }
    else{
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setTextWrap(true);
      tft.setCursor(20, 20);
      tft.setTextSize(3);
      tft.print(menu);
      tft.setCursor(20, 60);
      tft.setTextSize(4);
      tft.print(menuTitle);
    }
}

void showsecondMenu(String headline, String valueOfThing){
   if(headline.equals("Assistent")){
    Serial.println(valueOfThing);
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextWrap(true);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(20, 20);
        tft.setTextSize(3);
        tft.print(headline);
        tft.setCursor(80, 60);
        tft.setTextSize(5);
        tft.print(valueOfThing);
   }
   else if(headline.equals("Dunkel")){
    Serial.println(valueOfThing);
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextWrap(true);
        tft.setTextColor(ST77XX_BLACK);
   }
   else{
    Serial.println(valueOfThing);
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextWrap(true);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(20, 20);
        tft.setTextSize(3);
        tft.print(headline);
        tft.setCursor(80, 60);
        tft.setTextSize(4);
        tft.print(valueOfThing);
   }
}