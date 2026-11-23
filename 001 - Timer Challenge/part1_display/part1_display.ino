// The display.

#include <TM1637Display.h>

// Pins
#define CLK_PIN     5
#define DIO_PIN     18

// Setup display
TM1637Display display(CLK_PIN, DIO_PIN);

void setup() {
  Serial.begin(115200);
  display.setBrightness(7);
  showTime(0);
  Serial.println("Ready");
}

void loop() {
  showTime(millis());
}

// Format and display time
void showTime(uint32_t ms) {
  int hundredths = min(ms / 10, (uint32_t)9999);
  display.showNumberDecEx(hundredths, 0b01000000, true);  // colon bit lights the dot: 12.34
}
