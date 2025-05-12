#include <Arduino.h>

#define NUM_HOLES 7

const int ledPins[NUM_HOLES]      = {46, 47, 48, 49, 50, 51, 52};
const int buzzerPins[NUM_HOLES]   = {36, 37, 38, 39, 40, 41, 42};
const int vibePins[NUM_HOLES]     = {26, 27, 28, 29, 30, 31, 32};
const int hallPins[NUM_HOLES]     = {A0, A1, A2, A3, A4, A5, A6};

// 🎵 Per-hole buzzer frequencies (Hz)
const int buzzerFrequencies[NUM_HOLES] = {
  2500, 1100, 1200, 1300, 1400, 1500, 1600
};

const int HALL_THRESHOLD = 20;
const unsigned long HOLE_LIFETIME = 10000;
const unsigned long PLUG_TIME_REQUIRED = 2000;

const int UNPLUGGED_BUFFER_SIZE = 5;
bool unpluggedBuffer[NUM_HOLES][UNPLUGGED_BUFFER_SIZE] = {false};
int unpluggedIndex[NUM_HOLES] = {0};

enum GameState { WAITING_FOR_PLUG, PLUGGING, PLUGGED };

GameState states[NUM_HOLES];
unsigned long stateStartTime[NUM_HOLES];
bool playingVictory[NUM_HOLES];

unsigned long lastHoleSelectTime = 0;
const unsigned long HOLE_SELECT_INTERVAL = 4000;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < NUM_HOLES; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buzzerPins[i], OUTPUT);
    pinMode(vibePins[i], OUTPUT);
    pinMode(hallPins[i], INPUT);
    states[i] = PLUGGED;
    stateStartTime[i] = millis();
    playingVictory[i] = false;
    for (int j = 0; j < UNPLUGGED_BUFFER_SIZE; j++) {
      unpluggedBuffer[i][j] = false;
    }
  }
  randomSeed(analogRead(A7));
}

void loop() {
  unsigned long now = millis();

  // Randomly activate a new hole every 4 seconds
  if (now - lastHoleSelectTime >= HOLE_SELECT_INTERVAL) {
    lastHoleSelectTime = now;
    int attempts = 0;
    while (attempts < 10) {
      int idx = random(NUM_HOLES);
      if (states[idx] == PLUGGED) {
        states[idx] = WAITING_FOR_PLUG;
        stateStartTime[idx] = now;
        Serial.print("Activating hole ");
        Serial.println(idx);
        break;
      }
      attempts++;
    }
  }

  // Update each hole
  for (int i = 0; i < NUM_HOLES; i++) {
    int hallValue = analogRead(hallPins[i]);
    bool isPlugged = hallValue < HALL_THRESHOLD;

    switch (states[i]) {
      case WAITING_FOR_PLUG: {
        digitalWrite(vibePins[i], HIGH);
        bool blink = (now % 1000) < 100;
        digitalWrite(ledPins[i], blink);
        if (blink) tone(buzzerPins[i], buzzerFrequencies[i]);
        else noTone(buzzerPins[i]);

        if (isPlugged) {
          states[i] = PLUGGING;
          stateStartTime[i] = now;
          Serial.print("Started plugging hole ");
          Serial.println(i);

          for (int j = 0; j < UNPLUGGED_BUFFER_SIZE; j++) {
            unpluggedBuffer[i][j] = false;
          }
          unpluggedIndex[i] = 0;
        }
        break;
      }

      case PLUGGING: {
        digitalWrite(ledPins[i], HIGH);
        digitalWrite(vibePins[i], HIGH);
        noTone(buzzerPins[i]);

        unpluggedBuffer[i][unpluggedIndex[i]] = !isPlugged;
        unpluggedIndex[i] = (unpluggedIndex[i] + 1) % UNPLUGGED_BUFFER_SIZE;

        int unpluggedCount = 0;
        for (int j = 0; j < UNPLUGGED_BUFFER_SIZE; j++) {
          if (unpluggedBuffer[i][j]) unpluggedCount++;
        }

        if (unpluggedCount == UNPLUGGED_BUFFER_SIZE) {
          states[i] = WAITING_FOR_PLUG;
          noTone(buzzerPins[i]);
          Serial.print("Plug removed too early at hole ");
          Serial.println(i);
        } else if (now - stateStartTime[i] >= PLUG_TIME_REQUIRED) {
          states[i] = PLUGGED;
          stateStartTime[i] = now;
          playingVictory[i] = true;
          Serial.print("Hole ");
          Serial.print(i);
          Serial.println(" plugged!");
        }
        break;
      }

      case PLUGGED: {
        digitalWrite(ledPins[i], LOW);
        digitalWrite(vibePins[i], LOW);

        unsigned long elapsed = now - stateStartTime[i];
        if (elapsed >= 450 && playingVictory[i]) {
          noTone(buzzerPins[i]);
          playingVictory[i] = false;
        } else if (elapsed >= 300 && playingVictory[i]) {
          tone(buzzerPins[i], buzzerFrequencies[i]);
        } else if (elapsed >= 150 && playingVictory[i]) {
          noTone(buzzerPins[i]);
        } else if (playingVictory[i]) {
          tone(buzzerPins[i], buzzerFrequencies[i]);
        }
        break;
      }
    }
  }
}
