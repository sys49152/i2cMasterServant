
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
  char c;          // one byte
  uint16_t param;  // two bytes 

  while (Wire.available()<3) { // at least 3 bytes before we continue
    delay(10);
  }
  c = Wire.read();             // receive byte as a command
  Serial.print(c);             // print the character
  param = Wire.read();             // receive byte
  param <<= 8;                     // shift left 8 bits -> high byte
  param |= Wire.read();            // "or in" low byte
  Serial.println(param);           // print the word
  switch(c) {
    case 'H':
      pinMode(param, OUTPUT);
      digitalWrite(param, HIGH);
      break;
    case 'L':
      pinMode(param, OUTPUT);
      digitalWrite(param, LOW);
      break;
    case 'I':
      pinMode(param, INPUT);
      inputVal = digitalRead(param);
      Serial.print(F("I inputVal "));
      Serial.println(inputVal);
      break;
    case 'i':
      pinMode(param, INPUT_PULLUP);
      inputVal = digitalRead(param);
      Serial.print(F("i inputVal "));
      Serial.println(inputVal);
      break;
    default:
      Serial.println(F("unknown command"));
      break;
    }
}

void writeEvent() {
  // only digitalRead supported so far
  Wire.write(inputVal);
}
