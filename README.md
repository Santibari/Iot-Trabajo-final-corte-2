# Iot-Trabajo-final-corte-2

## Inegrantes
- Santiago Bazzani Rincon 
- Juan David Heano 

# ESP32-Carro-HTTP-MQTT

## Descripción del Proyecto
**ESP32-Carro-HTTP-MQTT** es un proyecto académico que simula el control de un carro mediante un **servidor HTTP** y **publicación en MQTT**, utilizando un **ESP32** como unidad principal.  

El sistema permite enviar comandos al ESP32 a través de peticiones HTTP para simular movimientos como *adelante*, *atrás*, *izquierda* o *derecha*, con parámetros de velocidad y duración.  
Aunque los pines para motores y LEDs están definidos, **no existe conexión física**, ya que se trata de una **prueba de simulación lógica**.  

Los resultados del control se observan en el **monitor serial**, junto con la publicación de datos en formato JSON hacia un **broker MQTT público**.

---

## Conceptos Clave

### Servidor HTTP
El ESP32 actúa como un **servidor HTTP local**, capaz de procesar peticiones GET en dos rutas principales:

- `/move`: recibe parámetros (`direccion`, `velocidad`, `duracion`) y simula el movimiento.
- `/status`: devuelve el estado actual del servidor.

Para gestionar la duración del movimiento, se emplea un **temporizador no bloqueante** mediante la función `millis()`, evitando el uso de `delay()` y permitiendo mantener la respuesta del servidor activa mientras se ejecuta una simulación de movimiento.

El servidor responde con datos en formato **JSON**, informando sobre el éxito o error de la operación.

---

### MQTT
**MQTT (Message Queuing Telemetry Transport)** es un protocolo de mensajería **ligero y eficiente**, ideal para entornos **IoT (Internet of Things)**.  

El ESP32 utiliza la biblioteca `PubSubClient` para conectarse al broker público:

- **Broker:** `test.mosquitto.org`
- **Puerto:** `1883`
- **Tópico:** `carro/movimiento`

Cada vez que el servidor HTTP recibe una orden de movimiento, el ESP32 **publica un mensaje JSON** en el tópico MQTT, conteniendo información del cliente, dirección, velocidad y duración.  

Este mecanismo permite **monitorear remotamente** las acciones del carro desde cualquier cliente MQTT suscrito al mismo tópico.

## Retardo exponencial con jitter

Cuando un dispositivo intenta reconectarse (por ejemplo al broker MQTT o a una red WiFi) y falla, no conviene reconectarse inmediatamente muchas veces seguidas, porque eso puede saturar la red o el servidor.

Por eso se usa el retardo exponencial con jitter, que:

- Duplica el tiempo de espera entre intentos fallidos (exponencial).

- Añade una pequeña cantidad aleatoria (jitter) para evitar que varios dispositivos fallen y se reconecten todos al mismo tiempo.

La simulación de respuesta fue implementada dentro del bloque handleControl() del servidor HTTP, justo después de interpretar los comandos de dirección y velocidad. En esta sección, además de mostrar los mensajes en el monitor serial, se añadió la estructura JSON "response" que simula la salida esperada del servidor, indicando el estado del carro con los campos "direction" y "speed", permitiendo visualizar la confirmación del comando recibido como si se tratara de una interacción real con un servidor remoto.

**Donde se uso:**
```json
void handleControl() {
  String direction = server.arg("direction");
  String speed = server.arg("speed");

  Serial.println("Comando recibido:");
  Serial.println("Dirección: " + direction);
  Serial.println("Velocidad: " + speed);

  // Simulación de respuesta JSON
  String response = "{ \"response\": { \"direction\": \"" + direction + "\", \"speed\": \"" + speed + "\" } }";
  server.send(200, "application/json", response);
}
```

---

### Simulación del Carro
El sistema no controla motores ni componentes físicos.  
Los movimientos se simulan mediante **mensajes impresos en el monitor serial**, por ejemplo:

### Diagrama De secuencias

![Diagrama de Secuencias](/img/IOT.jpg)

---

## Resultados Optenidos y pruebas hechas:

**Antes de iniciar:** Verificamos que si se pudo contectar el microcontrolador al wifi

![Prueba1](/img/Img1.png)

**Prueba en Postman:**
![Prueba1](/img/Img2.png)

**Prueba de Movimiento:**
![Prueba1](/img/Img3.png)
Resultado: 

[HTTP] Movimiento recibido: adelante - Velocidad: 150 - Duración: 3
[MQTT] Publicado en carro/movimiento -> {"direccion":"adelante","velocidad":150,"duracion":3,"ip_cliente":"192.168.1.10"}

**Prueba MQTT con Mosquitto (usando Postman):**
Usamos una maquina para correr Mosquitto
![Prueba1](/img/Img4.png)

- http://192.168.101.111/status

![Prueba1](/img/prueba1.png)

**prueba 2:**
- http://192.168.101.111/move?direccion=adelante&velocidad=200&duracion=3

![Prueba1](/img/prueba2.png)

---

## Resultados optenidos: 

# Pruebas con Postman y MQTT

Durante la etapa de validación del proyecto, se realizaron pruebas para verificar la correcta comunicación entre el **ESP32**, el **servidor HTTP**, y el **broker MQTT**.  
El objetivo fue confirmar que los comandos enviados desde un cliente (Postman o aplicación MQTT) fueran recibidos, procesados y publicados correctamente en los tópicos correspondientes.

## Pruebas HTTP con Postman

Se utilizaron peticiones `POST` desde **Postman** para enviar comandos de movimiento al servidor web del ESP32.

### Ejemplo de solicitud
```json
{
  "direccion": "izquierda",
  "velocidad": 120,
  "duracion": 3
}
```

### Respuesta simulada esperada
```json
{
  "status": "OK",
  "mensaje": "Movimiento ejecutado correctamente",
  "detalle": {
    "direccion": "izquierda",
    "velocidad": 120,
    "duracion": 3,
    "publicado_MQTT": true
  }
}
```

### Explicación técnica
- El **ESP32** recibe el cuerpo JSON con los parámetros del movimiento.
- Procesa la dirección y velocidad, y lo muestra por el **Monitor Serial**.
- Luego, publica el mismo mensaje en el **tópico MQTT** correspondiente (`/carro/movimiento`).
- La respuesta en Postman confirma que el mensaje fue recibido y reenviado al broker MQTT con éxito.

---

## Pruebas de comunicación MQTT

Se utilizó un cliente MQTT (como **MQTT Explorer** o **MQTTX**) para suscribirse al tópico:

```
/carro/movimiento
```

### Mensaje recibido (simulado)
```json
{
  "direccion": "izquierda",
  "velocidad": 120,
  "duracion": 3
}
```

### Respuesta en el monitor serial del ESP32
```
[HTTP] Comando recibido vía POST
Dirección: izquierda
Velocidad: 120
Duración: 3 segundos
[MQTT] Publicado en tópico /carro/movimiento
```

### Explicación técnica
- Cada vez que Postman envía un comando, el ESP32 lo reenvía al broker MQTT.
- El cliente suscrito visualiza los mismos datos en tiempo real.
- Esto demuestra la integración entre el **servidor HTTP (control local)** y **el servicio MQTT (comunicación remota o IoT)**.

---

# Resultados obtenidos

- Se verificó que las peticiones **HTTP POST** desde Postman son correctamente interpretadas por el ESP32.  
- Los datos se muestran en el **monitor serial** con los parámetros de dirección, velocidad y duración.  
- Cada comando se publica automáticamente en el **broker MQTT**, siendo visible en el tópico `/carro/movimiento`.  
- Las simulaciones confirman el flujo completo de datos: **cliente → ESP32 (HTTP) → broker MQTT → suscriptores**.

---

# Conclusión

El proyecto demuestra exitosamente la integración de tecnologías **HTTP y MQTT** sobre un **ESP32**, logrando la simulación de control de movimiento de un vehículo a través de peticiones remotas.  
Las pruebas con **Postman** y el cliente **MQTT** evidencian una comunicación estable y una correcta publicación de datos, validando la arquitectura IoT propuesta.  
Este resultado confirma que el sistema es funcional, escalable y aplicable a entornos de control remoto o automatización.
