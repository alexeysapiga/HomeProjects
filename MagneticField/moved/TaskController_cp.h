
#ifndef TASK_CONTROLLER_H_
#define TASK_CONTROLLER_H_
#include"timer-api.h"
extern Settings _setings;
extern int CurrentPosition;

extern LiquidCrystal lcd;
const unsigned int MAX_DIGIT = 1024;
const unsigned int WORKING_TIMER = TIMER_DEFAULT;
const int PRESCALER[] = { 1, TIMER_PRESCALER_1_1, 8, TIMER_PRESCALER_1_8, 64, TIMER_PRESCALER_1_64, 256, TIMER_PRESCALER_1_256, 1024, TIMER_PRESCALER_1_1024 };
#define CPU_FREQ 16000000

void SetVariableOnPort(unsigned int var) {  

	if (var >= MAX_DIGIT - 1)
	{
		var = MAX_DIGIT - 1;
	}
	CurrentPosition = var;
	PORTD = (PORTD & 0xE0) | (var & 0x1F);
	PORTB = (PORTB & 0xF) | ((var & 0x1E0) >> 1);
	PORTE = (PORTE & 0xBF) | ((var & 0x200) >> 3);
}

class Task
{
public:
	Task() :_pause(false), _direction(0), _to(0), _isValid(false){}
	~Task() {
		Stop();
	}
	Task(int from, int to, unsigned int time) :_pause(false), _to(to), _isValid(true){
		if (to == from)
		{
			SetVariableOnPort(to);
		}
		else {
			SetVariableOnPort(from);
			_direction = (_to - from) > 0 ? 1 : -1;
						
			unsigned int freq = fabs(to - from) / time;
			if (freq > 0) {				
				int number = -1;
				int prescaler = -1;
				char cnt = sizeof(PRESCALER) / (sizeof(int) * 2);
				for (int i = 0; i < cnt; i++) {
					unsigned int curPrescaler = PRESCALER[2 * i];
					unsigned int curNumber = (CPU_FREQ / curPrescaler) / freq;
					if (curNumber <= 65535) {
						number = curNumber;
						prescaler = PRESCALER[2 * i + 1];
						break;
					}
				}
				lcd.setCursor(0,0);
				lcd.print(String(number) + " ");
				if (number > 0) {
					timer_init_ISR(TIMER_DEFAULT, prescaler, number - 1);
				}
				else {
					SetVariableOnPort(to);
					_isValid = false;
					//std::cout << "ERROR" << std::endl;
					//print error
				}
			}
			else
			{				
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
	}
	
	bool Update() {
		lcd.setCursor(6,0);
				lcd.print("W");
		if (!_isValid) {
			return true;
		}
		
		SetVariableOnPort(CurrentPosition);
		if (_pause) {
			return false;
		}
		CurrentPosition += _direction;		
		if ((CurrentPosition - _to) * _direction > 0) {
			CurrentPosition = _to;
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
		_countRepeat = countRepeat;
		_forward = true;
		_currentTask = Task(_setings._startPos, _setings._endPos, _setings._time);
		_isFinished = !_currentTask.IsValid();
	}
	
	void Reset() {
		_countRepeat = 0;
		_forward = false;
		_currentTask.Stop();
		_currentTask = Task(CurrentPosition, _setings._startPos, _setings._timeBack);
		_isFinished = !_currentTask.IsValid();
	}
		
	void Update(int timer) {
		lcd.setCursor(7,0);
				lcd.print("WW" + String(timer)+"RR");
		if (timer != WORKING_TIMER) {
			return;
		}
		if (_currentTask.Update()){
			_currentTask.Stop();
			
			if (_forward) {
				_forward = false;
				_currentTask = Task(_setings._endPos, _setings._startPos, _setings._timeBack);
			} else {
				_countRepeat -= 1;
				if (_countRepeat > 0){
					_forward = true;
					_currentTask = Task(_setings._startPos, _setings._endPos, _setings._time);
				} else {
					_currentTask = Task();
					_isFinished = true;
				}					
			}
		}
	}
	
	bool IsFinished() {
		return _isFinished;
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
