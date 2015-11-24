//Capture full defintion data

const int analogPin = 0;                    // analog pin (Concept2)

int preva = -9999999;
int a;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  a = analogRead(analogPin);
  if(abs(a-preva) > 2)//where the value has changed by more than 2, 
  {//print it out.
    Serial.print(micros());
    Serial.print("\t");
    Serial.println(a);
    preva = a;
    //delay(1);
  }
}
