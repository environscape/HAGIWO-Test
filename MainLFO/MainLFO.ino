#include <stdlib.h>
#include <stdio.h>

const int KNOB_PIN = 1;
const int BUTTON_PIN = 7;
const int CLK_IN = 2;
const int CV_IN = 0;
const int PIN_NUM = 6;

const int CV_PIN[PIN_NUM] = { 3, 5, 6, 9, 10, 11 };
float cv[PIN_NUM] = { 0, 0, 0, 0, 0, 0 };
float coef[PIN_NUM] = { 1, 0.73, 0.57, 0.41, 0.29, 0.13 };

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CLK_IN, INPUT);
  for (int i = 0; i < PIN_NUM; i++)
    pinMode(CV_PIN[i], OUTPUT);

  showKnobValue();
  delay(3000);
}

bool tmp_btn = 0;

void loop() {
  //read btn
  if (digitalRead(BUTTON_PIN) == 0 && tmp_btn == 1) {
    tmp_btn = 0;
  }
  if (digitalRead(BUTTON_PIN) == 1 && tmp_btn == 0) {
    tmp_btn = 1;
  }

  //view knob value
  if (!digitalRead(BUTTON_PIN)) {
    showKnobValue();
    return;
  }


  float knob_coef = (4096 - analogRead(KNOB_PIN)) / 1024;
  //SAW UP
  for (int i = 0; i < PIN_NUM; i++) {
    cv[i] += 0.2 * coef[i] * knob_coef;
    if (cv[i] > 255) cv[i] = 0;
  }

  for (int i = 0; i < PIN_NUM; i++)
    analogWrite(CV_PIN[i], cv[i]);


  Serial.print(" a1=");
  Serial.print(analogRead(KNOB_PIN));
  Serial.print(" d7=");
  Serial.print(digitalRead(BUTTON_PIN));
  Serial.print(" \n");
}

void showKnobValue() {
  int tmp_knob_value = (4096 - analogRead(KNOB_PIN));
  int tmp_knob_value2 = map(tmp_knob_value, 0, 4096, 0, 6);
  Serial.print(" tmp_knob_value=");
  Serial.print(tmp_knob_value2);
  for (int i = 0; i < PIN_NUM; i++) {
    if (tmp_knob_value2 > i) {
      digitalWrite(CV_PIN[i], HIGH);
    } else {
      digitalWrite(CV_PIN[i], LOW);
    }
  }
}
