#include <ArduinoJson.h>

double valores[8];

void setup() {
  Serial1.begin(9600, SERIAL_8N1, 16, 17); // Inicia la comunicación serial a 115200 bps
  Serial.begin(115200);
}

void loop() {
  if (Serial1.available() > 0) {
    // Lee la línea de datos recibida hasta encontrar un salto de línea
    String data = Serial1.readStringUntil('\n');

    // Verifica si los datos comienzan con un delimitador o señal de inicio
    if (data.startsWith("{")) {
      // Crear un objeto JSON
      StaticJsonDocument<200> jsonDocument;

      // Deserializar la cadena JSON recibida
      deserializeJson(jsonDocument, data);

      // Obtener los valores double
      double valores[8];
      for (int i = 0; i < 8; i++) {
        char key[6]; // Tamaño suficiente para almacenar "datoX"
        sprintf(key, "dato%d", i + 1);
        valores[i] = jsonDocument[key];
      }

      // Ahora tienes los 8 valores en el array 'valores'
      // Haz lo que necesites con estos valores
      for (int i = 0; i < 8; i++) {
        Serial.print(valores[i]);
        Serial.print(",");
      }
      Serial.println(""); // Imprime un salto de línea al final

    } else {
      // Datos incorrectos, imprime un mensaje de error
      Serial.println("Error: Datos incorrectos recibidos");
    }
  }
}
