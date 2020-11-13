
#include <Wire.h>

bool inputVal = LOW;

void setup() {
  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event (read from master)
  Wire.onRequest(writeEvent);   // register request (write to master)
  Serial.begin(115200);         // start serial for output
}

void loop() {
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  char c;
  byte x;

  while (Wire.available()) { // loop through all but the last
    c = Wire.read();             // receive byte as a command
    Serial.print(c);             // print the character
    x = Wire.read();             // receive byte
    Serial.println(x);           // print the byte
  }
  switch(c) {
    case 'H':
      pinMode(x, OUTPUT);
      digitalWrite(x, HIGH);
      break;
    case 'L':
      pinMode(x, OUTPUT);
      digitalWrite(x, LOW);
      break;
    case 'I':
      pinMode(x, INPUT);
      inputVal = digitalRead(x);
      Serial.print(F("I inputVal "));
      Serial.println(inputVal);
      break;
    case 'i':
      pinMode(x, INPUT_PULLUP);
      inputVal = digitalRead(x);
      Serial.print(F("i inputVal "));
      Serial.println(inputVal);
      break;
    default:
      Serial.println(F("unknown command"));
  }
}

void writeEvent() {
  // only digitalRead supported so far
  Wire.write(inputVal);
}
