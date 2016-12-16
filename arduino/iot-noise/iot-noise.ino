/*
 * IoT Noise
 * copyright (c) Andre Filliettaz, 2016
 * 
 * Released under GLv3.
 * Please refer to LICENSE file for licensing information.
 *
 * Reference:
 * SPL meter - Davide Girone: http://davidegironi.blogspot.com.br/2014/02/a-simple-sound-pressure-level-meter-spl.html#.WCxnlHUrK03
 * ESP8266   - FilipeFlop: http://blog.filipeflop.com/wireless/esp8266-arduino-tutorial.html 
 */
//#define LCD

#include <SoftwareSerial.h>

#ifdef LCD
#include <LiquidCrystal.h>
#endif

#include "adc.h"
#include "audioget.h"

// Definitions
// Pins
#define PIN_RX 2    // RX pin of arduino
#define PIN_TX 3    // TX pin of arduino
#define PIN_RST 10  // RST pin

// Data
#define ID "7"

// Debug option
#define DEBUG true

// Constants
const int MAX = 20;
#ifdef LCD
const int ncols = 16;
const int nrows = 2;
#endif

// Network
#define WLAN_SSID "filliettaz"
#define WLAN_PASS "20081991"
#define SERVER_ADDR "192.168.43.179"
#define SERVER_PORT 8080

// Network
SoftwareSerial Wifi(PIN_RX,PIN_TX);

// LCD
#ifdef LCD
LiquidCrystal lcd(9,8,7,6,5,4);
#endif

void setup() {
    // begin the serials connections
    Serial.begin(9600);
    Wifi.begin(9600);

    #ifdef LCD
    lcdSetup();
    #endif

    // Pin modes
    pinMode(PIN_RST, OUTPUT);

    while (!Serial);

    #ifdef LCD
    lcdPrint("Connecting...", 0, 2000, true);
    #endif

    resetWifi();
    connectWifi();
    Serial.println("End setup");
}

void loop() {
    // count and average spl
    int count = 1;
    int ns = 1;
    long spend = 0;
    double avgspl = 0.0;

    // get current reference voltage
    double refv = adc_getrealvref();

    int16_t audiorms = 0;
    int16_t audiospl = 0;

    String tmvlist = "[";

    // input channel is setted to 0
    audioget_init();

    while(true) {
        // mark time
        long start = millis();
        // get samples
        audioget_getsamples();
        // compute fft
        audioget_computefft();
        // apply weighting
        audioget_doweighting();
        
        // get rms and spl
        audiorms = audioget_getrmsval();
        float rmsvolts = 0;

        if (audiorms <= 0) {
            rmsvolts = 1e37;
        } else {
            rmsvolts = adc_getvoltage(audiorms, refv);
        }

        audiospl = audioget_getspl(rmsvolts, AUDIOGET_VOLTREF, AUDIOGET_DBREF);
        spend += millis() - start;

        if (spend > 200) {
            tmvlist.concat(spend);
            tmvlist.concat(",");
            tmvlist.concat(avgspl);

            // send data to the server
            if (count > MAX+1) {
                #ifdef LCD
                sendScreen();
                lcdPrint("Sending...", 0, 2000, true);
                #endif

                tmvlist.concat( "]");
                sendToServer(tmvlist);
                count = 0;
                tmvlist = "[";
            } else {
                #ifdef LCD
                // write in the lcd
                printScreen(audiorms, floor(audiospl));
                #endif

                // save the average
                tmvlist.concat(",");
            }
            count++;
            spend = 0;
            ns = 0;
            avgspl = 0.0;
        } else {
            avgspl += abs(avgspl - audiospl)/ns;
        }
        ns++;
    }
}

/* -------------------------------------------------------------------------- */
/* ESP8266 API */ 
/* -------------------------------------------------------------------------- */
void resetWifi() {
    flush(Wifi);
    digitalWrite(PIN_RST, LOW);
    delay(1000);
    digitalWrite(PIN_RST, HIGH);
    delay(500);
    ATCommand("AT+CWMODE=1\r\n", "CWMODE=1", 1000, DEBUG);
    ATCommand("AT+CIPMUX=0\r\n", "CIPMUX=0", 1000, DEBUG);
}

void connectWifi() {
    String cmd = "AT+CWJAP=\"";
    cmd.concat(WLAN_SSID);   cmd.concat("\",\"");
    cmd.concat(WLAN_PASS);   cmd.concat("\"\r\n");
    ATCommand(cmd, "CWJAP", 10000, DEBUG);
}

void sendToServer(String value) {
    #ifdef LCD
    lcdPrint("CIPSTART", SERVER_ADDR+":"+SERVER_PORT,1000,false);
    #endif

    Wifi.print("AT+CIPSTART=\"TCP\",\"");
    Wifi.print(SERVER_ADDR); Wifi.print("\",");
    Wifi.print(SERVER_PORT); Wifi.print("\r\n");
    delay(10000);

    if (Wifi.find("OK")) {
        #ifdef LCD
        lcdPrint("TCP conn OK!", 0, 2000, true);
        #endif
        
        int len = 0;

        // prepare JSON data
        len += strlen("{\"id\":\"");
        len += strlen(ID);
        len += strlen("\",\"vref\":\"");
        len += strlen(String(AUDIOGET_VOLTREF).c_str());
        len += strlen("\",\"dbref\":\"");
        len += strlen(String(AUDIOGET_DBREF).c_str());
        len += strlen("\",\"tmvlist\":");
        len += value.length();
        len += strlen("}");
        
        int sendsize = 0;

        // prepare POST String
        sendsize += strlen("POST /csv/upload HTTP/1.1\r\n");
        sendsize += strlen("Host: "); 
        sendsize += strlen(SERVER_ADDR);
        sendsize += strlen("\r\n");
        sendsize += strlen("Content-Length: ");
        sendsize += strlen(String(len).c_str());
        sendsize += strlen("\r\n");
        sendsize += strlen("Content-Type: application/json\r\n\r\n");
        sendsize += strlen("{\"id\":\"");
        sendsize += strlen(ID);
        sendsize += strlen("\",\"vref\":\"");
        sendsize += strlen(String(AUDIOGET_VOLTREF).c_str());
        sendsize += strlen("\",\"dbref\":\"");
        sendsize += strlen(String(AUDIOGET_DBREF).c_str());
        sendsize += strlen("\",\"tmvlist\":");
        sendsize += value.length();
        sendsize += strlen("}");
        sendsize += strlen("\r\n\r\n");

        Wifi.print("AT+CIPSEND=");
        Wifi.print(sendsize);
        Wifi.print("\r\n");
        delay(10000);

        // Server is waiting for data
        if (Wifi.find(">")) {
            flush(Wifi);
            // Start the sending process
            #ifdef LCD
            lcdPrint("Sending...", millis(), 2000, false);
            #endif

            // Send data
            Wifi.print("POST /csv/upload HTTP/1.1\r\n");
            Wifi.print("Host: "); 
            Wifi.print(SERVER_ADDR);
            Wifi.print("\r\n");
            Wifi.print("Content-Length: ");
            Wifi.print(len);
            Wifi.print("\r\n");
            Wifi.print("Content-Type: application/json\r\n\r\n");
            Wifi.print("{\"id\":\"");
            Wifi.print(ID);
            Wifi.print("\",\"vref\":\"");
            Wifi.print(AUDIOGET_VOLTREF);
            Wifi.print("\",\"dbref\":\"");
            Wifi.print(AUDIOGET_DBREF);
            Wifi.print("\",\"tmvlist\":");
            Wifi.print(value);
            Wifi.print("}");            
            Wifi.print("\r\n\r\n");
            delay(10000);

            // Log the ending
            #ifdef LCD
            lcdPrint("Ending...", millis(), 2000, false);
            #endif
        } else {
            #ifdef LCD
            lcdPrint("Send Problems", 0, 2000, true);
            #endif

            flush(Wifi);
        }

        flush(Wifi);
        ATCommand("AT+CIPCLOSE\r\n", "CIPCLOSE", 5000, DEBUG);
    } else {
        #ifdef LCD
        lcdPrint("Conn Problems", 0, 2000, false);
        #endif

        resetWifi();
        connectWifi();
    }
}

String ATCommand(String command, String type, const int timeout, bool debug) {
    // Send AT command to the module
    #ifdef LCD
    lcdPrint("Sending command!", type, 0, false);
    #endif

    String response = "";
    flush(Wifi);
    Wifi.print(command.c_str());
    long int cur_time = millis();
    while (cur_time + timeout > millis()) {
        while (Wifi.available()) {
            // The esp has data so display its output to the serial window
            char c = Wifi.read();    // read the next char
            response.concat(c);
        }
    }
    if (debug) {
        Serial.println(response);
    }
    return response;
}

#ifdef LCD
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
    lcd.print("RMS: ");
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
#endif

void flush(SoftwareSerial serial) {
    while (serial.available() > 0) {
        serial.read();
    }
}
