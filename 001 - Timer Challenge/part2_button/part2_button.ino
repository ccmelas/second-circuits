// The button.
// Compares polling the button in loop() with catching it in an interrupt.

#include <TM1637Display.h>

// Pins
#define CLK_PIN     5
#define DIO_PIN     18
#define BUTTON_PIN  23  // other leg to GND; the internal pull-up means no resistor

// Setup display
TM1637Display display(CLK_PIN, DIO_PIN);

// Button Down Interrupt
// Demo only: printing from an interrupt is not safe on the ESP32 and can crash it,
// especially when a bouncy press fires this several times. Part 4 does it properly:
// the interrupt only records the time, and loop() does the rest.
void IRAM_ATTR onButtonDown() {
  Serial.println("Button pressed outside of loop");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonDown, FALLING);
  display.setBrightness(7);
  showTime(0);
  Serial.println("Ready");
}

void loop() {
  showTime(millis());
  handleButton();
  // On purpose: polling only every 3 s misses quick presses,
  // while the interrupt above still catches them.
  delay(3000);
  Serial.println("Loop complete");
}

// handle button state
void handleButton() {
  bool isPressed = digitalRead(BUTTON_PIN) == LOW;  // pull-up: pressed reads LOW
  if (isPressed) {
    Serial.println("Button is pressed");
  } else {
    Serial.println("Button is not pressed");
  }
}

// Format and display time
void showTime(uint32_t ms) {
  int hundredths = min(ms / 10, (uint32_t)9999);
  display.showNumberDecEx(hundredths, 0b01000000, true);
}
