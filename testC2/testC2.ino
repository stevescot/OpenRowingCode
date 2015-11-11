short microsecondsPerSample = 1;
const int numvalues =200;
unsigned long timesamples[numvalues];
int measurements[numvalues];
int analogPin = 3;
int iswitch = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
  iswitch++;
  switch(iswitch)
  {
    case 1:
    analogReference(INTERNAL);
    Serial.println("Internal 1.1V reference");
    break;
    default:
    analogReference(DEFAULT);
    Serial.println("Default analog reference");
    iswitch = 0;
    microsecondsPerSample = microsecondsPerSample *2;
    if(microsecondsPerSample >1000) 
    {
      microsecondsPerSample = 1;
      Serial.println("Waiting 10s then resample");
      delay(10000);
    }
    break;
  }
  // put your main code here, to run repeatedly:
  for(int i=0;i<numvalues; i++)
  {
    delayMicroseconds(microsecondsPerSample);
    timesamples[i] = micros();
    measurements[i] = analogRead(analogPin);
  }
  Serial.print("time(");
  Serial.print(microsecondsPerSample);
  Serial.println(")us\tvalue");
  for(int i=0;i<numvalues; i++)
  {
    Serial.print(timesamples[i]);
    Serial.print("\t");
    Serial.println(measurements[i]);
  }

}
