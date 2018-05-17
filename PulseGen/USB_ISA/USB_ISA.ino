#define PIN_WR 2
#define PIN_RD 12
#define PIN_RES_DD9 3

#define PIN_A27 A3
#define PIN_A28 A2
#define PIN_A29 A1
#define PIN_A_1 A0
#define PIN_A_0 13

void DoCommandInt(unsigned char command, unsigned int value) {
  switch (command) {
    case 0x01: SelectDD6(); WriteTimer(1, value); break;
    case 0x02: SelectDD6(); WriteTimer(0, value); break;
    case 0x03: SelectDD7(); WriteTimer(0, value); break;
    case 0x04: SelectDD6(); WriteTimer(2, value); break;
    case 0x05: SelectDD7(); WriteTimer(2, value); break;
    case 0x06: SelectDD7(); WriteTimer(1, value); break;
    case 0x07: SelectDD8(); WriteTimer(1, value); break;
    case 0x08: SelectDD8(); WriteTimer(2, value); break;
    case 0x09: SelectDD8(); WriteTimer(0, value); break;
    default: break;
  }
}

void DoCommand(unsigned char command, unsigned char value0, unsigned char value1) {
  switch (command) {
    case 0x0A: SelectDD6(); ConfigTimer(value0, value1); break;
    case 0x0B: SelectDD7(); ConfigTimer(value0, value1); break;
    case 0x0C: SelectDD8(); ConfigTimer(value0, value1); break;
    case 0x0D: SelectDD9(); WriteToDD9(value0, value1); break;
    default: Blink(5); break;
  }
}

void DoCommand(unsigned char command, unsigned char value0) {
  switch (command) {
    case 0x0E: ConfigDD9(value0);break;
    default:Blink(5); break;
  }
}

void setup() {
  DDRC = 0xFF;
  DDRB = 0xFF;
  DDRD = 0xFF;
  // put your setup code here, to run once:
Serial.begin(9600);
/*
DoCommand(0xD, 0x80);
// delay(1000);
DoCommand(0xC, 0x00, 0);
DoCommand(0xC, 0x00, 1);
  //DD6
DoCommand(0x9, 0, 1);
DoCommand(0x9, 1, 1);
DoCommand(0x9, 2, 1);
  //DD7
DoCommand(0xA, 0, 1);
DoCommand(0xA, 1, 2);
DoCommand(0xA, 2, 1);
  //DD8
DoCommand(0xB, 0, 1);
DoCommand(0xB, 1, 2);
DoCommand(0xB, 2, 1);

DoCommandInt(0x1, 10);
DoCommandInt(0x2, 10);
DoCommandInt(0x3, 6);
DoCommandInt(0x6, 100);
DoCommandInt(0x7, 100);
DoCommandInt(0x4, 400);


DoCommandInt(0x5, 600);



DoCommandInt(0x8, 100);
//SelectDD8();
 //WriteTimer(0, 10);
//delay(1);
DoCommand(0xC, 0xFF, 0x1);*/
//digitalWrite(PIN_A27, HIGH);
 /*ConfigDD9(0x80);  
  delay(1000);
  
  WriteToDD9(0x00, 0);
  WriteToDD9(0x00, 1);

    SelectDD6();
  ConfigTimer(0,1);
  ConfigTimer(1,1);
  ConfigTimer(2,1);
  
  WriteTimer(1, 100);//2 mcs
  WriteTimer(2, 400);//40 mcs
  WriteTimer(0, 100); //4 mcs
  digitalWrite(PIN_A27, HIGH);
  delay(1);
   SelectDD7();
  ConfigTimer(0,1);  
  ConfigTimer(1,2);  
  ConfigTimer(2,1);
  
  WriteTimer(1, 100);
  WriteTimer(0, 6);
  WriteTimer(2, 600);
  SelectDD8();
  ConfigTimer(0,1);
  WriteTimer(0, 10);
  ConfigTimer(1,2);
  WriteTimer(1, 100);
  ConfigTimer(2,1);
  WriteTimer(2, 100);
 
 delay(1);
 WriteToDD9(0xFF, 1);
   digitalWrite(PIN_A27, HIGH);*/
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Info from NMR Pulse Gen"); 
  delay(300);
  if (Serial.available() > 0) {
    unsigned char buff[128];
    int cntRead = Serial.readBytes(buff, 128);
    ProcessBuffer(buff, cntRead);    
  }
}

void WriteData(unsigned char data)
{
  PORTB &= 0xF0;
  PORTD &= 0xF;
  PORTB |= data & 0xF;
  PORTD |= data & 0xF0;
}



void Blink(unsigned char cnt){
  for (unsigned char i=0;i<cnt; i++){
     digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
  }
}

void ProcessBuffer(unsigned char* buf, int len) {
  int pos = 0;
  while (pos < len) {
    if (buf[pos] <= 0x09) {
      unsigned int value = 0xFFFF;
      unsigned char v0 = buf[pos + 1];
      unsigned char v1 = buf[pos + 2];
      value = value & (buf[pos + 1] << 8);
      value = value | buf[pos + 2];      
      DoCommandInt(buf[pos], value);
      pos += 3;
    } else if (buf[pos] < 0xE) {
     DoCommand(buf[pos], buf[pos + 1], buf[pos + 2]);
     pos += 3;
    }
    else {
      DoCommand(buf[pos], buf[pos + 1]);      
      pos += 2;
    }
  }
}

void ConfigTimer(unsigned char timer, unsigned char mode) {
  unsigned char timerValue = 0x30;
  timerValue |= timer << 6;
  timerValue |= mode << 1;
  
  digitalWrite(PIN_RD, HIGH); //RD
  digitalWrite(PIN_WR, HIGH);   //WR
  digitalWrite(PIN_A_0, HIGH); //A0
  digitalWrite(PIN_A_1, HIGH); //A1
  WriteData(timerValue);
  delayMicroseconds(2);
  digitalWrite(PIN_WR, LOW);   //WR  
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);   //WR
  //digitalWrite(PIN_A27, HIGH);
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
  
  WriteData(low); 
  delayMicroseconds(2);
  digitalWrite(PIN_WR, LOW);   //WR 
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);   //WR

  WriteData(high);
  delayMicroseconds(2);
  digitalWrite(PIN_WR, LOW);   //WR
  delayMicroseconds(2);
  digitalWrite(PIN_WR, HIGH);   //WR
  //digitalWrite(PIN_A27, HIGH);
}

void ConfigDD9(unsigned char val) {
  SelectDD9();  
  digitalWrite(PIN_RES_DD9, HIGH);  
  digitalWrite(PIN_RES_DD9, LOW);  
  digitalWrite(PIN_RD, HIGH);  
  
  WriteData(val);
  digitalWrite(PIN_RES_DD9, LOW);
  digitalWrite(PIN_A_0, HIGH);
  digitalWrite(PIN_A_1, HIGH);
  digitalWrite(PIN_RD, HIGH);
  digitalWrite(PIN_WR, LOW);
  digitalWrite(PIN_WR, HIGH); 
 // digitalWrite(PIN_A27, HIGH);
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
  WriteData(data);
  digitalWrite(PIN_WR, HIGH);   //WR
 // digitalWrite(PIN_A27, HIGH);
}


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
