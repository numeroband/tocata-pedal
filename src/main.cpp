#include "controller.h"

#include <Arduino.h>

static tocata::Controller controller{
  {18, 5, 17}, // ins
  {19, 16}     // outs
};

void setup() {
  delay(1000);
  Serial.begin(115200);
	SPIFFS.begin();
  controller.begin();
}

void loop() {
  controller.loop();
}