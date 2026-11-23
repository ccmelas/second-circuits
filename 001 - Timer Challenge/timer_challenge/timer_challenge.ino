// Timer challenge.
// One button: tap = start / pause / continue, hold 1 s = reset.

#include <TM1637Display.h>

// Pins
#define CLK_PIN     5   // GPIO 5 is a boot strapping pin; fine here, but 4, 13, 16 or 17 are safer picks
#define DIO_PIN     18
#define BUZZER_PIN  19
#define BUTTON_PIN  23

// Setup display
TM1637Display display(CLK_PIN, DIO_PIN);
int           shownValue                = -1; // what's currently on the display
const int     DISPLAY_BRIGHTNESS        = 7;  // 0 (dimmest) to 7 (brightest)

// Timing
const uint32_t DEBOUNCE_MS   = 20;     // a press must stay down this long to count
const uint32_t RELEASE_MS    = 30;     // the button must stay up this long to count as released
const uint32_t LONG_PRESS_MS = 1000;   // hold this long to reset
const uint32_t MAX_TIME_MS   = 99990;  // the display tops out at 99.99

// Feedback tones
const int TONE_HIGH       = 2000;  // start / continue
const int TONE_LOW        = 1000;  // pause
const int TONE_RESET      = 500;   // reset
const int BEEP_MS         = 60;    // length of a normal beep
const int RESET_BEEP_MS   = 300;   // length of the reset tone

// Game State
enum class GameState { Ready, Running, Paused };
GameState gameState       = GameState::Ready;
uint32_t accumulatedMs    = 0;  // total from finished segments
uint32_t segmentStartMs   = 0;  // when the current segment began

// Game event lets us track what the game did
enum class GameEvent { None, Started, Paused, Continued, Reset, MaxedOut };

// Button state
// Set by the interrupt the instant the button goes down, so the timestamp
// stays accurate even if the loop is busy.
volatile bool     pressPending      = false;
volatile uint32_t pressTimeMs       = 0;
bool              pausedOnPress     = false;  // this press already paused the timer, so its tap is used up
bool              held              = false;  // a confirmed press is still being held
bool              longPressReported = false;  // already reported a long press for this hold
uint32_t          holdStartMs       = 0;      // when the current press went down
uint32_t          lastDownMs        = 0;      // last time we saw the button down

// Button event lets us know what the button did
enum class ButtonEvent {
  None,
  Press,      // just went down (reported straight away)
  Tap,        // released before a long press
  LongPress   // held for LONG_PRESS_MS
};

// Reports a button event and when it occurred
struct ButtonReport {
  ButtonEvent event;
  uint32_t    time;
};

// Button Down Interrupt
// Only records the time. Printing and beeping happen in loop(), where they are safe.
void IRAM_ATTR onButtonDown() {
  if (!pressPending) {          // keep the first edge, ignore bounce after it
    pressTimeMs  = millis();
    pressPending = true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonDown, FALLING);
  display.setBrightness(DISPLAY_BRIGHTNESS);
  showTime(0);
  Serial.println("Ready");
}

void loop() {
  uint32_t now = millis();

  // The button reports, the game decides
  ButtonReport button = readButton(now);
  switch (button.event) {
    case ButtonEvent::Press:     announce(onPress(button.time)); break;
    case ButtonEvent::Tap:       announce(onTap(button.time));   break;
    case ButtonEvent::LongPress: announce(resetTimer());         break;
    case ButtonEvent::None:                                      break;
  }

  announce(checkOverflow(now));
  showTime(elapsedMs(now));
}

// Reads a button and reports its state
ButtonReport readButton(uint32_t now) {
  bool isDown = (digitalRead(BUTTON_PIN) == LOW);
  if (isDown) lastDownMs = now;

  // 1. The interrupt caught a press: confirm it's real before reporting it
  if (pressPending) {
    uint32_t t = pressTimeMs;
    if (held) {
      pressPending = false;                       // bounce while holding: ignore
    } else if ((int32_t)(now - t) >= (int32_t)DEBOUNCE_MS) {
      pressPending = false;
      if (isDown) {                               // still down after the debounce window
        held              = true;
        longPressReported = false;
        holdStartMs       = t;
        return { ButtonEvent::Press, t };
      }
    }
  }

  // 2. Still holding: report a long press, once per hold
  if (held && isDown && !longPressReported && now - holdStartMs >= LONG_PRESS_MS) {
    longPressReported = true;
    return { ButtonEvent::LongPress, holdStartMs };
  }

  // 3. Released, and stayed up long enough to rule out bounce
  if (held && !isDown && now - lastDownMs >= RELEASE_MS) {
    held = false;
    if (!longPressReported) {
      return { ButtonEvent::Tap, holdStartMs };   // a short hold counts as a tap
    }
  }

  return { ButtonEvent::None, 0 };
}

// Total time on the clock at "now"
uint32_t elapsedMs(uint32_t now) {
  if (gameState == GameState::Running) return accumulatedMs + (now - segmentStartMs);
  return accumulatedMs;
}

// Handles onPress: pausing happens on the press, so the stop time is exact
GameEvent onPress(uint32_t t) {
  pausedOnPress = (gameState == GameState::Running);
  if (!pausedOnPress) return GameEvent::None;

  accumulatedMs = min(accumulatedMs + (t - segmentStartMs), MAX_TIME_MS);
  gameState = GameState::Paused;
  return GameEvent::Paused;
}

// Handles onTap: starting and continuing wait for the release,
// so holding to reset never starts the clock
GameEvent onTap(uint32_t t) {
  if (pausedOnPress) return GameEvent::None;     // this press already paused the timer

  switch (gameState) {
    case GameState::Ready:
      accumulatedMs  = 0;
      // Timed from when the button went down, so the clock jumps from 00.00
      // to the length of the tap when you let go. That keeps the timing fair.
      segmentStartMs = t;
      gameState = GameState::Running;
      return GameEvent::Started;

    case GameState::Paused:
      segmentStartMs = t;
      gameState = GameState::Running;
      return GameEvent::Continued;

    case GameState::Running:
      return GameEvent::None;                    // handled by onPress()
  }
  return GameEvent::None;
}

// Resets game time
GameEvent resetTimer() {
  gameState = GameState::Ready;
  accumulatedMs = 0;
  return GameEvent::Reset;
}

// Ensures we don't max out the display
GameEvent checkOverflow(uint32_t now) {
  if (gameState == GameState::Running && elapsedMs(now) >= MAX_TIME_MS) {
    accumulatedMs = MAX_TIME_MS;
    gameState = GameState::Paused;
    return GameEvent::MaxedOut;
  }
  return GameEvent::None;
}

// Beeps the buzzer based on a game event
void announce(GameEvent event) {
  switch (event) {
    case GameEvent::Started:
      tone(BUZZER_PIN, TONE_HIGH, BEEP_MS);
      Serial.println("Start");
      break;
    case GameEvent::Continued:
      tone(BUZZER_PIN, TONE_HIGH, BEEP_MS);
      Serial.println("Continue");
      break;
    case GameEvent::Paused:
      tone(BUZZER_PIN, TONE_LOW, BEEP_MS);
      Serial.printf("Pause at %lu ms\n", (unsigned long)accumulatedMs);
      break;
    case GameEvent::Reset:
      tone(BUZZER_PIN, TONE_RESET, RESET_BEEP_MS);
      Serial.println("Reset");
      break;
    case GameEvent::MaxedOut:
      tone(BUZZER_PIN, TONE_LOW, BEEP_MS);
      Serial.println("Hit 99.99, paused");
      break;
    case GameEvent::None:
      break;
  }
}

// Format and display time
void showTime(uint32_t ms) {
  int hundredths = min(ms / 10, (uint32_t)9999);
  if (hundredths == shownValue) return;   // nothing changed, skip the slow display write
  shownValue = hundredths;
  display.showNumberDecEx(hundredths, 0b01000000, true);
}
