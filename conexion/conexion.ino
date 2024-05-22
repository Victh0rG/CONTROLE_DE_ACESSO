//Biblioteca -----------------------------------------------------------------------------------------
#include <Adafruit_Fingerprint.h>                           // Responsável pelo Sensor Biométrico
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// Definindo rede ------------------------------------------------------------------------------------
#define WIFI_SSID "Republica Da Computaria"
#define WIFI_PASSWORD "curry2930"
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6

//Declarações de Variáveis----------------------------------------------------------------------------
const uint32_t password = 0x0;                              // Senha padrão do sensor biométrico
WebServer server(80);                                       // Servidor na porta 80
const int LED1 = 26;
const int LED2 = 27;

bool led1State = false;
bool led2State = false;

int digital = -1;

String ipServidorExterno = "192.168.100.160";

String globalToken;
int labId;
bool fechadura = false;

//Declaração de variaveis para função de tempo--------------------------------------------------------
unsigned long currentTime=0;
unsigned long previousTime=0;

//Objetos --------------------------------------------------------------------------------------------
Adafruit_Fingerprint fingerSensor = Adafruit_Fingerprint(&Serial2, password);
HTTPClient http;

//Funções Auxiliares ---------------------------------------------------------------------------------
void printMenu();                                           // Responsável pelo Menu
String pegaComando();                                       // Responsável por Pegar Comando
int cadastraDigital();                                     // Responsável por Cadastrar Digital
void verificaDigital();                                     // Responsável por Verificar Digital
void contDigital();                                         // Responsável por Contar Digitais
void deleteDigital();                                       // Responsável por Deletar Digital
void limpaDatabase();                                       // Responsável por Limpar Banco de Dados
void sendHtml();                                            // Responsável pelas requisiçoes
void criarLab();
//void sendCad();


//Setup ----------------------------------------------------------------------------------------------
void setup(){
    Serial.begin(115200);                                   // Inicializa o Serial
    fingerSensor.begin(57600);                              // Inicializa o Sensor
    if(!fingerSensor.verifyPassword()){                     // Verifica se não é a senha do sensor
        Serial.println("Não foi possível conectar ao sensor. Verifique a senha ou a conexão");
        while(true){};                                      // Loop Infinito
    }

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);       // Inicializa Wifi
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println(" Connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", sendHtml);
  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle LED #");
    Serial.println(led);

    switch (led.toInt()) {
      case 1:
        led1State = !led1State;
        digitalWrite(LED1, led1State);
        Serial.print("Led stat" + led1State);
        
        break;
      case 2:
        led2State = !led2State;
        digitalWrite(LED2, led2State);
        Serial.print("Led stat" + led2State);
        
        break;
    }
    //endppoint para cadastro
    //server.on("/Cad", sendCad);
    sendHtml();
  });

  // Endpoint para laboratorio
 // server.on("/sendLab", sendLab);

   server.on(UriBraces("/CadastroDigital/PessoaId{}"), HTTP_GET, []() {
    Serial.println("Cadastrando a digital");

    // Captura o ID da URL
    String ID = server.pathArg(0);
    Serial.print("ID: ");
    Serial.println(ID);

    // acender um led indicando pra pessoa colocar o dedo na funcao de cadastrar na hora que tira e coloca o dedo coloca pra esse msm led piscar ou algo do tipo
    int CadDigital = cadastraDigital();
    if(CadDigital >0){
      Serial.print("mandado digital para o Servidor Externo");

      bool vinculo = httpVincularDigital(ID.toInt(), CadDigital);
      // se falahar na comunicação com o servidor externo a digital que foi cadastrada sera apagada para manter uma atomicidadde
      if(!vinculo){
        deleteDigital(CadDigital);
        DynamicJsonDocument doc(200);
        doc["Erro"] = "Erro a cadastrar digital, tente Novamente";
        // Converta o documento JSON para uma string
        String jsonResponse2;
        serializeJson(doc, jsonResponse2);

        // Envie a resposta JSON
        server.send(404, "application/json", jsonResponse2);
        Serial.println("cadastral cdastrada, mas erro no servidor Exteno, digital apagada do banco de digital");
      }
      // Crie um documento JSON
      DynamicJsonDocument doc(200);
      doc["digital_id"] = CadDigital;
      doc["PessoaId"] = ID;

      // Converta o documento JSON para uma string
      String jsonResponse;
      serializeJson(doc, jsonResponse);

      // Envie a resposta JSON
      server.send(200, "application/json", jsonResponse);
      
      Serial.println("Cadastrou a digital");
    }
    else{
      // Crie um documento JSON
      DynamicJsonDocument doc(200);
      doc["Erro"] = "Erro a cadastrar digital, tente Novamente";
      // Converta o documento JSON para uma string
      String jsonResponse;
      serializeJson(doc, jsonResponse);

      // Envie a resposta JSON
      server.send(404, "application/json", jsonResponse);
      Serial.println("falha ao cadastrar digital");
    }
});

  server.begin();
  Serial.println("HTTP server started");
  //------------------- chamadno funções da api
  httpLogin();
  criarLab("lab 1");

  delay(3000);
 // sendLab();
 //limpaDatabase();
}


//Loop -----------------------------------------------------------------------------------------------
void loop(){

  server.handleClient();
  delay(2);
  int digital = getFingerprintID();

  if (digital > 0){
    registroLog(digital);
    // se quiser coloca um servoMotor pra ele abrir e fechar, seria mas interesante, tem uns video no youtube que explica como usar o servo, é rapidinho
    if(fechadura){
      fechadura = false;
    }else{
      fechadura = true;
    }
  }
                                         
}

// tentativa do codigo de leitura
int getFingerprintID() {
  uint8_t fp = fingerSensor.getImage();
  if (fp != FINGERPRINT_OK)  return -1;

  fp = fingerSensor.image2Tz();
  if (fp != FINGERPRINT_OK)  return -1;

  fp = fingerSensor.fingerFastSearch();
  if (fp != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("\nEncontrado ID #"); Serial.println(fingerSensor.fingerID);
  return fingerSensor.fingerID;
}

 //METODO PARA CIRAR UM LAB --> RETONRNAR UM ENEDERECO IP

  //METODO PARA REGISTRAR UMA PESSOA QUANDO A TRAVA FOR LIBERADA E FOR CADASTTRADO UMA DIGITAL

//Imprime o Menu na Serial ---------------------------------------------------------------------------
void printMenu(){
    Serial.println();
    Serial.println("Digite um dos números do menu abaixo");
    Serial.println("1 - Cadastrar digital");
    Serial.println("2 - Verificar digital");
    Serial.println("3 - Mostrar quantidade de digitais cadastradas");
    Serial.println("4 - Apagar digital em uma posição");
    Serial.println("5 - Apagar banco de digitais");
}

//Pega Comando ---------------------------------------------------------------------------------------
String pegaComando(){
    while(!Serial.available()) delay(100);
    return Serial.readStringUntil('\n');                    // Retorna o que foi digitado na Serial
}

//Cadastra Digital -----------------------------------------------------------------------------------
int cadastraDigital(){
    Serial.println("Qual a posição para guardar a digital? (1 a 120)"); // Imprime na Serial             // Faz a leitura do comando digitado
    
    int location = findNextFreeID();               // Transforma em inteiro
    if (location == -1) {
        Serial.println("Não há posições livres para cadastrar uma nova digital.");
        return 0;
    }

    Serial.print("A próxima posição livre é: ");
    Serial.println(location);


    if(location < 1 || location > 120){                     // Verifica se a posição é invalida
        Serial.println("Posição inválida");                 // Imprime na Serial
        return 0;
    }

    Serial.println("Encoste o dedo no sensor");             // Imprime na Serial
    while (fingerSensor.getImage() != FINGERPRINT_OK){};    // Espera até pegar uma imagem válida
    
    if (fingerSensor.image2Tz(1) != FINGERPRINT_OK){        // Converte a imagem para primeiro padrão
        Serial.println("Erro image2Tz 1");                  // Erro de Imagem
        return 0;
    }
    
    Serial.println("Tire o dedo do sensor");                // Imprime na Serial
    delay(2000);                                            // Espera 2s
    while (fingerSensor.getImage() != FINGERPRINT_NOFINGER){}; // Espera até tirar o dedo
    Serial.println("Encoste o mesmo dedo no sensor");       // Imprime na Serial
    while (fingerSensor.getImage() != FINGERPRINT_OK){};    // Espera até pegar uma imagem válida

    if(fingerSensor.image2Tz(2) != FINGERPRINT_OK){         // Converte a imagem para o segundo padrão
        Serial.println("Erro image2Tz 2");                  // Erro de Imagem
        return 0;
    }

    if(fingerSensor.createModel() != FINGERPRINT_OK){       // Cria um modelo com os dois padrões
        Serial.println("Erro createModel");                 // Erro para criar o Modelo
        return 0;
    }

    if(fingerSensor.storeModel(location) != FINGERPRINT_OK){// Guarda o modelo da digital no sensor
        Serial.println("Erro já existe uma biométrica cadastrada"); // Já existe 
        return 0;
    }

    Serial.print("Sucesso!!!Digital cadastrada na posicao"); 
    Serial.println(location);                          // Guardada com sucesso
    return location;
}

int findNextFreeID() {
    const int maxID = 169;
    for (int id = 1; id <= maxID; id++) {
        if (fingerSensor.loadModel(id) != FINGERPRINT_OK) {
            return id;
        }
    }
    return -1;
}

//Verifica Digital -----------------------------------------------------------------------------------
void verificaDigital(){
    Serial.println("Encoste o dedo no sensor");             // Imprime na Serial
    while (fingerSensor.getImage() != FINGERPRINT_OK){};    // Espera até pegar uma imagem válida

    if (fingerSensor.image2Tz() != FINGERPRINT_OK){         // Converte a imagem para utilizar
        Serial.println("Erro image2Tz");                    // Imprime na Serial
        return;
    }

    if (fingerSensor.fingerFastSearch() != FINGERPRINT_OK){ // Procura pela digital no banco de dados
        Serial.println("Digital não encontrada");           // Imprime na Serial
        return;
    }

    Serial.print("Digital encontrada com confiança de ");   // Se chegou aqui a digital foi encontrada
    Serial.print(fingerSensor.confidence);                  // Imprime a confiança quanto maior melhor
    Serial.print(" na posição ");                           // Imprime a posição da digital
    Serial.println(fingerSensor.fingerID);                  // Imprime a posição da digital
}

//Conta Qtde de Digitais -----------------------------------------------------------------------------
void contDigital(){
    fingerSensor.getTemplateCount();                        // Atribui a "templateCount" a qtde
    Serial.print("Digitais cadastradas: ");                 // Imprime na Serial
    Serial.println(fingerSensor.templateCount);             // Imprime na Serial a quantidade salva
}

//Deleta uma Digital ---------------------------------------------------------------------------------
void deleteDigital(int digital){
   
    int location = digital;                   // Transforma em inteiro

    if(location < 1 || location > 149){                     // Verifica se a posição é invalida
        Serial.println("Posição inválida");                 // Imprime na Serial
        return;
    }

    if(fingerSensor.deleteModel(location) != FINGERPRINT_OK){ // Apaga a digital na posição location
        Serial.println("Erro ao apagar digital");           // Imprime na Serial
    }
    else{
        Serial.println("Digital apagada com sucesso!!!");   // Imprime na Serial
    }
}

//Limpa Banco de Dados -------------------------------------------------------------------------------
void limpaDatabase(){
    Serial.println("Tem certeza? (S/N)");                   // Imprime na Serial
    String command = pegaComando();                         // Faz a leitura do comando digitado
    command.toUpperCase();                                  // Coloca tudo em maiúsculo

    if(command == "S" || command == "SIM"){                 // Verifica se foi digitado "S" ou "SIM"
        Serial.println("Apagando banco de digitais...");    // Imprime na Serial

        if(fingerSensor.emptyDatabase() != FINGERPRINT_OK){ // Apaga todas as digitais
            Serial.println("Erro ao apagar banco de digitais"); // Imprime na Serial
        }
        else{
            Serial.println("Banco de digitais apagado com sucesso!!!"); // Imprime na Serial
        }
    }
    else{
        Serial.println("Cancelado");                        // Imprime na Serial
    }
}
// METODO PARA QUANDO ALGUEM ADCIONAR UMA DIGITAL RETORNE UM OK O NUMERO DA DIGITAL E O NUMERO DO LAB
// CRIAR UM METODO PARA CRIAR UM LABORATORIO

// solicitar uma digital
// fazer um metodo para pegar o ID da digital
// retornar o id da digital


// Pagina HTML ---------------------------------------------------------------------------------------
void sendHtml() {
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>LED 1</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>
        </div>
      </body>
    </html>
  )";
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");
  server.send(200, "text/html", response);
}

//  Cliente requisição -------------------------------------------------------------------------------
void httpLogin() {
    String serverUrl = "http://" + ipServidorExterno + ":8080/auth/login";
    const char* serverUrlCStr = serverUrl.c_str();
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Criando o JSON para enviar no corpo da requisição
    String jsonPayload = "{\"login\":\"adm\", \"pass\":\"1234\"}";

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
        Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
        String payload = http.getString();
        Serial.println("Response payload:");
        Serial.println(payload);

        // Processar o payload da resposta
        if (payload.length() > 0) {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            const char* token = doc["token"];
            if (token) {
                globalToken = String(token);
                Serial.print("Extracted token: ");
                Serial.println(globalToken);
            } else {
                Serial.println("Token not found in response.");
            }
        } else {
            Serial.println("Response payload is empty.");
        }
    } else {
        Serial.printf("Error code: %d\n", httpResponseCode);
    }

    http.end();
}

bool httpVincularDigital(int pessoaId, int digitalId) {
    String serverUrl = "http://" + ipServidorExterno + ":8080/pessoa/cadastrarDigital";
    const char* serverUrlCStr = serverUrl.c_str();

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + globalToken);

    // Criando o JSON para enviar no corpo da requisição
    String jsonPayload = "{\"digitalId\":" + String(digitalId) + 
                        ",\"pessoaId\":" + String(pessoaId) + 
                        ",\"labId\":" + String(labId) + "}";

    int httpResponseCode = http.PUT(jsonPayload);

    if (httpResponseCode >= 200 || httpResponseCode <= 300) {
        Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
        String payload = http.getString();
        Serial.println("Response payload:");
        Serial.println(payload);
        http.end();
        return true; 
    } else {
        Serial.printf("Error code: %d\n", httpResponseCode);
        http.end();
        return false; 
    }
    
}


void registroLog(int digitalId) {
    String serverUrl = "http://" + ipServidorExterno + ":8080/espacoLog/registro";
    const char* serverUrlCStr = serverUrl.c_str();

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + globalToken);
    String jsonPayload;
    // Criando o JSON para enviar no corpo da requisição
    if(fechadura){
        jsonPayload = "{\"digitalId\":" + String(digitalId) + 
                        ",\"espacoId\":" + String(labId) + 
                        ",\"tipoAbertura\":" + String(1) + "}";
    }else{
      jsonPayload = "{\"digitalId\":" + String(digitalId) + 
                        ",\"espacoId\":" + String(labId) + 
                        ",\"tipoAbertura\":" + String(0) + "}";
    }


    int httpResponseCode = http.POST(jsonPayload);

    Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
    String payload = http.getString();
    Serial.println("Response payload:");
    Serial.println(payload);

    http.end();
}

void criarLab(String nomeLab) {
    String serverUrl = "http://" + ipServidorExterno + ":8080/espaco/registro";
    const char* serverUrlCStr = serverUrl.c_str();

    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + globalToken);

    // Converte o endereço IP para uma string
    String ipAddress = WiFi.localIP().toString();

    // Criando o JSON para enviar no corpo da requisição
    String jsonPayload = "{\"nome\":\"" + nomeLab + "\", \"EnderecoIp\":\"" + ipAddress + "\"}";

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
        Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
        String responsePayload = http.getString();
        Serial.println("Response payload:");
        Serial.println(responsePayload);

        // Parseando a resposta JSON para extrair o campo 'id'
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, responsePayload);

        if (!error) {
            if (doc.containsKey("id")) {
                labId = doc["id"];
                Serial.printf("ID recebido: %d\n", labId);
            } else {
                Serial.println("Campo 'id' não encontrado na resposta JSON.");
            }
        } else {
            Serial.println("Falha ao parsear o JSON de resposta:");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("Error code: %d\n", httpResponseCode);
    }

    http.end();
}
/*
void sendCad() {
  server.send(201, "text/plain", "Lab criado", response);
}
*/

void loop() {
    // put your main code here, to run repeatedly:   
    
}
