// Pin configuration
const int LED_PIN = 46;
const int BUZZER_PIN = 36;
const int VIBRATION_PIN = 26;
const int HALL_SENSOR_PIN = A4;  // Analog hall sensor input

// Threshold for analog hall sensor (tune as needed)
const int HALL_THRESHOLD = 20;

enum GameState {
  WAITING_FOR_PLUG,
  PLUGGING,
  PLUGGED
};

GameState state = WAITING_FOR_PLUG;
unsigned long stateStartTime = 0;
unsigned char playingVictory = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);  // Required for tone(), even though tone handles PWM
  pinMode(VIBRATION_PIN, OUTPUT);
  pinMode(HALL_SENSOR_PIN, INPUT);

  Serial.begin(9600);
  Serial.println("Game started.");
}

void loop() {
  int hallValue = analogRead(HALL_SENSOR_PIN);
  bool isPlugged = hallValue < HALL_THRESHOLD;

  Serial.print("Hall sensor value: ");
  Serial.println(hallValue);

  unsigned long now = millis();

  switch (state) {
    case WAITING_FOR_PLUG: {
      // Blink LED and beep buzzer
      digitalWrite(VIBRATION_PIN, HIGH);
      unsigned long blinkPhase = now % 1000;
      bool blink = blinkPhase < 100;

      digitalWrite(LED_PIN, blink);
      if (blink) {
        tone(BUZZER_PIN, 1000);  // 1kHz beep
      } else {
        noTone(BUZZER_PIN);
      }

      if (isPlugged) {
        state = PLUGGING;
        stateStartTime = now;
        Serial.println("Started plugging...");
      }
      break;
    }

    case PLUGGING: {
      digitalWrite(LED_PIN, HIGH);
      noTone(BUZZER_PIN);
      digitalWrite(VIBRATION_PIN, HIGH);

      if (!isPlugged) {
        Serial.println("Plug removed too early.");
        state = WAITING_FOR_PLUG;
        noTone(BUZZER_PIN);
      } else if (now - stateStartTime >= 2000) {
        Serial.println("Hole plugged successfully!");
        state = PLUGGED;
        stateStartTime = now;
        playingVictory = true;
      }
      break;
    }

    case PLUGGED: {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(VIBRATION_PIN, LOW);

      if ((now - stateStartTime) >= 300 && playingVictory) {
        noTone(BUZZER_PIN);
        playingVictory = false;
      } else if ((now - stateStartTime) >= 200 && playingVictory) {
        tone(BUZZER_PIN, 1000);
      } else if ((now - stateStartTime) >= 100 && playingVictory) {
        noTone(BUZZER_PIN);
      } else if (playingVictory) {
        tone(BUZZER_PIN, 1000);
      }

      if (now - stateStartTime >= 10000) {
        Serial.println("Restarting game.");
        noTone(BUZZER_PIN);
        state = WAITING_FOR_PLUG;
      }
      break;
    }
  }
}
