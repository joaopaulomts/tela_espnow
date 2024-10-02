#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "SANDRO";
const char* password = "1335555#";

AsyncWebServer server(80);

const int ledPin = 21;
const int triggerPin = 19;
const int echoPin = 18;

float distance = 0.0;
const float distanceLimit = 15.0;
int detectionCount = 0;
bool objetoDetectado = false;
unsigned long lastDetectionTime = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW Distance Sensor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    h1 { color: #333; }
    .led-status { margin: 20px; font-size: 24px; }
    .alert { color: red; font-size: 18px; }
  </style>
</head>
<body>
  <h1>Production Line Monitoring</h1>
  <p class="led-status">Distance: <span id="distance">0</span> cm</p>
  <p class="led-status">Object Detections: <span id="detections">0</span></p>
  <p class="led-status">Time Since Last Detection: <span id="timer">0</span> seconds</p>
  <p class="alert" id="alert" style="display:none;">Alert: Possible issue with the production line!</p>

  <script>
    // Variáveis para controle do cronômetro
    var lastDetectionTime = Date.now();
    var alertTimeout = 10000; // 10 segundos
    var timerInterval = setInterval(updateTimer, 1000); // Atualizar a cada segundo

    // Função para atualizar a interface com a distância, detecções e tempo
    function updateData() {
      fetch('/getStatus')
        .then(response => response.json())
        .then(data => {
          document.getElementById('distance').innerHTML = data.distance;
          document.getElementById('detections').innerHTML = data.detectionCount;

          // Reseta o cronômetro se um objeto for detectado
          if (data.objetoDetectado) {
            lastDetectionTime = Date.now();
            document.getElementById('alert').style.display = 'none';  // Esconde o alerta
          }
        });
    }

    // Função para atualizar o cronômetro
    function updateTimer() {
      var timeSinceLastDetection = Math.floor((Date.now() - lastDetectionTime) / 1000);
      document.getElementById('timer').innerHTML = timeSinceLastDetection;

      // Exibir alerta se passar mais de 10 segundos sem detecção
      if (timeSinceLastDetection >= 10) {
        document.getElementById('alert').style.display = 'block';
      }
    }

    // Atualiza os dados da distância e do contador a cada 1 segundo
    setInterval(updateData, 1000);
  </script>
</body>
</html>)rawliteral";

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Serial.print("Bytes received: ");
  Serial.println(len);
}
  
float measureDistance(){

  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

void setup(){

  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  if(esp_now_init() != ESP_OK){
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/getStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    distance = measureDistance();
    if(distance < distanceLimit){
      digitalWrite(ledPin, HIGH);
      if(!objetoDetectado){
        objetoDetectado = true;
        detectionCount++;
        lastDetectionTime = millis();
      }
    }else{
      digitalWrite(ledPin, LOW);
      objetoDetectado = false;
    }
    delay(250);

    String json = "{\"distance\": " + String(distance) + ", \"detectionCount\": " + String(detectionCount) + ", \"objetoDetectado\": " + String(objetoDetectado ? "true" : "false") + "}";
    request->send(200, "application/json", json);
  });
  server.begin();
}

void loop(){

  distance = measureDistance();

  if(distance < distanceLimit){
    digitalWrite(ledPin, HIGH);
    if(!objetoDetectado){
      objetoDetectado = true;
      detectionCount++;
      lastDetectionTime = millis();
    }
  }else{
    digitalWrite(ledPin, LOW);
    objetoDetectado = false;
  }
  delay(250);
}