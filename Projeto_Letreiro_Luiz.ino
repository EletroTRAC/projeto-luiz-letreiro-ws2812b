#include <Wire.h>
#include <FastLED.h>
#include <AHTxx.h>
#include <WiFi.h>
#include "time.h"

#include "credentials.h"

#define NUM_LEDS 300

#define DATA_PIN 12

int R = 0, G = 150, B = 255;

int R_d = 0, G_d = 0, B_d = 0;

CRGB leds[NUM_LEDS];

AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);

float ahtValue;

const char* ssid = SSID;
const char* password = PASS;

const char* ntpServer = "south-america.pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

void atualizaTemperatura(int temp);
void atualizaHora(int hora);
void atualizaMin(int min);

void setup(void) {

    Serial.begin(115200);

    initWiFi();
    
    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);

    while (aht10.begin() != true)
    {
        Serial.println(F("AHT1x não conectado ou falha ao carregar o coeficiente de calibração"));

        delay(5000);
    }

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printInfosTimeESensor();
}

void loop(void) {
    static long long lastReadMillis = 0;
    if (millis() - lastReadMillis > 2000)
    {
      printInfosTimeESensor();
      lastReadMillis = millis();
    }
}

 void initWiFi(void) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Conectando-se à: ");
        Serial.println(ssid);
        delay(1000);
    }
    Serial.println("Conexão bem-sucedida!");
 }

void printInfosTimeESensor(void)
{
    static char* lastValueMin;
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }

    char timeHour[3];
    char timeMin[3];

    strftime(timeHour,3, "%H", &timeinfo);
    strftime(timeMin,3, "%M", &timeinfo);

    Serial.print(timeHour );
    Serial.print(":");
    Serial.println(timeMin);

    if (timeMin != lastValueMin)
    {
        atualizaHora(int(timeHour));
        atualizaMin(int(timeMin));
        lastValueMin = timeMin;
    }

    /*  Atualização temperatura e Umidade */

    ahtValue = aht10.readTemperature();

    if (ahtValue != AHTXX_ERROR)
    {
        atualizaTemperatura(int(ahtValue));
    }
    else
    {
        printStatus(); //print temperature command status

        if   (aht10.softReset() == true) Serial.println(F("reset success"));
        else                             Serial.println(F("reset failed"));
    }
    atualizaTemperatura(int(ahtValue));
}

void printStatus(void)
{
  switch (aht10.getStatus())
  {
    case AHTXX_NO_ERROR:
      Serial.println(F("Sem erro."));
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      break;

    default:
      Serial.println(F("unknown status"));    
      break;
  }
}