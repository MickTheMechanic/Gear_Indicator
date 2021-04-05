//v1.6
//Mick's neopixel shiftlight and 16 segment gear indicator

#include <Adafruit_NeoPixel.h>
const byte Brightness = A0;              //Input from photoresistor
const byte x_axis = A4;                  //Input from X and Y poti's for gear position
const byte y_axis = A5;
const byte Tacho = 2;                    //5v square wave input from engine ECU
const byte Data = 3;                     //Outputs for UAA2022 display driver
const byte VDR = 4;
const byte CLK = 5;
const byte CO = 6;
const byte LEDData = 7;                  //Output for neopixels
const unsigned int onFrequency = 20;     // represents engine speed higher than cranking and lower than idle
const unsigned int minFrequency = 123;   // represents engine speed to begin turning on LEDs
const unsigned int maxFrequency = 235;   // represents engine speed when shift flash should occur
const unsigned int shiftFrequency = 250; // represents engine speed when overrev flash should occur
const float Up = 2.055;                  // voltage Y Axis poti between 3rd and neutral gearstick position
const float Down = 1.985;                 // Vlotage Y Axis poti between 4th and neutral gearstick position
const float Left = 2.590;                // voltage X Axis poti between 1st and 3rd gearstick position
const float Right = 2.505;               // Voltage x Axis poti between 3rd and 5th gearstick position
const unsigned long refreshPeriod = 1000;//number of milliseconds for brightness refresh
const unsigned long shiftPeriod = 850;   //Delay to avoid neutral showing between gearshifts
const unsigned long timeoutVal = 50000;  //timeout value for pulseIn in microseconds
bool hasStartupSequenceRun = false;      // only run startup sequence once when engine starts
int brightnessValue;
int brightnessOutput;
int x_input;
int y_input;
float x_position;
float y_position;
float ighigh, iglow;
unsigned long igfreq;
unsigned long igcal1, igcal2;
unsigned long startMillis;
unsigned long currentMillis;
unsigned int mask = 0x8000;              //For Bitbang
unsigned int Neutral = 0xC2C2;           //Each "gear" int stores 2 bytes of information in hex to be sent to display driver via Bitbang
unsigned int firstGear = 0x800A;
unsigned int secondGear = 0x2727;
unsigned int thirdGear = 0xA527;
unsigned int fourthGear = 0x8183;
unsigned int fifthGear = 0xA5A5;
unsigned int Reverse = 0x43A7;
#define NUMPIXELS 16 //number of neopixel LED's connected
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LEDData, NEO_GRB + NEO_KHZ800);
const uint32_t tachColor[NUMPIXELS] =    //Set color for each individual LED
{
  Adafruit_NeoPixel::Color(0, 120, 0), //green
  Adafruit_NeoPixel::Color(0, 120, 0),
  Adafruit_NeoPixel::Color(0, 120, 0),
  Adafruit_NeoPixel::Color(15, 105, 0),
  Adafruit_NeoPixel::Color(30, 90, 0),
  Adafruit_NeoPixel::Color(45, 75, 0),
  Adafruit_NeoPixel::Color(60, 60, 0), //orange
  Adafruit_NeoPixel::Color(75, 45, 0),
  Adafruit_NeoPixel::Color(90, 30, 0),
  Adafruit_NeoPixel::Color(105, 15, 0),
  Adafruit_NeoPixel::Color(120, 0, 0), //red
  Adafruit_NeoPixel::Color(0, 0, 120), //blue
  Adafruit_NeoPixel::Color(0, 0, 120),
  Adafruit_NeoPixel::Color(0, 0, 120),
  Adafruit_NeoPixel::Color(0, 0, 120),
  Adafruit_NeoPixel::Color(0, 0, 120),
};
const unsigned int lightShiftFreq[NUMPIXELS] = //set frequency when each individual LED should turn on
{
  minFrequency,
  130,
  137,
  144,
  151,
  158,
  165,
  172,
  179,
  186,
  193,
  200,
  207,
  214,
  221,
  228,
};

void setup()
{
  pixels.begin();         //initializes NeoPixel library.
  startMillis = millis(); //initial start time
  //Serial.begin(9600);   //for serial print of xy voltages, used for initial calibration of gear position poti's
  pinMode(Brightness, INPUT);
  pinMode(x_axis, INPUT);
  pinMode(y_axis, INPUT);
  pinMode(Tacho, INPUT);
  pinMode(Data, OUTPUT);
  pinMode(VDR, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(CO, OUTPUT);
  pinMode(LEDData, OUTPUT);
  digitalWrite(VDR, HIGH); //Set latch pin high for bitbang
  digitalWrite(CLK, HIGH); //Set clock pin high for bitbang
}

void Display (uint16_t val)     //Bitbang to display driver (16 bits)
{
  digitalWrite(VDR, LOW);       //set latch low
  for ( int i = 0; i < 16; i++) //start bangin' bits
  {
    if ( (val & mask) == 0) {
      digitalWrite(Data, LOW);
    }
    else {
      digitalWrite(Data, HIGH);
    }
    digitalWrite(CLK, LOW);     // toggle clock
    digitalWrite(CLK, HIGH);
    val = val << 1;
  }
  digitalWrite(VDR, HIGH);      //set latch high
}

void loop()
{
  //Serial.println(y_position);                //Read gear position poti's output voltage via serial, used for calibration
  //Serial.println(x_position);
  //Brightness control code
  currentMillis = millis();
  if (currentMillis - startMillis >= refreshPeriod)            //brightness refresh, frequency of refresh is determined by refreshPeriod
  {
    brightnessValue = analogRead(Brightness);                  //Create brightness value from photoresistor input
    brightnessOutput = map(brightnessValue, 1, 1023, 1, 255);  //Map Values between 0 to 255 for an output value
    pixels.setBrightness(brightnessOutput);                    //Set brightness value for neopixels on the refresh period instead of loop to avoid data corruption
    startMillis = currentMillis;
  }
  analogWrite(CO, brightnessOutput);                  //Send brightness value as constant PWM to display driver.
  //Tacho signal code
  ighigh = pulseIn(Tacho, HIGH, timeoutVal);          //measure period of tacho signal
  iglow = pulseIn(Tacho, LOW, timeoutVal);
  igcal1 = 1000 / ((ighigh / 1000) + (iglow / 1000));
  ighigh = pulseIn(Tacho, HIGH, timeoutVal);          //do it again
  iglow = pulseIn(Tacho, LOW, timeoutVal);
  igcal2 = 1000 / ((ighigh / 1000) + (iglow / 1000));
  if ((igcal1 - igcal2) < 8)                          //to filter out noise, measurement is only valid if they are similar in value
  {
    igfreq = (igcal1 + igcal2) / 2;                   //if similar, average is taken
  }
  //Start sequence code
  if (hasStartupSequenceRun == false)
  {
    if (igfreq > onFrequency)                   //run start sequence when engine fires up
    {
      for (int i = 0; i < NUMPIXELS; ++i)
      {
        pixels.setPixelColor(i, tachColor[i]);  //LEDs illuminate one after another
        pixels.show();
        delay(50);
      }
      for (int a = 0; a < 10; a++)
      {
        pixels.fill(pixels.Color(0, 0, 120));   //LEDs flash
        pixels.show();
        digitalWrite(CO, LOW);                  //flash display
        delay(30);
        pixels.fill(pixels.Color(0, 0, 0));
        pixels.show();
        analogWrite(CO, brightnessOutput);      //flash display
        delay(30);
      }
      for (int i = 0; i < NUMPIXELS; ++i)      //re-illuminate LEDs in order to de-illuminate them one after another
      {
        pixels.setPixelColor(i, tachColor[i]);
        pixels.show();
      }
      for (int i = NUMPIXELS - 1; i >= 0; --i) //de-illuminate LEDs one after another
      {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();
        delay(50);
      }
      hasStartupSequenceRun = true;            //start sequence has run, dont run again
      pixels.fill(pixels.Color(0, 0, 0));
      pixels.show();
    }
    if (igfreq < onFrequency)                  //reset hasStartupSequenceRun if engine stops
    {
      hasStartupSequenceRun = false;
    }
  }
  //Shiftlight code
  if (igfreq < maxFrequency)       //shiftlight normal operating range
  {
    for (int i = 0; i < NUMPIXELS; ++i)
    {
      if (igfreq > lightShiftFreq[i])
      {
        pixels.setPixelColor(i, tachColor[i]);
      }
      else
      {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
    }
    pixels.show();
  }
  else if (igfreq >= maxFrequency && igfreq < shiftFrequency) //shift flash
  {
    pixels.fill(pixels.Color(0, 0, 120));      //shift flash=blue
    pixels.show();
    analogWrite(CO, brightnessOutput);         //flash display
    delay(30);
    pixels.fill(pixels.Color(0, 0, 0));
    pixels.show();
    digitalWrite(CO, LOW);                     //flash display
    delay(30);
  }

  else if (igfreq >= shiftFrequency)           //overrev flash
  {
    pixels.fill(pixels.Color(120, 0, 0));      //overrev flash=red
    pixels.show();
    analogWrite(CO, brightnessOutput);         //flash display
    delay(30);
    pixels.fill(pixels.Color(0, 0, 0));
    pixels.show();
    digitalWrite(CO, LOW);                     //flash display
    delay(30);
  }
  //Gear indicator code
  x_input = analogRead(x_axis);                     // read the input from x axis poti.
  x_position = x_input * (5.0 / 1023.0);            // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  y_input = analogRead(y_axis);                     // repeat for y axis
  y_position = y_input * (5.0 / 1023.0);
  if (y_position < Up && y_position > Down)         //display gear according to x and y position of gearstick
  {
    if (currentMillis - startMillis >= shiftPeriod) //Wait to see if a gearshift occurs, if not display neutral
    {
      Display(Neutral);
    }
  }
  if (x_position > Left && y_position > Up)
  {
    Display(firstGear);
  }
  if (x_position > Left && y_position < Down)
  {
    Display(secondGear);
  }
  if (x_position < Left && x_position > Right && y_position > Up)
  {
    Display(thirdGear);
  }
  if (x_position < Left && x_position > Right && y_position < Down)
  {
    Display(fourthGear);
  }
  if (x_position < Right && y_position > Up)
  {
    Display(fifthGear);
  }
  if (x_position < Right && y_position < Down)
  {
    Display(Reverse);
  }
}
