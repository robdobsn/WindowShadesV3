
int HC595_SER = D0;              // 75HC595 pins
int HC595_SCK = D1;              //
int HC595_RCK = D2;              //
int LED_ACT = D3;
int LED_OP = D4;

void setup()
{
  pinMode(HC595_SER, OUTPUT);    // sets pin as output
  pinMode(HC595_SCK, OUTPUT);
  pinMode(HC595_RCK, OUTPUT);
  pinMode(LED_ACT, OUTPUT);
  pinMode(LED_OP, OUTPUT);
}

void setSwitches(int dataVal)
{
    for (int bitCount = 0; bitCount < 8; bitCount++)
    {
        // Set the data
        digitalWrite(HC595_SER, dataVal & 0b00000001);
        dataVal = dataVal >> 1;
        // Clock the data into the shift-register
        digitalWrite(HC595_SCK, HIGH);
        delayMicroseconds(1);
        digitalWrite(HC595_SCK, LOW);
        delayMicroseconds(1);
    }
    // Move the value into the output register
    digitalWrite(HC595_RCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(HC595_RCK, LOW);
}

void loop()
{
    setSwitches(0b10101010);
    digitalWrite(LED_OP, 1);
    digitalWrite(LED_ACT, 0);
    delay(100);
    setSwitches(0b01010101);
    digitalWrite(LED_OP, 0);
    digitalWrite(LED_ACT, 1);
    delay(100);
}
