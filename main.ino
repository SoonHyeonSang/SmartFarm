#include <Servo.h>
#include <DHT.h>
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Setting GPIO PIN
#define DHTPIN          12
#define DHTTYPE         DHT11
#define SERVOPIN        9
#define LIGHTPIN        4
#define FAN_PIN         32
#define WATER_PUMP_PIN  31

// Setting
#define USE_NETWORK   1
#define USE_BLUETOOTH 1
#define USE_SCHEDULE  1
#define DEBUG         1

float temperature, humidity;
int angle = 0;
int co2_ppm = 0; 
int RBG_R = 4; 
int RBG_G = 35; 
int RBG_B = 36; 
int cdcValue = 0;
int waterValue = 0;
int lightOutput = 0;
int fanOutput = 0;
int timeout = 0; 
bool water_State = false ;// add code - Lee
unsigned water_Time = 0 ;// add code - Lee
unsigned local_time = 0; // add code - Lee

char sData[64] = { 0x00, };
char rData[32] = { 0x00, };
char nData[32] = { 0x00, };
int rPos = 0;
int nPos = 0;
int right = 10;
int displayToggle = 1;

DHT dht(DHTPIN, DHTTYPE);
Servo servo; 
LiquidCrystal_I2C lcd(0x27, 16, 2);

//========================================================
#include "WiFiEsp.h"
char ssid[] = "SHS";   // your network SSID (name)
char pass[] = "jk3360110";  // your network password
int status = WL_IDLE_STATUS;  // the Wifi radio's status
WiFiEspServer server_f(400);
//========================================================

//--------------------------------------------
//  Scheduling [DO] Function
#define SCH_DO_LIGHT_CH   1
#define SCH_DO_WPUMP_CH   2
#define SCH_DO_SMOTOR_CH  4
#define SCH_DO_PAN_CH     8

#define MAX_DO    4
#define MAX_SHEDULE 24


typedef struct Scheduling {
  int iTime;
  int iDoing;
  int iParameter[4];
}SCHEDULE;

DS3231 Clock;
bool Century,h12,PM;
int gYear, gMonth, date, DoW, gHour, gMinute, gSecond;
int prehour = -1;

SCHEDULE Set_Shedule[MAX_SHEDULE] = {
  {1, 15, {0, 0, 0, 1}},  // 1시에 1+8(조명+팬) {10,0,0,켜라}  
  {2, 15, {0, 1, 1, 0}},
  {3, 15, {0, 0, 0, 1}},    
  {4, 15, {0, 0, 0, 0}},
  {5, 15, {0, 0, 0, 1}}, 
  {6, 15, {0, 1, 1, 0}},    
  {7, 15, {0, 0, 0, 1}},
  {8, 15, {0, 0, 0, 0}},    
  {9, 15, {0, 0, 0, 1}},
  {10, 15, {10, 1, 1, 0}},
  {11, 15, {10, 0, 0, 1}},   
  {12, 15, {10, 0, 0, 0}},
  {13, 15, {10, 0, 0, 1}},    
  {14, 15, {10, 1, 1, 0}},
  {15, 15, {10, 0, 0, 1}},
  {16, 15, {10, 0, 0, 0}},    
  {17, 15, {10, 0, 0, 1}},
  {18, 15, {10, 1, 1, 0}},    
  {19, 15, {10, 1, 0, 0}},
  {20, 15, {10, 0, 0, 1}},
  {21, 15, {0, 0, 0, 1}},    
  {22, 15, {0, 1,1, 0}},
  {23, 15, {0, 0, 0, 1}},    
  {24, 15, {0, 0, 0, 0}} // 24시에 2+4(물펌프+서보모터(창문)) { 끄고, 켜고,켜고,끄라}
};

void DoLight(int iPara){
  analogWrite(LIGHTPIN, (int)(25 * iPara));
}

void DoWPump(int iPara){
  if(iPara == 0) 
    digitalWrite(WATER_PUMP_PIN, 0);
  //else digitalWrite(WATER_PUMP_PIN, 1);
  else {
    digitalWrite(WATER_PUMP_PIN, 1);
    water_State = true;
    }
           
}

void DoSMoter(int iPara){
 if(iPara == 0) 
  angle = 10;
 else 
  angle = 80;
 servo.write(angle);
 //delay(200);  
}

void DoPan(int iPara){
  if(iPara == 0) 
  digitalWrite(FAN_PIN, 0);
  else 
  digitalWrite(FAN_PIN, 1); 
}

int SearchingAction()
{
  int i;

  // gDefaultShedule가 24개의 array
  for(i=0;i<MAX_SHEDULE;i++)
  {
    if (gHour == Set_Shedule[i].iTime)
      return i;
  }
   return -1;
}

void DoAction(int iDo, int iPara[]){
  int i=0;
  int iIndex;
  for(i=0;i<MAX_DO;i++)
  { 
   // iIndex = iDo & 2 ^ i; 
    iIndex = iDo & (1<<i); 
    
    switch(iIndex)
    {
      case SCH_DO_LIGHT_CH:
        DoLight(iPara[i]);
        break;
  
      case SCH_DO_WPUMP_CH:
        DoWPump(iPara[i]);
        break;
        
      case SCH_DO_SMOTOR_CH:
        DoSMoter(iPara[i]);
        break;
  
      case SCH_DO_PAN_CH:
        DoPan(iPara[i]);
        break;
        
      default:
        break;
    }
  }
}

void ReadDS3231(){ //rtc
  gHour=Clock.getHour(h12,PM);
  }

//--------------------------------------------
void printLCD(int col, int row , char *str) {
    for(int i=0 ; i < strlen(str) ; i++){
      lcd.setCursor(col+i , row);
      lcd.print(str[i]);
    }}

void printWifiStatus(){
#if 1 // #if DEBUG
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
#endif

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  delay(10);
#if 1 // #if DEBUG
  Serial.print("IP Address: ");
  Serial.println(ip);
#endif
   char ipno2[26] ;
   sprintf(ipno2, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
     printLCD(0, 1, ipno2);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  
#if 1 // #if DEBUG
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
#endif
}

void setup() {
  pinMode(LIGHTPIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(RBG_R,OUTPUT);
  pinMode(RBG_G,OUTPUT);
  pinMode(RBG_B,OUTPUT);
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  dht.begin();

  while (!Serial) {
    ; // wait for serial port to connect. 
  }

  lcd.init();
  lcd.backlight();
  printLCD(0, 0, "MMB_SmartFarm");
  printLCD(0, 1, "NETWORKING...");  

#if USE_NETWORK
  // initialize serial for ESP module
  Serial2.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial2);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
     #if DEBUG
    Serial.println("WiFi shield not present");
   #endif
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
     #if DEBUG
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
   #endif
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
   #if DEBUG
  Serial.println("You're connected to the network");
   #endif
  printWifiStatus(); // display IP address on LCD
  delay(2000);
   
  server_f.begin(); 
#endif
  
  #if DEBUG
  Serial.println("START");
  #endif
}

void loop() { 

  // put your main code here, to run repeatedly:
  timeout += 1;
   
   if(timeout % 10 == 0) {

     cdcValue = analogRead(0);
     cdcValue /= 10;
     //Serial.print(cdcValue); Serial.print(",");
     
     waterValue = analogRead(1);
     waterValue /= 10;
     //Serial.print(waterValue); Serial.print(",");

     humidity = dht.readHumidity();
     temperature = dht.readTemperature();

    if(temperature){

      if(temperature<=23){
        digitalWrite(LIGHTPIN, HIGH);
        digitalWrite(FAN_PIN, LOW);
      }
      else if(temperature>=27){
        digitalWrite(LIGHPIN, LOW);
        digitalWrite(FAN_PIN, HIGH):
      }
    }

    if(waterValue){

      if(waterValue){
        digitalWrite(WATER_PUMP_PIN, HIGH);
      }
      else if(waterValue){
        digitalWrite(WATER_PUMP_PIN, LOW);
      }
    }
    

     lcd.clear();
     displayToggle = !displayToggle;
     if(displayToggle == 1) {
        memset(sData, 0x00, 64);
        sprintf(sData, "temp %02dC humi %02d%%", (int)temperature,
        (int)humidity);
        printLCD(0, 0, sData);
        memset(sData, 0x00, 64);
        sprintf(sData, "cdc%-04d soil%-04d", cdcValue, waterValue);
        printLCD(0, 1, sData);
     }
     else {
        memset(sData, 0x00, 64);
        sprintf(sData, "temp %02dC humi %02d%%", (int)temperature, 
        (int)humidity);
        printLCD(0, 0, sData);
        memset(sData, 0x00, 64);
        sprintf(sData, "co2 %d ppm", co2_ppm);
        printLCD(0, 1, sData);
     }

 sprintf(sData, "{ \"temp\":%02d,\"humidity\":%02d,\"cdc\":%-04d,\"water\":%-04d,\"co2\":%-04d }",
 (int)temperature, (int)humidity, cdcValue, waterValue, co2_ppm);
    // Serial.println(sData);

    // Bluetooth Data Sending
    Serial1.println(sData);
  }

  // Bluetooth Control
  while(0 < Serial1.available()) 
  {
    char ch = Serial1.read();
    rData[rPos] = ch; rPos += 1;
    Serial.print(ch);

    if(ch == '\n')
    {               
#if DEBUG
      Serial.print("rPos=");
      Serial.print(rPos);
      Serial.print(" ");
      Serial.println(rData);
#endif
  
      if(memcmp(rData, "C_S-", 4) == 0)
      {
        if(rData[4] == '0') angle = 10;
        else angle = 80;
        servo.attach(SERVOPIN);
        servo.write(angle); 
        delay(500);
        servo.detach();
#if DEBUG
        Serial.print("server_f_MOTOR=");
        Serial.println(angle);
#endif
      }
  
      if(memcmp(rData, "C_F-", 4) == 0)
      {
        if(rData[4] == '0') digitalWrite(FAN_PIN, 0);
        else digitalWrite(FAN_PIN, 1);
#if DEBUG
        Serial.print("FAN=");
        Serial.println(rData[4]);
#endif
      }
  
      if(memcmp(rData, "C_L-", 4) == 0)
      {
        int light = atoi(rData+4);
        analogWrite(LIGHTPIN, (int)(25 * light));
#if DEBUG
        Serial.print("LIGHT=");
        Serial.println(25 * light); // light);
#endif
      }
  
      if(memcmp(rData, "C_W-", 4) == 0)
      {
        if(rData[4] == '0') digitalWrite(WATER_PUMP_PIN, 0);
        //else digitalWrite(WATER_PUMP_PIN, 1);
        else {
        digitalWrite(WATER_PUMP_PIN, 1);
        water_State = true;
        }
    
#if DEBUG
        Serial.print("WATER=");
        Serial.println(rData[4]);
#endif
      }
  
      rPos = 0;
      memset(rData, 0x00,32);
      break;
    }
    delay(10);
  }

  // Network Communication  
#if USE_NETWORK
  WiFiEspClient c = server_f.available();
  if(c) 
  {
#if DEBUG
    Serial.println("N#RECV: ");
#endif
    boolean bDataRead = false;
  
    nPos = 0;
  
    while (0 < c.available()) {
      char ch = c.read();
#if DEBUG
      Serial.write(ch);
#endif
      bDataRead = true;
      nData[nPos] = ch; nPos += 1;
    }
  
    if(bDataRead == true) {
#if DEBUG
      Serial.print("nData=");
      Serial.println(nData);
      Serial.println();
#endif
      c.print(sData);
  
      if(5 <= nPos)
      {   
#if DEBUG
        Serial.println(nData);
#endif
  
        if(memcmp(nData, "C_S-", 4) == 0)
        {  
          if(nData[4] == '0') angle = 10;
          else angle = 80;
          servo.attach(SERVOPIN);
          servo.write(angle); 
          delay(500);
          servo.detach();
#if DEBUG
          Serial.print("N#server_f_MOTOR=");
          Serial.println(angle);
#endif
        }
  
        if(memcmp(nData, "C_F-", 4) == 0)
        {
  
          if(nData[4] == '0') digitalWrite(FAN_PIN, 0);
          else digitalWrite(FAN_PIN, 1);
#if DEBUG
          Serial.print("N#FAN=");
          Serial.println(nData[4]);
#endif  
        }
  
        if(memcmp(nData, "C_L-", 4) == 0)
        {
          int light = atoi(nData+4);
          analogWrite(LIGHTPIN, (int)(25 * light));
          
          analogWrite(RBG_R, (int)(25 * light));
          analogWrite(RBG_G, (int)(25 * light));
          analogWrite(RBG_B, (int)(25 * light));
#if DEBUG
          Serial.print("N#LIGHT=");
          Serial.println(light);
#endif
  
        } 
  
        if(memcmp(nData, "C_W-", 4) == 0)
        {  
          if(nData[4] == '0') digitalWrite(WATER_PUMP_PIN, 0);
          else digitalWrite(WATER_PUMP_PIN, 1);
#if DEBUG
          Serial.print("N#WATER=");
          Serial.println(nData[4]);
#endif
        }
        nPos = 0;
        memset(nData, 0x00, 32);
      }
    }
  
    delay(10);
  }
#endif  // USE_NETWORK

#if USE_SCHEDULE
  ReadDS3231();
  
  // 시간이 바뀌는지 확인 (prehour = 9, hour = 10)
  if (prehour != gHour)
  {
    // 정의된 스케줄 중에 그 시간이 있는지 확인
    int iTime = SearchingAction();
    // prehour를 다시 설정
    prehour = gHour;
    // 정의된 스케줄 중에 그 시간이 있을 때만 DoAction
    if(iTime != -1)
    {
      DoAction(Set_Shedule[iTime].iDoing, Set_Shedule[iTime].iParameter);   
    }
  }
#endif

  // water_pump timer 5 Minutes
  if (water_State){
    water_Time +=1; 
     if ((water_Time) > 2500){
     digitalWrite(WATER_PUMP_PIN, 0);
     water_State = false; 
     water_Time =0;
    }
  }
/*
  local_time = millis(); 
  Serial.print("local_time = ");
  Serial.println(local_time);
  */
  
  delay(100); 
}
