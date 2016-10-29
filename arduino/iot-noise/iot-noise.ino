#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

// Definitions
// Pins
#define PIN_MIC A0
#define PIN_RX 2    // RX pin of arduino
#define PIN_TX 3    // TX pin of arduino
#define PIN_RST 10  // Reset pin of ESP8266
// Data
#define ID "1"

// Debug option
#define DEBUG true

// Constants
const double VBASE = 0.1919052878;
const int SAMPLE = 100;
const int MAX = 50;
const int ncols = 16;
const int nrows = 2;

// Network
const String WLAN_SSID = "SSID";
const String WLAN_PASS = "PASS";
const String SERVER_ADDR = "192.168.0.3";
const long SERVER_PORT = 8080;

// Global
int count = 1;
float avgSpl = 0.0;

// Network
SoftwareSerial Wifi(PIN_RX,PIN_TX);

// LCD
LiquidCrystal lcd(9,8,7,6,5,4); 

void setup() {
    // begin the serials connections
    Serial.begin(9600);
    Wifi.begin(9600);
    lcdSetup();

    while (!Serial);
    lcdPrint("Connecting...", 0, 2000, true);
    resetWifi();
    connectWifi();
}

void loop() {
    // read the value from the mic
    float avg = soundSampling(PIN_MIC);

    // convert it to dB value
    float spl = calculateDecibels(avg);

    // send data to the server
    if (count > MAX+1) {
        sendScreen();
        Serial.flush();
        lcdPrint("Sending...", 0, 2000, true);
        sendToServer(spl);
        count = 0;
    } else {
        // write in the lcd
        printScreen(avg, floor(spl));

        // save the average
        avgSpl += abs(avgSpl - spl)/count;
    }
    count++;
}

/* -------------------------------------------------------------------------- */
/* Sound functions */
/* -------------------------------------------------------------------------- */
/* Read a <SAMPLE> values from the mic and calculate the avarage */
float soundSampling(int anPort) {
    float avg = 0.0f;
    int value = 0;

    for (int i=0; i<SAMPLE; i++) {
        value = analogRead(anPort);
        avg += abs(value - 511);
        delay(3);
    }

    return avg/SAMPLE;
}

/* Convert a certain voltage value to decibels */
float calculateDecibels(float value) {
    float vreff = (value/VBASE);
    return (20 * log10(vreff));
}

/* -------------------------------------------------------------------------- */
/* ESP8266 API */ 
/* -------------------------------------------------------------------------- */
void resetWifi(void) {
    ATCommand("AT+RST\r\n", "RST", 5000, DEBUG);
    ATCommand("AT+CWMODE=1\r\n", "CWMODE=1", 3000, DEBUG);
    ATCommand("AT+CIPMUX=0\r\n", "CIPMUX=0", 3000, DEBUG);
}

void connectWifi() {
    String cmd = "AT+CWJAP=\"";
    cmd += WLAN_SSID;   cmd += "\",\"";
    cmd += WLAN_PASS;   cmd += "\"\r\n";
    ATCommand(cmd, "CWJAP", 10000, DEBUG);
}

void sendToServer(float value) {
    String valueStr = "";
    String vreff = "";
    String json = "";
    valueStr += value;
    vreff += VBASE;

    lcdPrint("SPL: " + valueStr, 0, 2000, true);

    // Start the connection
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += SERVER_ADDR; cmd += "\",";
    cmd += SERVER_PORT; cmd += "\r\n";
    lcdPrint("CIPSTART", SERVER_ADDR+":"+SERVER_PORT,1000,false);
    Wifi.print(cmd);
    delay(10000);

    if (Wifi.find("OK")) {
        lcdPrint("TCP conn OK!", 0, 2000, true);
        
        // prepare command to send
        cmd = "AT+CIPSEND=";

        // prepare JSON data
        json = "{\"id\":\"";
        json += ID;
        json += "\",\"vreff\":\"";
        json += vreff;
        json += "\",\"spl\":\"";
        json += valueStr;
        json += "\"}";
        
        int len = json.length();

        // prepare POST String
        String postRequest = "POST /csv/upload HTTP/1.1\r\n";
        postRequest += "Host: " + SERVER_ADDR;
        postRequest += "\r\n";
        postRequest += "Content-Length: ";
        postRequest += len;
        postRequest += "\r\n";
        postRequest += "Content-Type: application/json\r\n\r\n";

        postRequest += json;
        postRequest += "\r\n\r\n";

        cmd += strlen(postRequest.c_str());
        cmd += "\r\n";

        // Send the command
        lcdPrint("CIPSEND", strlen(postRequest.c_str()), 1000, false);
        Wifi.print(cmd);
        delay(5000);

        // Server is waiting for data
        if (Wifi.find(">")) {
            flush(Wifi);
            // Start the sending process
            lcdPrint("Sending...", millis(), 2000, false);

            // Send data
            // ATCommand(postRequest, "POST", 20000, DEBUG);
            Wifi.print(postRequest);
            delay(20000);

            // Log the ending
            lcdPrint("Ending...", millis(), 2000, false);

            delay(100);
        } else {
            lcdPrint("Send Problems", 0, 2000, true);
            flush(Wifi);
        }
        
        ATCommand("AT+CIPCLOSE\r\n", "CIPCLOSE", 5000, DEBUG);

        }
    } else {
        lcdPrint("Conn Problems", 0, 2000, false);
        resetWifi();
        connectWifi();
    }
}

String ATCommand(String command, String type, const int timeout, bool debug) {
    // Send AT command to the module
    lcdPrint("Sending command!", type, 0, false);
    String response = "";
    flush(Wifi);
    Wifi.print(command.c_str());
    long int cur_time = millis();
    while (cur_time + timeout > millis()) {
        while (Wifi.available()) {
            // The esp has data so display its output to the serial window
            char c = Wifi.read();    // read the next char
            response += c;
        }
    }
    if (debug) {
        Serial.println(response);
    }
    return response;
}

/* -------------------------------------------------------------------------- */
/* LCD Screen API */
/* -------------------------------------------------------------------------- */
void lcdSetup()
{
  lcd.begin(ncols, nrows);
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();
}

void printScreen(float avg, int spl) {
    // Print first line
    lcd.setCursor(0,0);
    lcd.print("Avg Amp: ");
    lcd.setCursor(9, 0);
    for (int i=9; i<ncols; i++) {
        lcd.print(" ");
    }
    lcd.setCursor(9, 0);
    lcd.print(avg);

    // Print second line
    lcd.setCursor(0, 1);
    lcd.print("SPL: ");
    lcd.setCursor(5, 1);
    for (int i=5; i<ncols; i++) {
        lcd.print(" ");
    }
    lcd.setCursor(5, 1);
    lcd.print(spl);
    lcd.print("dB");
}

void sendScreen() {
    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("Recording...");
    delay(1000);
    lcd.clear();
}

void lcdPrint(String str, int line, int timeout, bool clean) {
    lcd.clear();
    lcd.setCursor(0,line);
    lcd.print(str);
    delay(timeout);

    if (clean) {
        lcd.clear();
    }
}

void lcdPrint(String fst, String snd, int timeout, bool clean) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(fst);
    lcd.setCursor(0,1);
    lcd.print(snd);
    delay(timeout);

    if (clean) {
        lcd.clear();
    }
}

void flush(SoftwareSerial serial) {
    while (serial.available() > 0) {
        char gbg = serial.read();
    }
}