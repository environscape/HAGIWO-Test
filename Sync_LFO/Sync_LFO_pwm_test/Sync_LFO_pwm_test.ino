#include <FlexiTimer2.h>

#include <avr/io.h>

unsigned int frq = 62500;  // PWM周波数。60kHzあたりまで機能するが、マージンとって50kHzとする。
float duty = 0.5;          // duty比率
int count = 0;

byte mode = 1;  //

int set_freq = 1;   //
int freq_max = 30;  //外部クロック周期(*100usec)
int amp = 1;        //change pwm duty
float amp_rate = 1.0;
int phase = 1;
int mod = 0;  //self modulation

long timer3 = 0;
long timer2 = 0;
long timer1 = 0;
long timer0 = 0;


void setup() {
  Serial.begin(115200);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(11, OUTPUT);

  // FlexiTimer2::set(5, 1.0 / 100000, timer_count);  // 50usec/count
  // FlexiTimer2::start();
  // TCCR1A = 0b00100011;//TCCR1B;
  // TCCR1B = 0b00011001;  //分周比1
  // TCCR1B &= B11111000;  //fast pwm setting
  // TCCR1B |= B00000001;  //fast pwm setting

  // TCCR1A = 0b00100011;
  // TCCR1B = 0b00010001;
  // timer2 = micros();

  TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS22);
  // TCCR2B = TCCR2B & 0b11111000 | 0x01;//31250hz

  OCR2A = 180;
  OCR2B = 50;
}

void loop() {

  Serial.print("knob ");
  int value = analogRead(0) >> 2;
  Serial.print(value);
  Serial.println("   ");

  analogWrite(10, value);
  analogWrite(9, value);
  analogWrite(6, value);
  analogWrite(5, value);
  analogWrite(3, value);
  OCR2A = value;
  OCR2B = value;
  // }

  // void timer_count() {
  //   ext_count++;
  //   set_freq++;
  //     duty = 1;
}