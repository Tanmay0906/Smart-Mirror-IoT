#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <time.h>

// ========== LCD CONFIGURATION ==========
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== WIFI CONFIGURATION ==========
const char* ssid = "Manash MJ";
const char* password = "12345678";

// ========== TIME CONFIGURATION ==========
const int timezone = 19800;  // IST offset
const char* ntpServer = "pool.ntp.org";

// Current time variables
int currentHour = 12;
int currentMinute = 0;
int currentSecond = 0;
int currentDay = 1;
int currentMonth = 1;
int currentYear = 2024;
String dayOfWeek = "Mon";

// Formatted strings
String formattedTime = "12:00";
String formattedDate = "01/01/24";
String formattedFullDate = "Mon, 01/01/24";
bool timeSynced = false;

// ========== DHT11 CONFIGURATION ==========
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ========== WEB SERVER ==========
ESP8266WebServer server(80);

// ========== GLOBAL VARIABLES ==========
float temperature = 25.0;
float humidity = 50.0;
String comfortStatus = "Comfortable";

// NEW: Weather Forecast Variables
float forecast3hr = 22.0;
float forecast6hr = 24.0;
float forecast9hr = 26.0;
String weatherMessage = "Good weather for outdoor activities";
String forecastStatus[3] = {"Sunny", "Partly Cloudy", "Clear"};
float forecastHumidity[3] = {65, 68, 70};

// NEW: Productivity Analysis
String productivityLevel = "High";
String productivityTip = "Focus on important tasks now";
String recommendedActivity = "Work/Creative Tasks";
int productivityScore = 85; // 0-100

// NEW: Comfort Analysis
String comfortMessage = "Ideal indoor conditions";
String healthTip = "Stay hydrated";
int comfortIndex = 90; // 0-100

// NEW: Data logging for CSV
String dataLog = "Timestamp,Temperature,Humidity,ComfortStatus,ComfortIndex,ProductivityScore\n";

// Display modes
enum DisplayMode {
  MODE_PROJECT_INFO,
  MODE_TIME_SENSORS,
  MODE_DETAILED_DATA,
  MODE_WEATHER_FORECAST,
  MODE_PRODUCTIVITY_COMFORT,
  MODE_CREDITS
};

DisplayMode currentMode = MODE_PROJECT_INFO;
unsigned long lastModeSwitch = 0;
const unsigned long MODE_DURATION = 4000; // 4 seconds per mode (reduced for 6 modes)

// Day names
const char* DAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Days in month
const int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// ========== HELPER FUNCTIONS ==========
int getMin(int a, int b) {
  return (a < b) ? a : b;
}

// ========== SETUP FUNCTION ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== SMART MIRROR SYSTEM ===");
  Serial.println("Project Stage 1");
  Serial.println("Developed by: Manash & Team");
  Serial.println("Initializing...");
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Mirror");
  lcd.setCursor(0, 1);
  lcd.print("Project Stage 1");
  delay(2000);
  
  // Show credits
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Developed by:");
  lcd.setCursor(0, 1);
  lcd.print("Manash & Team");
  delay(2000);
  
  // Initialize DHT
  dht.begin();
  Serial.println("DHT11 initialized");
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure time
  configTime(timezone, 0, ntpServer);
  
  // Try to sync time automatically
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
  }
  
  // Initialize forecasts (in real project, this would come from API)
  generateWeatherForecast();
  
  // Setup web server
  setupWebServer();
  
  // Setup OTA
  ArduinoOTA.setHostname("smart-mirror");
  ArduinoOTA.begin();
  
  Serial.println("\n=== SYSTEM READY ===");
  
  // Show ready message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
}

// ========== CONNECT WIFI ==========
void connectWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
    lcd.setCursor(0, 1);
    lcd.print("Manual Mode");
  }
  delay(1000);
}

// ========== SYNC TIME WITH NTP ==========
void syncTime() {
  Serial.println("Syncing time with NTP...");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Syncing Time");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...");
  
  struct tm timeinfo;
  
  int retry = 0;
  bool gotTime = false;
  
  while (retry < 10 && !gotTime) {
    if (getLocalTime(&timeinfo, 5000)) {
      gotTime = true;
      timeSynced = true;
      updateTimeFromSystem(&timeinfo);
      
      Serial.println("Time synced successfully!");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time Synced!");
      lcd.setCursor(0, 1);
      lcd.print(formattedTime);
    } else {
      Serial.print(".");
      retry++;
      delay(1000);
    }
  }
  
  if (!gotTime) {
    timeSynced = false;
    Serial.println("\nTime sync failed!");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auto Sync Failed");
    lcd.setCursor(0, 1);
    lcd.print("Manual Time");
  }
  
  delay(1000);
}

// ========== UPDATE TIME FROM SYSTEM ==========
void updateTimeFromSystem(struct tm* timeinfo) {
  if (timeinfo == NULL) return;
  
  currentHour = timeinfo->tm_hour;
  currentMinute = timeinfo->tm_min;
  currentSecond = timeinfo->tm_sec;
  currentDay = timeinfo->tm_mday;
  currentMonth = timeinfo->tm_mon + 1;
  currentYear = timeinfo->tm_year + 1900;
  
  int dayIndex = timeinfo->tm_wday;
  dayOfWeek = DAY_NAMES[dayIndex];
  
  formatTime();
}

// ========== FORMAT TIME AND DATE ==========
void formatTime() {
  // Format time as HH:MM
  char timeBuf[6];
  sprintf(timeBuf, "%02d:%02d", currentHour, currentMinute);
  formattedTime = String(timeBuf);
  
  // Format date as DD/MM/YY
  char dateBuf[9];
  sprintf(dateBuf, "%02d/%02d/%02d", currentDay, currentMonth, currentYear % 100);
  formattedDate = String(dateBuf);
  
  // Format full date as Day, DD/MM/YY
  formattedFullDate = dayOfWeek + ", " + formattedDate;
}

// ========== UPDATE MANUAL TIME ==========
void updateManualTime() {
  static unsigned long lastMillis = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastMillis >= 1000) {
    lastMillis = currentMillis;
    
    currentSecond++;
    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;
      
      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour++;
        
        if (currentHour >= 24) {
          currentHour = 0;
          currentDay++;
          
          int daysInMonth = DAYS_IN_MONTH[currentMonth - 1];
          
          if (currentMonth == 2 && isLeapYear(currentYear)) {
            daysInMonth = 29;
          }
          
          if (currentDay > daysInMonth) {
            currentDay = 1;
            currentMonth++;
            
            if (currentMonth > 12) {
              currentMonth = 1;
              currentYear++;
            }
          }
          
          updateDayOfWeek();
        }
      }
    }
    
    formatTime();
  }
}

// ========== CHECK LEAP YEAR ==========
bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// ========== UPDATE DAY OF WEEK ==========
void updateDayOfWeek() {
  static int dayIndex = 1;
  dayIndex = (dayIndex + 1) % 7;
  dayOfWeek = DAY_NAMES[dayIndex];
}

// ========== READ DHT11 SENSOR ==========
void readDHT11() {
  static unsigned long lastRead = 0;
  
  if (millis() - lastRead < 2000) return;
  lastRead = millis();
  
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (!isnan(temp) && !isnan(hum)) {
    temperature = temp;
    humidity = hum;
    
    // Analyze comfort and productivity
    analyzeComfortConditions(temp, hum);
    analyzeProductivity(temp, hum, currentHour);
    
    // Log data for CSV (every 2 readings ~4 seconds)
    static int logCounter = 0;
    logCounter++;
    if (logCounter >= 2) {
      logData();
      logCounter = 0;
    }
  }
}

// ========== LOG DATA FOR CSV ==========
void logData() {
  String timestamp = formattedDate + " " + formattedTime;
  String logEntry = timestamp + ",";
  logEntry += String(temperature, 1) + ",";
  logEntry += String(humidity, 0) + ",";
  logEntry += comfortStatus + ",";
  logEntry += String(comfortIndex) + ",";
  logEntry += String(productivityScore) + "\n";
  
  // Keep only last 1000 entries to save memory
  int lineCount = 0;
  for (unsigned int i = 0; i < dataLog.length(); i++) {
    if (dataLog.charAt(i) == '\n') lineCount++;
  }
  
  if (lineCount > 1000) {
    // Remove oldest entry
    int firstNewline = dataLog.indexOf('\n');
    if (firstNewline != -1) {
      dataLog = dataLog.substring(firstNewline + 1);
    }
  }
  
  dataLog += logEntry;
  Serial.println("Data logged: " + timestamp);
}

// ========== ANALYZE COMFORT CONDITIONS ==========
void analyzeComfortConditions(float temp, float hum) {
  // Determine comfort status
  if (temp >= 22 && temp <= 25 && hum >= 45 && hum <= 55) {
    comfortStatus = "Perfect";
    comfortMessage = "Ideal indoor conditions";
    healthTip = "Perfect for any activity";
    comfortIndex = 95;
  } else if (temp >= 20 && temp <= 27 && hum >= 40 && hum <= 60) {
    comfortStatus = "Comfortable";
    comfortMessage = "Good indoor conditions";
    healthTip = "Stay hydrated";
    comfortIndex = 85;
  } else if (temp > 27 && hum > 60) {
    comfortStatus = "Hot&Humid";
    comfortMessage = "Uncomfortable conditions";
    healthTip = "Use fan/AC, drink water";
    comfortIndex = 40;
  } else if (temp > 27) {
    comfortStatus = "Hot";
    comfortMessage = "Too warm indoors";
    healthTip = "Stay cool, use ventilation";
    comfortIndex = 50;
  } else if (temp < 20 && hum < 40) {
    comfortStatus = "Cold&Dry";
    comfortMessage = "Dry and chilly";
    healthTip = "Use humidifier, wear layers";
    comfortIndex = 45;
  } else if (temp < 20) {
    comfortStatus = "Cold";
    comfortMessage = "Chilly conditions";
    healthTip = "Warm clothing needed";
    comfortIndex = 60;
  } else if (hum > 60) {
    comfortStatus = "Humid";
    comfortMessage = "High humidity";
    healthTip = "Use dehumidifier";
    comfortIndex = 65;
  } else if (hum < 40) {
    comfortStatus = "Dry";
    comfortMessage = "Low humidity";
    healthTip = "Use humidifier";
    comfortIndex = 70;
  } else {
    comfortStatus = "Normal";
    comfortMessage = "Average conditions";
    healthTip = "Maintain current routine";
    comfortIndex = 80;
  }
}

// ========== ANALYZE PRODUCTIVITY ==========
void analyzeProductivity(float temp, float hum, int hour) {
  // Calculate productivity score based on time and conditions
  int timeScore = 0;
  String timeBasedActivity = "";
  
  // Time of day analysis
  if (hour >= 6 && hour <= 10) {
    timeScore = 90; // Morning - high productivity
    timeBasedActivity = "Best for focused work";
  } else if (hour >= 11 && hour <= 15) {
    timeScore = 75; // Afternoon - moderate
    timeBasedActivity = "Good for meetings";
  } else if (hour >= 16 && hour <= 19) {
    timeScore = 60; // Evening - lower
    timeBasedActivity = "Light tasks/planning";
  } else {
    timeScore = 40; // Night - low
    timeBasedActivity = "Rest/relaxation";
  }
  
  // Temperature adjustment
  int tempScore = 0;
  if (temp >= 20 && temp <= 23) tempScore = 90;
  else if (temp >= 18 && temp <= 25) tempScore = 75;
  else if (temp >= 16 && temp <= 27) tempScore = 60;
  else tempScore = 40;
  
  // Final productivity score
  productivityScore = (timeScore * 0.6 + tempScore * 0.4);
  
  // Determine productivity level and tip
  if (productivityScore >= 80) {
    productivityLevel = "High";
    productivityTip = "Focus on important tasks now";
    recommendedActivity = "Work/Creative Tasks";
  } else if (productivityScore >= 60) {
    productivityLevel = "Moderate";
    productivityTip = "Good for routine work";
    recommendedActivity = "Meetings/Planning";
  } else {
    productivityLevel = "Low";
    productivityTip = "Take breaks, light tasks";
    recommendedActivity = "Rest/Review Tasks";
  }
}

// ========== GENERATE WEATHER FORECAST ==========
void generateWeatherForecast() {
  // Simulated weather forecast (in real project, use API)
  randomSeed(millis());
  forecast3hr = temperature + random(-2, 4);
  forecast6hr = temperature + random(-1, 5);
  forecast9hr = temperature + random(0, 6);
  
  // Simulate weather conditions
  String conditions[] = {"Sunny", "Cloudy", "Rainy", "Clear"};
  forecastStatus[0] = conditions[random(0, 4)];
  forecastStatus[1] = conditions[random(0, 4)];
  forecastStatus[2] = conditions[random(0, 4)];
  
  forecastHumidity[0] = humidity + random(-5, 6);
  forecastHumidity[1] = humidity + random(-8, 9);
  forecastHumidity[2] = humidity + random(-10, 11);
  
  // Generate weather message
  generateWeatherMessage();
}

// ========== GENERATE WEATHER MESSAGE ==========
void generateWeatherMessage() {
  // Analyze if it's good to go out
  float avgTemp = (forecast3hr + forecast6hr + forecast9hr) / 3;
  float avgHum = (forecastHumidity[0] + forecastHumidity[1] + forecastHumidity[2]) / 3;
  
  if (avgTemp >= 18 && avgTemp <= 28 && avgHum <= 70) {
    if (forecastStatus[0] == "Sunny" || forecastStatus[0] == "Clear") {
      weatherMessage = "Great for outdoor activities!";
    } else if (forecastStatus[0] == "Cloudy") {
      weatherMessage = "Good for going out";
    } else {
      weatherMessage = "Carry umbrella if going out";
    }
  } else if (avgTemp > 30) {
    weatherMessage = "Too hot, avoid afternoon out";
  } else if (avgTemp < 15) {
    weatherMessage = "Chilly, wear warm clothes";
  } else if (avgHum > 80) {
    weatherMessage = "Very humid, stay indoors";
  } else if (forecastStatus[0] == "Rainy") {
    weatherMessage = "Rain expected, avoid going out";
  } else {
    weatherMessage = "Average conditions outside";
  }
}

// ========== DISPLAY ON LCD ==========
void displayOnLCD() {
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate < 1000) return;
  lastUpdate = millis();
  
  // Cycle through display modes (6 modes total)
  if (millis() - lastModeSwitch > MODE_DURATION) {
    currentMode = (DisplayMode)((currentMode + 1) % 6);
    lastModeSwitch = millis();
    lcd.clear();
  }
  
  switch(currentMode) {
    case MODE_PROJECT_INFO:
      displayProjectInfo();
      break;
    case MODE_TIME_SENSORS:
      displayTimeSensors();
      break;
    case MODE_DETAILED_DATA:
      displayDetailedData();
      break;
    case MODE_WEATHER_FORECAST:
      displayWeatherForecast();
      break;
    case MODE_PRODUCTIVITY_COMFORT:
      displayProductivityComfort();
      break;
    case MODE_CREDITS:
      displayCredits();
      break;
  }
}

// ========== DISPLAY PROJECT INFO ==========
void displayProjectInfo() {
  lcd.setCursor(0, 0);
  lcd.print("Smart Mirror");
  lcd.setCursor(0, 1);
  lcd.print("Project Stage 1");
}

// ========== DISPLAY TIME AND SENSORS ==========
void displayTimeSensors() {
  // Line 1: Time and date
  lcd.setCursor(0, 0);
  String line1 = formattedTime + " " + dayOfWeek;
  if (line1.length() < 16) {
    line1 += " " + formattedDate;
  }
  if (line1.length() > 16) {
    line1 = line1.substring(0, 16);
  }
  lcd.print(line1);
  
  // Line 2: Temperature and humidity
  lcd.setCursor(0, 1);
  String tempStr = String(temperature, 1);
  String humStr = String(humidity, 0);
  String line2 = tempStr + "C " + humStr + "%";
  
  if (line2.length() > 16) {
    line2 = line2.substring(0, 16);
  }
  lcd.print(line2);
}

// ========== DISPLAY DETAILED DATA ==========
void displayDetailedData() {
  // Line 1: Temperature and Comfort
  lcd.setCursor(0, 0);
  String line1 = "Temp: " + String(temperature, 1) + "C";
  lcd.print(line1);
  
  // Line 2: Humidity and Status
  lcd.setCursor(0, 1);
  String line2 = "Hum: " + String(humidity, 0) + "% " + comfortStatus.substring(0, 4);
  if (line2.length() > 16) {
    line2 = line2.substring(0, 16);
  }
  lcd.print(line2);
}

// ========== DISPLAY WEATHER FORECAST ==========
void displayWeatherForecast() {
  // Line 1: Weather forecast title
  lcd.setCursor(0, 0);
  lcd.print("Weather Forecast");
  
  // Line 2: 3-hour forecast
  lcd.setCursor(0, 1);
  String forecast = "3h:" + String(forecast3hr, 1) + "C ";
  forecast += "6h:" + String(forecast6hr, 1) + "C";
  if (forecast.length() > 16) {
    forecast = forecast.substring(0, 16);
  }
  lcd.print(forecast);
}

// ========== DISPLAY PRODUCTIVITY AND COMFORT ==========
void displayProductivityComfort() {
  // Line 1: Productivity score
  lcd.setCursor(0, 0);
  String line1 = "Prod Score: " + String(productivityScore);
  if (line1.length() > 16) {
    line1 = line1.substring(0, 16);
  }
  lcd.print(line1);
  
  // Line 2: Comfort index
  lcd.setCursor(0, 1);
  String line2 = "Comfort: " + String(comfortIndex) + "/100";
  if (line2.length() > 16) {
    line2 = line2.substring(0, 16);
  }
  lcd.print(line2);
}

// ========== DISPLAY CREDITS ==========
void displayCredits() {
  // Line 1: Developed by
  lcd.setCursor(0, 0);
  lcd.print("Developed by:");
  
  // Line 2: Manash and his team
  lcd.setCursor(0, 1);
  lcd.print("Manash & Team");
}

// ========== SETUP WEB SERVER ==========
void setupWebServer() {
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Smart Mirror Dashboard</title>";
    html += "<style>";
    html += "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #ffffff 0%, #f0f8ff 100%); color: #333; min-height: 100vh; }";
    html += ".container { max-width: 1200px; margin: auto; }";
    html += ".header { text-align: center; background: linear-gradient(135deg, #4f6df5 0%, #3a56d4 100%); padding: 30px; border-radius: 15px; margin-bottom: 30px; color: white; box-shadow: 0 10px 25px rgba(58, 86, 212, 0.3); }";
    html += ".header h1 { font-size: 2.8em; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.2); font-weight: 700; }";
    html += ".header-subtitle { font-size: 1.1em; opacity: 0.9; margin-bottom: 20px; }";
    html += ".time-display { font-size: 4em; font-weight: bold; color: white; margin: 20px 0; text-shadow: 3px 3px 6px rgba(0,0,0,0.3); }";
    html += ".date-display { font-size: 1.4em; color: rgba(255,255,255,0.95); background: rgba(255,255,255,0.15); display: inline-block; padding: 10px 25px; border-radius: 50px; margin-top: 10px; }";
    html += ".card-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 25px; }";
    html += ".card { background: white; border-radius: 20px; padding: 30px; box-shadow: 0 15px 35px rgba(0,0,0,0.08); transition: all 0.4s ease; border: 1px solid #eef2ff; }";
    html += ".card:hover { transform: translateY(-10px) scale(1.02); box-shadow: 0 20px 45px rgba(58, 86, 212, 0.15); }";
    html += ".card-header { border-bottom: 3px solid #4f6df5; padding-bottom: 15px; margin-bottom: 25px; color: #2c3e50; font-size: 1.4em; font-weight: 600; }";
    html += ".status-badge { display: inline-block; padding: 8px 18px; border-radius: 25px; font-size: 0.95em; font-weight: 600; margin: 5px; }";
    html += ".status-good { background: linear-gradient(135deg, #28a745 0%, #20c997 100%); color: white; }";
    html += ".status-warning { background: linear-gradient(135deg, #ffc107 0%, #fd7e14 100%); color: white; }";
    html += ".status-info { background: linear-gradient(135deg, #4f6df5 0%, #6c5ce7 100%); color: white; }";
    html += ".forecast-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; text-align: center; }";
    html += ".forecast-item { padding: 15px; background: linear-gradient(135deg, #f8f9ff 0%, #eef2ff 100%); border-radius: 15px; border: 2px solid #e0e7ff; }";
    html += ".forecast-item strong { color: #2c3e50; font-size: 1.1em; }";
    html += ".progress-bar { height: 25px; background: #e9ecef; border-radius: 12px; margin: 15px 0; overflow: hidden; }";
    html += ".progress-fill { height: 100%; background: linear-gradient(90deg, #4f6df5, #6c5ce7); border-radius: 12px; }";
    html += ".control-btn { display: inline-block; padding: 15px 30px; background: linear-gradient(135deg, #4f6df5 0%, #6c5ce7 100%); color: white; border: none; border-radius: 12px; font-size: 1.1em; font-weight: 600; cursor: pointer; margin: 10px; transition: all 0.3s; text-decoration: none; }";
    html += ".control-btn:hover { opacity: 0.9; transform: translateY(-3px); }";
    html += ".control-btn-refresh { background: linear-gradient(135deg, #28a745 0%, #20c997 100%); }";
    html += ".control-btn-export { background: linear-gradient(135deg, #6c757d 0%, #495057 100%); }";
    html += ".message-box { padding: 20px; background: linear-gradient(135deg, #f0f7ff 0%, #e6f2ff 100%); border-left: 5px solid #4f6df5; margin: 20px 0; border-radius: 12px; }";
    html += ".data-point { display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid #f0f0f0; }";
    html += ".data-point:last-child { border-bottom: none; }";
    html += ".data-label { font-weight: 600; color: #2c3e50; }";
    html += ".data-value { font-weight: 700; color: #4f6df5; font-size: 1.1em; }";
    html += ".credits-box { background: linear-gradient(135deg, #2c3e50 0%, #1a252f 100%); color: white; padding: 25px; border-radius: 15px; text-align: center; margin-top: 30px; }";
    html += ".footer { text-align: center; margin-top: 50px; padding: 25px; background: linear-gradient(135deg, #2c3e50 0%, #1a252f 100%); color: white; border-radius: 15px; }";
    html += ".footer p { margin: 5px 0; opacity: 0.9; }";
    html += "@media (max-width: 768px) { .card-grid { grid-template-columns: 1fr; } .time-display { font-size: 3em; } }";
    html += "</style>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>Smart Mirror Dashboard</h1>";
    html += "<div class='header-subtitle'>Real-time Room Monitoring & Analysis System</div>";
    html += "<div class='time-display'>" + formattedTime + "</div>";
    html += "<div class='date-display'>" + formattedFullDate + "</div>";
    html += "</div>";
    
    html += "<div class='card-grid'>";
    
    // Card 1: System Status
    html += "<div class='card'>";
    html += "<div class='card-header'>System Status</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>WiFi Status</span>";
    if (WiFi.status() == WL_CONNECTED) {
      html += "<span class='status-badge status-good'>Connected</span>";
    } else {
      html += "<span class='status-badge status-warning'>Disconnected</span>";
    }
    html += "</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Time Status</span>";
    if (timeSynced) {
      html += "<span class='status-badge status-good'>Auto-Synced</span>";
    } else {
      html += "<span class='status-badge status-warning'>Manual Time</span>";
    }
    html += "</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Data Logging</span>";
    html += "<span class='status-badge status-info'>Active</span>";
    html += "</div>";
    
    html += "<div class='message-box'>";
    html += "<strong>System Info:</strong> Smart Mirror v2.0 | Enhanced Features | Auto Time Sync";
    html += "</div>";
    html += "</div>";
    
    // Card 2: Room Conditions
    html += "<div class='card'>";
    html += "<div class='card-header'>Room Conditions</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Temperature</span>";
    html += "<span class='data-value'>" + String(temperature, 1) + " °C</span>";
    html += "</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Humidity</span>";
    html += "<span class='data-value'>" + String(humidity, 0) + " %</span>";
    html += "</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Comfort Level</span>";
    html += "<span class='status-badge status-info'>" + comfortStatus + "</span>";
    html += "</div>";
    
    html += "<div class='data-label'>Comfort Index</div>";
    html += "<div class='progress-bar'>";
    html += "<div class='progress-fill' style='width:" + String(comfortIndex) + "%'></div>";
    html += "</div>";
    html += "<div style='text-align: center; font-weight: 600; color: #4f6df5;'>" + String(comfortIndex) + "/100</div>";
    
    html += "<div class='message-box'>";
    html += "<strong>Tip:</strong> " + healthTip;
    html += "</div>";
    html += "</div>";
    
    // Card 3: Weather Forecast
    html += "<div class='card'>";
    html += "<div class='card-header'>Weather Forecast</div>";
    
    html += "<div class='forecast-grid'>";
    html += "<div class='forecast-item'>";
    html += "<strong>3 Hours</strong><br>";
    html += String(forecast3hr, 1) + "°C<br>";
    html += "<small>" + forecastStatus[0] + "</small>";
    html += "</div>";
    html += "<div class='forecast-item'>";
    html += "<strong>6 Hours</strong><br>";
    html += String(forecast6hr, 1) + "°C<br>";
    html += "<small>" + forecastStatus[1] + "</small>";
    html += "</div>";
    html += "<div class='forecast-item'>";
    html += "<strong>9 Hours</strong><br>";
    html += String(forecast9hr, 1) + "°C<br>";
    html += "<small>" + forecastStatus[2] + "</small>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='message-box'>";
    html += "<strong>Recommendation:</strong> " + weatherMessage;
    html += "</div>";
    html += "</div>";
    
    // Card 4: Productivity Analysis
    html += "<div class='card'>";
    html += "<div class='card-header'>Productivity Analysis</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Current Level</span>";
    String badgeClass = "status-badge ";
    if (productivityScore >= 80) {
      badgeClass += "status-good";
    } else if (productivityScore >= 60) {
      badgeClass += "status-warning";
    } else {
      badgeClass += "status-warning";
    }
    html += "<span class='" + badgeClass + "'>" + productivityLevel + "</span>";
    html += "</div>";
    
    html += "<div class='data-label'>Productivity Score</div>";
    html += "<div class='progress-bar'>";
    html += "<div class='progress-fill' style='width:" + String(productivityScore) + "%'></div>";
    html += "</div>";
    html += "<div style='text-align: center; font-weight: 600; color: #4f6df5;'>" + String(productivityScore) + "/100</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Recommended Activity</span>";
    html += "<span style='font-weight: 600; color: #2c3e50;'>" + recommendedActivity + "</span>";
    html += "</div>";
    
    html += "<div class='message-box'>";
    html += "<strong>Tip:</strong> " + productivityTip;
    html += "</div>";
    html += "</div>";
    
    // Card 5: Comfort Analysis
    html += "<div class='card'>";
    html += "<div class='card-header'>Comfort Analysis</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Condition</span>";
    html += "<span style='font-weight: 600; color: #2c3e50;'>" + comfortStatus + "</span>";
    html += "</div>";
    
    html += "<div class='data-point'>";
    html += "<span class='data-label'>Message</span>";
    html += "<span style='font-style: italic; color: #495057;'>" + comfortMessage + "</span>";
    html += "</div>";
    
    html += "<div class='message-box'>";
    if (comfortIndex >= 80) {
      html += "<strong>✓ Excellent Conditions:</strong> Perfect environment for work and relaxation.";
    } else if (comfortIndex >= 60) {
      html += "<strong>✓ Good Conditions:</strong> Comfortable for most activities.";
    } else {
      html += "<strong>⚠ Improvement Needed:</strong> Consider adjusting temperature or humidity.";
    }
    html += "</div>";
    html += "</div>";
    
    // Card 6: Controls
    html += "<div class='card'>";
    html += "<div class='card-header'>System Controls</div>";
    
    html += "<div style='text-align: center; margin: 30px 0;'>";
    html += "<a href='/refresh' class='control-btn control-btn-refresh'>Refresh Data</a>";
    html += "<a href='/exportcsv' class='control-btn control-btn-export'>Export CSV</a>";
    html += "</div>";
    
    html += "<div class='message-box'>";
    html += "<strong>System Notes:</strong><br>";
    html += "• Time sync is automatic when WiFi connected<br>";
    html += "• Data logged every 4 seconds for analysis<br>";
    html += "• CSV contains temperature, humidity, comfort & productivity data";
    html += "</div>";
    html += "</div>";
    
    html += "</div>"; // Close card-grid
    
    // Credits Section
    html += "<div class='credits-box'>";
    html += "<h3 style='color: white; margin-top: 0;'>Project Credits</h3>";
    html += "<p style='font-size: 1.2em; margin: 10px 0;'><strong>Developed by:</strong></p>";
    html += "<p style='font-size: 1.5em; font-weight: bold; color: #4f6df5;'>Manash & His Team</p>";
    html += "<p>Smart Mirror System | Project Stage 1 | Silchar, Assam</p>";
    html += "<p>Special thanks to all team members for their contributions</p>";
    html += "</div>";
    
    // Footer
    html += "<div class='footer'>";
    html += "<p><strong>Smart Mirror System v2.0</strong> | Project Stage 1 | Developed by Manash & Team</p>";
    html += "<p>LCD Display Cycles (4s each): Project → Time/Sensors → Detailed Data → Weather → Productivity/Comfort → Credits</p>";
    html += "<p>Data last updated: " + formattedTime + " | " + formattedDate + "</p>";
    html += "</div>";
    
    html += "</div>"; // Close container
    html += "</body></html>";
    
    server.send(200, "text/html", html);
  });
  
  server.on("/refresh", []() {
    generateWeatherForecast();
    readDHT11(); // Re-read sensors
    server.send(200, "text/plain", "Data refreshed successfully!");
    delay(1000);
    server.sendHeader("Location", "/");
    server.send(303);
  });
  
  server.on("/exportcsv", []() {
    // Create CSV content
    String csvContent = "Smart Mirror Data Export\n";
    csvContent += "Generated on: " + formattedFullDate + " " + formattedTime + "\n";
    csvContent += "System Version: 2.0\n";
    csvContent += "Developed by: Manash & Team\n";
    csvContent += "Temperature Unit: °C\n\n";
    csvContent += dataLog;
    
    // Create filename
    String filenameDate = formattedDate;
    filenameDate.replace("/", "-");
    
    // Send as downloadable file
    server.sendHeader("Content-Type", "text/csv");
    server.sendHeader("Content-Disposition", "attachment; filename=smart_mirror_data_" + filenameDate + ".csv");
    server.send(200, "text/csv", csvContent);
    
    Serial.println("CSV exported successfully");
  });
  
  server.begin();
  Serial.println("Web server started");
}

// ========== MAIN LOOP ==========
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  // Update time automatically (if WiFi connected)
  if (timeSynced && WiFi.status() == WL_CONNECTED) {
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 60000) { // Every minute
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 1000)) {
        updateTimeFromSystem(&timeinfo);
      }
      lastTimeUpdate = millis();
    }
  } else {
    // Use manual time
    updateManualTime();
    
    // Try to reconnect and sync time if WiFi was disconnected
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 30000) { // Every 30 seconds
      if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
      } else if (!timeSynced) {
        syncTime();
      }
      lastReconnectAttempt = millis();
    }
  }
  
  // Read sensors every 2 seconds
  static unsigned long lastSensorUpdate = 0;
  if (millis() - lastSensorUpdate > 2000) {
    readDHT11();
    lastSensorUpdate = millis();
  }
  
  // Update weather forecast every 30 minutes
  static unsigned long lastForecastUpdate = 0;
  if (millis() - lastForecastUpdate > 1800000) {
    generateWeatherForecast();
    lastForecastUpdate = millis();
  }
  
  // Update display
  displayOnLCD();
  
  delay(10);
}