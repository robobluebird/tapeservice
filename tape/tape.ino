#include <Wire.h>
#include <Servo.h>

Servo playServo;
Servo eraseServo;
Servo reverseServo;
Servo recordServo;

#define WIRE_ADDRESS 0x04

volatile int ticks = 0;

int commandThatWasReceived = -1;
int stepIndex = -1;
int taskIndex = -1;
int tickLimit = -1;
int futureTickLimit = -1;
int mode = 0;

long timeRewindBegan = -1;
long timeOfLastTick = -1;
long lastTickUpdate = -1;

char action[20];

bool findingTheStart = false;
bool advancing = false;
bool startedTurningOnClearTape = false;
bool turning = false;
bool stopTurning = false;
bool endOfTape = false;
bool checkingLength = false;

volatile int check = -1;

int checkCount = 0;

void (*steps[10])(void);
void (*tasks[10])(void);

void setup() {
  clearSteps();

  Serial.begin(9600);

  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);

  Serial.begin(9600);

  while (!Serial) {}

  playServo.attach(8);
  eraseServo.attach(9);
  reverseServo.attach(10);
  recordServo.attach(11);

  delay(2000);

  standbyMode();

  Wire.begin(WIRE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  pinMode(6, INPUT);

  pinMode(3, INPUT);
  attachInterrupt(digitalPinToInterrupt(3), tick, RISING);

  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), transition, CHANGE);
}

void tick() {
  if (turning) {
    long now = millis();

    if (now - timeOfLastTick > 20) {
      ticks++;
      timeOfLastTick = now;
    }
  }
}

void transition() {
  if (turning) {
    check = digitalRead(2);
  }
}

void loop() {
  if (turning) {
    if (check > -1) {
      if (digitalRead(2) == check) {
        checkCount++;

        if (checkCount >= 1800) {
          if (check == 0) {
            if (mode == 1 || mode == 2 || mode == 3 || mode == 5) {
              endOfTape = true;
            } else {
              delay(500);
              stopTurning = true;
            }
          } else {
            if (findingTheStart) {
              if (mode == 3) {
                stopTurning = true;
              } else if (mode == 4) {
                timeRewindBegan = -1;
              }
            }
          }

          check = -1;
          checkCount = 0;
        }
      } else {
        check = -1;
        checkCount = 0;
      }
    }

    if (endOfTape) {
      stopAndStandby();
      endOfTape = false;

      if (checkingLength) {
        checkingLength = false;
      } else {
        notifyEndOfTape();
      }
    } else if (stopTurning) {
      stopAndStandby();
      stopTurning = false;
    } else if (tickLimit > -1 && ticks >= tickLimit) {
      stopAndStandby();
      notifyTicks();
      tickLimit = -1;
      advancing = false;
    } else if (findingTheStart && startedTurningOnClearTape && timeRewindBegan > 0 && millis() - timeRewindBegan > 3000) {
      stopAndStandby();
      timeRewindBegan = -1;
    } else {
      if (!findingTheStart && !advancing) {
        if (millis() - lastTickUpdate > 2000) {
          if (mode == 3 || mode == 5) {
            notifyTicks();
          } else if (mode == 4) {
            notifyNegativeTicks();
          }

          lastTickUpdate = millis();
        }
      }
    }
  } else {
    if (steps[stepIndex + 1] != NULL) {
      stepIndex++;

      if (steps[stepIndex] == startMotor) {
        if (digitalRead(2) == 0) {
          startedTurningOnClearTape = true;

          if (findingTheStart && steps[stepIndex - 1] == reverseMode) {
            timeRewindBegan = 0;
          }
        } else {
          startedTurningOnClearTape = false;
        }
      }

      steps[stepIndex]();
    } else {
      clearSteps();

      if (tasks[taskIndex + 1] != NULL) {
        taskIndex++;

        tasks[taskIndex]();
      } else {
        clearTasks();
      }
    }
  }
}

void stopAndStandby() {
  stopMotor();
  standbyMode();
}

void notifyStartOfTape() {
  findingTheStart = false;
  sprintf(action, "start");
  notify();
}

void notifyEndOfTape() {
  sprintf(action, "end");
  notify();
}

void notifyTapeLength() {
  sprintf(action, "ticks:%d", ticks);
  notify();
}

void notifyTicks() {
  sprintf(action, "ticks:%d", ticks);
  notify();
}

void notifyNegativeTicks() {
  sprintf(action, "ticks:-%d", ticks);
  notify();
}

void notifyOutOfSpace() {
  sprintf(action, "space");
  notify();
}

void notifyDone() {
  sprintf(action, "done");
  notify();
}

void notify() {
  digitalWrite(6, HIGH);
  delay(500);
  digitalWrite(6, LOW);
}

void clearSteps() {
  for (int i = 0; i < 10; i++) {
    steps[i] = NULL;
  }

  stepIndex = -1;
}

void clearTasks() {
  for (int i = 0; i < 10; i++) {
    tasks[i] = NULL;
  }

  taskIndex = -1;
}

void newTape() {
  clearTasks();

  tasks[0] = startOfTape;
  tasks[1] = tapeLength;
}

void advanceToTickLimit(int limit) {
  clearTasks();

  futureTickLimit = limit;
  advancing = true;

  tasks[0] = startOfTape;
  tasks[1] = advance;
}

void advance() {
  clearSteps();

  tickLimit = futureTickLimit;
  futureTickLimit = -1;

  steps[0] = fastForwardMode;
  steps[1] = startMotor;
}

void startOfTape() {
  clearSteps();

  findingTheStart = true;

  steps[0] = reverseMode;
  steps[1] = startMotor;
  steps[2] = playMode;
  steps[3] = playMode2;
  steps[4] = startMotor;
  steps[5] = notifyStartOfTape;
}

void tapeLength() {
  clearSteps();

  checkingLength = true;

  steps[0] = fastForwardMode;
  steps[1] = startMotor;
  steps[2] = notifyTapeLength;
}

void startMotor() {
  digitalWrite(7, HIGH);
  lastTickUpdate = millis();
  turning = true;
  ticks = 0;
}

void stopMotor() {
  digitalWrite(7, LOW);
  turning = false;
}

void standbyMode() {
  fastForwardMode();
  mode = 0;
}

void fastForwardMode() {
  playServo.write(60);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 1;
  delay(1000);
}

void playMode() {
  playServo.write(165);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 2;
  delay(1000);
}

void playMode2() {
  playServo.write(140);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 3;
  delay(1000);
}

void reverseMode() {
  playServo.write(60);
  eraseServo.write(60);
  reverseServo.write(20);
  recordServo.write(90);
  mode = 4;
  delay(1000);
}

void recordMode() {
  playServo.write(140);
  eraseServo.write(145);
  reverseServo.write(90);
  recordServo.write(60);
  mode = 5;
  delay(1000);
}

void receiveData(int byteCount) {
  if (byteCount == 1) {
    commandThatWasReceived = Wire.read();

    switch (commandThatWasReceived) {
      case 1:
        playMode();
        break;
      case 2:
        fastForwardMode();
        break;
      case 3:
        reverseMode();
        break;
      case 4:
        recordMode();
        break;
      case 5:
        startMotor();
        break;
      case 6:
        stopMotor();
        break;
      case 7:
        playMode2();
        break;
      case 8:
        startOfTape();
        break;
      case 9:
        if (strcmp(action, "") == 0) {
          sprintf(action, "nothing");
        }
        break;
      case 10:
        newTape();
        break;
      case 11:
        sprintf(action, "ticks:%d", ticks);
        break;
      default:
        break;
    }
  } else if (byteCount > 1) {
    Wire.read(); // first byte = the "command" or "register" byte, we don't use it

    char message[33];
    int i = 0;

    while (Wire.available()) {
      char x = Wire.read();
      message[i] = x;
      i++;
    }

    message[i] = '\0';

    if (strstr(message, "advance") != NULL) {
      char *s = strchr(message, ':');
      s++;
      advanceToTickLimit(atoi(s));
    }

    commandThatWasReceived = 27;
  }
}

void sendData() {
  if (strcmp(action, "") != 0) {
    Wire.write(action);
    sprintf(action, "");
  } else {
    Wire.write(commandThatWasReceived);
    commandThatWasReceived = -1;
  }
}
