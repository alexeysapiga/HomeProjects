/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/
#include <LiquidCrystal.h>

#include <Bounce2.h>
#include <EEPROM.h>



int CurrentPosition = 0;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;
unsigned long currentTime = 0;
unsigned long loopTime = 0;

struct Settings {
  unsigned int _startPos;
  unsigned int _endPos;
  unsigned int _maxPos;
  unsigned int _time; //in secondsCurrentPosition
  unsigned int _countPeriods;
  unsigned int _timeBack; //in seconds
  unsigned int _manualStep;
  Settings() {
    _startPos = 0;
    _endPos = _maxPos = 1023;
    _time = 150;
    _countPeriods = 3;
    _timeBack = 30;
    _manualStep = 1;
  }
} _setings;
#include "DisplayPages.h"
PagesManager _pagesManager;

#define NUM_BUTTONS 3
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {A1, A2, A3};
Bounce * buttons = new Bounce[NUM_BUTTONS];

LiquidCrystal lcd(3, 4, 9, 10, 11, 12);
// the setup function runs once when you press reset or power the board

void setup() {  
  
  
  for (int i = 0; i <= 9; i++) {
    pinMode(i, OUTPUT);  
  } 
    lcd.begin(16, 2);
  
  pinMode(A0, INPUT); 
  pinMode(A1, INPUT); 
  pinMode(A2, INPUT); 
  pinMode(A3, INPUT);
  pinMode(A4, INPUT); 
  pinMode(A5, INPUT); 
  currentTime = millis();
  loopTime = currentTime; 

   for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach( BUTTON_PINS[i] , INPUT_PULLUP  );       //setup the bounce instance for the current button
    buttons[i].interval(25);              // interval in ms
  }
  _pagesManager.Init();  
}


void readEncoder() {
  
  if(currentTime >= (loopTime + 3)){ // проверяем каждые 5мс (200 Гц)
    encoder_A = digitalRead(A4);     // считываем состояние выхода А энкодера 
    encoder_B = digitalRead(A5);     // считываем состояние выхода B энкодера    
    if((!encoder_A) && (encoder_A_prev)){    // если состояние изменилось с положительного к нулю
      if(encoder_B) {
        // выход В в полож. сост., значит вращение по часовой стрелке
        // увеличиваем яркость, не более чем до 255
        
        _pagesManager.OnEncoder(1);
      }   
      else {
        // выход В в 0 сост., значит вращение против часовой стрелки     
        // уменьшаем яркость, но не ниже 0
        _pagesManager.OnEncoder(-1);
      }    
    }   
    encoder_A_prev = encoder_A;     // сохраняем значение А для следующего цикла 

  }                       
}

// the loop function runs over and over again forever
void loop() {
  
  currentTime = millis();
  readEncoder();

  for (int i = 0; i < NUM_BUTTONS; i++)  {
    // Update the Bounce instance :
    buttons[i].update();  
    if ( buttons[i].fell() ) {
      _pagesManager.OnKeyPressed(i);
    }
  }
  
  _pagesManager.Draw();
  loopTime = currentTime;
}
