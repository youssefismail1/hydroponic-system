#include <DHT.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "GravityTDS.h"
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);//(rs, en, d4, d5, d6, d7)

/*for DHT22 sensor*/
//Constants
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

//Variables

float h = 0;  //Stores humidity value
float t = 0; //Stores temperature value
/*====================================================================================*/
/*for ultrasonic sensor*/
// defines pins numbers
const int trigPin = 9;
const int echoPin = 10;
// defines variables
long duration = 0.0;
float distance = 0.0;
float actual_water_level = 0.0;
/*====================================================================================*/
/*for DS18B20 sensor*/
#define ONE_WIRE_BUS 6 // Data wire is plugged into digital pin 6 on the Arduino
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire); // Pass oneWire reference to DallasTemperature library
float Celsius = 0.0;
/*====================================================================================*/
/*for PH sensor*/
float calibration_value = 21.34 - 0.14; //the calibration value is defined, which can be modified as required to get an accurate pH value of solutions.
unsigned long int avgval; //Store the average value of the sensor feedback
int buffer_arr[10],temp = 0;
float ph_act = 0.0;
/*====================================================================================*/
/*for TDS sensor*/
#define TdsSensorPin A1
GravityTDS gravityTds;
float tdsValue = 0.0;
/*====================================================================================*/
// EC isolator
#define EC_Isolator A3// 3906 PNP TYPE TRANSISTOR THIS is used to connect and disconnect the 3.3V wire 
#define EC_GND_Wire A2// 2N2222 NPN TRANSISTOR THIS IS USED TO CONNECT AND DISCONNECT THE GND WIRE
/*====================================================================================*/
/*flags*/
int flag_error = 0;
float ph_acid_values[2] = {0 , 0};
float ph_base_values[2] = {0 , 0};
float ph_normal_values[2] = {0 , 0};
int i_normal = 0;
int i_acid = 0;
int i_base = 0;
/*====================================================================================*/
/*for push button and count down timer*/
#define buttonpin 13 // chance to change it's pin from 13
int button_state = 0;
int num_mins = 9;
int num_sec = 0;
int count = 0;
/*====================================================================================*/
//PH sensor
String PH_UP_PUMP = "No current decision";
String PH_DOWN_PUMP = "No current decision";
String PH_CONDITION = "No current decision";

//PPM
String PPM_CONDITION = "No current decision";
String PUMPS_A_B = "No current decision";
String WATER_PUMP_UP_FOR_PPM = "No current decision";

//AIR_TEMP
String AIR_TEMP_CONDITION = "No current decision";
String FAN_FOR_AIR_TEMP = "No current decision";

//WATER_LEVEL
String VALVE_CONDITION = "No current decision";
String WATER_PUMP_UP_FOR_WATER_LEVEL = "No current decision";
String WATER_LEVEL_CONDITION = "No current decision";

//water_temp
String WATER_TEMP_CONDITION = "No current decision";
String COOLER_CONDITION = "No current decision";

//HUMDITIY
String HUMD_CONDITION = "No current decision";
String FAN_FOR_HUMDITIY = "No current decision";

String label;
/*====================================================================================*/
/*for actuators*/
// pins of the following is going to change 
int nutrientA = 4;//IN1
int nutrientB = 5;//IN2
int acid = 6;//IN3
int base = 7;//IN4

int level_up_pump = 8;
/*====================================================================================*/
void setup() 
{
  lcd.begin(16,2);
  Serial.begin(9600);
  delay(100);

  dht.begin();

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization 

  sensors.begin();

  pinMode(EC_Isolator, OUTPUT);
  pinMode(EC_GND_Wire, OUTPUT);
  digitalWrite(EC_Isolator, HIGH);
  digitalWrite(EC_GND_Wire, LOW); 

  //for push button
  pinMode(buttonpin , INPUT);

  //for dosing pumps "all of them are active low"
  pinMode(nutrientA , OUTPUT);  
  pinMode(nutrientB , OUTPUT);
  pinMode(acid , OUTPUT);
  pinMode(base , OUTPUT);
  digitalWrite(nutrientA , HIGH);
  digitalWrite(nutrientB , HIGH);
  digitalWrite(acid , HIGH);
  digitalWrite(base , HIGH);

  //for water level up pump
  pinMode(level_up_pump , OUTPUT);  
}

void loop() 
{
  if(flag_error == 0)
  {
    digitalWrite(EC_Isolator,HIGH); 
    digitalWrite(EC_GND_Wire, LOW);
    PH_sensor(); // first we get readings of PH sensor
    digitalWrite(EC_Isolator,LOW); 
    digitalWrite(EC_GND_Wire, HIGH);
    delay(1000);
      
    waterproof_sensor();
    TDS_sensor();
    temperature_and_humidity_sensor();
    ultrasonic_sensor();
    delay(100);
  }

  
  /*checking if there is a problem with any of sensors*/
   if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
   {
     ph_act = -1;
     PH_CONDITION = "Check PH sensor";
  
   }
   if((tdsValue >= 1000) || (tdsValue <= 160))// for TDS sensor
   {
     tdsValue = -2;
     PPM_CONDITION = "Check TDS sensor";
   }
   if ((isnan(h)) || (t == 0))// for DHT22 sensor
   {
     t = -3;
     AIR_TEMP_CONDITION = "Check DHT22_T";

     h = -5;
     HUMD_CONDITION = "Check DHT22_H";
   }
   if((distance == 0) || (distance >= 35))// for ultrasonic sensor
   {
     actual_water_level  = -4;
     WATER_LEVEL_CONDITION = "Check Ultrasonic";
   }
   if((Celsius <= -100) || (Celsius >= 50))// for Water_temp sensor
   {
     Celsius = -6;
     WATER_TEMP_CONDITION = "Check DS18B20";
   }

   /*for PH normal which means not adding acid or base*/
   if(((label.equals("[0]")) || (label.equals("[3]")) || (label.equals("[4]")) || (label.equals("[5]")) || (label.equals("[6]")) || (label.equals("[9]")) || (label.equals("[10]")) || (label.equals("[11]")) || (label.equals("[12]"))) && (ph_act != -1))
   {
    ph_normal_values[1] = ph_act;
    while(i_normal == 0)
    {
      if(((ph_normal_values[0] - ph_normal_values[1] <= 0.3) || (ph_normal_values[0] - ph_normal_values[1] >= 0)) && ((ph_normal_values[1] - ph_normal_values[0] <= 0.3) || (ph_normal_values[1] - ph_normal_values[0] >= 0)))
      {
        ph_normal_values[0] = ph_act;
        i_normal = 1;
      }
      else
      {
        digitalWrite(EC_Isolator,HIGH); 
        digitalWrite(EC_GND_Wire, LOW);
        PH_sensor(); // first we get readings of PH sensor
        digitalWrite(EC_Isolator,LOW); 
        digitalWrite(EC_GND_Wire, HIGH);
        delay(1000);
      
        if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
        {
          ph_act = -1;
          PH_CONDITION = "Check PH sensor";   
        }
        if(ph_act != -1)
        {
          ph_normal_values[1] = ph_act;
          i_normal = 0;
        }
        else
        {
          i_normal = 1;
        }
      }
      
    }
   }
   i_normal = 0;

   /*for PH low which means adding base*/
   if(((label.equals("[2]")) || (label.equals("[7]")) || (label.equals("[13]")) || (label.equals("[16]")) || (label.equals("[17]")) || (label.equals("[20]")) || (label.equals("[22]")) || (label.equals("[23]")) || (label.equals("[25]"))) && (ph_act != -1))
   {
    ph_base_values[1] = ph_act;
    while(i_base == 0)
    {
      if((ph_base_values[1] - ph_base_values[0] <= 0.5) && (ph_base_values[1] - ph_base_values[0] >= 0))
      {
        ph_base_values[0] = ph_act;
        i_base = 1;
      }
      else
      {
        digitalWrite(EC_Isolator,HIGH); 
        digitalWrite(EC_GND_Wire, LOW);
        PH_sensor(); // first we get readings of PH sensor
        digitalWrite(EC_Isolator,LOW); 
        digitalWrite(EC_GND_Wire, HIGH);
        delay(1000);
      
        if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
        {
          ph_act = -1;
          PH_CONDITION = "Check PH sensor";   
        }
        if(ph_act != -1)
        {
          ph_base_values[1] = ph_act;
          i_base = 0;
        }
        else
        {
          i_base = 1;
        }
      }
      
    }
   }
   i_base = 0;

   /*for PH high which means adding acid*/
   if(((label.equals("[1]")) || (label.equals("[8]")) || (label.equals("[14]")) || (label.equals("[15]")) || (label.equals("[18]")) || (label.equals("[19]")) || (label.equals("[21]")) || (label.equals("[24]")) || (label.equals("[26]"))) && (ph_act != -1))
   {
    ph_acid_values[1] = ph_act;
    while(i_acid == 0)
    {
      if((ph_acid_values[0] - ph_acid_values[1] <= 0.6) && (ph_acid_values[0] - ph_acid_values[1] >= 0))
      {
        ph_acid_values[0] = ph_act;
        i_acid = 1;
      }
      else
      {
        digitalWrite(EC_Isolator,HIGH); 
        digitalWrite(EC_GND_Wire, LOW);
        PH_sensor(); // first we get readings of PH sensor
        digitalWrite(EC_Isolator,LOW); 
        digitalWrite(EC_GND_Wire, HIGH);
        delay(1000);
      
        if((ph_act >= 10) || (ph_act <= 0))// for PH sensor
        {
          ph_act = -1;
          PH_CONDITION = "Check PH sensor";   
        }
        if(ph_act != -1)
        {
          ph_acid_values[1] = ph_act;
          i_acid = 0;
        }
        else
        {
          i_acid = 1;
        }
      }
      
    }
   }
   i_acid = 0;
   
   
  if (flag_error == 0)
  {
    Serial.println(ph_act , 2);// PH
    Serial.println(tdsValue , 2);// PPM
    Serial.println(t , 2);//AIR_TEMP
    Serial.println(actual_water_level , 2);// actual water level
    Serial.println(h , 2);// humidity
    Serial.println(Celsius , 2);// Water_temp
    delay(1000);
    if((ph_act == -1) || (tdsValue == -2) || (t == -3) || (actual_water_level == -4) || (h == -5) || (Celsius == -6))
    {
      flag_error = 1; 
    }
  }
  
  
  if (Serial.available())
  {
      label = Serial.readStringUntil('\n');
      label.trim();

      if(label.equals("[0]"))// all normal
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
        
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off";
        
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_HUMDITIY = "Fan is off";
        ph_normal_values[0] = ph_act;
      }

   else if(label.equals("[1]"))// all high 
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
         digitalWrite(acid , LOW);
         delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
         digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on";
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[2]"))// all low  
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off";
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[3]"))// PH & PPM normal & Temp low
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[4]"))// PH & PPM normal & Temp high
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "PWater UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[5]"))// PH & Temp normal & PPM low 
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[6]"))// PH & Temp normal & PPM high
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[7]"))// Temp & PPM normal & PH low 
     {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[8]"))// Temp & PPM normal & PH high
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[9]"))// PH normal & PPM & Temp low 
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[10]"))// PH normal & PPM & Temp high
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[11]"))// PH normal & PPM high & Temp low  
     {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[12]"))// PH normal & PPM low & Temp high
      {
        PH_CONDITION = "PH is normal";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump off";
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_normal_values[0] = ph_act;
      }
    else if(label.equals("[13]"))// PPM normal & PH & Temp low 
     {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[14]"))// PPM normal & PH & Temp high 
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[15]"))// PPM normal & PH high & Temp low
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[16]"))// PPM normal & PH low & Temp high 
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is normal";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[17]"))// Temp normal & PH & PPM low
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[18]"))// Temp normal & PH & PPM high
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[19]"))// Temp normal & PH high & PPM low 
     {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[20]"))// Temp normal & PH low & PPM high 
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is normal";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[21]"))// PH high & PPM & Temp low 
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off";
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        } 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[22]"))// PH low & PPM & Temp high 
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[23]"))// PPM high & PH & Temp low
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[24]"))// PPM low & PH & Temp high 
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off";
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        } 
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_acid_values[0] = ph_act;
      }
    else if(label.equals("[25]"))// Temp high & PH & PPM low
      {
        PH_CONDITION = "PH is low";
        PH_DOWN_PUMP = "PH down pump off";
        PH_UP_PUMP = "PH up pump on";
        digitalWrite(base , LOW);
        delay(10000);// open it for 10 sec to increase ph level by 0.3 to 0.5
        digitalWrite(base , HIGH);
                
        PPM_CONDITION = "PPM is low";
        PUMPS_A_B = "A&B pumps on";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
        for(int i = 0 ; i < 3 ; i++)
        {// EACH LOOP INCREASE PPM LEVEL BY 11 PPM AND FOR 3 ROUNDS IT IS GOING TO BE 33 PPM 
          digitalWrite(nutrientA , LOW);
          delay(21000);
          digitalWrite(nutrientA , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient A and nutrient B

          digitalWrite(nutrientB , LOW);
          delay(21000);
          digitalWrite(nutrientB , HIGH);
          delay(2000);// 2 seconds waiting between adding nutrient B and nutrient A
        }
                
        AIR_TEMP_CONDITION = "Air_T is high";
        FAN_FOR_AIR_TEMP = "Fan is on";
        ph_base_values[0] = ph_act;
      }
    else if(label.equals("[26]"))// Temp low & PH & PPM high 
      {
        PH_CONDITION = "PH is high";
        PH_DOWN_PUMP = "PH down pump on";
        PH_UP_PUMP = "PH up pump off";
        digitalWrite(acid , LOW);
        delay(10000);// open it for 10 sec to decrease ph level by 0.5 to 0.7
        digitalWrite(acid , HIGH);
                
        PPM_CONDITION = "PPM is high";
        PUMPS_A_B = "A&B pumps off";
        WATER_PUMP_UP_FOR_PPM = "Water UpPump on"; 
                
        AIR_TEMP_CONDITION = "Air_T is low";
        FAN_FOR_AIR_TEMP = "Fan is off";
        ph_acid_values[0] = ph_act;
      }
    
  }
  if(flag_error == 0)// which means nothing wrong with any of sensors
   {// note that these limits is going to change 
     if ((actual_water_level >= 15) && (actual_water_level <= 25))// normal 
     {
       VALVE_CONDITION = "Valve off";
       WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump off";
       WATER_LEVEL_CONDITION = "Water Lev normal";
     }
     else if (actual_water_level < 15)// low
     {
       VALVE_CONDITION = "Valve off";
       WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump on";
       WATER_LEVEL_CONDITION = "Water Lev low";
       digitalWrite(level_up_pump , HIGH);
       delay(3000);// open it for increasing water level by adding 130.5 mL
       digitalWrite(level_up_pump , LOW);
     }
     else if ((actual_water_level > 25) && (actual_water_level < 35))// high
     {
       VALVE_CONDITION = "Valve on";
       WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump off";
       WATER_LEVEL_CONDITION = "Water Lev high";
     }
  
     /*DHT22 (HUMIDITY) sensor*/
     if ((h <= 70) && (h > 0))// normal
     {
       HUMD_CONDITION = "humidity normal";
       FAN_FOR_HUMDITIY = "Fan is off"; 
     }
     else if (h > 70)// high
     {
       HUMD_CONDITION = "humidity high";
       FAN_FOR_HUMDITIY = "Fan is on"; 
     }
  
     /*water temeprature sensor*/
     if ((Celsius <= 26) && (Celsius > 0))// normal
     {
       WATER_TEMP_CONDITION = "wat_temp normal";
       COOLER_CONDITION = "Cooler off";
     }
     else if (Celsius > 26)// high
     {
       WATER_TEMP_CONDITION = "wat_temp high";
       COOLER_CONDITION = "Cooler on";
     }

     Serial.println(PH_CONDITION);
     Serial.println(PH_DOWN_PUMP);
     Serial.println(PH_UP_PUMP);
      
     Serial.println(PPM_CONDITION);
     Serial.println(PUMPS_A_B);
     Serial.println(WATER_PUMP_UP_FOR_PPM);
      
        
     Serial.println(AIR_TEMP_CONDITION);
     Serial.println(FAN_FOR_AIR_TEMP);
        
     Serial.println(WATER_LEVEL_CONDITION);
     Serial.println(VALVE_CONDITION);
     Serial.println(WATER_PUMP_UP_FOR_WATER_LEVEL);
        
     Serial.println(HUMD_CONDITION);
     Serial.println(FAN_FOR_HUMDITIY);
        
     Serial.println(WATER_TEMP_CONDITION);
     Serial.println(COOLER_CONDITION);
     delay(13600);

     //for PH sensor
     lcd.setCursor(0,0); 
     lcd.print("PH = ");          
     lcd.print(ph_act , 2); 
     lcd.setCursor(0,1);           
     lcd.print(PH_UP_PUMP); // PH_CONDITION = "PH - pump on" or "PH + pump ↓"
     delay(3000);
     lcd.clear();
    
     lcd.setCursor(0,0); 
     lcd.print("PH = ");          
     lcd.print(ph_act , 2); 
     lcd.setCursor(0,1);           
     lcd.print(PH_DOWN_PUMP); // PH_CONDITION = "PH + pump ↑" or "PH + pump ↓"
     delay(3000);
     lcd.clear();
    
     //for TDS sensor
     lcd.setCursor(0,0); 
     lcd.print("TDS = ");          
     lcd.print(tdsValue , 2); 
     lcd.print(" PPM"); 
     lcd.setCursor(0,1);           
     lcd.print(PUMPS_A_B);//PUMPS_A_B = "PPM A&B PUMPS ↑" or "PPM A&B PUMPS ↓"
     delay(3000);
     lcd.clear();
    
     lcd.setCursor(0,0);
     lcd.print("TDS = ");           
     lcd.print(tdsValue , 2);
     lcd.print(" PPM");  
     lcd.setCursor(0,1);           
     lcd.print(WATER_PUMP_UP_FOR_PPM);// WATER_PUMP_UP = "water PUMP ↑" or "water PUMP ↓"
     delay(3000);
     lcd.clear();
  
     //for air temperature sensor
     lcd.setCursor(0,0); 
     lcd.print("Air_temp= ");          
     lcd.print(t , 2);
     lcd.print("C"); 
     lcd.setCursor(0,1);           
     lcd.print(FAN_FOR_AIR_TEMP);// HUMD_CONDITION = "fan ↓" or "fan ↑"
     delay(3000);
     lcd.clear();
  
     //for water level sensor
     lcd.setCursor(0,0);  
     lcd.print("Dist = ");        
     lcd.print(distance , 2);
     lcd.print(" cm"); 
     lcd.setCursor(0,1);           
     lcd.print(WATER_PUMP_UP_FOR_WATER_LEVEL);// WATER_PUMP_UP = "water PUMP ↓" or "water PUMP ↑"
     delay(3000);
     lcd.clear();
    
     lcd.setCursor(0,0);          
     lcd.print("Dist = ");        
     lcd.print(distance , 2);
     lcd.print(" cm"); 
     lcd.setCursor(0,1);           
     lcd.print(VALVE_CONDITION);//VALVE_CONDITION = "valve ↓" or "valve ↑"
     delay(3000);
     lcd.clear();
  
     //for humdidty sensor
     lcd.setCursor(0,0); 
     lcd.print("hum = ");          
     lcd.print(h , 2); 
     lcd.print(" %"); 
     lcd.setCursor(0,1);           
     lcd.print(FAN_FOR_HUMDITIY);// AIR_TEMP_CONDITION = "fan ↓" or "fan ↑" 
     delay(3000);
     lcd.clear();
    
     //for water temperature sensor
     lcd.setCursor(0,0);
     lcd.print("wat_temp= ");            
     lcd.print(Celsius , 2); 
     lcd.print("C");  
     lcd.setCursor(0,1);           
     lcd.print(COOLER_CONDITION);// WATER_TEMP_CONDITION = "Chiller ↓" or "Chiller ↑"
     delay(3000);
     lcd.clear();
   }
   
   if(flag_error == 1)
   {
      //PH sensor
      PH_DOWN_PUMP = "PH down pump off";
      PH_UP_PUMP = "PH up pump off";
      digitalWrite(acid , HIGH);
      digitalWrite(base , HIGH);

      //PPM
      PUMPS_A_B = "A&B pumps off";
      WATER_PUMP_UP_FOR_PPM = "Water UpPump off"; 
      digitalWrite(nutrientA , HIGH);
      digitalWrite(nutrientB , HIGH);
      
      //AIR_TEMP
      FAN_FOR_AIR_TEMP = "Fan is off";
      
      //WATER_LEVEL
      VALVE_CONDITION = "Valve off";
      WATER_PUMP_UP_FOR_WATER_LEVEL = "Water UpPump off";
      WATER_LEVEL_CONDITION = "Check Ultrsonic";
      digitalWrite(level_up_pump , LOW);// relay of water level pump is active high
      //water_temp
      COOLER_CONDITION = "Cooler off";
      
      //HUMDITIY
      FAN_FOR_HUMDITIY = "Fan is off";
   }
   if(ph_act == -1)
   {
    lcd.setCursor(0,0);          
    lcd.print(PH_CONDITION);  
    delay(3000);
    lcd.clear();
   }
   if(tdsValue == -2)
   {
    lcd.setCursor(0,0);          
    lcd.print(PPM_CONDITION); 
    delay(3000);
    lcd.clear();
   }
   if(t == -3)
   {
    lcd.setCursor(0,0); 
    lcd.print(AIR_TEMP_CONDITION);           
    delay(3000);
    lcd.clear();
   }
   if(distance == -4)
   {
    lcd.setCursor(0,0); 
    lcd.print(WATER_LEVEL_CONDITION);           
    delay(3000);
    lcd.clear();
   }
   if(h == -5)
   {
    lcd.setCursor(0,0); 
    lcd.print(HUMD_CONDITION);          
    delay(3000);
    lcd.clear();
   }
   if(Celsius == -6)
   {
    lcd.setCursor(0,0); 
    lcd.print(WATER_TEMP_CONDITION);           
    delay(3000);
    lcd.clear();
   }
     
   while(num_mins > 0)
   {
    lcd.setCursor(0,0);
    lcd.print("Countdown:");
    lcd.setCursor(0,1);
    lcd.print("0");
    lcd.print(num_mins);
    lcd.print(":");
    lcd.print("0");
    lcd.print(num_sec);
    delay(1000);
    lcd.clear();
    num_sec = 59;
    num_mins--;
    while(num_sec > 0)
    {
     if(countDigits(num_sec) == 2)
     {
      lcd.setCursor(0,0);
      lcd.print("Countdown:");
      lcd.setCursor(0,1);
      lcd.print("0");
      lcd.print(num_mins);
      lcd.print(":");
      lcd.print(num_sec);
      delay(1000);
      lcd.clear();
     }
     else if((countDigits(num_sec) == 1) || (countDigits(num_sec) == 0))
     {
      lcd.setCursor(0,0);
      lcd.print("Countdown:");
      lcd.setCursor(0,1);
      lcd.print("0");
      lcd.print(num_mins);
      lcd.print(":");
      lcd.print("0");
      lcd.print(num_sec);
      delay(1000);
      lcd.clear();
     }
     button_state = digitalRead(buttonpin);
     if(button_state == 1)
     {
      flag_error = 0;
      delay(100);
      lcd.setCursor(0,0);
      lcd.print("button pressed");
      delay(100);
      lcd.clear();
     }
     num_sec--;
    }  
   }
   num_mins = 9;// according to our calculations
   num_sec = 0;
     
}

uint8_t countDigits(int num)
{
  uint8_t count = 0;
  while(num)
  {
    num = num / 10;
    count++;
  }
  return count;
}

void temperature_and_humidity_sensor()
{
//Read data and store it to variables h (humidity) and t (temperature)
    // Reading temperature or humidity takes about 250 milliseconds!
    h = dht.readHumidity();
    t = dht.readTemperature();
}

void ultrasonic_sensor()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.0343 / 2;
  actual_water_level = 35 - distance; /*note that actual_water_level is the actual water level and we need to figure it*/
}

void PH_sensor()
{
 for(int i=0;i<10;i++) 
 { 
 buffer_arr[i]=analogRead(A0);
 delay(30);
 }
 for(int i=0;i<9;i++)
 {
 for(int j=i+1;j<10;j++)
 {
 if(buffer_arr[i]>buffer_arr[j])
 {
 temp=buffer_arr[i];
 buffer_arr[i]=buffer_arr[j];
 buffer_arr[j]=temp;
 }
 }
 }
 avgval=0;
 for(int i=2;i<8;i++)
 avgval+=buffer_arr[i];
 float volt= (float)avgval * (5.0) / 1024 / 6;
 ph_act = -5.70 * volt + calibration_value;
}

void waterproof_sensor()
{
  sensors.requestTemperatures();// Send the command to get temperatures

  Celsius = sensors.getTempCByIndex(0);        //<==
}

void TDS_sensor()
{
    sensors.requestTemperatures();
    gravityTds.setTemperature(sensors.getTempCByIndex(0));  // set the temperature and execute temperature compensation
    gravityTds.update();  //sample and calculate
    tdsValue = gravityTds.getTdsValue();  // then get the value
}
