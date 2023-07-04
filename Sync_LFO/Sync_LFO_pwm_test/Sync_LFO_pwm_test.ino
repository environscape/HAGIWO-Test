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

long timer2 = 0;


void setup() {
  Serial.begin(115200);
  pinMode(10, OUTPUT);
  pinMode(3, INPUT);

  // FlexiTimer2::set(5, 1.0 / 100000, timer_count);  // 50usec/count
  // FlexiTimer2::start();
  // TCCR1A = 0b00100011;//TCCR1B;
  // TCCR1B = 0b00011001;  //分周比1
  // TCCR1B &= B11111000;  //fast pwm setting
  // TCCR1B |= B00000001;  //fast pwm setting
  TCCR1A = 0b00100011;
  TCCR1B = 0b00010001; 
  timer2 = micros();
}

void loop() {

  Serial.print(analogRead(0));
  Serial.print(" mode= ");
  Serial.print(mode);
  Serial.println("   ");

  analogWrite(10, analogRead(0) >> 2);

// }

// void timer_count() {
//   ext_count++;
//   set_freq++;
//     duty = 1;

}