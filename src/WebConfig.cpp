/*
File WebConfig.cpp
Version 1.4
Author Gerald Lechner
contakt lechge@gmail.com
Description
This library builds a web page with a smart phone friendly form to edit
a free definable number of configuration parameters.
The submitted data will bestored in the LittleFS
The library works with ESP8266 and ESP32
Dependencies:
  ESP8266WebServer.h
  ArduinoJson.h
*/

#include <WebConfig.h>
#include <Arduino.h>
#if defined(ESP32)
#include "LittleFS.h"
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#endif
#include <ArduinoJson.h>
#include <FS.h>
#include "Preferences.h"

const char *inputtypes[] = {"text", "password", "number", "date", "time", "range", "check", "radio", "select", "color", "float"};

// HTML templates
// Template for header and begin of form
const char HTML_START[] PROGMEM =
    "<!DOCTYPE HTML>\n"
    "<html lang='de'>\n"
    "<head>\n"
    "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n"
    "<meta name='viewport' content='width=320' />\n"
    "<title>ESP Config Portal</title>\n"
    "<style>\n"
    "body {\n"
    "  background-color: #d2f3eb;\n"
    "  font-family: Arial, Helvetica, Sans-Serif;\n"
    "  Color: #000000;\n"
    "  font-size:12pt;\n"
    "  width:320px;\n"
    "}\n"
    ".titel {\n"
    "font-weight:bold;\n"
    "text-align:center;\n"
    "width:100%%;\n"
    "padding:5px;\n"
    "}\n"
    ".zeile {\n"
    "  width:100%%;\n"
    "  padding:5px;\n"
    "  text-align: center;\n"
    "}\n"
    "button {\n"
    "font-size:14pt;\n"
    "width:150px;\n"
    "border-radius:10px;\n"
    "margin:5px;\n"
    "}\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "<div id='main_div' style='margin-left:15px;margin-right:15px;'>\n"
    "<div class='titel'>ESP Config Portal %s</div>\n"
    "<form method='post'>\n";

// Template for one input field
const char HTML_TEX_SIMPLE[] PROGMEM =
    "  <div class='zeile'><b>%s</b></div>\n";
const char HTML_ENTRY_SIMPLE[] PROGMEM =
    "  <div class='zeile'><b>%s</b></div>\n"
    "  <div class='zeile'><input type='%s' value='%s' name='%s'></div>\n";
const char HTML_ENTRY_AREA[] PROGMEM =
    "  <div class='zeile'><b>%s</b></div>\n"
    "  <div class='zeile'><textarea rows='%i' cols='%i' name='%s'>%s</textarea></div>\n";
const char HTML_ENTRY_NUMBER[] PROGMEM =
    "  <div class='zeile'><b>%s</b></div>\n"
    "  <div class='zeile'><input type='number' min='%i' max='%i' value='%s' name='%s'></div>\n";
const char HTML_ENTRY_RANGE[] PROGMEM =
    "  <div class='zeile'><b>%s</b></div>\n"
    "  <div class='zeile'>%i&nbsp;<input type='range' min='%i' max='%i' value='%s' name='%s'>&nbsp;%i</div>\n";
const char HTML_ENTRY_CHECKBOX[] PROGMEM =
    "  <div class='zeile'><b>%s</b><input type='checkbox' %s name='%s'></div>\n";
const char HTML_ENTRY_RADIO_TITLE[] PROGMEM =
    " <div class='zeile'><b>%s</b></div>\n";
const char HTML_ENTRY_RADIO[] =
    "  <div class='zeile'><input type='radio' name='%s' value='%s' %s>%s</div>\n";
const char HTML_ENTRY_SELECT_START[] PROGMEM =
    " <div class='zeile'><b>%s</b></div>\n"
    " <div class='zeile'><select name='%s'>\n";
const char HTML_ENTRY_SELECT_OPTION[] PROGMEM =
    "  <option value='%s' %s>%s</option>\n";
const char HTML_ENTRY_SELECT_END[] PROGMEM =
    " </select></div>\n";
const char HTML_ENTRY_MULTI_START[] PROGMEM =
    " <div class='zeile'><b>%s</b></div>\n"
    " <div class='zeile'><fieldset style='text-align:left;'>\n";
const char HTML_ENTRY_MULTI_OPTION[] PROGMEM =
    "  <input type='checkbox' name='%s', value='%i' %s>%s<br>\n";
const char HTML_ENTRY_MULTI_END[] PROGMEM =
    " </fieldset></div>\n";

// Template for save button and end of the form with save
const char HTML_END_ERROR_SAVE[] PROGMEM =
    "  <div class='zeile'><b>ERROR IN SAVING</b></div>\n"
    "<div class='zeile'><button type='submit' name='SAVE'>Save</button>\n"
    "<button type='submit' name='RST'>Restart</button></div>\n"
    "</form>\n"
    "</div>\n"
    "</body>\n"
    "</html>\n";

const char HTML_END_NOSAVE[] PROGMEM =
    "<button type='submit' name='RST'>Restart</button></div>\n"
    "</form>\n"
    "</div>\n"
    "</body>\n"
    "</html>\n";
const char HTML_END[] PROGMEM =
    "<div class='zeile'><button type='submit' name='SAVE'>Save</button>\n"
    "<button type='submit' name='RST'>Restart</button></div>\n"
    "</form>\n"
    "</div>\n"
    "</body>\n"
    "</html>\n";
// Template for save button and end of the form without save
const char HTML_BUTTON[] PROGMEM =
    "<button type='submit' name='%s'>%s</button>\n";

#define INPUTTEXT 0
#define INPUTPASSWORD 1
#define INPUTNUMBER 2
#define INPUTDATE 3
#define INPUTTIME 4
#define INPUTRANGE 5
#define INPUTCHECKBOX 6
#define INPUTRADIO 7
#define INPUTSELECT 8
#define INPUTCOLOR 9
#define INPUTFLOAT 10
#define INPUTTEXTAREA 11
#define INPUTMULTICHECK 12

WebConfig::WebConfig(boolean NVS, const char *NVSNamespace) : isNVS(NVS), nameSpace(NVSNamespace)
{
  _deviceNAme = "";
};

void WebConfig::setDescription(String parameter, WebServer *server)
{
  Staticindex = 0;
  addDescription(parameter);
  if (server != nullptr)
  {
    this->_server = server;
    server->on("/", [&]()
               { this->handleRoot(); });
  }
}
bool WebConfig::handleRoot()
{
  if (_server == nullptr)
    return false;
  this->handleFormRequest(_server);
  if (_server->hasArg("SAVE"))
  {
    uint8_t cnt = this->getCount();
    Serial.println("*********** Config recieved ************");
    for (uint8_t i = 0; i < cnt; i++)
    {
      Serial.print(this->getName(i));
      Serial.print(" = ");
      Serial.println(this->values[i]);
    }
    Serial.println("*********** Config done ************");
    return true;
  }
  else
  {
    return false;
  }
}

void WebConfig::addDescription(String parameter)
{
  DeserializationError error;
  const int capacity = JSON_ARRAY_SIZE(MAXVALUES) + MAXVALUES * JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);
  char tmp[40];
  error = deserializeJson(doc, parameter);
  if (error)
  {
    Serial.println(parameter);
    Serial.print("JSON AddDescription: ");
    Serial.println(error.c_str());
  }
  else
  {
    JsonArray array = doc.as<JsonArray>();
    uint8_t j = 0;
    for (JsonObject obj : array)
    {
      if (Staticindex < MAXVALUES)
      {
        _description[Staticindex].optionCnt = 0;
        if (obj.containsKey("name"))
        {
          if (isNVS)
          {
            strlcpy(tmp, obj["name"], NVS_NAMELENGTH);
            if (strlen(tmp) > 15)
            {
              Serial.printf("WARNING NVS Key Too long!  %s , will be trimmed \n\r", _description[Staticindex].name);
            }
            strlcpy(_description[Staticindex].name, obj["name"], NVS_NAMELENGTH);
          }
          else
          {
            strlcpy(_description[Staticindex].name, obj["name"], NAMELENGTH);
          }
        }
        if (obj.containsKey("label"))
          strlcpy(_description[Staticindex].label, obj["label"], LABELLENGTH);
        if (obj.containsKey("type"))
        {
          if (obj["type"].is<const char *>())
          {
            uint8_t t = 0;
            strlcpy(tmp, obj["type"], 30);
            while ((t < INPUTTYPES) && (strcmp(tmp, inputtypes[t]) != 0))
              t++;
            if (t > INPUTTYPES)
              t = 0;
            _description[Staticindex].type = t;
          }
          else
          {
            _description[Staticindex].type = obj["type"];
          }
        }
        else
        {
          _description[Staticindex].type = INPUTTEXT;
        }
        _description[Staticindex].max = (obj.containsKey("max")) ? obj["max"] : 99999;
        _description[Staticindex].min = (obj.containsKey("min")) ? obj["min"] : 0;
        if (isNVS)
        {

#if defined(ESP32)
          switch (_description[Staticindex].type)
          {
          case INPUTPASSWORD:
          case INPUTSELECT:
          case INPUTDATE:
          case INPUTTIME:
          case INPUTRADIO:
          case INPUTCOLOR:
          case INPUTTEXT:
          {
            values[Staticindex] = getStringNVS(_description[Staticindex].name);
            if (values[Staticindex] == "")
            {
              if (obj.containsKey("default"))
              {
                // strlcpy(tmp, obj["default"], 30);
                values[Staticindex] = String(obj["default"]);
                Serial.printf("laoded default value %s:%s\n\r", _description[Staticindex].name, values[Staticindex].c_str());
              }
            }
            else 
            {
              Serial.printf("laoded from NVS %s:%s\n\r", _description[Staticindex].name, values[Staticindex].c_str());
            }
            break;
          }
          case INPUTCHECKBOX:
          case INPUTRANGE:
          case INPUTNUMBER:
          {
            values[Staticindex] = getIntNVS(_description[Staticindex].name);
            if (values[Staticindex].toInt() == 0x7FFFFFFF)
            {
              if (obj.containsKey("default"))
              {
                // strlcpy(tmp, obj["default"], 30);
                values[Staticindex] = String(obj["default"]);
                Serial.printf("laoded default value %s:%s\n\r", _description[Staticindex].name, values[Staticindex].c_str());
              }
            }
            else 
            {
              Serial.printf("laoded from NVS %s:%s\n\r", _description[Staticindex].name, values[Staticindex].c_str());
            }
            break;
          }
          case INPUTFLOAT:
          {
            values[Staticindex] = String(getfloatNVS(_description[Staticindex].name));
            if (values[Staticindex].toFloat() == 0x7FFFFFFF)
            {
              if (obj.containsKey("default"))
              {
                // strlcpy(tmp, obj["default"], 30);
                values[Staticindex] = String(obj["default"]);
                Serial.printf("laoded default value %s:%s\n\r", _description[Staticindex].name, values[Staticindex].c_str());
              }
            }
            else 
            {
              Serial.printf("laoded from NVS %s:%s\n\r", _description[Staticindex].name, values[Staticindex].c_str());
            }
            break;
          }
          }
#endif
        }
        else
        {
          if (obj.containsKey("default"))
          {
            strlcpy(tmp, obj["default"], 30);
            values[Staticindex] = String(tmp);
          }
          else
          {
            values[Staticindex] = "0";
          }
        }
        if (obj.containsKey("options"))
        {
          JsonArray opt = obj["options"].as<JsonArray>();
          j = 0;
          for (JsonObject o : opt)
          {
            if (j < MAXOPTIONS)
            {
              _description[Staticindex].options[j] = o["v"].as<String>();
              _description[Staticindex].labels[j] = o["l"].as<String>();
            }
            j++;
          }
          _description[Staticindex].optionCnt = opt.size();
        }
      }
      Staticindex++;
    }
  }
  if (isNVS)
  {
    _deviceNAme = getStringNVS("deviceName");
  }
  else
  {
    _deviceNAme = WiFi.macAddress();
  }
  //_deviceNAme.replace(":", "");
  if (!isNVS)
  {
    if (!LittleFS.begin())
    {
      LittleFS.format();
      LittleFS.begin();
    }
  }
};

void createSimple(char *buf, const char *name, const char *label, const char *type, String value)
{
  sprintf(buf, HTML_ENTRY_SIMPLE, label, type, value.c_str(), name);
}

void createTextarea(char *buf, DESCRIPTION descr, String value)
{
  // max = rows min = cols
  sprintf(buf, HTML_ENTRY_AREA, descr.label, descr.max, descr.min, descr.name, value.c_str());
}

void createNumber(char *buf, DESCRIPTION descr, String value)
{
  sprintf(buf, HTML_ENTRY_NUMBER, descr.label, descr.min, descr.max, value.c_str(), descr.name);
}

void createRange(char *buf, DESCRIPTION descr, String value)
{
  sprintf(buf, HTML_ENTRY_RANGE, descr.label, descr.min, descr.min, descr.max, value.c_str(), descr.name, descr.max);
}

void createCheckbox(char *buf, DESCRIPTION descr, String value)
{
  if (value != "0")
  {
    sprintf(buf, HTML_ENTRY_CHECKBOX, descr.label, "checked", descr.name);
  }
  else
  {
    sprintf(buf, HTML_ENTRY_CHECKBOX, descr.label, "", descr.name);
  }
}

void createRadio(char *buf, DESCRIPTION descr, String value, uint8_t index)
{
  if (value == descr.options[index])
  {
    sprintf(buf, HTML_ENTRY_RADIO, descr.name, descr.options[index].c_str(), "checked", descr.labels[index].c_str());
  }
  else
  {
    sprintf(buf, HTML_ENTRY_RADIO, descr.name, descr.options[index].c_str(), "", descr.labels[index].c_str());
  }
}

void startSelect(char *buf, DESCRIPTION descr)
{
  sprintf(buf, HTML_ENTRY_SELECT_START, descr.label, descr.name);
}

void addSelectOption(char *buf, String option, String label, String value)
{
  if (option == value)
  {
    sprintf(buf, HTML_ENTRY_SELECT_OPTION, option.c_str(), "selected", label.c_str());
  }
  else
  {
    sprintf(buf, HTML_ENTRY_SELECT_OPTION, option.c_str(), "", label.c_str());
  }
}

void startMulti(char *buf, DESCRIPTION descr)
{
  sprintf(buf, HTML_ENTRY_MULTI_START, descr.label);
}

void addMultiOption(char *buf, String name, uint8_t option, String label, String value)
{
  if ((value.length() > option) && (value[option] == '1'))
  {
    sprintf(buf, HTML_ENTRY_MULTI_OPTION, name.c_str(), option, "checked", label.c_str());
  }
  else
  {
    sprintf(buf, HTML_ENTRY_MULTI_OPTION, name.c_str(), option, "", label.c_str());
  }
}

//***********Different type for ESP32 WebServer and ESP8266WebServer ********
// both classes have the same functions
#if defined(ESP32)
// function to respond a HTTP request for the form use the default file
// to save and restart ESP after saving the new config
void WebConfig::handleFormRequest(WebServer *server)
{
  handleFormRequest(server, CONFFILE);
}
// function to respond a HTTP request for the form use the filename
// to save. If auto is true restart ESP after saving the new config
void WebConfig::handleFormRequest(WebServer *server, const char *filename)
{
#else
// function to respond a HTTP request for the form use the default file
// to save and restart ESP after saving the new config
void WebConfig::handleFormRequest(ESP8266WebServer *server)
{
  handleFormRequest(server, CONFFILE);
}
// function to respond a HTTP request for the form use the filename
// to save. If auto is true restart ESP after saving the new config
void WebConfig::handleFormRequest(ESP8266WebServer *server, const char *filename)
{
#endif
  //******************** Rest of the function has no difference ***************
  bool saved = false;
  bool errorSaving = false;

  uint8_t a, v;
  String val;
  if (server->args() > 0)
  {
    if (server->hasArg(F("deviceName")))
      _deviceNAme = server->arg(F("deviceName"));

    for (uint8_t i = 0; i < Staticindex; i++)
    {
      if (_description[i].type == INPUTCHECKBOX)
      {
        values[i] = "0";
        if (server->hasArg(_description[i].name))
          values[i] = "1";
      }
      else if (_description[i].type == INPUTMULTICHECK)
      {
        values[i] = "";
        for (a = 0; a < _description[i].optionCnt; a++)
          values[i] += "0"; // clear result
        for (a = 0; a < server->args(); a++)
        {
          if (server->argName(a) == _description[i].name)
          {
            val = server->arg(a);
            v = val.toInt();
            values[i].setCharAt(v, '1');
          }
        }
      }
      else
      {
        if (server->hasArg(_description[i].name))
          values[i] = server->arg(_description[i].name);
      }
    }
    if (server->hasArg(F("SAVE")) || server->hasArg(F("RST")))
    {

      saved = writeConfig();
      errorSaving = !saved;
      Serial.println(saved);
      if (server->hasArg(F("RST")))
      {
        ESP.restart();
      }
    }
  }
  boolean exit = false;
  if (server->hasArg(F("SAVE")) && _onSave)
  {
    _onSave(getResults());
  }
  if (server->hasArg(F("SAVE")) && _onSaveJson)
  {
    _onSaveJson(getResultsJson());
  }
  if (server->hasArg(F("SAVE")) && _onSave_null)
  {
    _onSave_null();
  }
  if (server->hasArg(F("DONE")) && _onDone)
  {
    _onDone(getResults());
    exit = true;
  }
  if (server->hasArg(F("CANCEL")) && _onCancel)
  {
    _onCancel();
    exit = true;
  }
  if (server->hasArg(F("DELETE")) && _onDelete)
  {
    _onDelete(_deviceNAme);
    exit = true;
  }
  if (!exit)
  {
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    sprintf(_buf, HTML_START, _deviceNAme.c_str());
    server->send(200, "text/html", _buf);
    if (_buttons == BTN_CONFIG)
    {
      createSimple(_buf, "deviceName", "device Name", "text", _deviceNAme);
      server->sendContent(_buf);
    }

    for (uint8_t i = 0; i < Staticindex; i++)
    {
      switch (_description[i].type)
      {
      case INPUTFLOAT:
      case INPUTTEXT:
        createSimple(_buf, _description[i].name, _description[i].label, "text", values[i]);
        break;
      case INPUTTEXTAREA:
        createTextarea(_buf, _description[i], values[i]);
        break;
      case INPUTPASSWORD:
        createSimple(_buf, _description[i].name, _description[i].label, "password", values[i]);
        break;
      case INPUTDATE:
        createSimple(_buf, _description[i].name, _description[i].label, "date", values[i]);
        break;
      case INPUTTIME:
        createSimple(_buf, _description[i].name, _description[i].label, "time", values[i]);
        break;
      case INPUTCOLOR:
        createSimple(_buf, _description[i].name, _description[i].label, "color", values[i]);
        break;
      case INPUTNUMBER:
        createNumber(_buf, _description[i], values[i]);
        break;
      case INPUTRANGE:
        createRange(_buf, _description[i], values[i]);
        break;
      case INPUTCHECKBOX:
        createCheckbox(_buf, _description[i], values[i]);
        break;
      case INPUTRADIO:
        sprintf(_buf, HTML_ENTRY_RADIO_TITLE, _description[i].label);
        for (uint8_t j = 0; j < _description[i].optionCnt; j++)
        {
          server->sendContent(_buf);
          createRadio(_buf, _description[i], values[i], j);
        }
        break;
      case INPUTSELECT:
        startSelect(_buf, _description[i]);
        for (uint8_t j = 0; j < _description[i].optionCnt; j++)
        {
          server->sendContent(_buf);
          addSelectOption(_buf, _description[i].options[j], _description[i].labels[j], values[i]);
        }
        server->sendContent(_buf);
        strcpy_P(_buf, HTML_ENTRY_SELECT_END);
        break;
      case INPUTMULTICHECK:
        startMulti(_buf, _description[i]);
        for (uint8_t j = 0; j < _description[i].optionCnt; j++)
        {
          server->sendContent(_buf);
          addMultiOption(_buf, _description[i].name, j, _description[i].labels[j], values[i]);
        }
        server->sendContent(_buf);
        strcpy_P(_buf, HTML_ENTRY_MULTI_END);
        break;
      default:
        _buf[0] = 0;
        break;
      }
      server->sendContent(_buf);
    }
    if (saved)
    {
      sprintf(_buf, HTML_TEX_SIMPLE, "SAVED!");
      server->sendContent(_buf);
    }
    if (_buttons == BTN_CONFIG)
    {
      if (saved && !errorSaving)
        server->sendContent(HTML_END_NOSAVE);
      else if (!saved & errorSaving)
      {
        server->sendContent(HTML_END_ERROR_SAVE);
      }
      else
        server->sendContent(HTML_END);
    }
    else
    {
      server->sendContent("<div class='zeile'>\n");
      if ((_buttons & BTN_DONE) == BTN_DONE)
      {
        sprintf(_buf, HTML_BUTTON, "DONE", "Done");
        server->sendContent(_buf);
      }
      if ((_buttons & BTN_CANCEL) == BTN_CANCEL)
      {
        sprintf(_buf, HTML_BUTTON, "CANCEL", "Cancel");
        server->sendContent(_buf);
      }
      if ((_buttons & BTN_DELETE) == BTN_DELETE)
      {
        sprintf(_buf, HTML_BUTTON, "DELETE", "Delete");
        server->sendContent(_buf);
      }
      server->sendContent("</div></form></div></body></html>\n");
    }
  }
}
// get the index for a value by parameter name
int16_t WebConfig::getIndex(const char *name)
{
  int16_t i = Staticindex - 1;
  while ((i >= 0) && (strcmp(name, _description[i].name) != 0))
  {
    i--;
  }
  return i;
}
// read configuration from file
boolean WebConfig::readConfig()
{
  if (isNVS)
    return false;
  String line, name, value;
  uint8_t pos;
  int16_t index;
  if (!LittleFS.exists(CONFFILE))
  {
    // if configfile does not exist write default values
    writeConfig(CONFFILE);
  }
  File f = LittleFS.open(CONFFILE, "r");
  if (f)
  {
    Serial.println(F("Read configuration"));
    uint32_t size = f.size();
    while (f.position() < size)
    {
      line = f.readStringUntil(10);
      pos = line.indexOf('=');
      name = line.substring(0, pos);
      value = line.substring(pos + 1);
      if ((name == "deviceName") && (value != ""))
      {
        _deviceNAme = value;
        Serial.println(line);
      }
      else
      {
        index = getIndex(name.c_str());
        if (!(index < 0))
        {
          value.replace("~", "\n");
          values[index] = value;
          if (_description[index].type == INPUTPASSWORD)
          {
            Serial.printf("%s=*************\n", _description[index].name);
          }
          else
          {
            Serial.println(line);
          }
        }
      }
    }
    f.close();
    return true;
  }
  else
  {
    Serial.println(F("Cannot read configuration"));
    return false;
  }
}

// write configuration to file
boolean WebConfig::writeConfig(const char *filename)
{
  if (isNVS)
    return false;
  String val;
  File f = LittleFS.open(filename, "w");
  if (f)
  {
    f.printf("deviceName=%s\n", _deviceNAme.c_str());
    for (uint8_t i = 0; i < Staticindex; i++)
    {
      val = values[i];
      val.replace("\n", "~");
      f.printf("%s=%s\n", _description[i].name, val.c_str());
    }
    return true;
  }
  else
  {
    Serial.println(F("Cannot write configuration"));
    return false;
  }
}
#if defined(ESP32)
boolean WebConfig::writeConfigNVS()
{
  String val;
  Preferences preferences;
  boolean ret = preferences.begin(nameSpace.c_str(), false);
  bool NOerrorOccured = true;
  if (ret)
  {
    preferences.putString("deviceName", _deviceNAme.c_str());
    for (uint8_t i = 0; i < Staticindex; i++)
    {
      val = values[i];
      val.replace("\n", "~");
      // NOerrorOccured &=
      size_t result;
      switch (_description[i].type)
      {
      case INPUTPASSWORD:
      case INPUTSELECT:
      case INPUTDATE:
      case INPUTTIME:
      case INPUTRADIO:
      case INPUTCOLOR:
      case INPUTTEXT:
        result = preferences.putString(_description[i].name, val);
        break;
      case INPUTCHECKBOX:
      case INPUTRANGE:
      case INPUTNUMBER:
        result = preferences.putInt(_description[i].name, val.toInt());
        break;
      case INPUTFLOAT:
        result = preferences.putFloat(_description[i].name, val.toFloat());
        break;
      default:
        result = 0;
        break;
      }
      Serial.printf("saving to nvs %s:%s ,returned %d \n\r", _description[i].name, val.c_str(), result);
      NOerrorOccured &= result > 0;
    }
    return NOerrorOccured;
  }
  else
  {
    Serial.println(F("Cannot write configuration to nvs "));
    return false;
  }
}
#endif

// write configuration to default file
boolean WebConfig::writeConfig()
{
  if (isNVS)
    return writeConfigNVS();
  else
    return writeConfig(CONFFILE);
}
// delete configuration file
boolean WebConfig::deleteConfig(const char *filename)
{
  return LittleFS.remove(filename);
}

#if defined(ESP32)
boolean WebConfig::deleteConfigNVS()
{
  Preferences preferences;
  if (preferences.begin(nameSpace.c_str(), false))
    return preferences.clear();
  return false;
}
#endif

// delete default configutation file
boolean WebConfig::deleteConfig()
{
  return deleteConfig(CONFFILE);
}

// get a parameter value by its name
const String WebConfig::getString(const char *name)
{
  int16_t index;
  index = getIndex(name);
  if (index < 0)
  {
    return "";
  }
  else
  {
    return values[index];
  }
}

#if defined(ESP32)
const String WebConfig::getStringNVS(const char *name)
{
  Preferences p;
  if (p.begin(this->nameSpace.c_str()))
  {
    if (p.isKey(name))
    {
      return p.getString(name);
    }
  }
  return "";
}

int WebConfig::getIntNVS(const char *name)
{
  Preferences p;
  if (p.begin(this->nameSpace.c_str()))
  {
    if (p.isKey(name))
    {
      return p.getInt(name);
    }
  }
  return 0x7FFFFFFF;
}

float WebConfig::getfloatNVS(const char *name)
{
  Preferences p;
  if (p.begin(this->nameSpace.c_str()))
  {
    if (p.isKey(name))
    {
      return p.getFloat(name);
    }
  }
  return 0x7FFFFFFF;
}
#endif

// Get results as a JSON string
String WebConfig::getResults()
{
  char buffer[1024];
  StaticJsonDocument<1000> doc;
  for (uint8_t i = 0; i < Staticindex; i++)
  {
    switch (_description[i].type)
    {
    case INPUTPASSWORD:
    case INPUTSELECT:
    case INPUTDATE:
    case INPUTTIME:
    case INPUTRADIO:
    case INPUTCOLOR:
    case INPUTTEXT:
      doc[_description[i].name] = values[i];
      break;
    case INPUTCHECKBOX:
    case INPUTRANGE:
    case INPUTNUMBER:
      doc[_description[i].name] = values[i].toInt();
      break;
    case INPUTFLOAT:
      doc[_description[i].name] = values[i].toFloat();
      break;
    }
  }
  serializeJson(doc, buffer);
  return String(buffer);
}

JsonObject WebConfig::getResultsJson()
{
  JsonObject doc;
  for (uint8_t i = 0; i < Staticindex; i++)
  {
    switch (_description[i].type)
    {
    case INPUTPASSWORD:
    case INPUTSELECT:
    case INPUTDATE:
    case INPUTTIME:
    case INPUTRADIO:
    case INPUTCOLOR:
    case INPUTTEXT:
      doc[_description[i].name] = values[i];
      break;
    case INPUTCHECKBOX:
    case INPUTRANGE:
    case INPUTNUMBER:
      doc[_description[i].name] = values[i].toInt();
      break;
    case INPUTFLOAT:
      doc[_description[i].name] = values[i].toFloat();
      break;
    }
  }
  return doc;
}

// Ser values from a JSON string
void WebConfig::setValues(String json)
{
  int val;
  float fval;
  char sval[255];
  DeserializationError error;
  StaticJsonDocument<1000> doc;
  error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("JSON: ");
    Serial.println(error.c_str());
  }
  else
  {
    for (uint8_t i = 0; i < Staticindex; i++)
    {
      if (doc.containsKey(_description[i].name))
      {
        switch (_description[i].type)
        {
        case INPUTPASSWORD:
        case INPUTSELECT:
        case INPUTDATE:
        case INPUTTIME:
        case INPUTRADIO:
        case INPUTCOLOR:
        case INPUTTEXT:
          strlcpy(sval, doc[_description[i].name], 255);
          values[i] = String(sval);
          break;
        case INPUTCHECKBOX:
        case INPUTRANGE:
        case INPUTNUMBER:
          val = doc[_description[i].name];
          values[i] = String(val);
          break;
        case INPUTFLOAT:
          fval = doc[_description[i].name];
          values[i] = String(fval);
          break;
        }
      }
    }
  }
}

const char *WebConfig::getValue(const char *name)
{
  int16_t index;
  index = getIndex(name);
  if (index < 0)
  {
    return "";
  }
  else
  {
    return values[index].c_str();
  }
}

int WebConfig::getInt(const char *name)
{
#if defined(ESP32)
  if (isNVS)
  {
    return getStringNVS(name).toInt();
  }
  else
#endif
    return getString(name).toInt();
}

float WebConfig::getFloat(const char *name)
{
#if defined(ESP32)

  if (isNVS)
  {
    return getStringNVS(name).toFloat();
  }
  else
#endif

    return getString(name).toFloat();
}

boolean WebConfig::getBool(const char *name)
{
#if defined(ESP32)
  if (isNVS)
  {
    return getStringNVS(name) != "0";
  }
  else
#endif

    return (getString(name) != "0");
}

// get the accesspoint name
const char *WebConfig::getDeviceName()
{
  return _deviceNAme.c_str();
}
// get the number of parameters
uint8_t WebConfig::getCount()
{
  return Staticindex;
}

// get the name of a parameter
String WebConfig::getName(uint8_t index)
{
  if (index < Staticindex)
  {
    return String(_description[index].name);
  }
  else
  {
    return "";
  }
}

// set the value for a parameter
void WebConfig::setValue(const char *name, String value)
{
  int16_t i = getIndex(name);
  if (i >= 0)
    values[i] = value;
}

// set the label for a parameter
void WebConfig::setLabel(const char *name, const char *label)
{
  int16_t i = getIndex(name);
  if (i >= 0)
    strlcpy(_description[i].label, label, LABELLENGTH);
}

// remove all options
void WebConfig::clearOptions(uint8_t index)
{
  if (index < Staticindex)
    _description[index].optionCnt = 0;
}

void WebConfig::clearOptions(const char *name)
{
  int16_t i = getIndex(name);
  if (i >= 0)
    clearOptions(i);
}

// add a new option
void WebConfig::addOption(uint8_t index, String option)
{
  addOption(index, option, option);
}

void WebConfig::addOption(uint8_t index, String option, String label)
{
  if (index < Staticindex)
  {
    if (_description[index].optionCnt < MAXOPTIONS)
    {
      _description[index].options[_description[index].optionCnt] = option;
      _description[index].labels[_description[index].optionCnt] = label;
      _description[index].optionCnt++;
    }
  }
}

// modify an option
void WebConfig::setOption(uint8_t index, uint8_t option_index, String option, String label)
{
  if (index < Staticindex)
  {
    if (option_index < _description[index].optionCnt)
    {
      _description[index].options[option_index] = option;
      _description[index].labels[option_index] = label;
    }
  }
}

void WebConfig::setOption(char *name, uint8_t option_index, String option, String label)
{
  int16_t i = getIndex(name);
  if (i >= 0)
    setOption(i, option_index, option, label);
}

// get the options count
uint8_t WebConfig::getOptionCount(uint8_t index)
{
  if (index < Staticindex)
  {
    return _description[index].optionCnt;
  }
  else
  {
    return 0;
  }
}

uint8_t WebConfig::getOptionCount(char *name)
{
  int16_t i = getIndex(name);
  if (i >= 0)
  {
    return getOptionCount(i);
  }
  else
  {
    return 0;
  }
}

// set form type to doen cancel
void WebConfig::setButtons(uint8_t buttons)
{
  _buttons = buttons;
}
// register onSave callback
void WebConfig::registerOnSave(std::function<void(String)> callback)
{
  _onSave = callback;
}
void WebConfig::registerOnSave(std::function<void(JsonObject)> callback)
{
  _onSaveJson = callback;
}
void WebConfig::registerOnSave(std::function<void()> callback)
{
  _onSave_null = callback;
}
// register onSave callback
void WebConfig::registerOnDone(void (*callback)(String results))
{
  _onDone = callback;
}
// register onSave callback
void WebConfig::registerOnCancel(void (*callback)())
{
  _onCancel = callback;
}
// register onDelete callback
void WebConfig::registerOnDelete(void (*callback)(String name))
{
  _onDelete = callback;
}
