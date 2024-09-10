#include <DNSServer.h>
#include <SimpleVector.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <SD.h> 

#define TITLE     "Leader Board"

#define SUBTITLE "Beacon"
#define xDEBUG     false
#define SD_HNDL   2
#define LED 16

const char* DEFAULT_SSID      = "Beacon-Access-Point";
const char* DEFAULT_PASSWORD  = "123456789";
const char* DEFAULT_IPADDR    = "192.168.4.1";
const char* DEFAULT_MODE      = "AP";
const int   DEFAULT_PORT      = 80;
const char  FIELD_DELIM           = ',';
const char  HTTP_REQUEST_DELIM    = ' ';
const char  HTTP_PARAM_DELIM      = '&';
const char  HTTP_PARAM_NAME_DELIM = '=';
const char* FILEPATH_NETCFG       = "/net/net.cfg";
const char* FILEPATH_PLAYERS      = "/www/players.cfg";
const byte  HTTP_CODE = 200;

const int   FIELD_ID    = 0;
const int   FIELD_NAME  = 1;
const int   FIELD_SCORE = 2;
const int   FIELD_TEAM  = 3;


const long timeoutTime = 2000;


ESP8266WebServer webServer(DEFAULT_PORT);
DNSServer dnsServer;

IPAddress ipAddress (172, 0, 0, 1);
bool      sd_ok     = false;
bool      wifi_ok   = false;


unsigned long currentTime = millis();
unsigned long previousTime = 0; 

String header(String t) {
  String a = "";
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }" 
    "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
    "div { padding: 0.5em; }"
    "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
    "input { width: 30%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; border-radius: 10px; }"
    "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
    ".styled-table {text-align: center; margin-left: auto; margin-right: auto ; border-collapse: collapse; margin: 25px 0;font-size: 0.9em;font-family: sans-serif;min-width: 400px;box-shadow: 0 0 20px rgba(0, 0, 0, 0.15);}"
    ".styled-table thead tr {background-color: #0066ff;color: #ffffff;text-align: left;}"
    ".styled-table th,"
    ".styled-table td {;padding: 12px 15px;text-align: center;}"
    ".styled-table tbody tr {border-bottom: 1px solid #dddddd;}"
    ".styled-table tbody tr:nth-of-type(even) {background-color: #f3f3f3;}"
    ".styled-table tbody tr:last-of-type {border-bottom: 2px solid #0066ff;}"  
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
    ".styled-table tbody tr.active-row {font-weight: bold;color: #0066ff;}"
    ".buttonPlus {background-color: #04AA6D;border: none;color: white;padding: 10px 10px;text-align: center;text-decoration: none;display: inline-block;font-size: 24px;margin: 4px 2px;cursor: pointer;}"
    ".buttonMinus {background-color: #FF0000;border: none;color: white;padding: 10px 10px;text-align: center;text-decoration: none;display: inline-block;font-size: 24px;margin: 4px 2px;cursor: pointer;}"
    "nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
    "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
    "textarea { width: 100%; }";
    
  String h = "<!DOCTYPE html><html>"
    "<head><title>" + a + " :: " + t + "</title>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<style>" + CSS + "</style>"
    "<meta charset=\"UTF-8\"></head>"
    "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h; }

String signOnPage() {
  return header(TITLE) + "<div></ol></div><div><form action=/wifipwd method=post><label>WiFi password:</label>"+
    "<input type=password name=pwd></input><input type=submit value=Start></form></div>" + footer();
}
String getPassword(String f){
  File flpwd = SD.open(f); 
  if (!flpwd) {
#ifdef DEBUG
            Serial.print("default wifi pwd:");
            Serial.println(DEFAULT_PASSWORD);
            Serial.print("failed to handle file:");
            Serial.print(f);
#endif 
    return DEFAULT_PASSWORD;
  }
  String record;
  while (flpwd.available()) {
    char c = flpwd.read();
    if ( c != '\n' && c != '\r' && c != '\r\n') {
      record += c;
    }
  }
  flpwd.close();
  if (record.length() == 0 ) {  
#ifdef DEBUG
            Serial.print("default wifi pwd:");
            Serial.println(DEFAULT_PASSWORD);
            Serial.print("empty record in file:");
            Serial.print(f);
#endif 
    return DEFAULT_PASSWORD;
  }

#ifdef DEBUG
            Serial.print("wifi pwd:");
            Serial.println(record);
            Serial.print("used file:");
            Serial.print(f);
#endif 
  return record;
}

String leadboardPage() {
  return header(TITLE) + renderLeadBoardHtml("/www/players.txt") + footer();
}

bool split(SimpleVector<String> &MsgAry, String MsgStr, char delim) {
  int j=0;
  for(int i =0; i < MsgStr.length(); i++){
    if(MsgStr.charAt(i) == delim){
      MsgAry.put(MsgStr.substring(j,i));
      j = i+1;
    }
  }
  MsgAry.put(MsgStr.substring(j,MsgStr.length())); 
  return true;
}
bool isNumeric( String v) {
  bool digit = true;
  for (int i = 0 ; i<v.length() ; i++){
    digit = isDigit(v.charAt(i));
  }
  return digit;
}
String input(String name) {
  
  if ( !webServer.hasArg(name))
    return String("");
    
  String a = webServer.arg(name);
  a.replace("<","&lt;");
  a.replace(">","&gt;");
  a.substring(0,200);
  return a;
}

  
String verifiyWifiPwd() {
   String pwd = input("pwd");
   if (pwd == getPassword("/net/pwd.cfg" ))
    return leadboardPage();
   else
    return signOnPage();
  
}
String notfound() {
  String value = webServer.arg("value");  
  return header(TITLE) + "<div>Page Not Found</ol></div>" + footer();
}

String footer() { 
  return "</div><div class=q><a>&#169; All rights reserved.</a></div>";
}

bool outputFileContents( String fileName){
  
  Serial.print("output of ");
  Serial.print(fileName);
  Serial.println(" -- start of file -- ");
  File flTmpPlayerFile = SD.open(fileName); 
  if (!flTmpPlayerFile)
    return false;

  while (flTmpPlayerFile.available()) {
    char c = flTmpPlayerFile.read();
    Serial.print(c);
  }
  flTmpPlayerFile.close() ;
  Serial.print("output of ");
  Serial.print(fileName);
  Serial.println(" -- end of file -- ");
  return (true);
}

void updateScore(String f) {
  String targetId = input("id");
  String incrementValue = input("by");
  if ( targetId.equalsIgnoreCase("") || incrementValue.equalsIgnoreCase("") || !isNumeric(incrementValue)) {
    Serial.println("missing params id and/or by or by is not numeric");
    return;
  }
#ifdef DEBUG
            Serial.print("target Id:");
            Serial.print(targetId);
            Serial.print("' increment by:'");
            Serial.print(incrementValue);
            Serial.println("'");
            
#endif  
  
  File flPlayerFile = SD.open("/www/players.txt"); 
  File flTmpPlayerFile = SD.open("/www/players.tmp",FILE_WRITE); 
  if (flTmpPlayerFile && flTmpPlayerFile) {
    String record;
    int recordNum=0;
    int iFieldIdNum=-1;
    int iFieldScore=-1;
    int iFieldTeam=-1;
    int iFieldName=-1;
    
    
    while (flPlayerFile.available()) 
    {
       char c = flPlayerFile.read();
       if ( c == '\n' || c == '\r' || c == '\r\n') 
       {
          if (record.length() > 1)
          {
#ifdef DEBUG
            Serial.print("line:");
            Serial.print(recordNum);
            Serial.print("->'");
            Serial.print(record);
            Serial.println("'");
#endif 
            SimpleVector<String> svFields;
            split(svFields,record,FIELD_DELIM);
            SimpleVector<String>::SimpleVectorIterator iter = svFields.begin(); // Create an iterator for the vector
            int i=0;
            String currentId = "";
            String currentScore = "";
            String currentTeam = "";
            String currentName = "";
            while (iter.hasNext()) 
            { 
              String fieldValue = iter.next();
              if (recordNum == 0) {
                if ( fieldValue.equalsIgnoreCase("id") ) // determine the id field number.
                  iFieldIdNum = i;
                  
                if ( fieldValue.equalsIgnoreCase("score") ) // determine the id field number.
                  iFieldScore = i;
                  
                if ( fieldValue.equalsIgnoreCase("name") ) // determine the id field number.
                  iFieldName = i;
                  
                if ( fieldValue.equalsIgnoreCase("team") ) // determine the id field number.
                  iFieldTeam = i;
              } else {
                if ( i == iFieldIdNum ) 
                    currentId = fieldValue;
                  if ( i == iFieldScore )
                    currentScore = fieldValue; 
                  if ( i == iFieldTeam ) 
                     currentTeam = fieldValue;
                  if ( i == iFieldName ) 
                    currentName = fieldValue;
              }
              i++;
            }

            if ( currentId.equalsIgnoreCase(targetId) ) {
              if ( isNumeric(currentScore) ) {
                currentScore =  String(currentScore.toInt() + incrementValue.toInt());
                flTmpPlayerFile.println(currentId + "," + currentName + "," + currentScore + "," + currentTeam);
              }
            } else {
              flTmpPlayerFile.println(record); // write record to tmp file.
            }
          }// end if: record is greater than 1 char
          else
          {
#ifdef DEBUG
            Serial.print("line:");
            Serial.print(recordNum);
            Serial.println(" -> Blank Record!!'");
#endif  
          }
          record="";
          recordNum++;
        }// end if: c is eol char.
        else
        {
          record += c;
        }
    }// end while:flPlayerFile.avail

    flPlayerFile.close();
    flTmpPlayerFile.close();
  } //end if flPlayerFile/flPlayerFileTmp
  
#ifdef DEBUG
    outputFileContents("/www/players.tmp");
#endif
  SD.rename("/www/players.txt","/www/players.txt.old");
  SD.remove("/www/players.txt");
  SD.rename("/www/players.tmp","/www/players.txt");
  return;
  
}

String renderLeadBoardHtml( String f ) {
#ifdef DEBUG
  Serial.print("opening file:");
  Serial.println(f);
#endif    
  File flPlayer = SD.open(f);  ///www/players.txt
  String html = "<div>";
  if (flPlayer) 
  {
#ifdef DEBUG
    Serial.println("file has been opened");
#endif    
    html += "<table class='styled-table'>";
    String record = "";
    int recordNum = 0;
    int iFieldIdNum = -1;
    while (flPlayer.available()) 
    {
      char c = flPlayer.read();
      
      if ( c == '\n' || c == '\r' || c == '\r\n') 
      {
        
#ifdef DEBUG
        Serial.print("line:");
        Serial.print(recordNum);
        Serial.print(" ->'");
        Serial.print(record);
        Serial.println("'");
#endif      
        String idFieldValue = "";
        recordNum++;
        if (record.length() > 0) 
        { // we have at least 1 char, in the record.  otherwise, its just a blank line. suppress blank line.
         
          if (recordNum == 1) 
            html += "<thead>";
    
          html += "<tr>";  
          int i = 0;
          SimpleVector<String> svFields;
          split(svFields,record,FIELD_DELIM);
          SimpleVector<String>::SimpleVectorIterator iter = svFields.begin(); // Create an iterator for the vector
          
          while (iter.hasNext()) 
          { // Check if there are more elements in the vector
            String fieldValue = iter.next(); // Get the next element from the vector
#ifdef DEBUG
              Serial.print("line:");
              Serial.print(recordNum);
              Serial.print(" field:'");
              Serial.print(i);
              Serial.print("'");
              Serial.print(fieldValue);
              Serial.println("'");
#endif 
            if (recordNum == 1) 
            {// header row
              html += "<th>" + fieldValue + "</th>";  
              if ( fieldValue.equalsIgnoreCase("id") ) // determine the id field number.
                iFieldIdNum = i;
            } 
            else 
            { // data row
              
              html += "<td>" + fieldValue + "</td>";  
              if ( i == iFieldIdNum )
                idFieldValue = fieldValue;
              
            }// end if : recordNum
            i++;
          } // end while : hasnext

          if ( recordNum == 1 )
            html+="<th></th></thead></tr><tbody>";
          else 
          {
            html+= "<td><a href='/api/1.0/adjustscore?id="+ idFieldValue + "&by=1' class='buttonPlus'>+</a>";
            html+= "<a href='/api/1.0/adjustscore?id="+ idFieldValue + "&by=-1' class='buttonMinus'>-</a></td>";
            html+="</tr>";  
          }
          record="";
        } 
        else 
        {
#ifdef DEBUG
          Serial.print("line:");
          Serial.print(recordNum);
          Serial.println(" -> Blank Record!!'");
#endif  
         }// end if: rec.length
      } 
      else
      {
        record += c;
        //end if: c is eol char 
      }
    
      
    }// end while:Player.avail
    html+="</tbody></table>";
   } 
   else 
   {
    html = "failure to render player table. file:" + f;
   }// end if: player file.available.
    
   html += "</div>";
   return (html);
   
}//end func:renderLeadBoardHtml

void reportURI(String uri, String d){
#ifdef DEBUG
  Serial.print(d);
  Serial.print(":uri:");
  Serial.println(uri);
#endif
  return;
}

bool wifi_accesspoint_init(){
  
  Serial.print("Setting WIFI/AP (Access Point)…");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ipAddress, ipAddress, IPAddress(255, 255, 255, 0));
  WiFi.softAP(DEFAULT_SSID); // no pwd
  webServer.begin();
  
  Serial.println("Initialization: DNS"); 
  dnsServer.start(53, "*", ipAddress); // DNS spoofing (Only for HTTP)

  Serial.println("Initialization: Routes");
  
  webServer.on("/",[]()                   { reportURI(webServer.uri(),"0");webServer.send(HTTP_CODE, "text/html", signOnPage());  });
  webServer.on("/wifipwd",[]()            { reportURI(webServer.uri(),"1");webServer.send(HTTP_CODE, "text/html", verifiyWifiPwd());  });
  webServer.on("/api/1.0/adjustscore",[](){ reportURI(webServer.uri(),"2"); updateScore("");webServer.send(HTTP_CODE, "text/html",leadboardPage());  });
  webServer.on("/generate_204",[]()       { reportURI(webServer.uri(),"3");webServer.send(HTTP_CODE, "text/html", signOnPage());  });
  webServer.on("/hotspot-detect.html",[](){ reportURI(webServer.uri(),"3");webServer.send(HTTP_CODE, "text/html", signOnPage());  });
  webServer.onNotFound([]()               { reportURI(webServer.uri(),"4");webServer.send(HTTP_CODE, "text/html", signOnPage());     });

  webServer.begin();
  return true;
}

bool sd_init(){
  Serial.println("Initializing SD card");
  if (!SD.begin(SD_HNDL)) { //15  
    Serial.println("Initialization: SD failed!");  
    return false;
  }
  
  return true;
}

void led_init(){
   pinMode(LED, OUTPUT);
   digitalWrite(LED, LOW);
}


void setup() {
  Serial.begin(115200);
  led_init();
   
  wifi_ok = wifi_accesspoint_init();
  if ( wifi_ok)
    Serial.println("Initialization: WIFI & DNS completed"); 
  
  sd_ok = sd_init();
  if(sd_ok)
    Serial.println("Initialization: SD completed"); 

  Serial.println("Initialization: done."); 
}

void blink_led( int wait ) {
  digitalWrite(LED, LOW);
  delay(wait/2);
  digitalWrite(LED, HIGH);
  delay(wait/2);
}


void on_led( ) {
  digitalWrite(LED, HIGH);
}

void off_led( ) {
  digitalWrite(LED, LOW);
}



void loop(){
 
  
  if ( !sd_ok || !wifi_ok ) {
    Serial.println("Initialization Failed..."); 
    blink_led(1000);
    blink_led(1000);
    blink_led(800);
    blink_led(800);
    return;
  }
  on_led();
  dnsServer.processNextRequest();
  webServer.handleClient();
  
  

  
  
}
