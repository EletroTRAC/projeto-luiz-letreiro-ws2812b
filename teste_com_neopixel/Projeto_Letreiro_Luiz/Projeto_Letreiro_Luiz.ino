#include <Wire.h>
#include <FastLED.h>
#include <AHTxx.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>
#include "time.h"

#define NUMPIXELS 300
#define PIN 16
#define BRIGHTNESS 255

int R = 0, G = 150, B = 255;
int R_d = 0, G_d = 0, B_d = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);
float ahtValue;

const char* ntpServer = "south-america.pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

String woeid = WOIEDCred; // WOIED de sua cidade
String chave = KEY_Cred; // Sua Chave da API HG Brasil Weather
char* descricaoTempoPrevisao3;

void atualizaTemperatura(int temp);
void atualizaHora(int hora);
void atualizaMin(int min);
void atualizarDadosDoTempo(void);

void setup(void) {

    Serial.begin(115200);

    initWiFi();
    
    strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();            // Turn OFF all pixels ASAP
    strip.setBrightness(BRIGHTNESS);

    while (aht10.begin() != true)
    {
        Serial.println(F("AHT1x não conectado ou falha ao carregar o coeficiente de calibração"));

        delay(5000);
    }

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printInfosTimeESensor();
}

void loop(void) {
    static unsigned long lastReadMillis = 0;
    if (millis() - lastReadMillis > 2000)
    {
      printInfosTimeESensor();
      lastReadMillis = millis();
    }
}

 void initWiFi(void) {
    WiFiManager wm;
    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    }
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
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

void atualizarDadosDoTempo(void) { // Função para atualizar variáveis do Tempo e da Previsão do tempo
  Serial.println(F("Conectando à API do Tempo..."));
  WiFiClient client; // Cria um cliente para se conectar ao servidor da API do Tempo
  client.setTimeout(10000); // Define o máximo de milissegundos para aguardar os dados de fluxo
  if (!client.connect("api.hgbrasil.com", 80)) { // Se conectado ao endereço do servidor, na porta 80, com sucesso...
    Serial.println(F("Conexão falhou :("));
    while (!client.connect("api.hgbrasil.com", 80)) { // enquanto não estiver conectado ao endereço do servidor,...
      delay(450);
      Serial.print(".");
    }
    Serial.println("Conexão com API restaurada!");
    return;
  }
  Serial.println(F("Conectado!"));
  // Envia pedido HTTP ao servidor da API do Tempo, solicitando dados de 4 dias (hoje + 3 dias à frente) em array_limit=4.
  //O número de array_limit é um inteiro limitando o número de itens em arrays do retorno
  client.println("GET /weather?array_limit=4&fields=only_results,date,time,description,currently,city,humidity,wind_speedy,sunrise,sunset,forecast,date,weekday,max,min,description,&key=" + chave
                 + "&woeid=" + woeid
                 + " HTTP/1.0");
  client.println(F("Host: api.hgbrasil.com"));
  client.println(F("Connection: close"));
  if (client.println() == 0) { // se o retorno de dados do servidor conectado for dde 0 bytes,...
    Serial.println(F("Falha ao enviar pedido"));
    client.stop(); // Desconecta-se do servidor
    return;
  }
  // Verifica o status do HTTP
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  // Deve ser "HTTP / 1.0 200 OK" ou "HTTP / 1.1 200 OK"
  if (strcmp(status + 9, "200 OK") != 0) {
    Serial.print(F("Resposta inesperada: "));
    Serial.println(status);
    client.stop();
    return;
  }
  // Pular cabeçalhos HTTP
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Resposta inválida"));
    client.stop();
    return;
  }
  // Alocar o documento JSON
  // Use arduinojson.org/v6/assistant para calcular a capacidade.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600; // Necessário 600 bytes de memória reservada para o processo de Desserialização do documento JSON
  StaticJsonDocument<1024> doc;
  // Analisa o objeto JSON
  Serial.println(client);
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() falhou: "));
    Serial.println(error.f_str());
    client.stop();
    return;
  }

  // Extrai os valores da Previsão do tempo e repassa para as variáveis globais
  Serial.println("\nPrevisão do Tempo\n");
  int indice = 1;
  for (JsonObject elem : doc["forecast"].as<JsonArray>()) {
    const char* description = elem["description"];
    if (indice == 3)
    {
      descricaoTempoPrevisao3 = (char *)description;
    }
    indice++;
  }
  // Desconecta-se do servidor
  client.stop();
}