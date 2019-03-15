#include <Wire.h>
#include <Servo.h>

Servo playServo;
Servo eraseServo;
Servo reverseServo;
Servo recordServo;

#define WIRE_ADDRESS 0x04

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
String responseContent = "";

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
  //  if (turning) {
  //    if (digitalRead(2) == 0) {
  //      stopMotor();
  //      standbyMode();
  //    }
  //  }
}

void loop() {
  if (turning) {
    if (tickLimit > 0 && ticks >= tickLimit) {
      stopMotor();
      standbyMode();
    }
  }
}

void clearSteps() {
  for (int i = 0; i < 10; i++) {
    steps[i] = NULL;
  }

  currentStep = -1;
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
      case 't':
        Serial.println("bep");
        number = digitalRead(2);
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
