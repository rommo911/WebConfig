/*

File WebConfig.h
Version 1.4
Author Gerald Lechner
contakt lechge@gmail.com

Description
This library builds a web page with a smart phone friendly form to edit
a free definable number of configuration parameters.
The submitted data will bestored in the SPIFFS
The library works with ESP8266 and ESP32

Dependencies:
  ESP8266WebServer.h
  ArduinoJson.h

*/
#ifndef WebConfig_h
#define WebConfig_h

#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(ESP32)
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#endif

//maximum number of parameters
#define MAXVALUES 20

//maximum number of options per parameters
#define MAXOPTIONS 15

//character limits
#define NAMELENGTH 20
#define LABELLENGTH 40

//name for the config file
#define CONFFILE "/WebConf.conf"



#define OPTION_INPUTTEXT "0"
#define OPTION_INPUTPASSWORD "1"
#define OPTION_INPUTNUMBER "2"
#define OPTION_INPUTDATE "3"
#define OPTION_INPUTTIME "4"
#define OPTION_INPUTRANGE "5"
#define OPTION_INPUTCHECKBOX "6"
#define OPTION_INPUTRADIO "7"
#define OPTION_INPUTSELECT "8"
#define OPTION_INPUTCOLOR "9"
#define OPTION_INPUTFLOAT "10"
#define OPTION_INPUTTEXTAREA "11"
#define OPTION_INPUTMULTICHECK "12"


//number of types
#define INPUTTYPES 13

#define BTN_CONFIG 0
#define BTN_DONE 1
#define BTN_CANCEL 2
#define BTN_DELETE 4
//data structure to hold the parameter Description
typedef //Struktur eines Datenpakets
struct {
  char name[NAMELENGTH];
  char label[LABELLENGTH];
  uint8_t type;
  int min;
  int max;
  uint8_t optionCnt;
  String options[MAXOPTIONS];
  String labels[MAXOPTIONS];
} DESCRIPTION;

class WebConfig {
  public:
  WebConfig(boolean NVS = false, const char* NVSNamespace = "default");
  //load form descriptions
  void setDescription(String parameter, WebServer* server);
  //Add extra descriptions
  void addDescription(String parameter);
  //function to respond a HTTP request for the form use the filename
  //to save.
  bool handleRoot();
  int16_t getIndex(const char* name);
  //read configuration from default file
  boolean readConfig();
  //write configuration to default file
  boolean writeConfig();
  //

  //delete configuration file
  boolean deleteConfig(const char* filename);
  //delete default configutation file
  boolean deleteConfig();

  boolean deleteConfigNVS();

  //get a parameter value by its name
  const String getString(const char* name);
  const char* getValue(const char* name);
  int getInt(const char* name);
  float getFloat(const char* name);
  boolean getBool(const char* name);
  //

  //get the accesspoint name
  const char* getDeviceName();
  //get the number of parameters
  uint8_t getCount();
  //get the name of a parameter
  String getName(uint8_t index);
  //Get results as a JSON string
  String getResults();
  //
  JsonObject getResultsJson();
  //Ser values from a JSON string
  void setValues(String json);
  //set the value for a parameter
  void setValue(const char* name, String value);
  //set the label for a parameter
  void setLabel(const char* name, const char* label);
  //remove all options
  void clearOptions(uint8_t index);
  void clearOptions(const char* name);
  //add a new option
  void addOption(uint8_t index, String option);
  void addOption(uint8_t index, String option, String label);
  //modify an option
  void setOption(uint8_t index, uint8_t option_index, String option, String label);
  void setOption(char* name, uint8_t option_index, String option, String label);
  //get the options count
  uint8_t getOptionCount(uint8_t index);
  uint8_t getOptionCount(char* name);
  //set form type to doen cancel
  void setButtons(uint8_t buttons);
  //register onSave callback
  void registerOnSave(std::function<void(String)>);
  void registerOnSave(std::function<void(JsonObject)>);
  void registerOnSave(std::function<void()>callback);
  //register onSave callback
  void registerOnDone(void (*callback)(String results));
  //register onSave callback
  void registerOnCancel(void (*callback)());
  //register onSave callback
  void registerOnDelete(void (*callback)(String name));

  //values for the parameter
  String values[MAXVALUES];
  private:
  const boolean isNVS;
  char _buf[1000];
  WebServer* _server{ nullptr };
  uint8_t _count;
  String _deviceNAme;
  String nameSpace;
  uint8_t _buttons = BTN_CONFIG;
  DESCRIPTION _description[MAXVALUES];
  std::function<void(String)> _onSave{ nullptr };
  std::function<void()> _onSave_null{ nullptr };
  std::function<void(JsonObject)> _onSaveJson{ nullptr };
  void (*_onDone)(String results) = NULL;
  void (*_onCancel)() = NULL;
  void (*_onDelete)(String name) = NULL;
  boolean writeConfig(const char* filename);

#if defined(ESP32)
  boolean writeConfigNVS();
  void handleFormRequest(WebServer* server, const char* filename);
  //function to respond a HTTP request for the form use the default file
  //to save and restart ESP after saving the new config
  void handleFormRequest(WebServer* server);
  //get the index for a value by parameter name
  const String getStringNVS(const char* name);
  int getIntNVS(const char* name);
  float getfloatNVS(const char* name);

#else
  void handleFormRequest(ESP8266WebServer* server, const char* filename);
  //function to respond a HTTP request for the form use the default file
  //to save and restart ESP after saving the new config
  void handleFormRequest(ESP8266WebServer* server);
  //get the index for a value by parameter name
#endif

};

#endif
