#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Firebase_ESP_Client.h> 
#include <TinyGPS++.h>  
#include <SoftwareSerial.h> 

#define trig 33
#define echo 32
#define vibration 34

// Configure GPS module pins  
TinyGPSPlus gps;  
SoftwareSerial gpsSerial(2, 4); // GPS Tx Pin - D2, GPS Rx Pin - D4  

// Configure GSM module pins (using SoftwareSerial)  
SoftwareSerial gsmSerial(16, 17); // GSM Tx Pin - D10, GSM Rx Pin - D11  

#define API_KEY "AIzaSyD1jegmijZOM86GgqIodhpv5BsoFjqIkd6U"
#define DATABASE_URL "https://road-accident-732f5-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "keshavgamer666@gmail.com"
#define USER_PASSWORD "keshav@5173";
const char *ssid = "Redmi 12 5G";
const char *password = "12345677";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WebServer server(80);

void handleRoot() {
  char msg[1500];
  snprintf(msg, 1500,
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta http-equiv='refresh' content='2'>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Document</title>"
    "<style>"
    "html { width: 100%%; height: 100%%; }"
    "body { margin: 0; padding: 0; }"
    "#main { width: 100vw; height: 100vh; position: fixed; }"
    "#nav { position: absolute; z-index: 1; width: 100vw; height: 15vh; background-color: rgba(0, 0, 0, 0.5);"
    " display: flex; justify-content: space-between; align-items: center; padding: 0px 50px; color: white; }"
    "#logo { width: 30vw; font-size: 2rem; }"
    "#dets { width: 40vw; display: flex; justify-content: space-evenly; align-items: center; font-size: 1.5rem; }"
    "#dets p { transition: all 1s ease-in-out 0s; }"
    "#dets p:hover { text-decoration: underline; }"
    "#dets button:hover { background-color: black; color: white; box-shadow: 2px 2px 5px rgb(255, 255, 255); }"
    "#dets button { height: 5vh; width: 5vw; border-radius: 7px; border: none; }"
    "</style>"
    "</head>"
    "<body>"
    "<div id='main'>"
    "<div id='nav'>"
    "<div id='logo'><h3><i>HOPE</i></h3></div>"
    "<div id='dets'><p>Home</p><p>About</p><p>Features</p><button>Login</button></div>"
    "</div>"
    "<span>Distance: %d cm</span>"
    "</div>"
    "</body>"
    "</html>",
    distance()
  );

  server.send(200, "text/html", msg);
}


void setup() {
  Serial.begin(115200);  
  gpsSerial.begin(9600); // Initialize GPS module at 9600 baud  
  gsmSerial.begin(9600); // Initialize GSM module at 9600 baud  
  pinMode(echo, INPUT);
  pinMode(trig, OUTPUT);
  pinMode(vibration, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("GPS and GSM Module Test");  
  delay(3000); // Allow time for GSM module to initialize
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Authentication
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Initialize Firebase
  Firebase.begin(&config, &auth);

  // Optional: Increase Firebase reconnect timeout to handle long periods without WiFi
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized");

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");
  
}

void loop() {
  // Check for new GPS data  
  while (gpsSerial.available() > 0) {  
    gps.encode(gpsSerial.read());  

    // Only send SMS with new location and valid date/time received  
    if (gps.location.isUpdated() && gps.date.isValid() && gps.time.isValid()) {  
      // Prepare the SMS content  
      String message = "Location: Latitude = " + String(gps.location.lat(), 6) +  
                       ", Longitude = " + String(gps.location.lng(), 6) +  
                       ", Date: " + String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year()) +  
                       ", Time: " + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());  

      // Send SMS with the location, date, and time  
      sendTestSMS(message); // Send the composed message  
    }  
  }  
  server.handleClient();
  int d = distance();
  if (Firebase.ready()) {
    Firebase.RTDB.setFloat(&fbdo, "/road-accident/distance", d);
    
    // Serial.println("Data sent to Firebase");
  }
  int vib = analogRead(vibration);
  Serial.println(vib);
  delay(1000);
}

int distance(){
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  int duration = pulseIn(echo, HIGH);
  int distance = duration * 0.0343 / 2;
  return distance;
}

void sendTestSMS(String message) {  
  gsmSerial.println("AT+CMGF=1"); // Set SMS to text mode  
  delay(1000);  
  gsmSerial.println("AT+CMGS=\"+918107080058\""); // Replace with your phone number  
  delay(1000);  

  // Send the message  
  gsmSerial.print(message); // Sent formatted GPS information  

  delay(100);  
  gsmSerial.write(26); // Send Ctrl+Z to indicate end of SMS  
  Serial.println("Test SMS Sent: " + message); // Log the sent message  
}