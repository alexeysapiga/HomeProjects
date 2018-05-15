#include <TimeLib.h>
#include <EEPROM.h>
#include <Servo.h>
#include <DS1307RTC.h>
#define MOTOR_PIN 9
#define LIGHT_PIN 10
#define MOTOR_SPEED 255 //0 - 255
#define MOTOR_WORKING_TIME 40000

const int LightOn = 510;
const int LightOff = 1320;
const int MorningFeed = 520;
const int EveningFeed = 1080;
Servo myservo;
int scheduleMotor[] = {520, 1080, 1440};
void setup() {
  // put your setup code here, to run once:
  //tmElements_t tm;
  //tm.Second = 00; 
  //tm.Minute = 33; 
  //tm.Hour = 10; 
  //tm.Wday = 4;
  //tm.Day = 21;
  //tm.Month=2;
  //tm.Year = 48;
  //RTC.write(tm);
  //FOR testing only 
  myservo.attach(11); 
  Serial.begin(9600);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  
  if (GetSavedFeed()==255){
    SaveFeed(0);
  }
  myservo.write(5);
  delay(3000);
  myservo.detach();
}

int TimeToMinutes(const tmElements_t& tm) {
  return tm.Hour*60 + tm.Minute;
}

void FeedFish(){
  analogWrite(MOTOR_PIN, MOTOR_SPEED);
  delay(MOTOR_WORKING_TIME);
  analogWrite(MOTOR_PIN, 0);
}


char GetSavedFeed() {
  unsigned char ch = EEPROM.read(1);
  //unsigned char cl = EEPROM.read(3);
  return ch;//((cl << 0) & 0xFF) + ((ch << 8) & 0xFF00);
}

void SaveFeed(char index) {
  EEPROM.write(1, index);// highByte(tm));
 //EEPROM.write(3, lowByte(tm));
}

void UpdateLight(int curTime) {
   if (curTime >= LightOn && curTime < LightOff) {
     digitalWrite(LIGHT_PIN, HIGH);
   }
   else {
    digitalWrite(LIGHT_PIN, LOW);
   }
}


void UpdateFeed(int curTime) {
  if (curTime >= MorningFeed && GetSavedFeed() != 2 && curTime < EveningFeed){//2 - morning
    SaveFeed(2);
    FeedFishServo();
  } else if (curTime >= EveningFeed && GetSavedFeed() != 3 ) {
    SaveFeed(3);
    //FeedFishServo();
  }
  /* char arrayMotorSize = sizeof(scheduleMotor)/sizeof(int);
   for (char k = 1; k < arrayMotorSize; k++) {
     if (curTime >= scheduleMotor[k-1] && curTime <= scheduleMotor[k]) {
        char saved = GetSavedFeed();
        if (k > saved || (k == 1 && saved == arrayMotorSize - 1)) {
          //FeedFish();
          FeedFishServo();
          SaveFeed(k);
          break;
        }
      }
    }*/
}

void FeedFishServo()
{
  myservo.attach(11);
  myservo.write(180);  
  delay(3000);
  myservo.write(160);  
  delay(500);
  myservo.write(180);  
  delay(10000);
  myservo.write(5);  
  delay(3000);
  myservo.detach();
}

void loop() {
   tmElements_t tm;
   RTC.read(tm);
   int curTime = TimeToMinutes(tm);
   Serial.print(curTime);  
   Serial.print(" "); 
   Serial.println(static_cast<int>(GetSavedFeed()));   
   UpdateLight(curTime);
   UpdateFeed(curTime);
   delay(10000);
}
