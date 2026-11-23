// The buzzer.
// Checks the buzzer works: a beep every loop, and a beep on each press.

#include <TM1637Display.h>

// Pins
#define CLK_PIN     5
#define DIO_PIN     18
#define BUZZER_PIN  19  // passive buzzer, so tone() can play different pitches
#define BUTTON_PIN  23

// Setup display
TM1637Display display(CLK_PIN, DIO_PIN);

// Button Down Interrupt
// Demo only: tone() and Serial are not safe to call from an interrupt on the ESP32.
// tone() hands the note to a background task through a queue, and doing that from here
// can stop the board with an assert. Part 4 fixes it: the interrupt records the press,
// and loop() decides when to beep.
void IRAM_ATTR onButtonDown() {
  tone(BUZZER_PIN, 2000, 60);
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
  tone(BUZZER_PIN, 2000, 60);  // test beep, so you can hear the buzzer is wired right
  delay(3000);
}

// handle button state
void handleButton() {
  bool isPressed = digitalRead(BUTTON_PIN) == LOW;
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
