#include <WebServer.h>
#include <ESPmDNS.h>
#include <WebConfig.h>

String params = "["
"{"
"'name':'ssid',"
"'label':'Name des WLAN',"
"'type':" OPTION_INPUTTEXT ","
"'default':''"
"},"
"{"
"'name':'pwd',"
"'label':'WLAN Passwort',"
"'type':" OPTION_INPUTPASSWORD ","
"'default':''"
"},"
"{"
"'name':'amount',"
"'label':'Menge',"
"'type':" OPTION_INPUTNUMBER ","
"'min':-10,'max':20,"
"'default':'1'"
"},"
"{"
"'name':'float',"
"'label':'Fließkomma Zahl',"
"'type':" OPTION_INPUTTEXT ","
"'default':'1.00'"
"},"
"{"
"'name':'area',"
"'label':'Mehr Text',"
"'type':" OPTION_INPUTTEXTAREA ","
"'default':'',"
"'min':40,'max':5"  //min = columns max = rows
"},"
"{"
"'name':'duration',"
"'label':'Dauer(s)',"
"'type':" OPTION_INPUTRANGE ","
"'min':5,'max':30,"
"'default':'10'"
"},"
"{"
"'name':'date',"
"'label':'Datum',"
"'type':" OPTION_INPUTDATE ","
"'default':'2019-08-14'"
"},"
"{"
"'name':'time',"
"'label':'Zeit',"
"'type':" OPTION_INPUTTIME ","
"'default':'18:30'"
"},"
"{"
"'name':'col',"
"'label':'Farbe',"
"'type':" OPTION_INPUTCOLOR ","
"'default':'#ffffff'"
"},"
"{"
"'name':'switch',"
"'label':'Schalter',"
"'type':" OPTION_INPUTCHECKBOX ","
"'default':'1'"
"},"
"{"
"'name':'gender',"
"'label':'Geschlecht',"
"'type':" OPTION_INPUTRADIO ","
"'options':["
"{'v':'m','l':'männlich'},"
"{'v':'w','l':'weiblich'},"
"{'v':'x','l':'anderes'}],"
"'default':'w'"
"},"
"{"
"'name':'continent',"
"'label':'Kontinent',"
"'type':" OPTION_INPUTSELECT ","
"'options':["
"{'v':'EU','l':'Europa'},"
"{'v':'AF','l':'Afrika'},"
"{'v':'AS','l':'Asien'},"
"{'v':'AU','l':'Australien'},"
"{'v':'AM','l':'Amerika'}],"
"'default':'AM'"
"},"
"{"
"'name':'wochentag',"
"'label':'Wochentag',"
"'type':" OPTION_INPUTMULTICHECK ","
"'options':["
"{'v':'0','l':'Sonntag'},"
"{'v':'1','l':'Montag'},"
"{'v':'2','l':'Dienstag'},"
"{'v':'3','l':'Mittwoch'},"
"{'v':'4','l':'Donnerstag'},"
"{'v':'5','l':'Freitag'},"
"{'v':'6','l':'Samstag'}],"
"'default':''"
"}"
"]";

WebServer server;
WebConfig conf;

boolean initWiFi() {
  boolean connected = false;
  WiFi.mode(WIFI_STA);
  Serial.print("Verbindung zu ");
  Serial.print(conf.values[0]);
  Serial.println(" herstellen");
  if (conf.values[0] != "") {
    WiFi.begin(conf.values[0].c_str(), conf.values[1].c_str());
    uint8_t cnt = 0;
    while ((WiFi.status() != WL_CONNECTED) && (cnt < 20)) {
      delay(500);
      Serial.print(".");
      cnt++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP-Adresse = ");
      Serial.println(WiFi.localIP());
      connected = true;
    }
  }
  if (!connected) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(conf.getDeviceName(), "", 1);
  }
  return connected;
}

void handleRoot() {
  conf.handleRoot();
  if (server.hasArg("SAVE")) {
    uint8_t cnt = conf.getCount();
    Serial.println("*********** Konfiguration ************");
    for (uint8_t i = 0; i < cnt; i++) {
      Serial.print(conf.getName(i));
      Serial.print(" = ");
      Serial.println(conf.values[i]);
    }
    if (conf.getBool("switch")) {
      Serial.printf("%s %s %i %5.2f \n", conf.getValue("ssid"), conf.getString("continent").c_str(), conf.getInt("amount"), conf.getFloat("float"));
    }
  }
}

void setup() {
  Serial.begin(74880);
  Serial.println(params);
  conf.setDescription(params, &server);
  conf.readConfig();
  initWiFi();
  char dns[30];
  sprintf(dns, "%s.local", conf.getDeviceName());
  if (MDNS.begin(dns)) {
    Serial.println("MDNS responder gestartet");
  }
  server.on("/", handleRoot);
  server.begin(80);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  //  MDNS.update();
}
