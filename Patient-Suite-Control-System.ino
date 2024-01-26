#include <ESP32Servo.h>
#include "DHTesp.h"
#include <WiFi.h>
#include <PubSubClient.h>

const char *WIFI_SSID = "97-3-8";           // WiFi SSID
const char *WIFI_PASSWORD = "tenantof9738"; // password
const char *MQTT_SERVER = "35.239.167.63";  // VM instance public IP address
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "IoTpatientmonitoring"; // MQTT topic

//*************************************************************************
//                    DHT11 sensor initalization
//*************************************************************************
#define dataPin A2 // Defines pin number to which the sensor is connected
DHTesp DHT;
WiFiClient espClient;
PubSubClient client(espClient);

//*************************************************************************
//               Servo sensor and Push Button 1 initalization
//*************************************************************************
Servo myservo; // create servo object to control a servo
int pos = 0;   // variable to store the servo position
const int pushButtonPin = 21;
int pushButtonState = 0;
int currentDirection = 1; // 1 for forward, -1 for reverse

//*************************************************************************
//                    Moisture sensor initalization
//*************************************************************************
//The threshold value minimum is 4095, means no water detected
//The value decrease as the moisture value increase, means more water detected
//more water, value lower
const int sensorPin = A4;  // Analog pin for soil moisture sensor
int MinMoistureValue = 4095;
int MaxMoistureValue = 2060;
int MinMoisture = 0;
int MaxMoisture = 100;
int Moisture = 0;

//*************************************************************************
//                    Infrared sensor initalization
//*************************************************************************
const int infrapin = 40;


//*************************************************************************
//                   MQ2 Smoke Sensor initalization
//*************************************************************************
const int MQ2pin = A3;
float gasValue;

//*************************************************************************
//                        Relay initalization
//*************************************************************************
const int relay_motor= 14; // the relay is connected to pin 14

//*************************************************************************
//                          LED initalization
//*************************************************************************
const int led_light = 47;

//*************************************************************************
//                   Buzzer and Push Button 2 initalization
//*************************************************************************
int pushButtonState2 = 0;
const int pushButtonPin2 = 39;
const int buzzerPin = 12;       // Onboard buzzer



//*************************************************************************
//                        Setup Wifi Function
//*************************************************************************
void setup_wifi(){
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
  delay(500);
  Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//*************************************************************************
//               Initialize All Sensors and Actuators PinMode
//*************************************************************************
void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);

  myservo.attach(48); // attaches the servo on pin 48 to the servo object
  pinMode(pushButtonPin, INPUT);
  pinMode(infrapin, INPUT_PULLUP);
  pinMode(relay_motor, OUTPUT);
  pinMode(led_light, OUTPUT);
  pinMode(pushButtonPin2, INPUT);
  pinMode(buzzerPin, OUTPUT);
  
  Serial.println("Gas sensor warming up!");
  delay(20000);  // allow the MQ-2 to warm up
}


//*************************************************************************
//              Reconnect if MQTT connection fails
//*************************************************************************
void reconnect(){
  while (!client.connected()){
    Serial.println("Attempting MQTT connection...");   
    if (client.connect("ESP32Client")){
      Serial.println("Connected to MQTT server");
    }
    else{
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 3 seconds...");
      delay(3000);
    }
  }
}


//*************************************************************************
//                          Loop Function
//*************************************************************************
void loop() {

  if (!client.connected()){
    reconnect();
  }

  client.loop();
  delay(2000); // delay 2 seconds


  
  DHT.setup(dataPin, DHTesp::DHT11);  // Reads the temperature data from the DHT11 sensor
  pushButtonState = digitalRead(pushButtonPin);      // Reads the data from the push button 1 sensor
  pushButtonState2 = digitalRead(pushButtonPin2);    // Reads the data from the push button 2 sensor
  int moistureSensorValue = analogRead(sensorPin);    // Reads the analog data from the moisutre sensor
  int infraredSensorStatus = !digitalRead(infrapin);  // Reverse the data | 0 - no obstacle detected, 1 - obstacle detected
  

  //*************************************************************************
  //          Servo motor to open or close window using push button 1
  //*************************************************************************
  
  if (pushButtonState == HIGH) {
    // Change direction when the button is pressed
    currentDirection *= -1;
    myservo.attach(48);
    // Move the servo based on the current direction
    if (currentDirection == 1) {
      for (pos = 0; pos <= 180; pos += 1) {
        myservo.write(pos);
        delay(15);
      }
    } 
    else {
      for (pos = 180; pos >= 0; pos -= 1) {
        myservo.write(pos);
        delay(15);
      }
    }
    myservo.detach();     // Detach the Servo variable from 48 pin to avoid conflict with onboard buzzer
  }

  //*************************************************************************
  //         Obtain the current temperature and humidity data
  //*************************************************************************
  float t = DHT.getTemperature(); // Gets the values of the temperature
  float h = DHT.getHumidity(); // Gets the values of the humidity

  // Printing the results on the serial monitor
  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("    Humidity = ");
  Serial.print(h);
  Serial.println(" % ");


  if(t >= 30){
    digitalWrite(relay_motor, HIGH);    // Switch on fan blade
  }
  else{
    digitalWrite(relay_motor, LOW);     // Switch off fan blade
  }

  // uses humidity to test as the temperature value is difficult to change
  // if(h < 85){
  //   digitalWrite(relay_motor, LOW);
  // }
  // else{
  //   digitalWrite(relay_motor, HIGH); 
  // }


  //*************************************************************************
  //         Light up when trigger emergency alarm using push button 2
  //*************************************************************************
  if(pushButtonState2 == HIGH)
  {
    digitalWrite(led_light, HIGH);
    tone(buzzerPin, 500);           // Trigger alarm
    delay(3500);
  }
  else{
    digitalWrite(led_light, LOW);
    noTone(buzzerPin);              // Stop alarm
  }

  //*************************************************************************
  //                  Detect moisutre of adult diaper
  //*************************************************************************
  // Map the sensor values to a range (adjust these values based on your sensor)
  Moisture = map(moistureSensorValue, MinMoistureValue, MaxMoistureValue, MinMoisture, MaxMoisture);

  Serial.print("Moisture Level: ");
  Serial.print(moistureSensorValue);
  Serial.print(" | Moisture Percentage: ");
  Serial.print(Moisture);
  Serial.println("%");

  delay(1000); 


  //*************************************************************************
  //                   Detect if patient falls down
  //*************************************************************************
  Serial.print("Infrared Sensor State: ");
  Serial.println(infraredSensorStatus);
  // value 1 means IR sensor detect object in front of it | value 0 means no object detected in front of it
  if(infraredSensorStatus == 1){
    digitalWrite(led_light, HIGH);
    tone(buzzerPin, 200);       // trigger alarm when patient falls down
  }
  else{
    digitalWrite(led_light, LOW);
    noTone(buzzerPin);
  }

  delay(1500); // Delays 1.5 secods


  //*************************************************************************
  //              Detect if smoke value exceeds threshold
  //*************************************************************************
  gasValue = analogRead(MQ2pin);

  Serial.print("Gas value: ");
  Serial.println(gasValue);

  if (gasValue > 3000) {
    digitalWrite(led_light, HIGH);
    tone(buzzerPin, 1000);
  } else {
    digitalWrite(led_light, LOW);
    noTone(buzzerPin); 
  }

  //*************************************************************************
  //              Publish all sensor data via MQTT connections  
  //*************************************************************************
  char payload[300];
  sprintf(payload, "Temperature: %.2f °C, Humidity: %.2f%%, Patient Fall Detection: %d, Biowaste Moisture: %d%%, Window Curtain Status: %d, Call Button: %d, Smoke Value: %.2f", t, h, infraredSensorStatus, Moisture, pushButtonState, pushButtonState2, gasValue);
  client.publish(MQTT_TOPIC, payload);
}
