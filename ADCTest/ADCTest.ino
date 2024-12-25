void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.print(" adc0=");    //func param
  Serial.print(analogRead(0));  //func param
  Serial.print(" adc1=");    //func param
  Serial.print(analogRead(1));  //func param
  Serial.print(" adc1=");    //func param
  Serial.print(analogRead(2));  //func param
  Serial.print(" adc2=");    //func param
  Serial.print(analogRead(3));  //func param
  Serial.print(" adc3=");    //func param
  Serial.print(analogRead(4));  //func param
  Serial.print(" adc4=");    //func param
  Serial.print(analogRead(5));  //func param
  Serial.print(" adc6=");    //func param
  Serial.print(analogRead(6));  //func param
  Serial.print(" adc7=");    //func param
  Serial.print(analogRead(7));  //func param
  Serial.print("\n");        //func param
}
