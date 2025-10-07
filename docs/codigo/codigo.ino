#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// ==========================
// CONFIGURACIÓN DE RED Wi-Fi
// ==========================
// Por seguridad no incluyo mis credenciales aquí
const char* ssid = "";      // Aquí tu red WiFi
const char* password = "";  // Aquí tu contraseña

// ==========================
// CONFIGURACIÓN MQTT (Mosquitto)
// ==========================
const char* mqtt_server = "";   // Dirección del broker MQTT
const int mqtt_port = ;         // Puerto del broker
const char* mqtt_topic = "";    // Tópico a publicar

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

// ==========================
// PINES DE MOTORES Y LEDs
// ==========================
int IN1 = 25;
int IN2 = 26;
int IN3 = 18;
int IN4 = 19;

int ledFrontLeft  = 22;
int ledFrontRight = 32;
int ledBackLeft   = 23;
int ledBackRight  = 33;

// ==========================
// FUNCIONES DE CONTROL
// ==========================
void stopCar() {
  ledcWrite(IN1, 0);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, 0);
  digitalWrite(ledFrontLeft, LOW);
  digitalWrite(ledFrontRight, LOW);
  digitalWrite(ledBackLeft, LOW);
  digitalWrite(ledBackRight, LOW);
}

void moveForward(int speed) {
  ledcWrite(IN1, speed);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, speed);
  ledcWrite(IN4, 0);
  digitalWrite(ledFrontLeft, HIGH);
  digitalWrite(ledFrontRight, HIGH);
}

void moveBackward(int speed) {
  ledcWrite(IN1, 0);
  ledcWrite(IN2, speed);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, speed);
  digitalWrite(ledBackLeft, HIGH);
  digitalWrite(ledBackRight, HIGH);
}

void turnLeft(int speed) {
  ledcWrite(IN1, 128);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, speed);
  ledcWrite(IN4, 0);
  digitalWrite(ledFrontLeft, HIGH);
}

void turnRight(int speed) {
  ledcWrite(IN1, speed);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 128);
  ledcWrite(IN4, 0);
  digitalWrite(ledFrontRight, HIGH);
}

// ==========================
// ENDPOINTS HTTP
// ==========================
void handleMove() {
  if (!server.hasArg("direccion") || !server.hasArg("velocidad") || !server.hasArg("duracion")) {
    server.send(400, "application/json", "{\"error\":\"Faltan parámetros\"}");
    return;
  }

  String direccion = server.arg("direccion");
  int velocidad = server.arg("velocidad").toInt();
  int duracion = server.arg("duracion").toInt();
  if (duracion > 5) duracion = 5; // límite de seguridad

  String ipCliente = server.client().remoteIP().toString();

  // Ejecutar movimiento
  if (direccion == "adelante") moveForward(velocidad);
  else if (direccion == "atras") moveBackward(velocidad);
  else if (direccion == "izquierda") turnLeft(velocidad);
  else if (direccion == "derecha") turnRight(velocidad);
  else {
    server.send(400, "application/json", "{\"error\":\"Dirección inválida\"}");
    return;
  }

  // Publicar en MQTT
  String mensaje = "{\"cliente\":\"" + ipCliente + "\",\"direccion\":\"" + direccion +
                   "\",\"velocidad\":" + String(velocidad) + ",\"duracion\":" + String(duracion) + "}";
  client.publish(mqtt_topic, mensaje.c_str());

  // Respuesta HTTP
  server.send(200, "application/json", "{\"status\":\"Movimiento ejecutado\"}");

  // Detener el carro después del tiempo indicado
  delay(duracion * 1000);
  stopCar();
}

void handleStatus() {
  server.send(200, "application/json", "{\"status\":\"Servidor operativo\"}");
}

// ==========================
// FUNCIÓN DE CONEXIÓN WiFi CON RETARDO EXPONENCIAL + JITTER
// ==========================
void connectWiFi() {
  int retry = 0;
  int maxRetries = 6;

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");

  while (WiFi.status() != WL_CONNECTED && retry < maxRetries) {
    long baseDelay = pow(2, retry) * 500; // 0.5s, 1s, 2s, 4s, etc.
    long jitter = random(-200, 200);      // +/- 0.2s
    long totalDelay = baseDelay + jitter;

    if (totalDelay < 0) totalDelay = 0;

    delay(totalDelay);
    Serial.print(".");

    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado a WiFi");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Error al conectar a WiFi después de varios intentos");
  }
}

// ==========================
// RECONEXIÓN MQTT CON RETARDO EXPONENCIAL + JITTER
// ==========================
void reconnectMQTT() {
  int retry = 0;
  int maxRetries = 6;  // evitar bucles infinitos

  while (!client.connected() && retry < maxRetries) {
    Serial.print("Intentando conectar al broker MQTT... ");

    if (client.connect("ESP32Carro")) {
      Serial.println("Conectado a servidor MQTT ✅");
      return;
    } else {
      retry++;
      long baseDelay = pow(2, retry) * 1000; // 1s, 2s, 4s, etc.
      long jitter = random(-500, 500);       // +/- 0.5s
      long totalDelay = baseDelay + jitter;

      if (totalDelay < 0) totalDelay = 0;

      Serial.print("Fallo MQTT. Reintentando en ");
      Serial.print(totalDelay / 1000.0);
      Serial.println(" segundos...");
      delay(totalDelay);
    }
  }

  if (!client.connected()) {
    Serial.println("❌ No se pudo conectar al broker MQTT tras varios intentos.");
  }
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); // inicializar aleatoriedad

  // Configurar pines
  pinMode(ledFrontLeft, OUTPUT);
  pinMode(ledFrontRight, OUTPUT);
  pinMode(ledBackLeft, OUTPUT);
  pinMode(ledBackRight, OUTPUT);

  ledcAttach(IN1, 1000, 8);
  ledcAttach(IN2, 1000, 8);
  ledcAttach(IN3, 1000, 8);
  ledcAttach(IN4, 1000, 8);
  stopCar();

  // Conectar WiFi con retardo exponencial + jitter
  connectWiFi();

  // Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);

  // Configurar endpoints HTTP
  server.on("/move", handleMove);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Servidor HTTP iniciado ✅");
}

// ==========================
// LOOP PRINCIPAL
// ==========================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();
  server.handleClient();
}
