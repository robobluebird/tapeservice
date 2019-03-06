#include <Wire.h>
#include <Servo.h>

Servo playServo;
Servo eraseServo;
Servo reverseServo;
Servo recordServo;

#define WIRE_ADDRESS 0x04

// 3 15 23

int number = 0;
volatile int ticks = 0;
long lastTick = 0;
long startedTurning = -1;
long timeout = -1;
int currentStep = -1;
int oldCount = 0;
int largestDelay = 0;
int tickLimit = -1;
int nextTickLimit = 0;
int count = 0;
int mode = 0;
long last = -1;

bool turning = false;

void (*steps[10])(void);

void setup() {
  clearSteps();

  Serial.begin(9600);

  pinMode(7, OUTPUT);

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

    if (now - last > 20) {
      ticks++;
      last = now;
    }
  }
}

void transition() {
  if (turning) {
    if (digitalRead(2) == 0) {
      stopMotor();
      standbyMode();
      Serial.println(ticks);
    } else if (true) {
      stopMotor();
      standbyMode();
      Serial.println(ticks);
    }
  }
}

void loop() {
  if (turning) {
    Serial.println(ticks - oldCount);
    oldCount = ticks;
    delay(1000);
//    if (timeout != -1 && millis() - timeout >= 60000) {
//      stopMotor();
//      standbyMode();
//      Serial.println(ticks);
//    } else if (tickLimit != -1 && ticks >= tickLimit) {
//      stopMotor();
//      standbyMode();
//      Serial.println(ticks);
//    }

//    if (tickLimit > 0 && ticks >= tickLimit) {
//      Serial.print("Hit tick limit of ");
//      Serial.print(tickLimit);
//      Serial.print(" with ");
//      Serial.println(ticks);
//      stopMotor();
//      standbyMode();
//      tickLimit = 0;
//      nextTickLimit = 0;
//    } else if (startedTurning != -1 && now - startedTurning > 1000) {
//      if (slowTicks == 0) {
//        stopMotor();
//        standbyMode();
//        Serial.print(now - startedTurning);
//        Serial.println(" startedTurning distance is greater than 500, stopping.");
//      } else {
//        startedTurning = -1;
//        Serial.println("We have movement so set startedTurning to -1");
//      }
//    } else if (now - lastSlowTick > 500) {
//      Serial.println(now);
//      Serial.println(lastSlowTick);
//      Serial.print(now - lastSlowTick);
//      Serial.println(" slowTick distance is greater than 1000, stopping");
//
//      stopMotor();
//      standbyMode();
//
//      if (steps[currentStep + 1] != NULL) {
//        Serial.print("Preparing to execute step ");
//        Serial.println(currentStep + 1);
//        currentStep++;
//        steps[currentStep]();
//      } else {
//        Serial.println("Finished all steps");
//        currentStep = -1;
//        clearSteps();
//      }
//    } else {
//    }
//  } else {
//    if (steps[currentStep + 1] != NULL) {
//      Serial.print("Preparing to execute step ");
//      Serial.println(currentStep + 1);
//      currentStep++;
//      steps[currentStep]();
//    }
  }

  if (Serial.available()) {
    char c = Serial.read();

    if (c == '1') {
      playMode();
    } else if (c == '2') {
      playMode2();
    } else if (c == '3') {
      fastForwardMode();
    } else if (c == '4') {
      reverseMode();
    } else if (c == '7') {
      recordMode();
    } else if (c == '5') {
      startMotor();
    } else if (c == '6') {
      stopMotor();
    } else if (c == 'l') {
      tapeLength();
      steps[0]();
    } else if (c == 'b') {
      startOfTape();
      steps[0]();
    } else if (c == 'e') {
      endOfTape();
      steps[0]();
    } else if (c == 'a') {
      advanceTo(60 * 5);
      steps[0]();
    } else if (c == 'c') {
      advanceTo2(60 * 5);
      steps[0]();
    } else if (c == 't') {
      tickTest();
    } else if (c == 's') {
      stopMotor();
      standbyMode();
    } else if (c == 'u') {
      Serial.println(digitalRead(2));
    }

    // f l u s h
    while (Serial.available()) {
      Serial.read();
    }
  }
}

void tapeLength() {
  clearSteps();

  steps[0] = startOfUsableTape;
  steps[1] = endOfUsableTape;
}

void startOfUsableTape() {
  clearSteps();

  steps[0] = reverseMode;
  steps[1] = startMotor;
  steps[2] = playMode;
  steps[3] = playMode2;
  steps[4] = startMotor;
}

void endOfUsableTape() {
  steps[0] = fastForwardMode;
  steps[1] = startMotor;
}

void calculateLength() {
//  float windTime = (ticks / TICKS_PER_SECOND) - (ticksAfterUsableTapeFF * 2);
//  int tapeTime = (windTime * FAST_TAPE_CONSTANT) / 60;
//
//  //Wire.write("LEN: %s", tapeLength);
//  Serial.print("The tape length is ");
//  Serial.print(tapeTime);
//  Serial.print(" minutes per side.");
}

void startOfTape() {
}

void endOfTape() {
//  clearSteps();
//
//  nextTickLimit = ticksAfterUsableTapeFF;
//
//  steps[0] = fastForwardMode;
//  steps[1] = startMotor;
//  steps[2] = setTickLimit;
//  steps[3] = reverseMode;
//  steps[4] = startMotor;
}

void timedRecord(int seconds, int startTime = -1) {
}

void tickTest() {
//  clearSteps();
//
//  nextTickLimit = 46;
//
//  steps[0] = reverseMode;
//  steps[1] = setTickLimit;
//  steps[2] = startMotor;
//
//  steps[0]();
}

void advanceTo(float seconds) {
//  clearSteps();
//
//  Serial.println(seconds);
//
//  seconds = seconds / ff_to_p;
//
//  Serial.println(seconds);
//
//  nextTickLimit = seconds * TICKS_PER_SECOND;
//
//  Serial.println(nextTickLimit);
//
//  steps[0] = reverseMode;
//  steps[1] = startMotor;
//  steps[2] = setTickLimit;
//  steps[3] = fastForwardMode;
//  steps[4] = startMotor;
}

void advanceTo2(float seconds) {
//  clearSteps();
//
//  Serial.println(seconds);
//
//  nextTickLimit = seconds * TICKS_PER_SECOND;
//
//  Serial.println(nextTickLimit);
//
//  steps[0] = reverseMode;
//  steps[1] = startMotor;
//  steps[2] = setTickLimit;
//  steps[3] = playMode;
//  steps[4] = playMode2;
//  steps[5] = startMotor;
}

void setTickLimit() {
  tickLimit = nextTickLimit;

  delay(500);
}

void clearSteps() {
  for (int i = 0; i < 10; i++) {
    steps[i] = NULL;
  }

  currentStep = 0;
}

void startMotor() {
  digitalWrite(7, HIGH);
  // timeout = millis();
  startedTurning = millis();
  turning = true;
  ticks = 0;
}

void stopMotor() {
  digitalWrite(7, LOW);
  turning = false;
}

void fastForwardMode() {
  playServo.write(60);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 3;
  delay(1000);
}

void standbyMode() {
  fastForwardMode();
}

void reverseMode() {
  playServo.write(60);
  eraseServo.write(60);
  reverseServo.write(20);
  recordServo.write(90);
  mode = 4;
  delay(1000);
}

void playMode() {
  playServo.write(165);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 1;
  delay(1000);
}

void playMode2() {
  // debounce = 50;
  playServo.write(140);
  eraseServo.write(60);
  reverseServo.write(90);
  recordServo.write(90);
  mode = 2;
  delay(1000);
}

void recordMode() {
  playServo.write(140);
  eraseServo.write(145);
  reverseServo.write(90);
  recordServo.write(70);
  mode = 7;
  delay(1000);
}

// callback for received data
void receiveData(int byteCount) {
  while (Wire.available()) {
    number = Wire.read();

    switch (number) {
      case 1:
        playMode();
        break;
      case 2:
        playMode2();
        // recordMode();
        break;
      case 3:
        fastForwardMode();
        break;
      case 4:
        reverseMode();
        break;
      case 5:
        startMotor();
        break;
      case 6:
        stopMotor();
        break;
      case 7:
        tapeLength();
      default:
        standbyMode();
        break;
    }
  }
}

// callback for sending data
void sendData() {
  Wire.write(number);
}
