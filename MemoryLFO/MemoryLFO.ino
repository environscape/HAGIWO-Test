//记忆旋钮电压模块
#define KNOB_PIN 4   //旋钮引脚
#define BTN1_PIN 12  //按钮引脚
#define BTN2_PIN 13  //按钮引脚
#define OUTA_PIN 11  //OUT PWM引脚

int value[768] = { 0 };  //给部分数组元素赋值
int position = 0;        //最大值为768
int lastBtn1 = 1;
int memStep = 768;

void setup() {
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);

  Serial.begin(115200);  //使用Serial.begin()函数来初始化串口波特率,参数为要设置的波特率
}

void loop() {
  delay(16);

  // Serial.print("knob= ");
  // Serial.println(analogRead(KNOB_PIN));
  // Serial.print("btn1= ");
  // Serial.println(digitalRead(BTN1_PIN));

  if (digitalRead(BTN1_PIN) != lastBtn1) {
    Serial.println("  change-----------------------   ");
    lastBtn1 = digitalRead(BTN1_PIN);
    position = 0;
  }
  if (digitalRead(BTN1_PIN) == 0 && lastBtn1 == 0) {
    Serial.println("  aaa   ");
    value[position] = analogRead(KNOB_PIN);
    if (position < 768) position++;
    lastBtn1 = 0;
    memStep = position;
  } else if (digitalRead(BTN1_PIN) == 1 && lastBtn1 == 1) {
    Serial.println("  bbb   ");

    if (position < memStep) position++;
    else position = 0;
  }

  Serial.print(" position= ");
  Serial.print(position);
  // Serial.print(" value= ");
  // Serial.print(value[position]);
  for (int i = 0; i < (value[position] >> 4); i++) {
    Serial.print("=");
  }
  Serial.println("     ");
  analogWrite(OUTA_PIN, value[position]);
}