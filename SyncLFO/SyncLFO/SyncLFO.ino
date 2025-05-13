#include <FlexiTimer2.h>
#include <avr/io.h>
#include "const.h"  //波形表  saw1 saw2 sine tri squ

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

float duty = 0.5;      //占空比
int wavePosition = 0;  //波形位置

byte waveType = 1;  //波形类型

int set_freq = 1;   //频率计数器
int freq_max = 30;  //外部时钟周期（*100usec）
int amp = 256;      //PWM占空比调整
int tmp_amp = 256;  //临时振幅值
int tmp_a1 = 0;     //临时A1值
int lgt8f328p = 1;  //  lgt8f328p =4 / arduion nano =1;

float amp_rate = 1.0;
int phase = 1;
int modulation = 0;  //自调制

bool ext_injudge = 0;  //0=使用内部时钟 , 1 =使用外部时钟
bool ext_pulse = 0;    //0=无外部输入
bool old_ext_pulse = 0;
long ext_count = 0;
long ext_count_result = 0;
long old_ext_count_result = 0;
long ext_period = 0;
bool reset_count = 0;

long timer2 = 0;

int a3 = 0;

// 提高PWM频率，设置为200000Hz，可以根据需要进一步调整
unsigned long pwm_freq = 200000;

/**
 * 初始化设置函数
 * 配置PWM输出引脚、定时器和串口通信
 * 设置PWM频率和占空比
 */
void setup() {
  pinMode(PWMPIN, OUTPUT);   // 设置PWM输出引脚
  pinMode(PWMPIN2, OUTPUT);  // 设置辅助输出引脚
  pinMode(CVINPin, INPUT);   // 设置电压控制输入引脚

  // 设置定时器2，每5微秒触发一次timer_count函数
  FlexiTimer2::set(5, 1.0 / 100000, timer_count);
  FlexiTimer2::start();  // 启动定时器

  Serial.begin(115200);  // 初始化串口通信，波特率115200

  // 设置TCCR1A寄存器 - 配置PWM模式和输出比较模式
  // 快速PWM模式，清除OC1A/OC1B比较匹配时(非-inverting模式)
  TCCR1A = 0b00100011;

  // 设置TCCR1B寄存器 - 配置PWM模式和时钟源
  // 0b00000001表示不分频，即PWM频率 = 系统时钟频率 / OCR1A
  TCCR1B = 0b00000001;

  // 根据设定的PWM频率计算OCR1A的值
  // OCR1A决定PWM周期：频率 = 系统时钟(16MHz) / OCR1A
  OCR1A = (unsigned int)(8000000 / pwm_freq);
}

/**
 * 主循环函数
 * 持续读取旋钮和传感器输入，更新波形参数
 * 处理外部时钟信号和波形生成
 */
void loop() {
  // 保存上一次的外部时钟状态
  old_ext_pulse = ext_pulse;
  // 读取当前外部时钟状态
  ext_pulse = digitalRead(EXTCLKPin);

  // 打印当前参数状态到串口监视器
  Serial.print("  freq= ");
  Serial.print(freq_max);
  Serial.print("  waveType= ");
  Serial.print(waveType);
  Serial.print("  modulation= ");
  Serial.print(modulation);
  Serial.print("  amp= ");
  Serial.print(amp);
  Serial.println(" ");

  // Serial.print("  ext_injudge= ");  //不要随便删除这些注释的log
  // Serial.print(ext_injudge);        //不要随便删除这些注释的log
  // Serial.print("  freq mod= ");     //不要随便删除这些注释的log
  // Serial.println(a3);               //不要随便删除这些注释的log

  // Serial.print(" KNOBPIN1= ");           //不要随便删除这些注释的log
  // Serial.print(analogRead(KNOBPIN1));    //不要随便删除这些注释的log
  // Serial.print(" KNOBPIN2= ");           //不要随便删除这些注释的log
  // Serial.print(analogRead(KNOBPIN2));    //不要随便删除这些注释的log
  // Serial.print(" KNOBPIN3= ");           //不要随便删除这些注释的log
  // Serial.println(analogRead(KNOBPIN3));  //不要随便删除这些注释的log

  // Serial.print("                             ");  //不要随便删除这些注释的logamp test
  // Serial.print(knob1_dec);                        //不要随便删除这些注释的logamp test
  // Serial.print("             ");                  //不要随便删除这些注释的logamp test
  // Serial.print(this_v);                           //不要随便删除这些注释的logamp test
  // Serial.print("             ");                  //不要随便删除这些注释的logamp test
  // Serial.print(tmp_amp);                          //不要随便删除这些注释的logamp test
  // Serial.println("             ");                //不要随便删除这些注释的logamp test

  //------------波形类型选择-------------------------------
  // 读取旋钮2的值并映射到波形类型(0-7)
  waveType = analogRead(KNOBPIN2) >> 7;

  //------------相位和时钟设置-------------------------------
  if (ext_injudge == 0) {  //使用内部时钟，相位功能关闭
    phase = 0;
    // 非线性映射旋钮1的值到频率范围
    freq_max = 1 + 0.0007 * (1023 - analogRead(KNOBPIN1)) * (1023 - analogRead(KNOBPIN1)) / 2;
    // 读取电压控制输入并应用到频率
    a3 = analogRead(CVINPin) >> 6;
    freq_max = freq_max - a3;
  } else if (ext_injudge == 1) {  //使用外部时钟，相位功能开启
    // 映射旋钮1的值到相位偏移(0-999)
    phase = map(analogRead(KNOBPIN1), 0, 1023, 0, 999);
  }

  //------------调制方式选择-------------------------------
  // 读取旋钮3的值并映射到调制方式(0-7)
  modulation = analogRead(KNOBPIN3) >> 7;

  // 根据调制方式应用不同的波形调制
  switch (modulation) {
    default:  //无调制
      break;
    case 1:  //使用saw1波形调制相位
      phase = phase + (pgm_read_word(&(saw1[wavePosition])));
      break;
    case 2:  //使用saw2波形调制相位
      phase = phase + (pgm_read_word(&(saw2[wavePosition])));
      break;
    case 3:
    case 4:  //使用sine波形调制相位
      phase = phase + (pgm_read_word(&(sine[wavePosition])));
      break;
    case 5:  //使用tri波形调制相位
      phase = phase + (pgm_read_word(&(tri[wavePosition])));
      break;
    case 6:  //使用squ波形调制相位
      phase = phase + (pgm_read_word(&(squ[wavePosition])));
      break;
    case 7:  //随机调制相位
      phase = phase + (pgm_read_word(&(tri[random(1, 1000)])));
      break;
  }

  //--------------振幅设置----------------
  // 将振幅值映射到0.01-1.0的比率范围
  int this_am = 0;
  this_am = map(amp, 0, 1023, 1, 100);
  amp_rate = (float)this_am / 100;

  // 读取旋钮1的值并计算变化量
  int this_v = analogRead(KNOBPIN1) / lgt8f328p;
  int knob1_dec = analogRead(KNOBPIN1) - tmp_a1;



  // 电压保持模式 - 当波形类型为0时，使用旋钮1控制振幅
  if (waveType == 0) {
    // 检测旋钮是否有明显转动
    if (-30 > knob1_dec || knob1_dec > 30) {
      if (tmp_amp != this_v) {
        amp = this_v;  // 更新振幅值
      }
      tmp_amp = this_v;
    }
  } else {
    amp = 256;  // 非保持模式下使用默认振幅
  }
  tmp_a1 = analogRead(KNOBPIN1);  // 保存当前旋钮值用于下次比较

  //------------外部时钟判断-------------------------------
  // 如果长时间没有外部时钟信号，切换到内部时钟
  if (ext_count > 160000) {
    ext_injudge = 0;
  } else {
    ext_injudge = 1;
  }

  //----------时钟设置-------------
  // 检测外部时钟上升沿
  if (ext_pulse == 1 && old_ext_pulse == 0) {
    // 计算两次脉冲之间的周期，用于外部同步
    old_ext_count_result = ext_count_result;
    ext_count_result = ext_count;
    ext_period = (old_ext_count_result + ext_count_result) / 1960;
    freq_max = ext_period;
  }

  // 检测外部时钟下降沿，重置计数器和波形位置
  if (old_ext_pulse == 0 && ext_pulse == 1) {
    ext_count = 0;
    wavePosition = 0;
  }

  // 根据PWM频率、占空比和振幅计算OCR1B的值
  unsigned int bbb = (unsigned int)(8000000 / pwm_freq * duty * amp_rate);
  OCR1B = bbb;

  // 控制第二个输出引脚，与PWM输出互补
  analogWrite(PWMPIN2, map(bbb, 0, 40, 255, 0));
}

/**
 * 定时器中断服务函数
 * 每5微秒触发一次，控制波形生成和频率
 */
void timer_count() {
  // 增加计数器值
  ext_count++;
  set_freq++;

  // 当达到频率上限时，更新波形
  if (set_freq >= freq_max) {
    set_freq = 0;

    // 更新波形位置
    wavePosition++;
    if (wavePosition + phase >= 1000 && waveType != 7) {
      wavePosition = wavePosition - 1000;
    }

    // 根据波形类型选择不同的波形表
    switch (waveType) {
      default:  //稳定保持电压
        duty = 1;
        digitalWrite(4, 0);
        break;
      case 1:  //saw1波形
        duty = (float)(pgm_read_word(&(saw1[wavePosition + phase]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 2:  //saw2波形
        duty = (float)(pgm_read_word(&(saw2[wavePosition + phase]))) / 1000;
        digitalWrite(4, 0);
        break;
      case 3:
      case 4:  //sine波形
        duty = (float)(pgm_read_word(&(sine[wavePosition + phase]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 5:  //tri波形
        duty = (float)(pgm_read_word(&(tri[wavePosition + phase]))) / 1000;
        digitalWrite(4, 0);
        break;
      case 6:  //squ波形
        duty = (float)(pgm_read_word(&(squ[wavePosition + phase]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 7:  //随机波形
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