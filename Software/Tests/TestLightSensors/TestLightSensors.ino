
int INSW_A0 = A0;              // Analog input pins
int INSW_A1 = A1;              //
int INSW_A2 = A2;              //

void setup()
{
  /*pinMode(INSW_A0, INPUT);    // sets pins as input
  pinMode(INSW_A1, INPUT);    //
  pinMode(INSW_A2, INPUT);    //*/
  Serial.begin(9600);
}

void loop()
{
    Serial.printlnf("%d\t%d\t%d", analogRead(INSW_A0), analogRead(INSW_A1), analogRead(INSW_A2));
    delay(1000);
}
