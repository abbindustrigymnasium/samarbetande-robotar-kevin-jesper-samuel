#include <Wire.h>
#include "EspMQTTClient.h"

EspMQTTClient client(
 "ABB_Indgym_Guest",           // Wifi ssid
  "Welcome2abb",           // Wifi password
  "maqiatto.com",  // MQTT broker ip
  1883,             // MQTT broker port
  "kevin.klarin@abbindustrigymnasium.se",            // MQTT username
  "1337",       // MQTT password
  "jesper",          // Client name
  onConnectionEstablished, // Connection established callback
  true,             // Enable web updater
  true              // Enable debug messages
);

#define DIRA 0 //Motor direction
#define PWMA 5 //Motor pwm
#define HALL 12 //Digital input from hallsensor

int pulses = 0;
int prev_pulses = 0;
int prev_time = 0;
int time_diff = 0; 
int current_time = 0;
int varv_diff = 0;
float error = 0;
float rpm = 0;
float speed_cm = 0;
float speed_km = 0;
float wheeldiameter = 3.6;
float pwm = 0;
float goal_speed = 0;
float Kp = 7.2;
float Ki = 10;
float integral_e = 0;


void Increase_pulses(){
  pulses += 1; //Increases pulses everytime the digital input 12 goes from HIGH to LOW
  Serial.println(pulses);
}

void RPM(){
  noInterrupts();//No interupts during the calculations
  current_time = millis();
  time_diff = (current_time - prev_time); //Time since last calculation
  varv_diff = (pulses - prev_pulses); //Pulses sice last calculation 
  rpm = (float(varv_diff)/float(time_diff))*60000/96; // pulse per millisecond multiplied with milliseconds per minute and 1:48 gear ratio
  prev_pulses = pulses; 
  prev_time = current_time;
  interrupts(); 
}

void Calculate_speed(){
  speed_cm = (rpm/60)*(3.14*wheeldiameter); //Takes rps multiplied with the wheeldiameter.
  speed_km = (speed_cm*0.036); // km/h
}

void Print_speed(){
  Serial.print("RPM:");
  Serial.println(rpm);
  Serial.println("hastighet cm/s");
  Serial.println(speed_cm);
  Serial.println("hastighet km/h");
  Serial.println(speed_km);
  Serial.println("PWM:");
  Serial.println(pwm);
}

void onConnectionEstablished()
{
  client.subscribe("kevin.klarin@abbindustrigymnasium.se/speed", [] (const String &payload)
  {
    goal_speed = payload.toInt();
 
  });

  client.publish("kevin.klarin@abbindustrigymnasium.se/lmaoxd", "Jesper is connected");
}

void set_PWM(){
  error = goal_speed - speed_cm; //error goal - is
  integral_e += error * time_diff / 1000; //cm
  pwm = error * Kp + integral_e * Ki; //pwm output
  if (pwm > 1023){ //pwm signal cannot be higher than 1023
    pwm = 1023;
  }
}

void setup() {
  pinMode(DIRA, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(HALL, INPUT);
  Serial.begin(115200);
  digitalWrite(DIRA, 0);
  attachInterrupt(digitalPinToInterrupt(HALL), Increase_pulses, FALLING);
}

void loop() {
  client.loop();
  analogWrite(PWMA, pwm);
  if ((millis()-prev_time) > 100){  // runs with 0.1 second delay
    RPM();
    Calculate_speed();
    set_PWM();
    if (speed_cm != 0.00){
      client.publish("kevin.klarin@abbindustrigymnasium.se/jesperdata", String(speed_cm));
     
    }
  }
}
