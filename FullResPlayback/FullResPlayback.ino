const int numvalues = 200;
int values[numvalues];
unsigned long times[numvalues];
int currentvalue;
int nextinsertedvalue= 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //read the first 800 lines in.
  for(int i = 0; i < 800; i++)
  {
    readLine();
  }
}

void loop() {
  unsigned long mics = micros();
    if(mics > times[currentvalue])
    {
      analogWrite(0,values[currentvalue]);
      currentvalue++;
      if(currentvalue > numvalues+1) 
          currentvalue = 0;
    }
    else if(mics < times[currentvalue] - 5000)
    {//time to do some reading
      readLine();
    }
}

void readLine()
{//read a line of serial input into the arrays
  if (Serial.available() > 0) {
    String line = "";
    char c = Serial.read();
    while(c != 9)//until we get to the tab
    {
      line += c;
      c = Serial.read();
    }
    times[nextinsertedvalue] = atol(line.c_str());
    line = "";
    while(c != 13)//until we get to the end of line
    {
      line += c;
      c = Serial.read();
    }
    values[nextinsertedvalue] = atoi(line.c_str());
    //read line feed.
    c = Serial.read();
    nextinsertedvalue++;
    if(nextinsertedvalue > numvalues-1)
    {
      nextinsertedvalue = 0;
    }
  }
}

