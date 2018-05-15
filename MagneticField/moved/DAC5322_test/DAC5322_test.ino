#include <SPI.h>
#define SYNC 3
#define LDAC 4
#define SCK 5
#define DIN 6
void setup() {
  // put your setup code here, to run once:
  pinMode(SYNC, OUTPUT);  
  pinMode(LDAC, OUTPUT);  
  pinMode(SCK, OUTPUT);  
  pinMode(DIN, OUTPUT);  
  pinMode(13, OUTPUT);  
  digitalWrite(SYNC, HIGH) ;
  digitalWrite(LDAC, HIGH) ;
   digitalWrite(SCK, HIGH) ;
  digitalWrite(13, LOW) ;
}

#define NOP asm volatile ("nop\n\t")
void busyWait(uint8_t count) { 
  for(uint8_t i = count; i > 0 ; i--) { NOP ; } 
}

void writeDac(unsigned int dacValue, char index) {
  unsigned int cur = 0x8000;
  dacValue &= 0xFFF;
  digitalWrite(SYNC, HIGH) ;
  digitalWrite(LDAC, HIGH) ;
  digitalWrite(SCK, HIGH) ;  
  NOP;
  NOP;  
  digitalWrite(SYNC, LOW) ;
    
  for (int i=0; i<16; i++){
      digitalWrite(SCK, HIGH);        
      digitalWrite(DIN,  ((dacValue & cur)>0)?HIGH:LOW) ;    
      cur = cur >> 1;
      digitalWrite(SCK, LOW);
  }
  
  digitalWrite(SYNC, HIGH) ;
  digitalWrite(LDAC, LOW) ;
  NOP;
  NOP;
  digitalWrite(LDAC, HIGH) ;

}

int curPos = 0;

void loop() {
  static bool once = false;
   
  if (!once){
    delay(1000);
    once = true;
  }

 writeDac(curPos, 0);
  curPos += 1;
  if (curPos >=4095){
    curPos = 0;
  }
  delay(10);
  
}
