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
  clearTasks();

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

// standby = 0, ff = 1, play = 2, play2 = 3, reverse = 4, record = 5
void loop() {
  if (turning) {
    if (check > -1) {
      if (digitalRead(2) == check) {
        checkCount++;

        if (checkCount >= 2000) {
          if (check == 0) {
            if (mode == 1 || mode == 2 || mode == 3 || mode == 5) {
              Serial.print("mode 1 2 3 5 check 0 hit, endOfTape = true. ");
              Serial.print("mode: ");
              Serial.println(mode);
              endOfTape = true;
            } else {
              Serial.println("mode 4 6 hit check 0, stopTurning = true");
              stopTurning = true;
            }
          } else {
            if (findingTheStart) {
              if (mode == 3) {
                Serial.println("check is 1, findingTheStart is true and mode 3 so stopTurning = true");
                stopTurning = true;
              } else if (mode == 4) {
                Serial.println("check is 1, findingTheStart is true and mode is 4 so timeRewindBegan = -1");
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
      Serial.println("endOfTape is true so stop and standby");
      stopAndStandby();
      endOfTape = false;

      if (checkingLength) {
        Serial.println("checkingLength is true so don't notify");
        checkingLength = false;
      } else {
        Serial.println("notify end of tape because not checkingLength");
        notifyEndOfTape();
      }
    } else if (stopTurning) {
      Serial.println("stopTurning is true so stop and standby");
      stopAndStandby();
      stopTurning = false;
    } else if (tickLimit > -1 && ticks >= tickLimit) {
      Serial.print("tickLimit ");
      Serial.print(tickLimit);
      Serial.println(" reached so stop");
      stopAndStandby();
      notifyTicks();
      tickLimit = -1;
      advancing = false;
    } else if (mode == 4 && findingTheStart && startedTurningOnClearTape && timeRewindBegan > 0 && millis() - timeRewindBegan > 3000) {
      Serial.println("findingTheStart AND startedOnClearTape AND timeRewindBegan > 0 AND it's been over 3 seconds");
      stopAndStandby();
      timeRewindBegan = -1;
    } else {
      if (!findingTheStart && !advancing) {
        if (millis() - lastTickUpdate > 5000) {
          Serial.println("sending tick update");
          
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
      Serial.print("stepIndex: ");
      Serial.println(stepIndex);
      Serial.print("step 0 null?: ");
      Serial.println(steps[0] == NULL);
      
      stepIndex++;

      if (steps[stepIndex] == startMotor) {
        if (digitalRead(2) == 0) {
          Serial.println("starting on clear tape");
          
          startedTurningOnClearTape = true;

          if (findingTheStart && steps[stepIndex - 1] == reverseMode) {
            Serial.println("reseting timeRewindBegan");
            timeRewindBegan = 0;
          }
        } else {
          startedTurningOnClearTape = false;
        }
      }

      steps[stepIndex]();
    } else {
      if (tasks[taskIndex + 1] != NULL) {
        Serial.println("there is still a task to do");
        taskIndex++;

        tasks[taskIndex]();
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
  sprintf(action, "finished");
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

void notify() {
  digitalWrite(6, HIGH);
  delay(500);
  digitalWrite(6, LOW);
}

void clearSteps() {
  Serial.println("clearing steps");
  
  for (int i = 0; i < 10; i++) {
    steps[i] = NULL;
  }

  stepIndex = -1;
}

void clearTasks() {
  Serial.println("clearing tasks");
  
  for (int i = 0; i < 10; i++) {
    tasks[i] = NULL;
  }

  taskIndex = -1;
}

void newTape() {
  Serial.println("new tape...");
  
  clearTasks();

  tasks[0] = startOfTape;
  tasks[1] = tapeLength;
  tasks[2] = clearTasks;
}

void advanceToTickLimit(int limit) {
  Serial.print("advancing to tick limit ");
  Serial.println(limit);
  
  clearTasks();

  futureTickLimit = limit;
  advancing = true;

  tasks[0] = startOfTape;
  tasks[1] = advance;
  tasks[2] = clearTasks;
}

void advance() {
  Serial.println("advancing task starting");
  
  clearSteps();

  tickLimit = futureTickLimit;
  futureTickLimit = -1;

  steps[0] = fastForwardMode;
  steps[1] = startMotor;
  steps[2] = clearSteps;
}

void startOfTape() {
  Serial.println("you ask me to find the start");
  
  clearSteps();

  findingTheStart = true;

  steps[0] = reverseMode;
  steps[1] = startMotor;
  steps[2] = playMode;
  steps[3] = playMode2;
  steps[4] = startMotor;
  steps[5] = notifyStartOfTape;
  steps[6] = clearSteps;
}

void tapeLength() {
  Serial.println("tape length...");
  
  clearSteps();

  checkingLength = true;

  steps[0] = fastForwardMode;
  steps[1] = startMotor;
  steps[2] = notifyTapeLength;
  steps[3] = clearSteps;
}

void startMotor() {
  Serial.println("starting motor");
  
  digitalWrite(7, HIGH);
  
  if (mode == 4) {
    timeRewindBegan = millis();
  }
  
  lastTickUpdate = millis();
  turning = true;
  ticks = 0;
}

void stopMotor() {
  Serial.println("stopping motor");
  
  digitalWrite(7, LOW);
  turning = false;
}

void standbyMode() {
  Serial.println("standby mode");
  
  fastForwardMode();
  mode = 0;
}

void fastForwardMode() {
  Serial.println("fast foward mode");
  
  playServo.write(60);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 1;
  delay(1000);
}

void playMode() {
  Serial.println("play mode 1");
  
  playServo.write(165);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 2;
  delay(1000);
}

void playMode2() {
  Serial.println("play mode 2");
  
  playServo.write(140);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 3;
  delay(1000);
}

void reverseMode() {
  Serial.println("reverse mode");
  
  playServo.write(60);
  eraseServo.write(60);
  reverseServo.write(20);
  recordServo.write(90);
  mode = 4;
  delay(1000);
}

void recordMode() {
  Serial.println("record mode");
  
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

    Serial.print("command received: ");
    Serial.println(commandThatWasReceived);

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

    Serial.print("message received: ");
    Serial.println(message);

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
