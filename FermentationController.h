#ifndef FERMENTATIONCONTROL_H_
#define FERMENTATIONCONTROL_H_

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DallasTemperature.h>

#include <EEPROM.h>

enum FermentationControllerErrors {
	NoError=0,
	AmbientSensorNotFound,
	FermentSensorNotFound
};

class FermentationConfig {
  public: 
    FermentationConfig() {
      FanEnabled = false;
      FanTempOn = 68;
      FanTempOff = 65;
      PumpEnabled = false;
      PumpOnTemp = 68;
      PumpOnInterval = 1000*60;    
      PumpOffInterval = 1000*60 * 5;    
      IntranetOnly = false;
      CheckInterval = 1000*60*5;
    }
    bool FanEnabled;
    int FanTempOn;
    int FanTempOff;
    
    bool PumpEnabled;
    int PumpOnInterval;  
    int PumpOffInterval;  
    int PumpOnTemp;

    int FanPin;
    int PumpPin;
    int FermentationSensorPin;

    int CheckInterval;
    bool IntranetOnly;
};

class FermentationControls {
public: 
		
  bool AmbientTempSensorPresent;
  bool FermentTempSensorPresent;
  
    FermentationControls():FermentationControls(0,0){}
    FermentationControls(int _FanPin, int _PumpPin) {
		PumpOnTime = 0;
		FanPin = _FanPin;
		PumpPin = _PumpPin;
		PumpOn = false;
		FanOn= false;
		AmbientTemp = 0;
		FermentationTemp = 80;
		
		FermentationSensorResolution = 12;
		AmbientTempSensorPresent = false;
		FermentTempSensorPresent = false;
    }
	
	bool IsFanOn(){
	  return FanOn;
	}
	void SetFanPin(int pin){
		FanPin = pin;
		if(pin > 0) {
			pinMode(pin, OUTPUT); 
		}
	}
	int GetFanPin() {
		return FanPin;
	}
	bool SetFanOn(bool on = true){
	  if(FanPin > 0) {
		digitalWrite(FanPin,on == true ? HIGH: LOW);
		FanOn = on;
	  }
	}
	bool IsPumpOn(){
	  return PumpOn;
	}
	void SetPumpPin(int pin){
		PumpPin = pin;
		if(pin > 0) {
			pinMode(pin, OUTPUT); 
		}
	}
	int GetPumpPin() {
		return PumpPin;
	}
	bool SetPumpOn(bool on){
	  if(PumpPin > 0) {		  
		digitalWrite(PumpPin,on == true ? HIGH: LOW);
		if(!PumpOn && on == true) { //record when the pump turned on
			PumpOnTime = millis();
		}
		if(PumpOn && on == false) { //record when the pump turned off
			PumpOffTime = millis();
		}
	    PumpOn = on;
	  }
	}
	long GetPumpOnTime() {
		return PumpOnTime;
	}
	long GetPumpOffTime() {
		return PumpOffTime;
	}
	float GetAmbientTemp() {
		return AmbientTemp;
	}
	float GetFermentTemp() {
		return FermentationTemp;
	}
	
	int GetFermenterSensorPin() {
		return FermenterSensorPin;
	}
	void SetFermenterSensorPin(int pin) {
		if(pin != FermenterSensorPin) {
			FermenterSensorPin = pin;
			InitFermentationSensor();
		}
	}
	
	int RefreshTemps() {
		//read temperatures
		if(AmbientTempSensorPresent) {
			float tempC = bmeAmbient.readTemperature();
			AmbientTemp = (tempC * 1.8F) + 32.0F;
		}
		if(FermentTempSensorPresent) {
			sensors->requestTemperatures();
			FermentationTemp = DallasTemperature::toFahrenheit(sensors->getTempC(FermenterSensorAddress));
		}
		if(!AmbientTempSensorPresent) {
		  return FermentationControllerErrors::AmbientSensorNotFound;
		}
		if(!FermentTempSensorPresent) {
		  return FermentationControllerErrors::FermentSensorNotFound;
		}
		return 0;
	}
	
	void Begin() {
		InitAmbientSensor();
		InitFermentationSensor();
	}

	
private:
	int FanPin;
	int PumpPin;
	
	bool PumpOn;
	bool FanOn;
	
	long PumpOnTime;
	long PumpOffTime;
	
	float AmbientTemp;
	float FermentationTemp;
	
	Adafruit_BMP280 bmeAmbient; // I2C
	
	int FermenterSensorPin;
	
	OneWire *oneWire;
	DallasTemperature *sensors = NULL;
	DeviceAddress FermenterSensorAddress;
	int FermentationSensorResolution;
	
	void InitAmbientSensor() {
		AmbientTempSensorPresent = bmeAmbient.begin();
	}
	
	
	void InitFermentationSensor() {
		if(sensors !=NULL) {
			delete sensors;
			sensors = NULL;
		}
		if(oneWire !=NULL) {
			delete oneWire;
			oneWire = NULL;
		}
		oneWire = new OneWire(GetFermenterSensorPin());
		sensors = new DallasTemperature(oneWire);
		sensors->begin();
		int deviceCount = sensors->getDeviceCount();
		if (!deviceCount || !sensors->getAddress(FermenterSensorAddress, 0)) {
		  FermentTempSensorPresent = false; 
		  return;
		}

		FermentTempSensorPresent = true; 
		sensors->setResolution(FermenterSensorAddress, FermentationSensorResolution);
		sensors->begin();
	}
	
};

class FermentationController {
	public:
		FermentationController() : FermentationController(0,0,0) {}
		
		FermentationController(int fanPin, int pumpPin, int fermentTempPin){
			LastRefreshTime = 0;
			IgnoreErrors = false;
			Controls.SetFanPin(fanPin);
			Controls.SetPumpPin(pumpPin);
			Controls.SetFermenterSensorPin(fermentTempPin);
		}
		
		~FermentationController(){}
		
		FermentationConfig Config;
		FermentationControls Controls;
		bool IgnoreErrors;
	
		void Begin() {
			if(IsSavedConfigValid()) {
				Serial.println("Saved config is valid. loading from EEPROM");
				LoadConfig();
			} else {
				Serial.println("Saved config is not valid");
			}
			Controls.Begin();
		}
		
		int Refresh() {
			if(millis() > (Config.CheckInterval + LastRefreshTime)) {
				return RefreshInternal();
			}
			return FermentationControllerErrors::NoError;
		}
		
		void SaveConfig() {
			Serial.println("SaveConfig");
			SaveConfigValidationMark();
			EEPROM.begin(eepromSize);
			EEPROM.put(eepromConfigOffset +1 , Config);
			EEPROM.commit();
			EEPROM.end();
			Serial.println("SaveConfig Complete");
		}
		
		void PrintConfig() {
			Serial.println("FanEnabled " + String(Config.FanEnabled));
			Serial.println("FanPin " + String(Controls.GetFanPin()));
			Serial.println("FanTempOn " + String(Config.FanTempOn));
			Serial.println("FanTempOff " + String(Config.FanTempOff));
			Serial.println("PumpEnabled " + String(Config.PumpEnabled));
			Serial.println("PumpPin " + String(Controls.GetPumpPin()));
			Serial.println("PumpOnInterval " + String(Config.PumpOnInterval));
			Serial.println("PumpOffInterval " + String(Config.PumpOffInterval));
			Serial.println("PumpOnTemp " + String(Config.PumpOnTemp));
			Serial.println("IgnoreErrors " + String(IgnoreErrors));
			Serial.println("FermentSensorPin " + String(Controls.GetFermenterSensorPin()));
			Serial.println("IntranetOnly " + String(Config.IntranetOnly));
			Serial.println("CheckInterval " + String(Config.CheckInterval));
		}
		
	private:
		long LastRefreshTime;
		
		int RefreshInternal() {
			Serial.println("\n*** RefreshInternal ***");
			float ambientTemp,fermentationTemp;
			LastRefreshTime = millis();
			//read temperatures
			Serial.println("Controls.RefreshTemps()");
			
			int error = Controls.RefreshTemps();
      Serial.println("error  = " + String(error));
			if(error > 0 && IgnoreErrors != true) {
				return error;
			}
			
			if(Controls.AmbientTempSensorPresent) {
        Serial.println("Controls.GetAmbientTemp()");
				ambientTemp =Controls.GetAmbientTemp();
				Serial.println("ambientTemp = " + String(ambientTemp));	
			}
			
			if(Controls.FermentTempSensorPresent) {
        Serial.println("Controls.GetFermentTemp()");
			  fermentationTemp =Controls.GetFermentTemp();
				Serial.println("fermentationTemp = " + String(fermentationTemp));
				
				//Check if we need to change fan's state
				Serial.println("Checking Fan");
				if(fermentationTemp >= Config.FanTempOn) {
					Serial.println("Fermentation Fan Temp Exceeded");
					if(Config.FanEnabled && !Controls.IsFanOn()) {
						Serial.println("Controls.SetFanOn(true)");
						Controls.SetFanOn(true);
					}
				} else if(fermentationTemp < Config.FanTempOff && /*Config.FanEnabled &&*/ Controls.IsFanOn()) {
					Serial.println("Controls.SetFanOn(false)");
					Controls.SetFanOn(false);
				}
				
				//if pump is on just make sure its only on for the specified amt of time
				Serial.println("Checking Pump");
				if(Controls.IsPumpOn()) {
					Serial.println("Pump Is On");
					if(LastRefreshTime >  (Config.PumpOnInterval + Controls.GetPumpOnTime())){
						Serial.println("Controls.SetPumpOn(false)");
						Controls.SetPumpOn(false);
					}
				} else if(Config.PumpEnabled && fermentationTemp >= Config.PumpOnTemp) {//Check if we need to change pump's state
					Serial.println("Fermentation Pump Temp Exceeded");
					bool pumpCanTurnOn = (Config.PumpOffInterval + LastRefreshTime) > Controls.GetPumpOffTime();
					if(pumpCanTurnOn) {
						Serial.println("Controls.SetPumpOn(true)");
						Controls.SetPumpOn(true);
					}
				}
			} else {
				Controls.SetFanOn(false);
				Controls.SetPumpOn(false);
			}
			
			Serial.println("*** RefreshInternal Complete ***");
			
			if(!Controls.AmbientTempSensorPresent) {
			  return FermentationControllerErrors::AmbientSensorNotFound;
			}
			if(!Controls.FermentTempSensorPresent) {
			  return FermentationControllerErrors::FermentSensorNotFound;
			}
			return 0;
		}
		
		
		const char eepromConfigValidationMark = 'f';
		int eepromConfigOffset = 32;
		const int eepromSize = 256;
		void SaveConfigValidationMark() {
			Serial.println("SaveConfigValidationMark");
			EEPROM.begin(eepromSize);
			EEPROM.put(eepromConfigOffset, eepromConfigValidationMark);
			EEPROM.commit();
			EEPROM.end();
			Serial.println("SaveConfigValidationMark Complete");
		}
		
		bool IsSavedConfigValid() {
			EEPROM.begin(eepromSize);
			char c;
			EEPROM.get(eepromConfigOffset,c);
			EEPROM.end();
			if(c == eepromConfigValidationMark){
				return true;
			}
			return false;
		}
		
		void LoadConfig() {
			Serial.println("LoadConfig");
			EEPROM.begin(eepromSize);
			EEPROM.get(eepromConfigOffset + 1,Config);
			EEPROM.end();
			Serial.println("LoadConfig Complete");
		}
};

#endif

