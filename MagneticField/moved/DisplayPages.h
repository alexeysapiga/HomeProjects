
#ifndef DISPLAY_PAGES_H_
#define DISPLAY_PAGES_H_

#include "TaskController.h"

extern LiquidCrystal lcd;
extern Settings _setings;
extern int CurrentPosition;
#define MAX_CNT_PAGES 4
#define CALL_METHOD(mt, pg) (this->*(mt[pg]))();
const unsigned char SINGLE_PAGE = 0;
const unsigned char MANUAL_PAGE = 1;
const unsigned char AUTO_PAGE = 2;
const unsigned char SETTINGS_PAGE = 3;


const unsigned char BUTTON_START = 0;
const unsigned char BUTTON_RESET = 1;
const unsigned char BUTTON_MODE = 2;

const char* CLEAR_LINE_STRING = "                ";
const char* CLEAR_5_STRING = "     ";
const char* CLEAR_4_STRING = "    ";
const char* CLEAR_3_STRING = "   ";
const char* CLEAR_2_STRING = "  ";
unsigned int TIME_ARRAY [] = {30, 150, 300, 600, 900, 1200 };
unsigned int POS_ARRAY [] = {0, 128, 512, 1024, 2048, 3000, 4095 };
unsigned int TIME_BACK_ARRAY [] = {10, 30, 60, 90, 120 };
unsigned int STEP_ARRAY [] = {1, 2, 3, 5, 10, 50};

template<class T>
void PrintNumber(LiquidCrystal& lcd, const T& value, char numDigits){
	static char buf[5];	
	memset(&buf, 0, sizeof(char) * 5);
	char len = sprintf(buf, "%d", value);
	if (numDigits - len > 0) {
		memset(buf + len, ' ', sizeof(char)*(numDigits - len));
	}
	lcd.print(buf);	
}


template <class T>
class VariableVraper
{
public:
	VariableVraper() :_variable(nullptr), _tempVar(T()), _min(T()), _max(T()), _step(T()), _array(nullptr), _size(0), _index(0){
	}
	VariableVraper(T* var, T* array, unsigned int size) :_variable(var), _tempVar(*var), _min(T()), _max(T()), _step(T()), _array(array), _size(size), _index(0){
	}
	VariableVraper(T* var, T min, T max, T step) :_variable(var), _tempVar(*var), _min(min), _max(max), _step(step), _array(nullptr), _size(0), _index(0){
	}
	void Next() {
		if (_array) {
			_index += 1;
			if (_index >= _size) {
				_index = 0;
			}
			_tempVar = _array[_index];
		}
		else {
			_tempVar += _step;
			if (_tempVar >= _max) {
				_tempVar = _max;
			}
		}
	}
	void Prev() {
		if (_array) {
			if (_index >= 1) {
				_index -= 1;
			}
			else {
				_index = _size - 1;
			}
			
			_tempVar = _array[_index];
		}
		else {
			if (_tempVar >= _min + _step) {
				_tempVar -= _step;
			}
			else {
				_tempVar = _min;
			}
		}
	}
	T Get() {
		return _tempVar;
	}
	void Apply() {
		*_variable = _tempVar;
	}
	bool Changed() {
		return (*_variable) != _tempVar;
	}
		
	void Reset() {
		_tempVar = *_variable;
	}
private:
	T* _variable;
	T _tempVar;
	T _min;
	T _max;
	T _step;
	unsigned int _index;
	unsigned int _size;
	T* _array;	
};

class SettingsVarDisplay
{
	public:
	typedef void (SettingsVarDisplay::*VoidFunction)(void);
	enum SettingsVar
	{
		Time,
		StartPos,
		EndPos,		
		TimeBack,
		CountPeriods,
		ManualStep,
		MaxVars,
		MaxPages
	};
	SettingsVarDisplay():_currentSettingsPage(0) {
		_variables[SettingsVar::StartPos] = VariableVraper <unsigned int>(&_setings._startPos, POS_ARRAY, sizeof(POS_ARRAY)/sizeof(unsigned int)); 
		_variables[SettingsVar::EndPos] = VariableVraper <unsigned int>(&_setings._endPos, POS_ARRAY, sizeof(POS_ARRAY)/sizeof(unsigned int));
		_variables[SettingsVar::Time] = VariableVraper <unsigned int>(&_setings._time, TIME_ARRAY, sizeof(TIME_ARRAY)/sizeof(unsigned int));
		_variables[SettingsVar::TimeBack] = VariableVraper <unsigned int>(&_setings._timeBack, TIME_BACK_ARRAY, sizeof(TIME_BACK_ARRAY)/sizeof(unsigned int));
		_variables[SettingsVar::CountPeriods] = VariableVraper <unsigned int>(&_setings._countPeriods, 1, 10, 1);
		_variables[SettingsVar::ManualStep] = VariableVraper <unsigned int>(&_setings._manualStep, STEP_ARRAY, sizeof(STEP_ARRAY)/sizeof(unsigned int));
		
		
		_pagesDraw[SettingsVar::Time] = &SettingsVarDisplay::DrawSettingsTime;		
		_pagesDraw[SettingsVar::StartPos] = &SettingsVarDisplay::DrawSettingsStartPos;
		_pagesDraw[SettingsVar::EndPos] = &SettingsVarDisplay::DrawSettingsEndPos;		
		_pagesDraw[SettingsVar::TimeBack] = &SettingsVarDisplay::DrawSettingsTimeBack;	
		_pagesDraw[SettingsVar::CountPeriods] = &SettingsVarDisplay::DrawSettingsCntSteps;	
		_pagesDraw[SettingsVar::ManualStep] = &SettingsVarDisplay::DrawSettingsManualStep;	
		_pagesDraw[SettingsVar::MaxVars] = &SettingsVarDisplay::DrawSettingsMaxPos;	
		
	}
	
	void Next() {
		if (_currentSettingsPage < SettingsVar::MaxVars){
			_variables[_currentSettingsPage].Next();
			lcd.setCursor(0,1);
			lcd.print(CLEAR_LINE_STRING);
		}
	}
	
	void Prev() {
		if (_currentSettingsPage < SettingsVar::MaxVars){			
			_variables[_currentSettingsPage].Prev();
			lcd.setCursor(0,1);
			lcd.print(CLEAR_LINE_STRING);
		}
	}
	
	void NextPage() {
		_currentSettingsPage +=1;
		if (_currentSettingsPage >= SettingsVar::MaxPages){
			_currentSettingsPage = 0;
		}
		DrawOnceSettings();
	}
	
	void Apply() {
		if (_currentSettingsPage < SettingsVar::MaxVars){
			_variables[_currentSettingsPage].Apply();
		}
		if (_currentSettingsPage == SettingsVar::StartPos) {
			SetVariableOnPort(_setings._startPos);
		}
		EEPROM.put<Settings>(1, _setings);
	}
	
	void DrawOnceSettings();
	
	void Draw() {
		
		CALL_METHOD(_pagesDraw, _currentSettingsPage)
		if (_currentSettingsPage < SettingsVar::MaxVars && _variables[_currentSettingsPage].Changed()) {
			lcd.setCursor(15, 0);
			lcd.print("*");
		} else {
			lcd.setCursor(15, 0);
			lcd.print(" ");
		}
	}
	
	void Init() {
		byte isEmpty = EEPROM.read(0)==255;
		if (!isEmpty) {
			EEPROM.get<Settings>(1, _setings);
		} else {
			EEPROM.write(0, 0);
			EEPROM.put<Settings>(1, _setings);
		}
		for (unsigned char i = SettingsVar::Time; i < SettingsVar::MaxVars; i++ ){
			_variables[i].Reset();
		}	
	}
	
	void ResetPage()  {
		_currentSettingsPage = 0;
	}
	
private:
	unsigned char _currentSettingsPage;	
	
	
	void DrawSettingsTime();
	void DrawSettingsStartPos();
	void DrawSettingsEndPos();
	void DrawSettingsMaxPos();
	void DrawSettingsCntSteps();
	void DrawSettingsTimeBack();
	void DrawSettingsManualStep();
	
	VariableVraper <unsigned int>  _variables[SettingsVar::MaxVars];
	VoidFunction _pagesDraw[SettingsVar::MaxPages];	
} ;

class PagesManager {
	public:
	PagesManager() 
	:_currentPage(0)
	{
		_pagesOnceDraw[SINGLE_PAGE] = &PagesManager::DrawOnceSingle;
		_pagesOnceDraw[MANUAL_PAGE] = &PagesManager::DrawOnceManual;
		_pagesOnceDraw[AUTO_PAGE] = &PagesManager::DrawOnceAutomatic;
		_pagesOnceDraw[SETTINGS_PAGE] = &PagesManager::DrawOnceSettings;
		
		_pagesDraw[SINGLE_PAGE] = &PagesManager::DrawSingle;
		_pagesDraw[MANUAL_PAGE] = &PagesManager::DrawManual;
		_pagesDraw[AUTO_PAGE] = &PagesManager::DrawAutomatic;
		_pagesDraw[SETTINGS_PAGE] = &PagesManager::DrawSettings;
		
		
	}
	void Init() {		
		_settingVarDisplay.Init();
		SetVariableOnPort(_setings._startPos);
		CALL_METHOD(_pagesOnceDraw, _currentPage)
	}
	void Draw() {
		CALL_METHOD(_pagesDraw, _currentPage)		
	}
	
	void OnEncoder(char dir);
	void OnKeyPressed(unsigned char button);
private:
	typedef void (PagesManager::*VoidFunction)(void);
	
	void DrawOnceSingle();
	void DrawSingle();
	void DrawOnceAutomatic();
	void DrawAutomatic();
	void DrawOnceManual();
	void DrawManual();
	//Settings
	
	void DrawSettings();
	void DrawOnceSettings();
	//
	void OnStartButtonPressed();
	void OnResetButtonPressed();
	void OnModeButtonPressed();
	
	unsigned char _currentPage;
	SettingsVarDisplay _settingVarDisplay;
	
	
	VoidFunction _pagesOnceDraw[MAX_CNT_PAGES];
	VoidFunction _pagesDraw[MAX_CNT_PAGES];
};

void PagesManager::OnEncoder(char dir) {
	if (_currentPage == SETTINGS_PAGE) {
		if (dir > 0 ) {
			_settingVarDisplay.Next();
		} else {
			_settingVarDisplay.Prev();
		}		
	} else if (_currentPage == MANUAL_PAGE) {
		int newPos = CurrentPosition - _setings._manualStep * dir;
		if (newPos <= 0) {
			newPos = 0;
		}
		if (newPos >= MAX_DIGIT - 1) {
			newPos = MAX_DIGIT - 1;
		}
		SetVariableOnPort(newPos);
	}
}

void PagesManager::OnKeyPressed(unsigned char button) {
		/*if (_currentPage == SETTINGS_PAGE) {
			
		} else*/
		{
			switch (button) {
				case BUTTON_START: OnStartButtonPressed(); break;
				case BUTTON_RESET: OnResetButtonPressed(); break;
				case BUTTON_MODE: OnModeButtonPressed(); break; 
				default:break;
			};
		}
}

void PagesManager::OnStartButtonPressed() {
	switch (_currentPage) {
		case SINGLE_PAGE: processProgram.Start(1); break;		
		case AUTO_PAGE: processProgram.Start(_setings._countPeriods);  break;
		case SETTINGS_PAGE: _settingVarDisplay.Apply();break;
		default:break;
	}
}

void PagesManager::OnResetButtonPressed() {	
	if (_currentPage==SETTINGS_PAGE) {
		_settingVarDisplay.NextPage();
	}
	else {
		processProgram.Reset();
	}
}	

void PagesManager::OnModeButtonPressed() {
	if (!processProgram.IsFinished()) {
		return;
	}
	_currentPage += 1;
	if (_currentPage >= MAX_CNT_PAGES) {
		_currentPage = 0;
	}
	lcd.clear();
	_settingVarDisplay.ResetPage();
	CALL_METHOD(_pagesOnceDraw, _currentPage)	
}					

void PagesManager::DrawOnceSingle() {
	lcd.setCursor(0, 0);
	lcd.print("S" + String(_setings._startPos));
	lcd.setCursor(6, 0);
	lcd.print(String("E") + String(_setings._endPos));
	lcd.setCursor(6, 1);
	lcd.print(String("T") + String(_setings._time));
	lcd.setCursor(11, 1);
	lcd.print(String("TB") + String(_setings._timeBack));
	lcd.setCursor(0, 1);
	lcd.print(String("C"));
}

void PagesManager::DrawSingle() {
	lcd.setCursor(1, 1);		
	PrintNumber(lcd, CurrentPosition, 4);
}

void PagesManager::DrawOnceAutomatic() {
	lcd.setCursor(0, 0);
	lcd.print("S" + String(_setings._startPos));
	lcd.setCursor(6, 0);
	lcd.print(String("E") + String(_setings._endPos));
	lcd.setCursor(12, 0);
	lcd.print("step");
	lcd.setCursor(6, 1);
	lcd.print(String("T") + String(_setings._time));
	lcd.setCursor(0, 1);
	lcd.print("C");
}

void PagesManager::DrawAutomatic() {
	lcd.setCursor(1, 1);
	PrintNumber(lcd, CurrentPosition, 4);
	
	lcd.setCursor(11, 1);
	char buf[6];
	memset(&buf, 0, sizeof(char)*6);
	char curPeriod = _setings._countPeriods - processProgram.GetCurRepeatStep();
	snprintf(buf, 6,"%d/%d  ",curPeriod, _setings._countPeriods);
	lcd.print(buf);
	//String(_currentPeriod) + String("/") + String(_setings._countPeriods));		
}

void PagesManager::DrawOnceManual() {
	lcd.setCursor(5, 0);
	lcd.print("MANUAL");
	lcd.setCursor(0, 1);
	lcd.print("Cur>");
	lcd.setCursor(9, 1);
	lcd.print("Step>");
}

void PagesManager::DrawManual() {
	lcd.setCursor(4, 1);
	PrintNumber(lcd, CurrentPosition, 4);
	lcd.setCursor(14, 1);
	PrintNumber(lcd, _setings._manualStep, 2);	
}

void PagesManager::DrawSettings(){
	_settingVarDisplay.Draw();
}

void PagesManager::DrawOnceSettings() {
	_settingVarDisplay.DrawOnceSettings();
}

void SettingsVarDisplay::DrawOnceSettings() {

	lcd.clear();
	lcd.setCursor(4, 0);
	lcd.print("SETTINGS");	
}

void SettingsVarDisplay::DrawSettingsTime(){
	lcd.setCursor(0, 1);	
	lcd.print("TIME(T)>" + String(_variables[SettingsVarDisplay::SettingsVar::Time].Get()));	
}

void SettingsVarDisplay::DrawSettingsStartPos(){
	lcd.setCursor(0, 1);

	lcd.print("StartPos(S)>" + String( _variables[SettingsVarDisplay::SettingsVar::StartPos].Get()));	
}

void SettingsVarDisplay::DrawSettingsEndPos(){
	lcd.setCursor(0, 1);
	
	lcd.print("EndPos(E)>" + String(_variables[SettingsVarDisplay::SettingsVar::EndPos].Get()));	
}

void SettingsVarDisplay::DrawSettingsMaxPos(){
	lcd.setCursor(0, 1);
	lcd.print("MaxPos(S)>" + String(_setings._maxPos));
}

void SettingsVarDisplay::DrawSettingsCntSteps(){
	lcd.setCursor(0, 1);
	
	lcd.print("Count Steps>" + String(_variables[SettingsVarDisplay::SettingsVar::CountPeriods].Get()));	
}

void SettingsVarDisplay::DrawSettingsTimeBack(){
	lcd.setCursor(0, 1);
	
	lcd.print("TimeBack(TB)>" + String(_variables[SettingsVarDisplay::SettingsVar::TimeBack].Get()));
}

void SettingsVarDisplay::DrawSettingsManualStep() {
	lcd.setCursor(0, 1);
	
	lcd.print("ManualStep>" + String(_variables[SettingsVarDisplay::SettingsVar::ManualStep].Get()));
}

#endif //DISPLAY_PAGES_H_
