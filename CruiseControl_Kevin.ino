#include "EspMQTTClient.h"

#define PWM_B 4 //D2 Motor
#define Dir_B 2 //D4 Motorns riktning
#define HE 13 //D7 Hallelement

volatile unsigned long lastM = 0;
volatile unsigned long Timeout = 0;

volatile unsigned long previousMillis = 0;
volatile unsigned long diffMillis;
volatile int counter = 0;
volatile float average = 0;
bool NewVal = false;

volatile float speed_now;
volatile float goal_speed = 0;
volatile float Kp = 8;
volatile float Ki = 7;
float dT = 0; 
float integralKi = 0;
float integralsum = 0;
float proportional = 0;
volatile int PWM = 0;
float error;

void onConnectionEstablished();

EspMQTTClient client(
 "ABB_Indgym_Guest",           // Wifi ssid
  "Welcome2abb",           // Wifi password
  "maqiatto.com",  // MQTT broker ip
  1883,             // MQTT broker port
  "kevin.klarin@abbindustrigymnasium.se",            // MQTT username
  "1337",       // MQTT password
  "mememaster69",          // Client name
  onConnectionEstablished, // Connection established callback
  true,             // Enable web updater
  true              // Enable debug messages
);


void setup() {
  Serial.begin(115200);
  pinMode(PWM_B, OUTPUT); //Styr motorns hastighet 0 - 1023
  pinMode(Dir_B, OUTPUT); //Får motorn att åka fram eller bak. HIGH är fram, LOW är bak
  pinMode(HE, INPUT);
  attachInterrupt(digitalPinToInterrupt(HE), count_RPM, FALLING); //Interruptar då hallelementet går från 1 till 0
  digitalWrite(Dir_B, HIGH);
  delay(1000);
}

void count_RPM() { //mäter tiden mellan pulser
  noInterrupts();
  diffMillis = micros() - previousMillis;
  previousMillis = micros();
  NewVal = true;
  interrupts();
  }

void onConnectionEstablished()
{
  client.subscribe("kevin.klarin@abbindustrigymnasium.se/speed", [] (const String &payload)
  {
    goal_speed = payload.toInt();
  });
  client.publish("kevin.klarin@abbindustrigymnasium.se/lmaoxd", "Kejvin has connected naow");
}

int set_RPM(float goal, float current_speed) {
  error = goal - current_speed;
  proportional = error * Kp;
  integralsum += dT * error; 
  integralKi = Ki * integralsum;
  PWM = proportional + integralKi;
  if (PWM > 1023) {
    PWM = 1023;
  } else if (PWM < 0) {
    PWM = 0;
  }
}

int calc_speed () {
  float theoretical_speed;
  float RPS_motor;
  float RPM_out;
    RPS_motor = 1/(diffMillis*pow(10,-6)); //Räknar ut varv per minut genom mellanrummet mellan två pulser.
    RPM_out = (RPS_motor/96); //48:1 utväxling på motorn, 2 utslag per varv
    theoretical_speed = RPM_out*1.25*11; //Utväxling till däck 1:1.25 Hjulomkrets ca 11 cm
    if (theoretical_speed < 200) { //förhindrar felmätningar från att förstöra medelvärdet
      speed_now = theoretical_speed;
      average = average + theoretical_speed;
      counter ++;
    }
    
    if (counter >= 100) { //använder counter för att undvika att sakta ner bilen med för många meddelanden till MQTT
      average = average/counter; //Skickar ett snittvärde
      //Serial.println(average);
      client.publish("kevin.klarin@abbindustrigymnasium.se/kevindata",String(average));
      //client.publish("kevin.klarin@abbindustrigymnasium.se/lmaoxd",String(PWM));
      average = 0;
      counter = 0;
    }
}


void loop() {
  client.loop();
  analogWrite(PWM_B,PWM);
  if (NewVal == true) {
    Timeout = millis();
    noInterrupts();
    NewVal = false;
    calc_speed();
    interrupts();
  }

  if (millis() - lastM >= 100) { //Gör alla uträkningar varje 0.1 sekunder
    dT = (millis() - lastM) * pow(10,-3); //Multiplicerar med 10^3 för att dT ska vara i sekunder, så att Ki inte behöver vara jättestort
    set_RPM(goal_speed, speed_now);
    lastM = millis();
  }
  
  if (millis() - Timeout >= 5000) { //Om ingen mätning gjorts på 5 sekunder
    PWM = 500; //Sätter hastigheten för att få igång motorn, bilen kommer eventuellt att anpassa PWM till rätt värde med hjälp av set_RPM
    speed_now = 0; //Bilen står still
    Timeout = millis();
    set_RPM(goal_speed, speed_now);
    /*client.publish("kevin.klarin@abbindustrigymnasium.se/lmaoxd","Timed out, attempting to restart");
    client.publish("kevin.klarin@abbindustrigymnasium.se/lmaoxd", String(PWM));*/
  }
}
