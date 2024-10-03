#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>

const char* ssid = "SANDRO";
const char* password = "1335555#";

AsyncWebServer server(80);

int contadorPecas = 0;
bool alertaAtivo = false;
unsigned long temporizador = 0;
unsigned long objetoAnterior = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Monitor de Linha de Producao</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; }
    .status { font-size: 24px; }
    .alert { color: red; display: none; }
  </style>
</head>
<body>
  <h1>Monitor de Linha de Producao</h1>
  <p class="status">Pecas detectadas: <span id="detections">0</span></p>
  <p class="status">Ultima peca detectada: <span id="timer">0</span> segundos</p>
  <p class="alert" id="alert">ALERTA! Possivel problema na linha de producao</p>

  <script>
    function updateData() {
      fetch('/getStatus')
        .then(response => response.json())
        .then(data => {
          document.getElementById('detections').innerHTML = data.contadorPecas;
          document.getElementById('timer').innerHTML = data.tempoUltimaDeteccao;
          document.getElementById('alert').style.display = data.alertaAtivo ? 'block' : 'none';
        });
    }
    setInterval(updateData, 1000);
  </script>
</body>
</html>)rawliteral";

// Estrutura para receber os dados
typedef struct struct_message {
  int contadorPecas;
  unsigned long temporizador;
  bool alertaAtivo;
} struct_message;

struct_message receivedData;

// Callback de recepção
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  // Atualiza os dados recebidos
  contadorPecas = receivedData.contadorPecas;
  temporizador = receivedData.temporizador;
  alertaAtivo = receivedData.alertaAtivo;
  objetoAnterior = millis();
}

void setup() {
  Serial.begin(115200);

  // Configura o Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Configura o servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/getStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"contadorPecas\": " + String(contadorPecas) +
                  ", \"tempoUltimaDeteccao\": " + String(temporizador) +
                  ", \"alertaAtivo\": " + String(alertaAtivo ? "true" : "false") + "}";
    request->send(200, "application/json", json);
  });

  server.begin();

  // Inicializa o ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar o ESP-NOW");
    return;
  }

  // Registra o callback de recepção
  esp_now_register_recv_cb(onReceive);
}

void loop() {
  // Nenhuma ação necessária no loop
}