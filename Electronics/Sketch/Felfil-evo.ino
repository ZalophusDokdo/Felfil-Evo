
//#include "FakePid.h"
#include "FelfilReader.h"
#include "FelfilController.h"
#include "FelfilMenu.h"
//
//#pragma once
#include <MenuBackend.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <WString.h>
#include <Max6675.h>
#include <PID_v1.h>

#define I2C_ADDR 0x27
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

#define Ts_Pin_Dt 12
#define Ts_Pin_Ss 10
#define Ts_Pin_Clk 13
#define Ts_Offset 12 // offset for temperature measurement.1 stannds for 0.25 Celsius

#define PwmPin 11
#define TempPin1 3
#define TempPin2 5
#define TempPin3 6

#define PWM_InputCurrentPin A0

#define EngineCurrentSamples 50
#define EngineCurrentSampleTime 1
#define EngineCurrentOffset -0.2

#define TemperatureSamples 5
#define TemperatureSampleTime 300

#define EngineCurrentLcdRefreshInterval 500

LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin, BACKLIGHT_PIN, POSITIVE);
Max6675 ts(Ts_Pin_Dt, Ts_Pin_Ss, Ts_Pin_Clk, Ts_Offset);

FelfilMenu* felfilMenu;
FelfilControler* felfilController;
FelfilReader* felfilReader;

TemperatureControlState temperatureControlState = Low;

// the setup function runs once when you press reset or power the board

void setup() {

	Serial.begin(9600);

	//setup menu
	felfilMenu = new FelfilMenu();
	felfilMenu->SetupPwm(0, 0, 5, 1);
	felfilMenu->SetupTemperature(100, 25, 250, 0.5);
	felfilMenu->SetupLcdDisplay(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin, BACKLIGHT_PIN, POSITIVE);
	felfilMenu->SetupClickEncoder(9, 8, 7);
	felfilMenu->SetupEngineCurrentRefreshInterval(EngineCurrentLcdRefreshInterval);

	//setup controller
	felfilController = new FelfilControler();
	felfilController->SetupPidOuputLimits(0, 6000);
	felfilController->SetupPidAggressiveTuning(6000, 0, 0);
	felfilController->SetupPidConservativeTuning(2000, 190, 1);
	felfilController->SetupPwmOutputPin(PwmPin);
	felfilController->SetupTemperatureOutputPins(TempPin1, TempPin2, TempPin3);
	felfilController->Initialize();

	//setup reader
	felfilReader = new FelfilReader(&ts, PWM_InputCurrentPin);
	felfilReader->SetupEngineCurrentSamples(EngineCurrentSamples, EngineCurrentSampleTime);
	felfilReader->SetupTemperatureSamples(TemperatureSamples, TemperatureSampleTime);
}

// the loop function runs over and over again until power down or reset
void loop() {

	//rilevazione temperatura e corrente sul motore
	double temperature = felfilReader->ReadTemperature();
	float engineCurrent = felfilReader->ReadEngineCurrent(temperatureControlState == High ? EngineCurrentOffset : 0);

	//gestione display
	felfilMenu->SetTempInput(temperature);
	felfilMenu->SetEngineCurrentInput(engineCurrent);
	felfilMenu->Loop();

	//setpoint iniziali non settati
	if (!felfilMenu->IsTempSetpointInitialized())
		return;

	//lettura valori di setpoint
	int pwmSetpoint = felfilMenu->GetPwmSetpoint();
	double tempSetpoint = felfilMenu->GetTempSetpoint();

	//gestione attuatori
	temperatureControlState = felfilController->ControlTemperature(temperature, tempSetpoint);
	felfilController->ControlPwm(engineCurrent, pwmSetpoint);

	//controllo di sicurezza per eventuale reset
	if (felfilController->IsProtectionModeActivated())
	{
		felfilController->Reset();
		felfilMenu->GoToProtectionMode();
	}
}

