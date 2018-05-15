
#ifndef TASK_CONTROLLER_H_
#define TASK_CONTROLLER_H_
#include"timer-api.h"
extern Settings _setings;
extern int CurrentPosition;
extern LiquidCrystal lcd;

const unsigned int MAX_DIGIT = 4096;
const unsigned int WORKING_TIMER = TIMER_DEFAULT;
const int PRESCALER[] = { 1, TIMER_PRESCALER_1_1, 8, TIMER_PRESCALER_1_8, 64, TIMER_PRESCALER_1_64, 256, TIMER_PRESCALER_1_256, 1024, TIMER_PRESCALER_1_1024 };

#define SYNC 5
#define LDAC 8
#define SCK 6
#define DIN 7
#define NOP asm volatile ("nop\n\t")

#define START_LED 13
#define RESET_LED 14

void WriteToDAC(unsigned int var) {
	//AD5322
	
	unsigned int cur = 0x8000;
	var &= 0xFFF;
	//digitalWrite(SYNC, HIGH) ;
	digitalWrite(LDAC, HIGH) ;
	//digitalWrite(SCK, HIGH) ;  
	//NOP;
	//NOP;  
	digitalWrite(SYNC, LOW) ;
	//DAC A
	for (int i=0; i<16; i++){
	  digitalWrite(DIN,  ((var & cur)>0)?HIGH:LOW) ;    
	  digitalWrite(SCK, HIGH);        
	  digitalWrite(SCK, LOW);
	  digitalWrite(DIN,LOW);
	  cur = cur >> 1;	  
	}
	//NOP;NOP;NOP;
	digitalWrite(SYNC, HIGH) ;
	digitalWrite(LDAC, LOW) ;
	digitalWrite(LDAC, HIGH) ;
	/*NOP;
	
	//DAC B
	digitalWrite(SYNC, LOW) ;
	digitalWrite(SCK, HIGH);        
	digitalWrite(DIN, HIGH);
	digitalWrite(SCK, LOW);
	cur = 0x4000;
	for (int i=0; i<15; i++){
	  digitalWrite(DIN,  ((var & cur)>0)?HIGH:LOW) ;    
	  digitalWrite(SCK, HIGH);        
	  digitalWrite(SCK, LOW);
	  digitalWrite(DIN,LOW);
	  cur = cur >> 1;	
	}
	NOP;NOP;NOP;
	digitalWrite(SYNC, HIGH) ;
	NOP;NOP;
	digitalWrite(LDAC, LOW) ;
	NOP;
	NOP;
	digitalWrite(LDAC, HIGH) ;*/
  
	/*PORTD = (PORTD & 0xE0) | (var & 0x1F);
	PORTB = (PORTB & 0xF) | ((var & 0x1E0) >> 1);
	PORTE = (PORTE & 0xBF) | ((var & 0x200) >> 3);*/
}

void SetVariableOnPort(unsigned int var) {  
	if (var >= MAX_DIGIT - 1)
	{
		var = MAX_DIGIT - 1;
	}
	CurrentPosition = var;
  WriteToDAC(var+100);
  NOP;
  WriteToDAC(var+100);
}

class Task
{
public:
	Task() :_pause(false), _direction(0), _to(0), _isValid(false), _curPos(0){}
	~Task() {
		Stop();
	}
	
	void Init(int from, int to, unsigned int time) {
		Stop();
		_curPos = from;
		_pause = false;
		_to = to;
		_isValid = true;
		if (to == from)
		{
			_curPos = to;
			SetVariableOnPort(to);
		}
		else {
			SetVariableOnPort(from);
			_direction = (_to - from) > 0 ? 1 : -1;
						
			float freq = fabs(static_cast<float>(to - from)) / static_cast<float>(time);
			if (freq > 0) {				
				long number = -1;
				int prescaler = -1;
				char cnt = sizeof(PRESCALER) / (sizeof(int) * 2);
				for (int i = 0; i < cnt; i++) {
					unsigned int curPrescaler = PRESCALER[2 * i];
					long curNumber = round(static_cast<float>(16000000 / curPrescaler) / freq);
					if (curNumber <= 65535) {
						number = curNumber;
						prescaler = PRESCALER[2 * i + 1];
						break;
					}
				}
				
				if (number > 0) {
					timer_init_ISR(WORKING_TIMER, prescaler, number - 1);
				}
				else {
					_curPos = to;
					SetVariableOnPort(to);
					_isValid = false;
					//std::cout << "ERROR" << std::endl;
					//print error
				}
			}
			else
			{				
				_curPos = to;
				SetVariableOnPort(to);
				_isValid = false;
				//std::cout << "ERROR" << std::endl;
				//print Error
			}
		}
	}
	void PuaseOrContinue() { _pause = !_pause; }
	
	void Stop() {		
		if (_isValid) {
			timer_stop_ISR(WORKING_TIMER);
		}
		_isValid = false;
	}
	
	bool Update() {
			
		if (!_isValid) {
			return true;
		}
		
		SetVariableOnPort(_curPos);
		if (_pause) {
			return false;
		}
		_curPos += _direction;		
		if ((_curPos - _to) * _direction > 0) {
			_curPos = _to;
			return true;
		}
		return false;
	}
	bool IsValid() {
		return _isValid;
	}
private:
	
	char _direction;
	bool _pause;	
	int _to;
	int _curPos;
	bool _isValid;
};

class ProcessProgram 
{
public:
	ProcessProgram():_forward(false),_countRepeat(0), _isFinished(true) {
	}
	
	void Start(char countRepeat) {
		if (_currentTask.IsValid()) {
			_currentTask.PuaseOrContinue();
			return;
		}
		digitalWrite(START_LED, HIGH);
		digitalWrite(RESET_LED, LOW);
		_countRepeat = countRepeat;
		_forward = true;		
		_currentTask.Init(_setings._startPos, _setings._endPos, _setings._time);		
		_isFinished = !_currentTask.IsValid();
	}
	
	void Reset() {
		if (_isFinished) {
			return;
		}
		digitalWrite(START_LED, LOW);
		digitalWrite(RESET_LED, HIGH);
		_countRepeat = 0;
		_forward = false;
		_currentTask.Stop();
		_currentTask.Init(CurrentPosition, _setings._startPos, _setings._timeBack);
		_isFinished = !_currentTask.IsValid();
	}
		
	void Update(int timer) {
		
		if (timer != WORKING_TIMER) {
			return;
		}
		if (_currentTask.Update()){
			_currentTask.Stop();

			if (_forward) {
				_forward = false;
				_currentTask.Init(_setings._endPos, _setings._startPos, _setings._timeBack);
			} else {
				_countRepeat -= 1;
				if (_countRepeat > 0){
					_forward = true;
					_currentTask.Init(_setings._startPos, _setings._endPos, _setings._time);
				} else {
					_countRepeat = 0;
					_isFinished = true;
					digitalWrite(START_LED,LOW);
					digitalWrite(RESET_LED,LOW);				
				}					
			}
		}
	}
	
	bool IsFinished() {
		return _isFinished;
	}
	
	char GetCurRepeatStep() {
		return _countRepeat;
	}
private:
	Task _currentTask;	
	char _countRepeat;
	bool _forward;
	bool _isFinished;
} processProgram;



void timer_handle_interrupts(int timer) {
	processProgram.Update(timer);
}



#endif //TASK_CONTROLLER_H_
