#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Homey.h>

//GPIO map
#define PIN_SOUND  5   //D1 digital output of sound module

WiFiManager wifiManager; //if no wifi config available then this device will start as access point

//Homey loop parameters
unsigned long previousMillis = 0;
const unsigned long interval = 100; //Interval for Homey check in milliseconds

//sound triggers
bool soundDetect = false;
bool interruptSound = false;
int offsetJitter = 50;          //sound durection needs langer than this before go back to silend
unsigned long timeStart = 0;
unsigned long timeJitter = 0;
unsigned long timeStop = 0;
unsigned long soundTime = 0;
unsigned long soundSilend = 0;

//clap variabels
bool clapDetectStart = false;
int clapDetectSoundOffset = 70;       //maximun time of sound needs for a clap (include offsetJitter time)
int clapDetectSilentOffset = 150;     //minimum time of silend after a clap
int clapDetectTotalOffset = 1500;     //total time to wait until claps commit
int clapCount = 0;                    //number of claps detect
unsigned long timeLastClapDetect = 0;

  
void setup() {
  Serial.begin(115200);

  pinMode(PIN_SOUND, INPUT_PULLUP); //Set button pin to input
  
  String deviceName = "ClapDetection-" + String(ESP.getChipId()); //Generate device name based on ID

  Serial.print("wifiManager...");
  Serial.println(deviceName);
  wifiManager.autoConnect(deviceName.c_str(), ""); //Start wifiManager
  Serial.println("Connected!");

  Homey.begin(deviceName); //Start Homeyduino
  Homey.setClass("button");
  //Homey.addCapability("button");  

  //setup interrupt on pins
  attachInterrupt(PIN_SOUND, handleInterrupt, RISING); //CHANGE, RISING and FALLING

  Serial.println("Setup completed");
}

void loop() {
  Homey.loop();
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    //Homey.trigger("float", currentFloatState);

  }

  checkSound();
  detectClap();

  if(millis() - timeStart < 500){
    Serial.print(soundDetect);
    Serial.print(interruptSound);
    Serial.print(lijnUitGetal(millis() - timeStart, 10));
    Serial.print(lijnUitGetal(millis() - timeStop, 10));
    Serial.print(lijnUitGetal(millis() - timeJitter, 10));
    Serial.print(lijnUitGetal(soundTime, 10));
    Serial.println(lijnUitGetal(soundSilend, 10));    
  }

}


String lijnUitGetal(int waarde, byte karakters){
  String newWaarde = String(waarde);
  for(byte a = newWaarde.length(); a < karakters; a++){
    newWaarde = " " + newWaarde;
  }
  return newWaarde;
}


void handleInterrupt(){
   interruptSound = true; //digitalRead(PIN_SOUND);
}

void checkSound(){
  bool tmpSound = interruptSound; 

  //detect jitter
  if(tmpSound && soundDetect){
    timeJitter = millis();  
  }

  //detect sound
  if(tmpSound && !soundDetect){
    //Serial.println("detect");
    soundDetect = true;
    timeStart = millis();
    timeJitter = millis(); 
    timeStop = millis(); 
    soundTime = 0;
    soundSilend = 0;
    clapDetectStart = true;
  }

  //detect silend
  if(!tmpSound && soundDetect){
    //Serial.print("silend ");
    if(millis() - timeJitter < offsetJitter){
      //jitter
      //Serial.println("jitter");
    }else{
      //no sound detect
      soundDetect = false;
      timeStop = millis();
      //Serial.println("true");
    }    
  }

  if(soundDetect) soundTime = millis() - timeStart;
  if(!soundDetect) soundSilend = millis() - timeStop;

  interruptSound = false;
}


void detectClap(){
  //detect clap
  if(clapDetectStart && soundTime < clapDetectSoundOffset && soundSilend > clapDetectSilentOffset){
    //clap detect
    clapCount++;
    timeLastClapDetect = millis();
    clapDetectStart = false;
    Serial.print("clap: ");
    Serial.println(clapCount);
  }

  //total of claps and reset
  if(clapCount > 0 && (millis() - timeLastClapDetect) > (clapDetectTotalOffset)){
    Serial.print("total claps: ");
    Serial.println(clapCount);
    Homey.trigger("handclaps", clapCount);
    clapCount = 0;
    clapDetectStart = false;
  }
}
