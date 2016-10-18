#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

// Definitions
// Pins
#define PIN_MIC A0
#define PIN_RX 2    // RX pin of arduino
#define PIN_TX 3    // TX pin of arduino
// Data
#define ID "1"
#define JSON_MODEL "{\"id\":\"%d\" , \"vreff\":\"%g\" , \"tmp\":\"%l\" , \"spl\":\"%s\"}"

#define DEBUG true

// Constants
const double VBASE = 0.1919052878;
const int SAMPLE = 100;
const int ncols = 16;
const int nrows = 2;

// Network
const String WLAN_SSID = "SSID";
const String WLAN_PASS = "PASS";
const String SERVER_ADDR = "192.168.0.3";   // internal network
const long SERVER_PORT = 8080;

// Global
int count = 0;

// Network
SoftwareSerial Wifi(PIN_RX,PIN_TX);

// LCD
LiquidCrystal lcd (9,8,7,6,5,4); 

void setup() {
    // begin the serials connections
    Serial.begin(9600);
    Wifi.begin(9600);
    lcdSetup();

    while (!Serial);
    lcdPrint("Connecting...", 0, 2000, false);
    resetWifi();
    connectWifi();
}

void loop() {
    // read the value from the mic
    float avg = soundSampling(PIN_MIC);

    // convert it to dB value
    float spl = calculateDecibels(avg);

    // send data to the server
    if (count > 50) {
        sendScreen();
        lcdPrint("Sending...", 0, 2000, false);
        sendToServer(spl);
        count = 0;
    } else {
        // write in the lcd
        printScreen(avg, floor(spl));
    }
    count++;
}

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

void resetWifi(void) {  
    ATCommand("AT+RST\r\n", 5000, DEBUG);
    ATCommand("AT+CWMODE=1\r\n", 3000, DEBUG);
    ATCommand("AT+CIPMUX=0\r\n", 3000, DEBUG);
}

void connectWifi() {
    String cmd = "AT+CWJAP=\"" + WLAN_SSID + "\",\"" + WLAN_PASS + "\"\r\n";
    ATCommand(cmd, 10000, DEBUG);
}

void sendToServer(float value) {
    String valueStr = "";
    valueStr += value;

    lcdPrint(valueStr + "dB", 1, 2000, true);

    char buffer[1024] = { '\0' };

    // Start the connection
    String cmd = "AT+CIPSTART=\"TCP\",\"" + SERVER_ADDR + "\"," + SERVER_PORT;
    cmd += "\r\n";
    lcdPrint("CIPSTART", SERVER_ADDR + " " + SERVER_PORT, 2000, false);
    Wifi.print(cmd);
    delay(10000);

    if (Wifi.find("OK")) {
        lcdPrint("TCP conn OK!", 0, 2000, true);
        Wifi.flush();
        
        // prepare command to send
        cmd = "AT+CIPSEND=";

        // prepare JSON data
        sprintf(buffer, JSON_MODEL, ID, VBASE, millis(), valueStr.c_str());

        // prepare POST String
        String postRequest = "POST /csv/upload HTTP/1.1\r\n";
        postRequest += "Host: " + SERVER_ADDR;
        postRequest += "\r\n";
        postRequest += "Accept: */* \r\n";
        postRequest += "Content-Length: " + strlen(buffer);
        postRequest += "\r\n";
        postRequest += "Content-Type: application/json\r\n\r\n";

        postRequest += buffer;
        postRequest += "\r\n";

        cmd += postRequest.length();
        cmd += "\r\n";

        // Send the command
        Serial.print(cmd);
        Wifi.print(cmd);
        delay(10000);

        if (Wifi.find(">")) {
            // Start the sending process
            Serial.print("Sending data... [");  Serial.print(millis()); 
            lcdPrint("]", 0, 2000, true);

            // Send data
            lcdPrint(postRequest, 0, 2000, true);
            Wifi.print(postRequest);
            delay(10000);


            if (Wifi.find("SEND OK")) {
                lcdPrint("Package Sent!", 0, 2000, true);

                while(Wifi.available()) {
                    String response = Wifi.readString();
                    lcdPrint(response, 0, 2000, true);
                }

            } else {
                lcdPrint("Unable to send", 0, 2000, true);
                Wifi.flush();
            }

            // Log the ending
            Serial.print("Ending... [");    Serial.print(millis());
            lcdPrint("]", 0, 2000, true);
            lcdPrint(buffer, 0, 2000, true);

            delay(100);
        } else {
            lcdPrint("Send Problems", 0, 2000, true);
            Wifi.flush();
        }
        
        ATCommand("AT+CIPCLOSE\r\n", 10000, DEBUG);
    } else {
        lcdPrint("Unable to connect restarting the module", 0, 2000, true);
        resetWifi();
        connectWifi();
    }
}

/* Use if you do not need the return in the Module's Serial Window' */
String ATCommand(String command, const int timeout, bool debug) {
    // Send AT command to the module
    lcdPrint("Sending command", 0, 2000, true);
    String response = "";
    Wifi.flush();
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
        Serial.print(response);
    }
    return response;
}

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
    lcd.print("dB ");
    lcd.print(count);
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