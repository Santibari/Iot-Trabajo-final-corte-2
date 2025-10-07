# Iot-Trabajo-final-corte-2

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

---

### Simulación del Carro
El sistema no controla motores ni componentes físicos.  
Los movimientos se simulan mediante **mensajes impresos en el monitor serial**, por ejemplo:

## Resultados Optenidos y pruebas hechas:

prueba 1:
http://192.168.101.111/status
![Prueba1](/img/prueba1.png)


prueba 2: 
http://192.168.101.111/move?direccion=adelante&velocidad=200&duracion=3
![Prueba1](/img/prueba2.png)
