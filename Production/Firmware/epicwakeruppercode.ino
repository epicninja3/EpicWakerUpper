#include <Adafruit_GFX.h> // graphics library
#include <Adafruit_ST7789.h> // driver for the ST7789 screen
#include <SPI.h> 
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Key.h>
#include <Keypad.h>
#include <array>
#include "time.h"

//#include <ArduinoJson.h>
// #include <ArduinoJson.hpp>

// Defining pins for the display, change according to your setup!!! Uses the white numbers on the ESP
#define TFT_SCLK 11 // labeled SCL on the screen
#define TFT_MOSI 10 // labeled SDA on the screen
#define TFT_RST 9
#define TFT_DC 8
#define TFT_CS 6
// #define TFT_BL 5

#define BUZZER 7
// #define SW1 
// #define SW2
// #define SW3
// #define SW4
// #define SW5
// #define SW6

const byte rows = 2; //byte is basically just int but it saves memory and is required for the Keypad library
const byte cols = 3; 

char keyMatrix[rows][cols]{
  {'4', '5', '6'},
  {'1', '2', '3'} //This is just the numbers (not pin #s) for each switch
};

byte rowPins[rows] = {5, 4};
byte colPins[cols] = {1, 2, 3};
Keypad wakerKeypad = Keypad(makeKeymap(keyMatrix), rowPins, colPins, rows, cols);

String mode = "main_screen";

String ssid = "wifi_ssid"; //wifi ssid add later
String password = "wifi_password"; //wifi password
const char* ntpServer = "pool.ntp.org"; //Ntp servers are a dedicated system that distributes exact time over network.

const long  gmtOffset_sec = -21600;     // This is the seconds offset from UTC, so this is CST
const int   daylightOffset_sec = 3600;  // This is for daylight savings, 3600 = 1hour, use 0 if not daylight savings. 
unsigned long lastWeatherCheck = 0; //this is the last time the weather was updated.
const unsigned long weatherInterval = 600000; // 10 minutes
String weatherUrl =  "https://api.open-meteo.com/v1/forecast?latitude=30.5083&longitude=-97.6789&daily=sunrise,sunset,temperature_2m_max,temperature_2m_min,moon_phase,rain_sum&hourly=temperature_2m,rain,weather_code,precipitation_probability&current=is_day,temperature_2m,rain,precipitation,snowfall,weather_code,wind_speed_10m&timezone=America%2FChicago"; // Add full link later with all the other API configs. 
double temperature = 0.0;
String temp_unit = "°F";
double temp_min = 0.0;
double temp_max = 0.0;
String sunset = "0:00";
String sunrise = "0:00";
double precip_prob = 0.0;
String moon_phase = "Full Moon";

//Arrays to store names for the weeks and months
const std::array<String, 7> daysOfWeek = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
const std::array<String, 12> months = {
  "January", "February", "March", "April", "May", "June", 
  "July", "August", "September", "October", "November", "December"
};


int alarmHour = 0;
int alarmMin = 0;
bool alarmTriggered = false;
bool alarmEnabled = false;

int timerHour = 0;
int timerMin = 0;
int timerSec = 0;
bool timerStarted = false;
bool timerPaused = false;
bool timerEnded = false;



// Fix setColRowStart() by exposing it via a subclass
class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);


// Weather stuff from Open Meteo
void getWeather(){
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;


    //request from API Url
    http.begin(weatherUrl); 
    int httpResponseCode = http.GET();

    String payload = http.getString();
    http.end();
    //Json parsing stuff 
    JSONVar myObject = JSON.parse(payload);

    //in case the JSON parsing doesn't work
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("JSON parsing input failed");
      return;
    }


    //Temperature
    temperature = (double)myObject["current"]["temperature_2m"];
    temp_unit = (String)myObject["current_units"]["temperature_units"];
    temp_min = (int)myObject["daily"]["temperature_2m_min"][0];
    temp_max = (int)myObject["daily"]["temperature_2m_max"][0];

    sunset = (String)myObject["daily"]["sunset"][0];
    sunrise = (String)myObject["daily"]["sunrise"][0];

    precip_prob = (double)myObject["hourly"]["precipitation_probability"];

    moon_phase = (String)myObject["daily"]["moon_phase"][0];


    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(temp_unit);
    // tft.print("☀️ ");
    
  } else {
    Serial.print("Wifi not connected");
    return;
  }
}



void keyChecker(){
  char keyClicked = wakerKeypad.getKey(); //.getKey will give you the number of the key that was clicked
  if (keyClicked){
    Serial.print("Key clicked");
    if (mode == "main_screen"){
      switch(keyClicked){
        case '1':
          mode = "weather_screen";
          //fullWeather(); //Show the full weather screen
          break;
        case '2':
          mode = "alarm_screen";
          break;
        case '3':
          mode = "stock_screen";
          break;
        case '4':
          mode = "timer_screen";
          break;
        case '5':
          break;
        case '6':
          break;
      }
    } else if (mode == "weather_screen") {
      switch(keyClicked){
        case '1':
          mode = "main_screen";
          break;
        case '2':
          break;
        case '3':
          break;
        case '4':
          break;
        case '5':
          break;
        case '6':
          break;
      }
    } else if(mode == "alarm_screen"){
      switch(keyClicked){
        case '1':
          mode = "main_screen";
          break;
        case '2':
          alarmHour = (alarmHour + 1) % 24;
          break;
        case '3':
          alarmMin = (alarmMin + 1) % 60;
          break;
        case '4':
          alarmEnabled = true;
          Serial.println("Alarm enabled");
          break;
        case '5':
          break;
        case '6':
          break;
      }
    } else if(mode == "alarm_ringing"){
      switch(keyClicked){
        case '1':
          tft.fillScreen(ST77XX_BLACK);
          mode = "main_screen";
          //Stop buzzer
          break;
        case '2':
          
          break;
        case '3':
          
          break;
        case '4':
          break;
        case '5':
          break;
        case '6':
          break;
      }
    } else if(mode == "timer_screen"){
      switch(keyClicked){
        case '1':
          mode = "main_screen";
          break;
        case '2':
          timerHour = (timerHour + 1) % 24;
          break;
        case '3':
          timerMin = (timerMin + 1) % 60;
          break;
        case '4':
          timerSec = (timerSec + 1) % 60;
          break;
        case '5':
          timerStarted = true;
          break;
        case '6':
          break;
      }
    } else if(mode == "timer_running"){
      switch(keyClicked){
        case '1':
          mode = "timer_paused";
          break;
        case '2':
          break;
        case '3':
          break;
        case '4':
          break;
        case '5':
          break;
        case '6':
          break;
      }
    }
  }
}

void mainScreen(){
  mode = "main_screen";
  struct tm timeinfo; // This is a built-in data structure in C++ and chunks data out into simple variables you can pull.
  
  
  if(getLocalTime(&timeinfo)){
    int currentHour = timeinfo.tm_hour;
    int currentMin = timeinfo.tm_min;
    int currentYear = timeinfo.tm_year + 1900;
    

    

    String monthName = months[timeinfo.tm_mon];
    String dayName = daysOfWeek[timeinfo.tm_wday];
    int monthDay = timeinfo.tm_mday;

    char timeStringBuff[9];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    
    //clear old time text
    tft.fillRect(38, 142, 200, 50, ST77XX_BLACK); 
    //clear old date text
    tft.fillRect(0, 50, 284, 40, ST77XX_BLACK); 

    
    // print out the new time
    tft.setCursor(38, 142);
    tft.print(timeStringBuff);

    //print date stuff
    tft.setCursor(10, 50);
    tft.print(dayName);
    tft.print(", ");
    tft.print(monthName);
    tft.print(monthDay);

    // if (currentHour == 6 && currentMin == 15){
    //   tft.setTextSize(3);
    //   tft.setCursor(5,270);
    //   tft.print("Wake up!");
    // }
  }

  tft.fillRect(0, 0, 20, 80, ST77XX_BLACK); 
  tft.setCursor(1,1);
  tft.print(temperature, 1);
  tft.print(temp_unit);
  delay(1000); //update every second

  
  //Reminder: unsigned long is a data structure that stores large numbers (long) and only stores positive numbers (unsigned).
  for (int i = 0; i < 10; i++){
    unsigned long currentMillis = millis(); // get current uptime
    if (currentMillis - lastWeatherCheck >= weatherInterval) {
    //this is saving the last time weather was checked using millis
    lastWeatherCheck = currentMillis; 
    
    getWeather(); 
    }
  }  
  delay(10);
}

void fullWeather(){
  mode = "weather_screen";
  tft.fillScreen(ST77XX_BLACK);
  getWeather();

  tft.setCursor(38, 142);
  tft.setTextSize(4);
  tft.print(temperature, 1);
  tft.print(temp_unit);

  tft.setCursor(2,2);
  tft.setTextSize(1);
  tft.print(String("Sunrise: " + sunrise));
  tft.print(String("Sunset: " + sunset));

  tft.setCursor(25,2);
  tft.setTextSize(1);
  tft.println(String("High: " + String(temp_max)));
  tft.println(String("Low: " + String(temp_min)));

  tft.setCursor(5, 142);
  tft.setTextSize(1);
  tft.print(String("Chance of Rain: " + String(precip_prob)));

}


void alarmScreen(){
  mode = "alarm_screen";
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(0,0);
  tft.setTextSize(1);
  tft.println("Use Button 2 for the hours");
  tft.println("Use Button 3 for the minutes");
  
  tft.setTextSize(2);
  tft.setCursor(30,142);
  tft.print("Next Alarm:");

  tft.print(String(String(alarmHour) + ":" + String(alarmMin)));
}


void alarmChecker() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 10)) {
    return;
  }

  //reset the alarmtrigger
  if (timeinfo.tm_hour != alarmHour || timeinfo.tm_min != alarmMin) {
    alarmTriggered = false;
  }

  
  if (alarmEnabled && timeinfo.tm_hour == alarmHour && timeinfo.tm_min == alarmMin && !alarmTriggered) {

    alarmTriggered = true;
    mode = "alarm_ringing";

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(30, 120);
    tft.print("Alarm ringing!");

    Serial.println("Alarm ringing!");
  }
}

void timerScreen(){
  mode = "timer_screen";
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(0,0);
  tft.setTextSize(1);
  tft.println("Use Button 2 for the hours");
  tft.println("Use Button 3 for the minutes");
  
  tft.setTextSize(2);
  tft.setCursor(30,142);
  

  tft.print(String(String(timerHour) + ":" + String(timerMin) + ":" + String(timerSec)));

}


void timerRun() {
  
  tft.fillScreen(ST77XX_BLACK);
  if (mode == "timer_pause"){
    timerPaused = !timerPaused;

    tft.fillRect(0, 0, 284, 40, ST77XX_BLACK);
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);

    if (timerPaused) {
      tft.print("Paused");
    } else {
      tft.print("Timer running");
    }
    

  }
  while (timerStarted) {
    mode = "timer_running";
    delay(1000);

    if (timerSec > 0) {
      timerSec--;
    }
    else if (timerMin > 0) {
      timerMin--;
      timerSec = 59;
    }
    else if (timerHour > 0) {
      timerHour--;
      timerMin = 59;
      timerSec = 59;
    }
    else {
      tft.setCursor(30, 120);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_WHITE);
      tft.print("Timer done");
      tone(BUZZER, 1000);
      delay(500);
      noTone(BUZZER); 

      timerStarted = false;
      timerEnded = true;
      break;
    }

    
    tft.fillRect(0, 100, 284, 50, ST77XX_BLACK);
    tft.setCursor(30, 120);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);

    char timerText[9];
    snprintf(timerText, sizeof(timerText), "%02d:%02d:%02d", timerHour, timerMin, timerSec);
    tft.print(timerText);
  }
}




// setup() runs ONCE when the board powers on
void setup() {
  
  Serial.begin(115200); // lets the board talk to your computer

  pinMode(BUZZER, OUTPUT);
  // pinMode(TFT_BL, OUTPUT); // Set the backlight pin mode, or just wire it to 3.3V
  //digitalWrite(TFT_BL, LOW); // Turns the backlight ON, for some reason this screen is active Low, so setting it to LOW is really HIGH

  tft.init(76, 284); // Our panel size (portrait)
  tft.setOffsets(82, 18); // Offsets for the weird resolution
  tft.invertDisplay(false); // Invert the colors (This display is flipped from normal)
  tft.setRotation(1); // Landscape, if it's upside down use 3!
  Serial.println("TFT Initialized!");

  tft.fillScreen(ST77XX_BLACK); // clear the screen

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(6);
  tft.setCursor(0,0); // Where the text is drawn, 0,0 is top left
  tft.print("Hello World!"); // Show whatever you want! Draws from the top left of the text/number/shape 
  Serial.begin(115200);

  //Wifi connecting stuff
  WiFi.begin(ssid, password);
  Serial.print("connecting to wifi");
  while (WiFi.status() != WL_CONNECTED){ // displays dots until the wifi is connected
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to Wifi");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //this gets the time, keep in the setup function

  tft.fillScreen(ST77XX_BLACK);
  

  
}


// loop() runs OVER and OVER, forever
void loop() {
  keyChecker();
  alarmChecker();
  

  if (mode == "main_screen"){
    mainScreen();
  }
  else if(mode == "weather_screen"){
    fullWeather();
  }
  else if (mode == "alarm_screen"){
    alarmScreen();
  }
  else if (mode == "alarm_ringing"){
    tone(BUZZER, 1000);
    delay(500);
    noTone(BUZZER); 
  }
  else if(mode == "timer_screen"){
    timerScreen();
  }
  else if (mode == "timer_started" || mode == "timer_pause"){
    timerRun();
  }
}



// ------- IGNORE THIS STUFF -------

// const char* alpacaKeyId     = "YOUR_ALPACA_KEY_ID"; 
// const char* alpacaSecretKey = "YOUR_ALPACA_SECRET_KEY";
// const char* targetSymbols   = "AAPL,MSFT,NVDA,TSLA,AMD"; 


// void fetchAlpacaSnapshots() {
//   if (WiFi.status() != WL_CONNECTED){
//     return;
//   }

//   NetworkClientSecure client;
//   client.setInsecure(); 

//   HTTPClient http;
  
//   String url = "https://paper-api.alpaca.markets/v2";
//   url += targetSymbols;

//   http.begin(client, url);
  
  
//   http.addHeader("APCA-API-KEY-ID", alpacaKeyId);
//   http.addHeader("APCA-API-SECRET-KEY", alpacaSecretKey);
//   http.addHeader("accept", "application/json");

//   int httpResponseCode = http.GET();

//   if (httpResponseCode == HTTP_CODE_OK) {
//     String payload = http.getString();
    
//     JsonDocument doc;
//     DeserializationError error = deserializeJson(doc, payload);

//     if (!error) {
      

//      //loops through each stock symbol and pulls the data for that stock
//       String symList = String(targetSymbols);
//       int lastIndex = 0;
      
//       for (int i = 0; i <= symList.length(); i++) {
//         if (symList.charAt(i) == ',' || i == symList.length()) {
//           String ticker = symList.substring(lastIndex, i);
//           lastIndex = i + 1;

//           // Access the snapshot for this specific stock ticker
//           JsonObject snapshot = doc[ticker];
          
//           if (!snapshot.isNull()) {
//             // "dailyBar" is todays stock metrics
//             float currentPrice = snapshot["dailyBar"]["c"]; //Closing price
//             float openPrice    = snapshot["dailyBar"]["o"]; //Opening price

//             float increasePercent = 0.0;
//             if (openPrice > 0) {
//               increasePercent = ((currentPrice - openPrice) / openPrice) * 100.0;
//             }

//             // Output data to the Serial Monitor
//             Serial.printf("Stock: %s | Price: $%0.2f | Today's Change: %+.2f%%\n", ticker.c_str(), currentPrice, increasePercent);
            
//             // ----------------------------------------------------
//             // TODO: DISPATCH TO YOUR EXTERNAL SCREEN DRIVER HERE
//             // ----------------------------------------------------
//           }
//         }
//       }
//       Serial.println("-----------------------------------------\n");
      
//     } else {
//       Serial.print("JSON Parsing failed: ");
//       Serial.println(error.c_str());
//     }
//   } else {
//     Serial.printf("Alpaca HTTP Error: %d\n", httpResponseCode);
//     if(httpResponseCode == 401 || httpResponseCode == 403) {
//       Serial.println("Check if the Key and Secret are correct");
//     }
//   }
//   http.end();
// }





