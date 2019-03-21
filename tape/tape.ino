#include <Wire.h>
#include <Servo.h>

Servo playServo;
Servo eraseServo;
Servo reverseServo;
Servo recordServo;

#define WIRE_ADDRESS 0x04

int number = 0;
volatile int ticks = 0;
long rewindTime = 0;
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
String responseContent = "";

volatile bool rewindTest = false;
volatile bool turning = false;
volatile bool stopTurning = false;
volatile bool endOfTape = false;

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
      stopTurning = true;
    } else {
      if (rewindTest && mode == 3) {
        rewindTest = false;
        stopTurning = true;
      } else if (mode == 5) {
        endOfTape = true;
      }
    }
  }
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'u') {
      Serial.println(digitalRead(2));
    } else if (c == 's') {
      notifyStartOfTape();
    } else if (c == 'r') {
      startOfTape();
    } else if (c == '1') {
      stopMotor();
      standbyMode();
    }
  }
  
  if (turning) {
    if (endOfTape) {
      stopMotor();
      standbyMode();
      endOfTape = false;
      notifyEndOfTape();
    } else if (stopTurning) {
      stopMotor();
      standbyMode();
      stopTurning = false;
    } else if (tickLimit > 0 && ticks >= tickLimit) {
      stopMotor();
      standbyMode();
    } else if (rewindTest && rewindTime > 0 && millis() - rewindTime > 2000) {
      stopMotor();
      standbyMode();
      rewindTime = 0;
    }
  } else {
    if (steps[currentStep + 1] != NULL) {
      currentStep++;

      if (rewindTest && steps[currentStep - 1] == reverseMode && steps[currentStep] == startMotor) {
        rewindTime = millis();
      }

      steps[currentStep]();
    }
  }
}

void notifyStartOfTape() {
  digitalWrite(6, HIGH);
  delay(500);
  digitalWrite(6, LOW);
}

void notifyEndOfTape() {
  digitalWrite(5, HIGH);
  delay(500);
  digitalWrite(5, LOW);
}

void clearSteps() {
  for (int i = 0; i < 10; i++) {
    steps[i] = NULL;
  }

  currentStep = -1;
}

void startOfTape() {
  clearSteps();

  if (digitalRead(2) == 0) {
    rewindTest = true;

    steps[0] = reverseMode;
    steps[1] = startMotor;
    steps[2] = playMode;
    steps[3] = playMode2;
    steps[4] = startMotor;
    steps[5] = notifyStartOfTape;
  } else {
    steps[0] = reverseMode;
    steps[1] = startMotor;
    steps[2] = notifyStartOfTape;
  }
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
  recordServo.write(70);
  mode = 5;
  delay(1000);
}

void receiveData(int byteCount) {
  while (Wire.available()) {
    number = Wire.read();

    switch (number) {
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
        responseContent = String(ticks);
        break;
      case 7:
        playMode2();
        break;
      case 8:
        startOfTape();
        break;
      default:
        break;
    }
  }
}

// callback for sending data
void sendData() {
  if (responseContent != "") {
    Wire.write(responseContent.c_str());
    responseContent = "";
  } else {
    Wire.write(number);
  }
}
