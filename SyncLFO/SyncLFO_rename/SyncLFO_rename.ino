#include <FlexiTimer2.h>
#include <avr/io.h>
#include "const.h"  //引入波形表  saw1 saw2 sine tri squ

#define OUTPUT_PIN 10          //输出信号    Output
#define OUTPUT2_PIN 6          //输出信号2   Output2
#define EXT_CLOCK_PIN 3        //输入时钟    Clk In
#define CV_INPUT_PIN A3        //输入压控    CV In
#define FREQ_KNOB_PIN 0        //旋钮1       Freq/Phase/Amp
#define WAVE_TYPE_KNOB_PIN 1   //旋钮2       WaveType
#define MODULATION_KNOB_PIN 2  //旋钮3       Modulation

float dutyCycle = 0.5;  //占空比
int wavePosition = 0;   //波形位置
byte waveType = 1;      //波形类型

int frequencyCounter = 1;  //频率计数器
int maxFrequency = 30;     //外部时钟周期（*100usec）
int amplitude = 256;       //PWM占空比调整
int tempAmplitude = 256;   //临时振幅值
int tempKnobValue = 0;     //临时旋钮值
int mcuType = 1;           //lgt8f328p =4 / arduion nano =1;

float amplitudeRate = 1.0;  //振幅比率
int phaseOffset = 1;        //相位偏移
int modulationType = 0;     //调制方式

bool useExternalClock = 0;         //0=使用内部时钟 , 1 =使用外部时钟
bool externalPulse = 0;            //0=无外部输入
bool lastExternalPulse = 0;        //上一次外部脉冲状态
long externalPulseCount = 0;       //外部脉冲计数
long externalPulseResult = 0;      //外部脉冲结果
long lastExternalPulseResult = 0;  //上一次外部脉冲结果
long externalClockPeriod = 0;      //外部时钟周期
bool resetCounter = 0;             //重置计数器标志

long timerValue = 0;  //定时器值

int voltageControlValue = 0;  //电压控制值

// 提高PWM频率，设置为200000Hz，可以根据需要进一步调整
unsigned long pwmFrequency = 200000;

/**
 * 初始化设置函数
 * 配置PWM输出引脚、定时器和串口通信
 * 设置PWM频率和占空比
 */
void setup() {
  Serial.begin(115200);          // 初始化串口通信，波特率115200
  pinMode(OUTPUT_PIN, OUTPUT);   // 设置PWM输出引脚
  pinMode(OUTPUT2_PIN, OUTPUT);  // 设置辅助输出引脚
  pinMode(CV_INPUT_PIN, INPUT);  // 设置电压控制输入引脚

  // 设置定时器2，每5微秒触发一次timerLoop函数
  FlexiTimer2::set(5, 1.0 / 100000, timerLoop);  //->timerLoop
  FlexiTimer2::start();                          // 启动定时器

  // 设置TCCR1A寄存器 - 配置PWM模式和输出比较模式
  // 快速PWM模式，清除OC1A/OC1B比较匹配时(非-inverting模式)
  TCCR1A = 0b00100011;

  // 设置TCCR1B寄存器 - 配置PWM模式和时钟源
  // 0b00000001表示不分频，即PWM频率 = 系统时钟频率 / OCR1A
  TCCR1B = 0b00000001;

  // 根据设定的PWM频率计算OCR1A的值
  // OCR1A决定PWM周期：频率 = 系统时钟(16MHz) / OCR1A
  OCR1A = (unsigned int)(8000000 / pwmFrequency);
}

/**
 * 主循环函数
 * 持续读取旋钮和传感器输入，更新波形参数
 * 处理外部时钟信号和波形生成
 */
void loop() {
  // 保存上一次的外部时钟状态
  lastExternalPulse = externalPulse;
  // 读取当前外部时钟状态
  externalPulse = digitalRead(EXT_CLOCK_PIN);

  // 打印当前参数状态到串口监视器
  Serial.print("  freq= ");
  Serial.print(maxFrequency);
  Serial.print("  waveType= ");
  Serial.print(waveType);
  Serial.print("  modulation= ");
  Serial.print(modulationType);
  Serial.print("  amp= ");
  Serial.print(amplitude);
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
  waveType = analogRead(WAVE_TYPE_KNOB_PIN) >> 7;

  //------------相位和时钟设置-------------------------------
  if (useExternalClock == 0) {  //使用内部时钟，相位功能关闭
    phaseOffset = 0;
    // 非线性映射旋钮1的值到频率范围
    maxFrequency = 1 + 0.0007 * (1023 - analogRead(FREQ_KNOB_PIN)) * (1023 - analogRead(FREQ_KNOB_PIN)) / 2;
    // 读取电压控制输入并应用到频率
    voltageControlValue = analogRead(CV_INPUT_PIN) >> 6;
    maxFrequency = maxFrequency - voltageControlValue;
  } else if (useExternalClock == 1) {  //使用外部时钟，相位功能开启
    // 映射旋钮1的值到相位偏移(0-999)
    phaseOffset = map(analogRead(FREQ_KNOB_PIN), 0, 1023, 0, 999);
  }

  //------------调制方式选择-------------------------------
  // 读取旋钮3的值并映射到调制方式(0-7)
  modulationType = analogRead(MODULATION_KNOB_PIN) >> 7;

  // 根据调制方式应用不同的波形调制
  switch (modulationType) {
    default:  //无调制
      break;
    case 1:  //使用saw1波形调制相位
      phaseOffset = phaseOffset + (pgm_read_word(&(saw1[wavePosition])));
      break;
    case 2:  //使用saw2波形调制相位
      phaseOffset = phaseOffset + (pgm_read_word(&(saw2[wavePosition])));
      break;
    case 3:
    case 4:  //使用sine波形调制相位
      phaseOffset = phaseOffset + (pgm_read_word(&(sine[wavePosition])));
      break;
    case 5:  //使用tri波形调制相位
      phaseOffset = phaseOffset + (pgm_read_word(&(tri[wavePosition])));
      break;
    case 6:  //使用squ波形调制相位
      phaseOffset = phaseOffset + (pgm_read_word(&(squ[wavePosition])));
      break;
    case 7:  //随机调制相位
      phaseOffset = phaseOffset + (pgm_read_word(&(tri[random(1, 1000)])));
      break;
  }

  //--------------振幅设置----------------
  // 将振幅值映射到0.01-1.0的比率范围
  int amplitudeMapped = 0;
  amplitudeMapped = map(amplitude, 0, 1023, 1, 100);
  amplitudeRate = (float)amplitudeMapped / 100;

  // 读取旋钮1的值并计算变化量
  int knobValue = analogRead(FREQ_KNOB_PIN) / mcuType;
  int knobDelta = analogRead(FREQ_KNOB_PIN) - tempKnobValue;

  // 电压保持模式 - 当波形类型为0时，使用旋钮1控制振幅
  if (waveType == 0) {
    // 检测旋钮是否有明显转动
    if (-30 > knobDelta || knobDelta > 30) {
      if (tempAmplitude != knobValue) {
        amplitude = knobValue;  // 更新振幅值
      }
      tempAmplitude = knobValue;
    }
  } else {
    amplitude = 256;  // 非保持模式下使用默认振幅
  }
  tempKnobValue = analogRead(FREQ_KNOB_PIN);  // 保存当前旋钮值用于下次比较

  //------------外部时钟判断-------------------------------
  // 如果长时间没有外部时钟信号，切换到内部时钟
  if (externalPulseCount > 160000) {
    useExternalClock = 0;
  } else {
    useExternalClock = 1;
  }

  //----------时钟设置-------------
  // 检测外部时钟上升沿
  if (externalPulse == 1 && lastExternalPulse == 0) {
    // 计算两次脉冲之间的周期，用于外部同步
    lastExternalPulseResult = externalPulseResult;
    externalPulseResult = externalPulseCount;
    externalClockPeriod = (lastExternalPulseResult + externalPulseResult) / 1960;
    maxFrequency = externalClockPeriod;
  }

  // 检测外部时钟下降沿，重置计数器和波形位置
  if (lastExternalPulse == 0 && externalPulse == 1) {
    externalPulseCount = 0;
    wavePosition = 0;
  }

  // 根据PWM频率、占空比和振幅计算OCR1B的值
  unsigned int pwmValue = (unsigned int)(8000000 / pwmFrequency * dutyCycle * amplitudeRate);
  OCR1B = pwmValue;

  // 控制第二个输出引脚，与PWM输出互补
  analogWrite(OUTPUT2_PIN, map(pwmValue, 0, 40, 255, 0));
}

/**
 * 定时器中断服务函数
 * 每5微秒触发一次，控制波形生成和频率
 */
void timerLoop() {
  // 增加计数器值
  externalPulseCount++;
  frequencyCounter++;

  // 当达到频率上限时，更新波形
  if (frequencyCounter >= maxFrequency) {
    frequencyCounter = 0;

    // 更新波形位置
    wavePosition++;
    if (wavePosition + phaseOffset >= 1000 && waveType != 7) {
      wavePosition = wavePosition - 1000;
    }

    // 根据波形类型选择不同的波形表
    switch (waveType) {
      default:  //稳定保持电压
        dutyCycle = 1;
        digitalWrite(4, 0);
        break;
      case 1:  //saw1波形
        dutyCycle = (float)(pgm_read_word(&(saw1[wavePosition + phaseOffset]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 2:  //saw2波形
        dutyCycle = (float)(pgm_read_word(&(saw2[wavePosition + phaseOffset]))) / 1000;
        digitalWrite(4, 0);
        break;
      case 3:
      case 4:  //sine波形
        dutyCycle = (float)(pgm_read_word(&(sine[wavePosition + phaseOffset]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 5:  //tri波形
        dutyCycle = (float)(pgm_read_word(&(tri[wavePosition + phaseOffset]))) / 1000;
        digitalWrite(4, 0);
        break;
      case 6:  //squ波形
        dutyCycle = (float)(pgm_read_word(&(squ[wavePosition + phaseOffset]))) / 1000;
        digitalWrite(4, 1);
        break;
      case 7:  //SNH随机波形
        digitalWrite(4, 0);
        wavePosition++;
        if (wavePosition >= 250) {
          wavePosition = 0;
          dutyCycle = random(1, 1000);
          dutyCycle = dutyCycle / 1000;
        }
        break;
    }
  }
}