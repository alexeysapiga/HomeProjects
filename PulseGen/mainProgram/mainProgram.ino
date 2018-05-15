#include <SPI.h>
#define PIN_WR 37
#define PIN_RD 36
#define PIN_RES_DD9 35

#define PIN_A27 34
#define PIN_A28 33
#define PIN_A29 32
#define PIN_A_1 31
#define PIN_A_0 30
#define LCD_RESET 48
#define LCD_RS 49
#define LCD_CS 53

const unsigned char Enable_II = B10000000;
const unsigned char Enable_I = B01000000;
const unsigned char Enable_I_II = B00100000;
const unsigned char Enable_III = B00010000;
const unsigned char Enable_I_II_III = B00001000;
const unsigned char Enable_ST = B00000100;
const unsigned char Enable_I_II_III_ST = B00000010;

unsigned char CurrentWorkingMode = 0;

void SelectDD9() {
  digitalWrite(PIN_A29, HIGH);
  digitalWrite(PIN_A28, HIGH);
  digitalWrite(PIN_A27, LOW);
}

void SelectDD8() {
  digitalWrite(PIN_A29, LOW);
  digitalWrite(PIN_A28, HIGH);
  digitalWrite(PIN_A27, LOW);
}

void SelectDD7() {
  digitalWrite(PIN_A29, HIGH);
  digitalWrite(PIN_A28, LOW);
  digitalWrite(PIN_A27, LOW);
}

void SelectDD6() {
  digitalWrite(PIN_A29, LOW);
  digitalWrite(PIN_A28, LOW);
  digitalWrite(PIN_A27, LOW);
}

void ConfigDD9() {
  SelectDD9();  
  digitalWrite(PIN_RES_DD9, HIGH);  
  digitalWrite(PIN_RES_DD9, LOW);  
  digitalWrite(PIN_RD, HIGH);  
  
  PORTA = B10000000;
  digitalWrite(PIN_RES_DD9, LOW);
  digitalWrite(PIN_A_0, HIGH);
  digitalWrite(PIN_A_1, HIGH);
  digitalWrite(PIN_RD, HIGH);
  digitalWrite(PIN_WR, LOW);
  digitalWrite(PIN_WR, HIGH); 
  digitalWrite(PIN_A27, HIGH);
}

void WriteToDD9(unsigned char data, char port){
  SelectDD9();  
  
  digitalWrite(PIN_RES_DD9, LOW);
  if (port==0) {
    digitalWrite(PIN_A_0, HIGH); //A0
    digitalWrite(PIN_A_1, LOW); //A1
  }
  else {
    digitalWrite(PIN_A_0, LOW); //A0
    digitalWrite(PIN_A_1, HIGH); //A1
  }
  digitalWrite(PIN_RD, HIGH); //RD
  digitalWrite(PIN_WR, LOW);   //WR
  PORTA = data;
  digitalWrite(PIN_WR, HIGH);   //WR
  digitalWrite(PIN_A27, HIGH);
}

void EnableMode(unsigned char mode) {
  CurrentWorkingMode |= mode;
  WriteToDD9(CurrentWorkingMode, 0);
}

void DisableMode(unsigned char mode) {
  CurrentWorkingMode &= ~mode;
  WriteToDD9(CurrentWorkingMode, 0);
}

unsigned int
Reverse(register unsigned int x)
{
  x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
  x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
  x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
  x = (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8));
  return x;
}

void ConfigTimer(unsigned char timer, unsigned char mode) {
  unsigned char timerValue = 0x30;
  timerValue |= timer << 6;
  timerValue |= mode << 1;
  
  digitalWrite(PIN_RD, HIGH); //RD
  digitalWrite(PIN_WR, HIGH);   //WR
  digitalWrite(PIN_A_0, HIGH); //A0
  digitalWrite(PIN_A_1, HIGH); //A1
  PORTA = timerValue;
  //Serial.print("TIMER VAl ");
  //Serial.println(timerValue);
 /* if (timer == 0) {
   PORTA = B01001100; 
  } else if (timer == 1) {
   PORTA = B01001110; 
  } else {
   PORTA = B01001101; 
  }*/
   delayMicroseconds(2);
  digitalWrite(PIN_WR, LOW);   //WR  
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);   //WR
}

void WriteTimer(unsigned char number, unsigned int value){
  
  unsigned char low = value & 0xFF;
  unsigned char high = (value & 0xFF00) >> 8;
 digitalWrite(PIN_RD, HIGH); //RD
 digitalWrite(PIN_WR, HIGH);   //WR

  if (number == 0) {
    digitalWrite(PIN_A_0, LOW); //A0
    digitalWrite(PIN_A_1, LOW); //A1  
  } else if (number == 1) {
    digitalWrite(PIN_A_0, HIGH); //A0
    digitalWrite(PIN_A_1, LOW); //A1
  } else {
    digitalWrite(PIN_A_0, LOW); //A0
    digitalWrite(PIN_A_1, HIGH); //A1
  }
  
  PORTA = low; 
  delayMicroseconds(2);
  digitalWrite(PIN_WR, LOW);   //WR 
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);   //WR

  PORTA = high;
  delayMicroseconds(2);
  digitalWrite(PIN_WR, LOW);   //WR
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);   //WR
  Serial.print("TIMER ");
  Serial.print(number);
  Serial.print(" ");
  Serial.print(high);
  Serial.print(" ");
  Serial.println(low);
}

void MakePulseI(){
  SelectDD6();
  ConfigTimer(0,1);
  ConfigTimer(1,1);
  ConfigTimer(2,1);
  
  WriteTimer(1, 1);//2 mcs
  WriteTimer(2, 400);//40 mcs
  WriteTimer(0, 1); //4 mcs
  digitalWrite(PIN_A27, HIGH);
 
}

void SetDD7(){
   SelectDD7();
  ConfigTimer(0,1);  
  ConfigTimer(1,2);  
  ConfigTimer(2,1);
  
  WriteTimer(1, 100);
  WriteTimer(0, 6);
  WriteTimer(2, 600);
}

void SetDD8(){
   SelectDD8();
  ConfigTimer(0,1);
  WriteTimer(0, 10);
  ConfigTimer(1,2);
  WriteTimer(1, 100);
  ConfigTimer(2,1);
  WriteTimer(2, 100);
}


void WriteLCD(){
  SPI.begin();
  SPI.beginTransaction(SPISettings(7300000, MSBFIRST, SPI_MODE3));
  //Startup
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RESET, LOW);
  delay(75);
  digitalWrite(LCD_RESET, HIGH);
  delay(525);
  //Init
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_CS, LOW);
  SPI.transfer(0x81);
  SPI.transfer(0x45);
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_CS, LOW);
  unsigned char initData[]={0xA1, 0xC0, 0xA6, 0x74, 0x00, 0x81, 0x45, 0x40,
  0x55, 0xAB, 0x27, 0x8A, 0x00, 0xB0, 0x10, 0x00, 0xA4, 0xAF };
  unsigned char sz = sizeof(initData)/sizeof(unsigned char);
  for (unsigned char i=0; i<18; i++){
    SPI.transfer(initData[i]);
  }
  digitalWrite(LCD_CS, HIGH);
  //Data
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_CS, LOW);
    SPI.transfer(0xB5);
    SPI.transfer(0x10);
    SPI.transfer(0x00);  
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  for (unsigned char i=0; i<100; i++){
    SPI.transfer(0xaa);
  }
  digitalWrite(LCD_CS, HIGH);
}

void setup() {
  // put your setup code here, to run once:
  DDRA = 0xFF;
  DDRC = 0xFF;
  PORTA = 0x00;
  PORTC = 0x00;
  pinMode(LCD_RESET, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  
    Serial.begin(9600);
  ConfigDD9();  
  delay(1000);
  CurrentWorkingMode = 0x00;  
  WriteToDD9(CurrentWorkingMode, 0);
  WriteToDD9(0x00, 1);

  MakePulseI();
  delay(1);
 SetDD7();
 SetDD8();
 
 delay(1);
 WriteToDD9(0xFF, 1);
   digitalWrite(PIN_A27, HIGH);

   WriteLCD();
}

void loop() {
  WriteLCD();
  delay(100);
  /*delay(100);
 // MakePulseI();
  delayMicroseconds(2);
  WriteToDD9(0xFF, 1);
  delay(10);
  WriteToDD9(0x00, 1);  */
}
/*  digitalWrite(PIN_RD, HIGH); //RD
  digitalWrite(PIN_WR, HIGH);   //WR
  digitalWrite(PIN_A_0, HIGH); //A0
  digitalWrite(PIN_A_1, HIGH); //A1
  PORTA = B01001110; 
  digitalWrite(PIN_WR, LOW);   //WR  
  digitalWrite(PIN_WR, HIGH);   //WR
   
  digitalWrite(PIN_A_0, HIGH); //A0
  digitalWrite(PIN_A_1, LOW); //A1
  PORTA = 0x00; 
  digitalWrite(PIN_WR, LOW);   //WR 
  digitalWrite(PIN_WR, HIGH);   //WR

  PORTA = 0x80;
  digitalWrite(PIN_WR, LOW);   //WR
  digitalWrite(PIN_WR, HIGH);   //WR
  
  digitalWrite(PIN_A27, HIGH);*/
