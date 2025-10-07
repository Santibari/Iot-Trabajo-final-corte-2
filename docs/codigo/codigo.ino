#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// ==========================
// CONFIGURACIÓN DE RED Wi-Fi
// ==========================
// Por temas de seguridad dejamos estos campos vacios
const char* ssid = ""; // Aqui se pone tu red a usar
const char* password = ""; // Aqui pones tu contraseña

// ==========================
// CONFIGURACIÓN MQTT (Mosquitto)
// ==========================
// Servidor MQTT (Mosquitto público)
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "carro/movimiento";


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

  // Detener el carro después del tiempo
  delay(duracion * 1000);
  stopCar();
}

void handleStatus() {
  server.send(200, "application/json", "{\"status\":\"Servidor operativo\"}");
}

// ==========================
// CONEXIÓN Wi-Fi Y MQTT
// ==========================
void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32Carro")) {
      Serial.println("Conectado a servidor MQTT");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);

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

  // Conectar a Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
  Serial.println(WiFi.localIP());

  // MQTT
  client.setServer(mqtt_server, mqtt_port);

  // Endpoints
  server.on("/move", handleMove);
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

// ==========================
// LOOP
// ==========================
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();
  server.handleClient();
}
