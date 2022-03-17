#include <WiFi.h>
#include <HTTPClient.h>
#include "mbedtls/md.h"
#include <ArduinoJson.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define MAX2719_DIN 26 //PIN DIN
#define MAX2719_CS 27 //PIN CS
#define MAX2719_CLK 25 //PIN CLK

const char * ssid = ""; //INSERT SSID
const char * password = ""; //INSERT PSK / PASSWORD

int bal = 0;

TaskHandle_t screen;
TaskHandle_t apiUpdate;

void setup() {
//-----------------------------------------WIFI CONNECTION----------------------------------------------------    
    WiFi.begin(ssid, password);
    Serial.begin(115200);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("LOADED");
    delay(2000);
//-----------------------------------DISPLAY-----------------------------------------------  
    digitalWrite(MAX2719_CS, HIGH);
    pinMode(MAX2719_DIN, OUTPUT);
    pinMode(MAX2719_CS, OUTPUT);
    pinMode(MAX2719_CLK, OUTPUT);
    
    // For test mode (all digits on) set to 0x01. Normally we want this off (0x00)
    output(0x0f, 0x0);
    delay(100);
    // Set all digits off initially
    output(0x0c, 0x0);
    delay(100);
    // Set brightness for the digits to high(er) level than default minimum (Intensity Register Format)
    output(0x0a, 0xFF);
    delay(100);
    // Set decode mode for ALL digits to output actual ASCII chars rather than just
    // individual segments of a digit
    output(0x09, 0xFF);
    delay(100);
    // Set digits blank
    output(0x01, 0x0F);
    output(0x02, 0x0F);
    output(0x03, 0x0F);
    output(0x04, 0x0F);
    output(0x05, 0x0F);
    output(0x06, 0x0F);
    output(0x07, 0x0F);
    output(0x08, 0x0F);
    delay(100);
    // Ensure ALL digits are displayed (Scan Limit Register)
    output(0x0b, 0x07);
    delay(100);
    // Turn display ON (boot up = shutdown display)
    output(0x0c, 0x01);
    delay(100);
    xTaskCreatePinnedToCore(
        doScreen,
        "Screen",
        10000,
        NULL,
        5,
        &screen,
        0);
    xTaskCreatePinnedToCore(
        updateBals,
        "apiUpdate",
        10000,
        NULL,
        6,
        &apiUpdate,
        1);  
}


float getFuturesBal() { //Gets the total wallet balance
    HTTPClient http;
    String url = String("https://openapi.debank.com/v1/user/total_balance?id=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"); //INSERT API URL HERE
    http.useHTTP10(true);
    http.begin(url);
    int rc = http.GET();
    delay(100);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getStream());
    http.end();
    bal = doc["total_usd_value"].as < float > ();
    Serial.println(bal);
    return (bal);
}


void doScreen(void * pvParameters) { 
    int i;
    for (;;) {
        TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE; //Feeds the watchdog
        TIMERG0.wdt_feed = 1;
        TIMERG0.wdt_wprotect = 0;
        yield();
        unsigned long remainder;
  
        byte tenmillions = bal / 10000000;
        remainder = bal % 10000000;
      
        byte millions = remainder / 1000000;
        remainder = remainder % 1000000;
        
        byte hundredthou = remainder / 100000;
        remainder = remainder % 100000;
      
        byte tenthou = remainder / 10000;
        remainder = remainder % 10000;
      
        byte thou = remainder / 1000;
        remainder = remainder % 1000;
      
        byte hundreds = remainder / 100;
        remainder = remainder % 100;
      
        byte tens = remainder / 10;
        remainder = remainder % 10;

        if (tenmillions==0){
          output(0x08, 0x0F);
        }
        else{
          output(0x08, tenmillions); // tenmillion
        }
        if ((millions==0)&&(tenmillions==0)){
          output(0x07, 0x0F);
        }
        else{
          output(0x07, millions); // million
        }
        if ((hundredthou==0)&&(millions==0)&&(tenmillions==0)){
          output(0x06, 0x0F);
        }
        else{
          output(0x06, hundredthou); // hundred thou
        }
        if ((tenthou==0)&&(hundredthou==0)&&(millions==0)&&(tenmillions==0)){
          output(0x05, 0x0F);
        }
        else{
          output(0x05, tenthou); // ten thou
        }  
        if ((thou==0)&&(tenthou==0)&&(hundredthou==0)&&(millions==0)&&(tenmillions==0)){
          output(0x04, 0x0F);
        }
        else{
          output(0x04, thou); // thousands
        }
        if ((hundreds==0)&&(thou==0)&&(tenthou==0)&&(hundredthou==0)&&(millions==0)&&(tenmillions==0)){
          output(0x03, 0x0F);
        }
        else{
          output(0x03, hundreds); // hundreds
        }
        if ((tens==0)&&(hundreds==0)&&(thou==0)&&(tenthou==0)&&(hundredthou==0)&&(millions==0)&&(tenmillions==0)){
          output(0x02, 0x0F);
        }
        else{
          output(0x02, tens); // tens
        }  
        output(0x01, remainder); // units
     }
}


void updateBals(void * pvParameters) { //Updates balances every 15 seconds
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    for (;;) {
        getFuturesBal();
        delay(15000);
    }
}


void loop() {
    vTaskDelete(NULL);
    yield();
}


void output(byte address, byte data)
{
  digitalWrite(MAX2719_CS, LOW);
  // Send out two bytes (16 bit)
  // parameters: shiftOut(dataPin, clockPin, bitOrder, value)
  shiftOut(MAX2719_DIN, MAX2719_CLK, MSBFIRST, address);
  shiftOut(MAX2719_DIN, MAX2719_CLK, MSBFIRST, data);
  digitalWrite(MAX2719_CS, HIGH);
}
