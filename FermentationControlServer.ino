#include <ESP8266WiFi.h> 
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WifiExtensions.h>
#include "FermentationController.h"

//OTA Firmware Updates libs
#include <OTAService.h>

ESP8266WebServer *webServer = NULL;
bool Connected = false;
String ConnectedTo = "";
int ApHidden = 0;
String ApSSID = "esp_device";
String ApPSK = "esp_device_pw"; //MUST BE AT LEAST 8 CHARS
String OTA_PASSWORD = "password";
String OTA_SSID = "esp_device";
const int NumNetworks = 1;
String Networks[][2] = {
  {"NETWORK","NETWORK_PASSWORD"}
};


FermentationController Controller(D8,D7,D4);

void setup() {
    Serial.begin(115200);   
    Serial.println("");
	  Controller.Begin();
    
    SetupAP();
	  SetupWebServer();   
    ConnectToNetwork();

  Serial.println("Starting OTA Service");
    OTAService.begin(8266,OTA_SSID,OTA_PASSWORD);
    
  Serial.println("setup complete, entering loop");
}

void loop() {
    OTAService.handle();
    HandleClient();
    yield();
    int error = Controller.Refresh();
}

void SetupAP() {
  WiFi.mode(WIFI_AP_STA);
  Serial.println("");
  Serial.println("Setting up Access Point");
  Serial.println("SSID: " + ApSSID);
  Serial.println("PSK: " + ApPSK);
  Serial.println("Hidden: " + String(ApHidden));
  Serial.println("Setting up Access Point");
  WiFi.softAPConfig(
        IPAddress(192,168,4,1), 
        IPAddress(192,168,4,1),  
        IPAddress(255,255,255,0) 
  );
  WiFi.softAP(ApSSID.c_str(), ApPSK.c_str(),1,ApHidden);
  Serial.println(WifiExtensions::IpToString(WiFi.softAPIP()));
  Serial.println("Setting up Access Point complete");
}

bool IsLocalIp(IPAddress &ip) {
  IPAddress myIp = WiFi.softAPIP();
  if(ip[0] == myIp[0] && ip[1] == myIp[1] && ip[2] == myIp[2]) {
    return true;
  }
  return false;
}

void SetupWebServer() {
  Serial.println("SetupWebServer");
	if(webServer != NULL) {
		delete webServer ;
		webServer = NULL;
	}
	webServer = new ESP8266WebServer(80);
	webServer->on("/", HandleRoot);
  webServer->on("/settings", HandleSettings);
  webServer->on("/update_settings", HandleUpdateSettings);
	webServer->begin();	
  Serial.println("SetupWebServer Complete");
}

void HandleClient() {
	if(webServer != NULL) {
		webServer->handleClient();
	}
}

const char *AccessDeniedPage= "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>UnAuthorized</title></head><body><h2>Authorization Required</h2></body></html>";
const char *HomePage = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Fermentation Controller Settings</title><style>body{padding:0;margin:0;top:0;left:0;position:absolute;height:100%;width:100%;font-family:Roboto,Arial,sans-serif}.header-section{color:#deb319;background-color:#4c4c53;padding:5px 50px;box-shadow:0 1px 10px -2px rgba(0,0,150,.8)}.body-section{padding:20px 50px}.body-content{display:flex}.section{background-color:rgba(0,0,0,.05);box-shadow:0 2px 10px -2px rgba(0,0,0,.8);border-radius:2px;margin:0 25px 20px 0;padding:0 0 10px}.section .section-header{border-radius:2px 2px 0 0;color:#fff;background-color:#4c4c53;padding:5px 10px}.section .section-header h3{padding:0;margin:0}.section .section-body{padding:10px 50px 10px 15px}.section label{display:inline-block;font-weight:700;min-width:220px}</style></head><body><div class=\"header-section\"><h2>Fermentation Controller</h2></div><div class=\"body-section\"><div class=\"body-content\"><div class='section'><div class=\"section-header\"><h3>Status</h3></div><div class=\"section-body\"><div class='sub-section'><label>Ambient Temperature</label><span>70</span></div><div class='sub-section'><label>Fermentation Temperature</label><span>68</span></div><div class='sub-section'><label>Fan</label><span>ON</span></div><div class='sub-section'><label>Water Pump</label><span>ON</span></div></div></div></div><br/><a href='settings'>Settings</a></div></body></html>";

const char *SettingsPageBegin =         "<!DOCTYPE html><html><head> <meta charset=\"UTF-8\"> <title>Fermentation Controller Settings</title> </head><body><style>body{padding:0;margin:0;top:0;left:0;position:absolute;height:100%;width:100%;font-family:Roboto,Arial,sans-serif}.header-section{color:#deb319;background-color:#4c4c53;padding:5px 50px;box-shadow:0 1px 10px -2px rgba(0,0,150,.8)}#submit-button,.form-section{box-shadow:0 2px 10px -2px rgba(0,0,0,.8)}.body-section{padding:20px 50px}.form-body{display:flex;flex-wrap:wrap;margin-bottom:50px}.form-section{background-color:rgba(0,0,0,.05);border-radius:2px;margin-bottom:25px;padding-bottom:10px;margin-right:20px;margin-left:0}.form-section .section-header{margin:0 0 10px;border-radius:2px 2px 0 0;padding:5px 10px;color:#fff;background-color:#4c4c53}.form-section .form-field{padding:3px 15px; padding-right:50px;}.form-section .form-field label{display:inline-block;min-width:200px}a{text-decoration:none}#submit-button{cursor:pointer;font-size:14pt;font-weight:700;border:1px solid #ccc;border-radius:2px;margin:2px 30px;padding:1px 30px}</style><div class=\"header-section\"><h2>Fermentation Controller Settings</h2></div><div class=\"body-section\"><form method=\"get\" action=\"update_settings\"><div class=\"form-body\">";
const char *SettingsPageFanSection =    "<div class=\"form-section\"> <h3 class=\"section-header\">Fan</h3> <div class=\"form-field\"><label>Enabled</label><input class=\"form-control\" type=\"checkbox\" name=\"FanEnabled\" {{FanEnabled}}/> </div><div class=\"form-field\"><label>IO Pin</label><input class=\"form-control\" type=\"number\" name=\"FanPin\" min=\"0\" max=\"20\" value=\"{{FanPin}}\"/></div><div class=\"form-field\"><label>ON Temperature</label><input class=\"form-control\" type=\"number\" name=\"FanTempOn\" min=\"40\" max=\"90\" value=\"{{FanTempOn}}\"/> </div><div class=\"form-field\"><label>OFF Temperature</label><input class=\"form-control\" type=\"number\" name=\"FanTempOff\" min=\"40\" max=\"90\" value=\"{{FanTempOff}}\"/> </div></div>";
const char *SettingsPagePumpSection =   "<div class=\"form-section\"> <h3 class=\"section-header\">Water Pump</h3> <div class=\"form-field\"><label>Enabled</label><input class=\"form-control\" type=\"checkbox\" name=\"PumpEnabled\" {{PumpEnabled}}/> </div><div class=\"form-field\"><label>IO Pin</label><input class=\"form-control\" type=\"number\" name=\"PumpPin\" min=\"0\" max=\"20\" value=\"{{PumpPin}}\"/></div><div class=\"form-field\"><label>ON Temperature</label><input class=\"form-control\" type=\"number\" name=\"PumpOnTemp\" min=\"40\" max=\"90\" value=\"{{PumpOnTemp}}\"/> </div><div class=\"form-field\"><label>ON Interval(ms)</label><input class=\"form-control\" type=\"number\" name=\"PumpOnInterval\" min=\"0\" max=\"999999\" value=\"{{PumpOnInterval}}\"/> </div><div class=\"form-field\"><label>OFF Interval (ms)</label><input class=\"form-control\" type=\"number\" name=\"PumpOffInterval\" min=\"0\" max=\"999999\" value=\"{{PumpOffInterval}}\"/></div></div>";
const char *SettingsPageMiscSection =   "<div class=\"form-section\"> <h3 class=\"section-header\">Misc</h3><div class=\"form-field\"><label>Ignore Errors</label><input class=\"form-control\" type=\"checkbox\" name=\"IgnoreErrors\" {{IgnoreErrors}}/></div><div class=\"form-field\"><label>Ferment Temp Pin</label><input class=\"form-control\" type=\"number\" name=\"FermentSensorPin\" min=\"0\" max=\"20\" value=\"{{FermentSensorPin}}\"/></div><div class=\"form-field\"><label>Check Interval (ms)</label><input class=\"form-control\" type=\"number\" name=\"CheckInterval\" min=\"1\" value=\"{{CheckInterval}}\"/> </div><div class=\"form-field\"><label>Intranet Only</label><input class=\"form-control\" type=\"checkbox\" name=\"IntranetOnly\" {{IntranetOnly}}/> </div></div>";
const char *SettingsPageEnd =           "</div><div><a href=\"/\">Cancel</a><button id=\"submit-button\" type=\"submit\">Save</button> </div></form></div></div></body></html>";

const char *SettingsUpdatedPage = "<!DOCTYPE html><html ><head> <meta charset='UTF-8'> <title>Fermentation Controller Settings Saved</title> </head><body><h3>Settings Saved!</h3><p>You are being redirected...</p><script type='text/javascript'>document.addEventListener('DOMContentLoaded', function(event){setTimeout(function(){window.location=window.location.origin;},3000)});</script></body></html>";

void HandleAccessDenied() {
    Serial.println("HandleAccessDenied. Remote IP: " + webServer->client().remoteIP().toString());
    webServer->send(401, "text/html", AccessDeniedPage);
}

void HandleRoot() {
  IPAddress ip = webServer->client().remoteIP();
  bool local = IsLocalIp(ip);
  Serial.println("Client Connected. Remote IP: " + ip.toString() + " IsLocal: " + (local ? "true" : "false"));
  if(!local && Controller.Config.IntranetOnly) {
    return HandleAccessDenied();
  }

  
  //get a new reading
  Serial.print("RefreshTemps");
  int error = Controller.Controls.RefreshTemps();
  Serial.println(" error = " + String(error));
  
  String page = HomePage;
  page.replace("{{AmbientTemp}}", String(Controller.Controls.GetAmbientTemp()));
  page.replace("{{FermentTemp}}",String(Controller.Controls.GetFermentTemp()));
  page.replace("{{FanOn}}",(Controller.Controls.IsFanOn() ? "ON" : "OFF"));
  page.replace("{{PumpOn}}",(Controller.Controls.IsPumpOn() ? "ON" : "OFF"));
  
	webServer->send(200, "text/html", page);
}

void HandleSettings() {
    IPAddress ip = webServer->client().remoteIP();
    bool local = IsLocalIp(ip);
    Serial.println("Client Connected. Remote IP: " + ip.toString() + " IsLocal: " + (local ? "true" : "false"));
    if(!local && Controller.Config.IntranetOnly) {
      return HandleAccessDenied();
    }
    String s1 = SettingsPageFanSection;
    s1.replace("{{FanEnabled}}",Controller.Config.FanEnabled ? "checked" : "");
    s1.replace("{{FanPin}}",String(Controller.Controls.GetFanPin()));
    s1.replace("{{FanTempOn}}",String(Controller.Config.FanTempOn));
    s1.replace("{{FanTempOff}}",String(Controller.Config.FanTempOff));
    
    String s2 = SettingsPagePumpSection;
    s2.replace("{{PumpEnabled}}",Controller.Config.PumpEnabled ? "checked" : "");
    s2.replace("{{PumpPin}}",String(Controller.Controls.GetPumpPin()));
    s2.replace("{{PumpOnInterval}}",String(Controller.Config.PumpOnInterval));
    s2.replace("{{PumpOffInterval}}",String(Controller.Config.PumpOffInterval));
    s2.replace("{{PumpOnTemp}}",String(Controller.Config.PumpOnTemp));
    
    String s3 = SettingsPageMiscSection;
    s3.replace("{{FermentSensorPin}}",String(Controller.Controls.GetFermenterSensorPin()));
    s3.replace("{{IntranetOnly}}",Controller.Config.IntranetOnly ? "checked" : "");
    s3.replace("{{IgnoreErrors}}",Controller.IgnoreErrors ? "checked" : "");
    s3.replace("{{CheckInterval}}",String(Controller.Config.CheckInterval));
    
    String page = SettingsPageBegin +s1 + s2 + s3 + SettingsPageEnd;
    webServer->send(200, "text/html", page);
}

void HandleUpdateSettings() {
  IPAddress ip = webServer->client().remoteIP();
  bool local = IsLocalIp(ip);
  Serial.println("Client Connected. Remote IP: " + ip.toString() + " IsLocal: " + (local ? "true" : "false"));
  if(!local && Controller.Config.IntranetOnly) {
    return HandleAccessDenied();
  }
  UpdateSettings();
  webServer->send(200, "text/html", SettingsUpdatedPage );
}

void UpdateSettings() {
    Serial.println("FanEnabled " + webServer->arg("FanEnabled"));
    Serial.println("FanPin " + webServer->arg("FanPin"));
    Serial.println("FanTempOn " + webServer->arg("FanTempOn"));
    Serial.println("FanTempOff " + webServer->arg("FanTempOff"));
    Serial.println("PumpEnabled " + webServer->arg("PumpEnabled"));
    Serial.println("PumpPin " + webServer->arg("PumpPin"));
    Serial.println("PumpOnInterval " + webServer->arg("PumpOnInterval"));
    Serial.println("PumpOffInterval " + webServer->arg("PumpOffInterval"));
    Serial.println("PumpOnTemp " + webServer->arg("PumpOnTemp"));
    
    Serial.println("IgnoreErrors " + webServer->arg("IgnoreErrors"));
    Serial.println("FermentSensorPin " + webServer->arg("FermentSensorPin"));
    Serial.println("IntranetOnly " + webServer->arg("IntranetOnly"));
    Serial.println("CheckInterval " + webServer->arg("CheckInterval"));
    
	
    Controller.Config.FanEnabled    =  webServer->arg("FanEnabled") == "on";
    Controller.Controls.SetFanPin(webServer->arg("FanPin").toInt());
    Controller.Config.FanTempOn    =   webServer->arg("FanTempOn").toInt();
    Controller.Config.FanTempOff    =  webServer->arg("FanTempOff").toInt();
    
    Controller.Config.PumpEnabled   =  webServer->arg("PumpEnabled") == "on";
    Controller.Controls.SetPumpPin(webServer->arg("PumpPin").toInt());
    Controller.Config.PumpOnInterval=  webServer->arg("PumpOnInterval").toInt();
    Controller.Config.PumpOffInterval=  webServer->arg("PumpOffInterval").toInt();
    Controller.Config.PumpOnTemp    =  webServer->arg("PumpOnTemp").toInt();
    
    Controller.IgnoreErrors         =  webServer->arg("IgnoreErrors") == "on";
    Controller.Controls.SetFermenterSensorPin( webServer->arg("FermentSensorPin").toInt());
    Controller.Config.IntranetOnly  =  webServer->arg("IntranetOnly") == "on";
    Controller.Config.CheckInterval =  webServer->arg("CheckInterval").toInt();
	
	  Controller.SaveConfig();
}

bool ConnectToNetwork() {
    Serial.println("ConnectToNetwork");
    int timeout=10000;
    int numNetworks = sizeof Networks / sizeof Networks[0];
    Serial.println("There are " + String(numNetworks) +" networks to attempt to connect to" );
    String ssid,pass;
    bool connected = false;
    for(int i=0;i < numNetworks;i++) {
      ssid = Networks[i][0];
      pass = Networks[i][1];
      Serial.println("Attempting to connect to " + ssid);
      WiFi.begin(ssid.c_str(), pass.c_str());
      long _timeout = millis() + timeout;
      do {
        //yield();
        delay(500);
        connected = WiFi.status() == WL_CONNECTED;
      } while (!connected && millis() < _timeout) ;

      if(connected) {
        Serial.println(WifiExtensions::IpToString(WiFi.localIP()));
        break;
      } else {
        Serial.println("Connect Failed");
      }
    }
    Serial.println("ConnectToNetwork done");
    
}
