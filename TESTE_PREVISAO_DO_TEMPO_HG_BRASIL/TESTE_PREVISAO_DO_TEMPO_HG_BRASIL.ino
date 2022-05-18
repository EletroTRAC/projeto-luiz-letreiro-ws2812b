#include <ArduinoJson.h>
#include <WiFi.h>

const char* HOMESSID = "Bosta no ventilador";
const char* HOMEPW = "barbosa2022";

#define CHOVE_LED_PIN 33
#define NUBLADO_LED_PIN 2
#define FAZ_SOL_LED_PIN 27

String woeid = "455827"; // WOIED de sua cidade
String chave = "c0ec6270"; // Sua Chave da API HG Brasil Weather

char* dataPrevisao1;
char* diaDaSemanaPrevisao1;
int maxPrevisao1;
int minPrevisao1;
char* descricaoTempoPrevisao1;
char* dataPrevisao2;
char* diaDaSemanaPrevisao2;
int maxPrevisao2;
int minPrevisao2;
char* descricaoTempoPrevisao2;
char* dataPrevisao3;
char* diaDaSemanaPrevisao3;
int maxPrevisao3;
int minPrevisao3;
char* descricaoTempoPrevisao3;
char* dataPrevisao4;
char* diaDaSemanaPrevisao4;
int maxPrevisao4;
int minPrevisao4;
char* descricaoTempoPrevisao4;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(CHOVE_LED_PIN, OUTPUT);
  pinMode(NUBLADO_LED_PIN, OUTPUT);
  pinMode(FAZ_SOL_LED_PIN, OUTPUT);
  connectWiFi();
}

void loop() {
  // put your main code here, to run repeatedly:
  atualizarDadosDoTempo();
  if (descricaoTempoPrevisao3 == "Tempo limpo")
  {
    Serial.println("TESTE TESTE TESTE TESTE TESTE");
  }
  
  delay(1500);
}

void connectWiFi(void) {
  WiFi.begin(HOMESSID, HOMEPW);
  printf("Connecting...");
  int TryNum = 0;
  while (WiFi.status() != WL_CONNECTED) {
    printf(".");
    delay(200);
    TryNum++;
    if (TryNum > 20) {
      printf("\nUnable to connect to WiFi. Please check your parameters.\n");
      for (;;);
    }
  }
  printf("Connected to: % s\n\n", HOMESSID);
}

void atualizarDadosDoTempo() { // Função para atualizar variáveis do Tempo e da Previsão do tempo
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
    const char* date = elem["date"];
    const char* weekday = elem["weekday"];
    int max = elem["max"];
    int min = elem["min"];
    const char* description = elem["description"];
    switch (indice) {
      case 1:
        dataPrevisao1 = (char *)date;
        diaDaSemanaPrevisao1 = (char *)weekday;
        maxPrevisao1 = max;
        minPrevisao1 = min;
        descricaoTempoPrevisao1 = (char *)description;
        break;
      case 2:
        dataPrevisao2 = (char *)date;
        diaDaSemanaPrevisao2 = (char *)weekday;
        maxPrevisao2 = max;
        minPrevisao2 = min;
        descricaoTempoPrevisao2 = (char *)description;
        break;
      case 3:
        dataPrevisao3 = (char *)date;
        diaDaSemanaPrevisao3 = (char *)weekday;
        maxPrevisao3 = max;
        minPrevisao3 = min;
        descricaoTempoPrevisao3 = (char *)description;
        break;
      case 4:
        dataPrevisao4 = (char *)date;
        diaDaSemanaPrevisao4 = (char *)weekday;
        maxPrevisao4 = max;
        minPrevisao4 = min;
        descricaoTempoPrevisao4 = (char *)description;
        break;
    }
    Serial.print("dia ");
    Serial.println(date);
    Serial.println(weekday);
    Serial.println(max);
    Serial.println(min);
    Serial.println(description);
    Serial.println("\n");
    indice++;
  }
  // Desconecta-se do servidor
  client.stop();
}