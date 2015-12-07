//Capture full defintion data

const int analogPin = 0;                    // analog pin (Concept2)

int preva = -9999999;
int a;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  
}

void loop() {
  a = analogRead(analogPin);
  if(preva!=a)//where the value has changed by more than 2, 
  {//print it out.
    Serial.println(micros());
    Serial.print("\t");
    Serial.println(a);
    preva = a;
    //delay(1);
  }
}
