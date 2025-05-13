#include <FlexiTimer2.h>
#include <avr/io.h>
#include "const.h"  //wave table : saw1 saw2 sine tri squ

#define PWMPIN 10    //输出信号    Output
#define PWMPIN2 6    //输出信号    Output2
#define EXTCLKPin 3  //输入时钟    Clk In
#define CVINPin A3   //输入压控    CV In
#define KNOBPIN1 0   //旋钮1       Freq/Phase/Amp
#define KNOBPIN2 1   //旋钮2       WaveType
#define KNOBPIN3 2   //旋钮3       Modulation

//获取到各个引脚并打印日志 ok
//调整震荡频率
//调试hold v amp调整
//时钟调试

float duty = 0.5;      // duty比率
int wavePosition = 0;  //wavePosition

byte waveType = 1;  //波形类型

int set_freq = 1;   //
int freq_max = 30;  //外部时钟周期（*100usec）
int amp = 256;      //change pwm duty
int tmp_amp = 256;  //change pwm duty
int tmp_a1 = 0;     //change pwm duty
int lgt8f328p = 1;  //  lgt8f328p =4 / arduion nano =1;

float amp_rate = 1.0;
int phase = 1;
int modulation = 0;  //self modulation

bool ext_injudge = 0;  //0=use internal clock , 1 = use external clock
bool ext_pulse = 0;    //0=no external in
bool old_ext_pulse = 0;
long ext_count = 0;
long ext_count_result = 0;
long old_ext_count_result = 0;
long ext_period = 0;
bool reset_count = 0;

long timer2 = 0;

int a3 = 0;

// 提高pwm_freq的值
unsigned int pwm_freq = 100000;  // 原来的50000改为100000，可根据需要调整

void setup() {
  pinMode(PWMPIN, OUTPUT);
  pinMode(PWMPIN2, OUTPUT);
  pinMode(CVINPin, INPUT);

  FlexiTimer2::set(5, 1.0 / 100000, timer_count);  // 50usec/count
  FlexiTimer2::start();

  Serial.begin(115200);

  // 修改TCCR1A和TCCR1B的设置，确保支持更高的PWM频率
  TCCR1A = 0b00100011;  // 更改这里
  TCCR1B = 0b00000001;  // 更改这里，分周比设置为1，可根据需要调整
  // 这里的设置可能需要根据你的具体需求和硬件调整，确保支持更高的PWM频率

  // timer2 = micros();
}

void loop() {
  old_ext_pulse = ext_pulse;
  ext_pulse = digitalRead(EXTCLKPin);

  Serial.print("  freq= ");
  Serial.print(freq_max);
  Serial.print("  waveType= ");
  Serial.print(waveType);
  Serial.print("  modulation= ");
  Serial.print(modulation);
  Serial.print("  amp= ");
  Serial.print(amp);
  // Serial.print("  ext_injudge= ");
  // Serial.print(ext_injudge);
  // Serial.print("  freq mod= ");
  // Serial.println(a3);

  // Serial.print(" KNOBPIN1= ");
  // Serial.print(analogRead(KNOBPIN1));
  // Serial.print(" KNOBPIN2= ");
  // Serial.print(analogRead(KNOBPIN2));
  // Serial.print(" KNOBPIN3= ");
  // Serial.println(analogRead(KNOBPIN3));

  Serial.println(" ");

  //------------waveType select-------------------------------
  waveType = analogRead(KNOBPIN2) >> 7;

  //------------phase and internal clock set-------------------------------
  if (ext_injudge == 0) {  //use internal clock , phase function off
    phase = 0;
    freq_max = 1 + 0.0007 * (1023 - analogRead(KNOBPIN1)) * (1023 - analogRead(KNOBPIN1)) / 2;
    a3 = analogRead(CVINPin) >> 6;

    freq_max = freq_max - a3;
  } else if (ext_injudge == 1) {  //use external clock , phase function on
    phase = map(analogRead(KNOBPIN1), 0, 1023, 0, 999);
  }

  //------------selc modulation-------------------------------
  modulation = analogRead(KNOBPIN3) >> 7;

  switch (modulation) {
    default:  //no modulation
      break;
    case 1:
      phase = phase + (pgm_read_word(&(saw1[wavePosition])));
      break;
    case 2:
      phase = phase + (pgm_read_word(&(saw2[wavePosition])));
      break;
    case 3:
    case 4:
      phase = phase + (pgm_read_word(&(sine[wavePosition])));
      break;
    case 5:
      phase = phase + (pgm_read_word(&(tri[wavePosition])));
      break;
    case 6:
      phase = phase + (pgm_read_word(&(squ[wavePosition])));
      break;
    case 7:
      phase = phase + (pgm_read_word(&(tri[random(1, 1000)])));
      break;
  }

  //--------------amp set----------------
  int this_am = 0;
  this_am = map(amp, 0, 1023, 1, 100);
  amp_rate = (float)this_am / 100;

  int this_v = analogRead(KNOBPIN1) / lgt8f328p;  //这里还是要改回来吧
  int knob1_dec = analogRead(KNOBPIN1) - tmp_a1;

  //amp test
  // Serial.print("                                               ");
  // Serial.print(knob1_dec);
  // Serial.print("             ");
  // Serial.print(this_v);
  // Serial.print("             ");
  // Serial.print(tmp_amp);
  // Serial.println("             ");


  if (waveType == 0) {                        //hold v 电平旋钮1检测逻辑
    if (-30 > knob1_dec || knob1_dec > 30) {  //当切换到hold v时 amp也不是立刻就修改的
      if (tmp_amp != this_v) {                //当旋钮电位发生变化时 才进行amp值得修改
        amp = this_v;                         //当旋钮1选择到电压保持时 旋钮3可以修改电压范围
      }
      tmp_amp = this_v;
    }
  } else {
    amp = 256;  //当不选择hold模式 则电平都开满
  }
  tmp_a1 = analogRead(KNOBPIN1);  //临时记下a1值

  //------------external in judge-------------------------------
  if (ext_count > 160000) {  //no external signal during 8 sec
    ext_injudge = 0;
  } else {
    ext_injudge = 1;
  }

  //----------clock setting-------------
  if (ext_pulse == 1 && old_ext_pulse == 0) {
    old_ext_count_result = ext_count_result;  //twice pulse average
    ext_count_result = ext_count;
    ext_period = (old_ext_count_result + ext_count_result) / 1960;
    freq_max = ext_period;
  }

  if (old_ext_pulse == 0 && ext_pulse == 1) {  //外部入力が有→無のとき
    ext_count = 0;
    wavePosition = 0;
  }
  output1and2();
}

void output1and2() {
  // モード指定
  TCCR1A = 0b00100001;
  TCCR1B = 0b00010001;  //分周比1
  // TOP値指定
  OCR1A = (unsigned int)(8000000 / pwm_freq);
  unsigned int bbb = (unsigned int)(8000000 / pwm_freq * duty * amp_rate);
  // Duty比指定
  OCR1B = bbb;  //这里相当于analogWrite(10);

  //  map(analogRead(KNOBPIN1), 0, 1023, 0, 999)
  analogWrite(PWMPIN2, map(bbb, 0, 40, 255, 0));  //对第D11引脚进行反向输出
  // Serial.print(OCR1B);
}

void timer_count() {

  ext_count++;
  set_freq++;

  if (set_freq >= freq_max) {
    set_freq = 0;

    wavePosition++;  //波表next
    if (wavePosition + phase >= 1000 && waveType != 7) {
      wavePosition = wavePosition - 1000;
    }

    switch (waveType) {
      default:  //steady hold v
        duty = 1;
        digitalWrite(4, 0);
        break;
      case 1:
        duty = (float)(pgm_read_word(&(saw1[wavePosition + phase]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 2:
        duty = (float)(pgm_read_word(&(saw2[wavePosition + phase]))) / 1000;
        digitalWrite(4, 0);
        break;
      case 3:
      case 4:
        duty = (float)(pgm_read_word(&(sine[wavePosition + phase]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 5:
        duty = (float)(pgm_read_word(&(tri[wavePosition + phase]))) / 1000;
        digitalWrite(4, 0);
        break;
      case 6:
        duty = (float)(pgm_read_word(&(squ[wavePosition + phase]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 7:
        digitalWrite(4, 0);
        wavePosition++;
        if (wavePosition >= 250) {
          wavePosition = 0;
          duty = random(1, 1000);
          duty = duty / 1000;
        }
        break;
    }
  }
} 