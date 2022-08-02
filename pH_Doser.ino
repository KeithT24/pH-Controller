#include <OneWire.h>

#include <DallasTemperature.h>

#include <AsyncTimer.h>

/* pH Doser and Water Top-off system v.1
 * Hardware: 
 * 2 peristaltic dosing pumps - 12V
 * 1 diaphram pump for top off - 12V
 * 3 3V optocoupler relay
 * 1 temp sensor
 * 1 atlas ph analog
 * 
 * Wifi Connection -
 * Homeassistant via MQTT
 * 
 * 
 * by Keith Tomlinson
 * 
 * TODO: 
 *       add mqtt for distance offset, top up time on, doser time on, 
 *       (optional) total on time for pumps (resetable for HASS)
 *       Add rez temperature
 */


#include "ph_grav.h"             
Gravity_pH pH = Gravity_pH(36);   
#include <PubSubClient.h>
//#include <esp_wifi.h>
#include <WiFi.h>
#include "global.h"
#include "stdio.h"
#include <Wire.h>

char pHstr[8];
char tempStr[8];
char diststr[8];


AsyncTimer doser_timer, top_timer;
enum system_mode ph_mode = ON;
void callback(char* topic, byte* payload, unsigned int length);
void doser_func();
void top_func();
void wifiSetup();
WiFiClient wifi;
PubSubClient client(homeassistant,1883,callback,wifi);
String success;
String data;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress rezTemp;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(phUpPin, OUTPUT);
  pinMode(topPin, OUTPUT);
  pinMode(phDownPin, OUTPUT);
  
  wifiSetup();
  doser_intervalId = doser_timer.setInterval(doser_func, doser_delay);
  top_intervalId = top_timer.setInterval(top_func, top_delay);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  if (pH.begin()) {                                     
    Serial.println("Loaded EEPROM");
  }
}

void getDistance(){
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  //Serial.print("Distance (cm): ");
  //Serial.println(distanceCm);
  distanceCm = distanceCm - dist_offset;
  delay(1000);
}


void callback(char* topic, byte* payload, unsigned int length) {
    //Serial.println("callback");
    
    char p[length+1];
    memcpy(p, payload, length);
    p[length] = '\0'; 
    //Serial.print(p);
   
    
    if(strcmp(topic, "pHmode")==0){
        //Serial.println("mode received");
        //Serial.println(p);
        if(strcmp(p,mode_names[ph_mode])==0)
        {
            //do nothing
            //Serial.println("mode same");
        }
        else if(strcmp(p,"On") == 0){
           ph_mode = ON;
           //Serial.println("mode changed");
        }
        else if(strcmp(p,"Cal7")==0){
          ph_mode = CAL7;
          //Serial.println("mode changed to 7");
        }
        else if(strcmp(p,"Cal4")==0){
          ph_mode = CAL4;
          //Serial.println("mode changed to 4");
        }
        else if(strcmp(p,"Cal10")==0){
          ph_mode = CAL10;
          //Serial.println("mode changed to 10");
        }
        else if(strcmp(p,"CalClear")==0){
          ph_mode = CALCLEAR;
          //Serial.println("mode changed to clear");
        }
        else
          ph_mode = ON;
    
    }
    else if(strcmp(topic, "pHdoser")==0){
      Serial.println("pHDoser: ");
      if(strcmp(p,"off")==0)
        phDoser = 0;
      else
        phDoser = 1;
      Serial.println(phDoser);
    }
    else if(strcmp(topic, "water_top")==0){
      Serial.printf("AutoTopUp: ");
      if(strcmp(p,"off")==0)
        topOff = 0;
      else
        topOff = 1;
      Serial.println(topOff);
    }

    else if(strcmp(topic, "doser_int")==0){
      //Serial.printf("DoserInt: ");
      
      unsigned long temp = strtoul(p,NULL,10);
      //Serial.printf("payload = %u\n",temp);
      doser_delay = temp * (unsigned long)60000;
      //t.setInterval(doser_func, doser_delay);
      doser_timer.changeDelay(doser_intervalId, doser_delay);
      Serial.printf("pH Doser Interval = %d\n",doser_delay);
      
    }
    else if(strcmp(topic, "top_int")==0){
      //Serial.println("TopInt");
      
      unsigned long temp = strtoul(p,NULL,10);
      //Serial.printf("payload = %u\n",temp);
      top_delay = temp * (unsigned long)60000;
      //t.setInterval(doser_func, doser_delay);
      top_timer.changeDelay(top_intervalId, top_delay);
      Serial.printf("Top Off interval = %d\n",top_delay);
      
    }
    else if(strcmp(topic, "pH_target")==0){
      //Serial.println("pH Target");
      
      float temp = strtod(p,NULL);
      //Serial.printf("payload = %.2f\n",temp);
      phTarget = temp;
      Serial.printf("pH Target = %.2f\n",phTarget);
      
    }
    else if(strcmp(topic, "pH_tolerance")==0){
      //Serial.println("pH Tolerance");
      float temp = strtod(p,NULL);
      //Serial.printf("payload = %.2f\n",temp);
      phTolerance = temp;
      Serial.printf("pH Tolerance = %.2f\n",phTolerance);
    }
    else if(strcmp(topic, "doser_time")==0){
      //Serial.println("Doser time");
      unsigned int temp = (unsigned int) strtoul(p,NULL,10);
      //Serial.printf("payload = %d\n",temp);
      doser_time = temp;
      Serial.printf("Doser time = %d\n",doser_time);
    }
    else if(strcmp(topic, "top_time")==0){
      //Serial.println("Top Off time\n");
      unsigned int temp = (unsigned int) strtoul(p,NULL,10);
      //Serial.printf("payload = %d\n",temp);
      top_time = temp;
      Serial.printf("Top time = %d\n",top_time);
    }
    else if(strcmp(topic, "dist_offset")==0){
      //Serial.println("dist_offset");
      float temp = strtod(p,NULL);
      //Serial.printf("payload = %.2f\n",temp);
      dist_offset = temp;
      Serial.printf("Distance offset = %.2f\n",dist_offset);
    }
    
    //free(p);
    //delay(1000);
}

String convertPayloadToStr(byte* payload, unsigned int length) {
  char s[length + 1];
  s[length] = 0;
  for (int i = 0; i < length; ++i)
    s[i] = payload[i];
  String tempRequestStr(s);
  return tempRequestStr;
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32_sender", "homeassistant", "ieho0sheigae9feez0iephe9kaiquoxeeyo6eitej5gu3uere1ahlu1Reethah2u")) {
      Serial.println("connected");
      // Subscribe
      bool test = client.subscribe("pHmode");
      client.subscribe("pHdoser");
      client.subscribe("water_top");
      client.subscribe("doser_int");
      client.subscribe("top_int");
      client.subscribe("pH_target");
      client.subscribe("pH_tolerance");
      client.subscribe("doser_time");
      client.subscribe("top_time");
      client.subscribe("dist_offset");
      
      client.publish("topPump","0");
      client.publish("upPump","0");
      client.publish("downPump","0");
   
      //Serial.print(test);
      //Serial.print("\n");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void phPub(){
    
    dtostrf(phReading,1,2,pHstr);
    client.publish("pH", pHstr);
    Serial.println("pH published");
    delay(100);
}
void modePub(){ 
    client.publish("pHmode", mode_names[ph_mode]);
    Serial.println(mode_names[ph_mode]);
    Serial.println("mode published");
    delay(100);
}
void distancePub(){
    
    dtostrf(distanceCm,1,2,diststr);
    client.publish("water_dist",diststr);
    Serial.println("distance published");
    delay(100);

}

void tempPub(){
  sensors.requestTemperatures(); 
  waterTemp = sensors.getTempFByIndex(0);
  dtostrf(waterTemp,1,2,tempStr);
  client.publish("water_temp",tempStr);
  Serial.printf("temp = %f\n", waterTemp);
  delay(100);
}

void WiFiReset() {
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}





void wifiSetup(){
  delay(10);
  
  WiFi.begin(ssid, pass,WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  WiFi.printDiag(Serial);
                             
  delay(200);
  
}

void top_func(){
  if(topOff && distanceCm > 1 && distanceCm < 100){
    Serial.printf("Topping up...\n");
    client.publish("topPump", "1");
    digitalWrite(topPin, HIGH);
    delay(top_time*1000);
    digitalWrite(topPin, LOW);
    client.publish("topPump", "0");
    Serial.printf("Topping up complete\n");
  }
  else
    Serial.printf("Top Off.. Off\n"); 
}


void doser_func(){
  if(phDoser){  
    if(phReading < (phTarget - phTolerance)){
      Serial.printf("Dosing up...\n");
      client.publish("upPump", "1");
      digitalWrite(phUpPin, HIGH);
      delay(doser_time*1000);
      digitalWrite(phUpPin, LOW);
      client.publish("upPump", "0");
      Serial.printf("Dosing Complete\n");
    }
    else if(phReading > (phTarget + phTolerance)){
      Serial.printf("Dosing down...\n");
      client.publish("downPump", "1");
      digitalWrite(phDownPin, HIGH);
      delay(doser_time*1000);
      digitalWrite(phDownPin, LOW);
      client.publish("downPump", "0");
      Serial.printf("Dosing Complete\n");
  }
    else{
      Serial.printf("pH in range\n");
    }
 } 
 else{
    Serial.printf("Auto Dosing Off\n");
 }  
  
}



void loop() {


    
    if(!client.connected()){
      reconnect();
    }
    
    client.loop();
    client.loop();
    client.loop();
    doser_timer.handle();
    top_timer.handle();
   
    //Serial.printf("Distance = %.2f \n",distanceCm);
    //Serial.printf("pHmode = %s\n", mode_names[ph_mode]);
    //Serial.printf("Doser Interval = %u minutes\n", doser_delay/60000);
    
    if(ph_mode == ON){
        if(millis() - lastRead > readInterval){
            getDistance();
            phReading = pH.read_ph();
            distancePub();
            phPub();
            modePub();
            tempPub(); 
            Serial.printf("pH = ");
            Serial.println(phReading);
            lastRead = millis();  
        }  
    }   
    else if(ph_mode == CAL7){
        //modePub();  
        pH.cal_mid();
        //delay(1000);                             
        Serial.println("MID CALIBRATED");
        delay(10);
        ph_mode = ON;
        modePub();
    }
        
    else if (ph_mode == CAL4){ 
        //modePub();  
        pH.cal_low(); 
        //delay(1000);                              
        Serial.println("LOW CALIBRATED");
        ph_mode = ON;
        delay(10);
        modePub();  
    }
        
    else if (ph_mode == CAL10){
        //modePub();  
        pH.cal_high(); 
        //delay(2000);
        Serial.println("HIGH CALIBRATED");
        ph_mode = ON;
        delay(10);
        modePub();  
    }
    else if (ph_mode == CALCLEAR){
        //modePub();  
        pH.cal_clear();
        //delay(1000);                              
        Serial.println("CALIBRATION CLEARED");
        ph_mode = ON;
        delay(10);
        modePub();  
    }
                             
  //delay(1000);
}
