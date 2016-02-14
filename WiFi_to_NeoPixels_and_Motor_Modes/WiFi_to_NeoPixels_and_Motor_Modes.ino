/*
 * Main Code for combined WiFi and Display controller
 * Pink/white colour ring means the chip is starting
 * All Red then all Green then all Blue shows the WiFI is connected
 */

#include <NeoPixelBus.h>
#include <RgbColor.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define pixelCount 12 //number of pixels in each RGB strip/ring
bool getNewData=false; //A flag that stops animated display modes in order to get new Commands
bool allowModeChange;  //A flag to show that we should allow a mode change
int modeNumberToUse;  //The last display mode received


//This is not a usefull table.  Too many 0s at the lower end
//***************Gamma correction table for LED brightness************************//
static const uint8_t gammaCorrect[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
    

//***************Set Outputs************************//
NeoPixelBus strip_04 = NeoPixelBus(pixelCount, 04);  //GPIO 4 Note 4 and 5 have their markings swapped on the vertical break out board
NeoPixelBus strip_05 = NeoPixelBus(pixelCount, 05); 
NeoPixelBus strip_15 = NeoPixelBus(pixelCount, 15); 
NeoPixelBus strip_02 = NeoPixelBus(pixelCount, 02);
NeoPixelBus strip_14 = NeoPixelBus(pixelCount, 14);

int ledPin_12 = 12;  // Vibrating Motor connected to digital pin 12
int ledPin_13 = 13;  // Vibrating Motor connected to digital pin 13 
int comms_LED = 01;

//***************Set WiFi connecion************************//
//const char* ssid     = "Hippy-Apple";
//const char* password = "1wabcdiaatcws13";

const char* ssid     = "Hippy-Vibing";
const char* password = "12345678";

//Returns a form with a colour picker.  Not in use anymore but handy for trying out colours
const char* html = "<html><head><style></style></head>"
                   "<body><form action='/' method='GET'><input type='color' name='color' value='#000000'/>"
                   "<input type='submit' name='submit' value='Update RGB Strip'/></form></body></html>";

//Returns a confirmation message
const char* webResponse = "<html><head><style></style></head>"
                   "<body>Received Command</body></html>"; 

//On and Off settings for LEDS
const RgbColor rgbBlack = RgbColor(0, 0, 0); 
const RgbColor rgbWhite = RgbColor(255, 255, 255); 

//Start the web server
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  /* Script file name MUST be included in every script */
  Serial.println();
  Serial.println("WiFi to NeoPixels and motor modes");
  Serial.println("Live Demo 3.02");
  Serial.println();

  Serial.print("LED Count : ");
  Serial.println(pixelCount);

  pinMode(ledPin_12, OUTPUT);
  pinMode(ledPin_13, OUTPUT);

  strip_04.Begin();
  strip_05.Begin();
  strip_15.Begin();
  strip_02.Begin();
  strip_14.Begin(); 

  //Flash LEDs to show system turned on
  DebugLED(0,0,254);
  delay(1000);
  DebugLED(0,254,0);
  delay(1000);
  DebugLED(254,0,0);
  delay(1000);
  DebugLED(100,0,0);

   
 
//***************WIFI client************************//
  WiFi.begin(ssid, password);
  for (int i = 0; i < strlen(ssid); ++i) {
    Serial.printf("%02x ", ssid[i]);
    }
    Serial.println("");

  
  Serial.print("\n\r \n\rWorking to connect to ");
  
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_command); //receive event
  server.begin();
  Serial.println("HTTP server started");

  //Flash LEDs to show connection made
  DebugLED(254,0,0);
  delay(500);
  DebugLED(0,254,0);
  delay(500);
  DebugLED(0,0,254);
  delay(500);
  DebugLED(100,100,100);
  digitalWrite(comms_LED, HIGH);
}

void loop() {
  server.handleClient(); //if no mode is running, this is where we receive data

  //turn TX/RX LED off
  digitalWrite(comms_LED, HIGH);

//Design mode.  
// Whilst calling a mode from the UI is preferable
// my current laptop shuts down WiFi when I plug in a Cat5 cable to "save power"
// As the laptop belongs to the company I don't have the BIOS password and so cannot stop this behavour
// So what?
// Well if I use the Cat5 to connect to the Vibing dedicated router I lose Internet connection.
// And my connection to Spotify drops.
// And then I can't listen to Jessie J while coding.
// Hence this mode that runs without the need to connect the UI to a router.
// Comment out when not in use - NO REALLY, COMMENT OUT

// When designing modes it is easier to use the i value from loops for maths

//Serial.println("Design mode");


// END Design mode.  



  //Serial.println("In Main Loop");
  if(allowModeChange)
  {
      Serial.println("In Mode Change");
      getNewData=false;
      allowModeChange=false;
    
     //Switch mode
     switch (modeNumberToUse) 
     {
         case 0:
          Set_Mode_000();
          break;
        case 1:
          Set_Mode_001();
          break;
        case 2:
          Set_Mode_002();
          break;
        case 3:
          Set_Mode_003();
          break;
        case 4:
          Set_Mode_004();
          break;
        case 5:
          Set_Mode_005();
          break;
        case 6:
          Set_Mode_006();
          break;
        case 7:
          Set_Mode_007();
          break;
        case 8:
          Set_Mode_008();
          break;
        case 9:
          Set_Mode_009();
          break;
        case 10:
          Set_Mode_010();
          break;
        case 11:
          Set_Mode_011();
          break;
        case 12:
          Set_Mode_012();
          break;
        case 13:
          Set_Mode_013();
          break;
        case 14:
          Set_Mode_014();
          break;
        case 15:
          Set_Mode_015();
          break;
        case 16:
          Set_Mode_016();
          break;
        case 17:
          Set_Mode_017();
          break;
        case 18:
          Set_Mode_018();
          break;
        case 19:
          Set_Mode_019();
          break;
    }
  }
}


/*
 * WiFi
 */

//Receive event handler
void handle_command() {
  Serial.println("Command Received: ");
  server.send(200, "text/html", webResponse);
    if (server.hasArg("mode")) {
      String modeToUse = server.arg("mode");  //get mode number

      //This is where the flags are set to interupt any display loop
      modeNumberToUse = modeToUse.toInt();
      getNewData=true;
      allowModeChange=true;
  }
}

/*
 * End WiFi
 */


/*
 * Display modes
 */ 

 //If the mode uses any LED positional information then you must use the LEDOffSet() function to set the correct LED
 //This is because in the physical build LED 1 is at the top of the ring. And we want that to be LED 12
 //When defining LED position value we use 1 to 12 but the code uses 0 to 11.
  //So if no positional info is required then use this:
 //strip_02.SetPixelColor(1,rgbColorToUse_02);  //from 0 to 11
 //Otherwise use this:
 //strip_02.SetPixelColor(LEDOffSet(1),rgbColorToUse_01);  //from 1 to 12
 //LEDOffSet() could be used in all cases but it takes time so why bother

 //When writing modes with loops try to set all parameters outside the loop


//Used to indicate application progress while debugging
//All LEDs set to the same colour
void DebugLED(int red, int green, int blue) {

  Serial.println("In DebugLED");

  RgbColor rgbColorToUse = RgbColor(gammaCorrect[red], gammaCorrect[green], gammaCorrect[blue]); 

  for(int i=0; i < pixelCount; i++) 
  {
      strip_04.SetPixelColor(i,rgbColorToUse);
      strip_05.SetPixelColor(i,rgbColorToUse);
      strip_15.SetPixelColor(i,rgbColorToUse);
      strip_02.SetPixelColor(i,rgbColorToUse);
      strip_14.SetPixelColor(i,rgbColorToUse);
  }

  strip_04.Show();
  strip_05.Show(); 
  strip_15.Show();
  strip_02.Show();
  strip_14.Show();
}

//Set each LED individually
//Used to help define new modes although can be used as a normal mode
//LEDs are just ON.  No loops
//There are 60 LEDS
//This is not fast
//Single shot mode with no animation
//Does not clear any LEDS so can be called more than once to 
//set different LEDS without affecting any other ring
void Set_Mode_000() 
{
  Serial.println("In Mode_000");

  int loopDelay = server.arg("loopdelay").toInt();
  int ringID = server.arg("ring").toInt();
  int brightnessModifier = server.arg("brightness").toInt();
   
  RgbColor rgbColorToUse_01 = CorrectedColour("colour_01",brightnessModifier);
  RgbColor rgbColorToUse_02 = CorrectedColour("colour_02",brightnessModifier);
  RgbColor rgbColorToUse_03 = CorrectedColour("colour_03",brightnessModifier);
  RgbColor rgbColorToUse_04 = CorrectedColour("colour_04",brightnessModifier);
  RgbColor rgbColorToUse_05 = CorrectedColour("colour_05",brightnessModifier);
  RgbColor rgbColorToUse_06 = CorrectedColour("colour_06",brightnessModifier);
  RgbColor rgbColorToUse_07 = CorrectedColour("colour_07",brightnessModifier);
  RgbColor rgbColorToUse_08 = CorrectedColour("colour_08",brightnessModifier);
  RgbColor rgbColorToUse_09 = CorrectedColour("colour_09",brightnessModifier);
  RgbColor rgbColorToUse_10 = CorrectedColour("colour_10",brightnessModifier);
  RgbColor rgbColorToUse_11 = CorrectedColour("colour_11",brightnessModifier);
  RgbColor rgbColorToUse_12 = CorrectedColour("colour_12",brightnessModifier);

  //Switch ring
  switch (ringID) 
  {
     case 1:
        Serial.println("Setting Ring on GPIO04");
        strip_04.SetPixelColor(LEDOffSet(1),rgbColorToUse_01);
        strip_04.SetPixelColor(LEDOffSet(2),rgbColorToUse_02);
        strip_04.SetPixelColor(LEDOffSet(3),rgbColorToUse_03);
        strip_04.SetPixelColor(LEDOffSet(4),rgbColorToUse_04);
        strip_04.SetPixelColor(LEDOffSet(5),rgbColorToUse_05);
        strip_04.SetPixelColor(LEDOffSet(6),rgbColorToUse_06);
        strip_04.SetPixelColor(LEDOffSet(7),rgbColorToUse_07);
        strip_04.SetPixelColor(LEDOffSet(8),rgbColorToUse_08);
        strip_04.SetPixelColor(LEDOffSet(9),rgbColorToUse_09);
        strip_04.SetPixelColor(LEDOffSet(10),rgbColorToUse_10);
        strip_04.SetPixelColor(LEDOffSet(11),rgbColorToUse_11);
        strip_04.SetPixelColor(LEDOffSet(12),rgbColorToUse_12);
        strip_04.Show();
        break;
        case 2:
        Serial.println("Setting Ring on GPIO05");
        strip_05.SetPixelColor(LEDOffSet(1),rgbColorToUse_01);
        strip_05.SetPixelColor(LEDOffSet(2),rgbColorToUse_02);
        strip_05.SetPixelColor(LEDOffSet(3),rgbColorToUse_03);
        strip_05.SetPixelColor(LEDOffSet(4),rgbColorToUse_04);
        strip_05.SetPixelColor(LEDOffSet(5),rgbColorToUse_05);
        strip_05.SetPixelColor(LEDOffSet(6),rgbColorToUse_06);
        strip_05.SetPixelColor(LEDOffSet(7),rgbColorToUse_07);
        strip_05.SetPixelColor(LEDOffSet(8),rgbColorToUse_08);
        strip_05.SetPixelColor(LEDOffSet(9),rgbColorToUse_09);
        strip_05.SetPixelColor(LEDOffSet(10),rgbColorToUse_10);
        strip_04.SetPixelColor(LEDOffSet(11),rgbColorToUse_11);
        strip_05.SetPixelColor(LEDOffSet(12),rgbColorToUse_12);
        strip_05.Show();
        break;
        case 3:
        Serial.println("Setting Ring on GPIO02");
        strip_02.SetPixelColor(LEDOffSet(1),rgbColorToUse_01);
        strip_02.SetPixelColor(LEDOffSet(2),rgbColorToUse_02);
        strip_02.SetPixelColor(LEDOffSet(3),rgbColorToUse_03);
        strip_02.SetPixelColor(LEDOffSet(4),rgbColorToUse_04);
        strip_02.SetPixelColor(LEDOffSet(5),rgbColorToUse_05);
        strip_02.SetPixelColor(LEDOffSet(6),rgbColorToUse_06);
        strip_02.SetPixelColor(LEDOffSet(7),rgbColorToUse_07);
        strip_02.SetPixelColor(LEDOffSet(8),rgbColorToUse_08);
        strip_02.SetPixelColor(LEDOffSet(9),rgbColorToUse_09);
        strip_02.SetPixelColor(LEDOffSet(10),rgbColorToUse_10);
        strip_02.SetPixelColor(LEDOffSet(11),rgbColorToUse_11);
        strip_02.SetPixelColor(LEDOffSet(12),rgbColorToUse_12);
        strip_02.Show();
        break;
        case 4:
        Serial.println("Setting Ring on GPIO15");
        strip_15.SetPixelColor(LEDOffSet(1),rgbColorToUse_01);
        strip_15.SetPixelColor(LEDOffSet(2),rgbColorToUse_02);
        strip_15.SetPixelColor(LEDOffSet(3),rgbColorToUse_03);
        strip_15.SetPixelColor(LEDOffSet(4),rgbColorToUse_04);
        strip_15.SetPixelColor(LEDOffSet(5),rgbColorToUse_05);
        strip_15.SetPixelColor(LEDOffSet(6),rgbColorToUse_06);
        strip_15.SetPixelColor(LEDOffSet(7),rgbColorToUse_07);
        strip_15.SetPixelColor(LEDOffSet(8),rgbColorToUse_08);
        strip_15.SetPixelColor(LEDOffSet(9),rgbColorToUse_09);
        strip_15.SetPixelColor(LEDOffSet(10),rgbColorToUse_10);
        strip_15.SetPixelColor(LEDOffSet(11),rgbColorToUse_11);
        strip_15.SetPixelColor(LEDOffSet(12),rgbColorToUse_12);
        strip_15.Show();
        break;
        case 5:
        Serial.println("Setting Ring on GPIO14");
        strip_14.SetPixelColor(LEDOffSet(1),rgbColorToUse_01);
        strip_14.SetPixelColor(LEDOffSet(2),rgbColorToUse_02);
        strip_14.SetPixelColor(LEDOffSet(3),rgbColorToUse_03);
        strip_14.SetPixelColor(LEDOffSet(4),rgbColorToUse_04);
        strip_14.SetPixelColor(LEDOffSet(5),rgbColorToUse_05);
        strip_14.SetPixelColor(LEDOffSet(6),rgbColorToUse_06);
        strip_14.SetPixelColor(LEDOffSet(7),rgbColorToUse_07);
        strip_14.SetPixelColor(LEDOffSet(8),rgbColorToUse_08);
        strip_14.SetPixelColor(LEDOffSet(9),rgbColorToUse_09);
        strip_14.SetPixelColor(LEDOffSet(10),rgbColorToUse_10);
        strip_14.SetPixelColor(LEDOffSet(11),rgbColorToUse_11);
        strip_14.SetPixelColor(LEDOffSet(12),rgbColorToUse_12);
        strip_14.Show();           
        break;
  }
}

//All LEDs are set to the same colour
//Single shot mode with no animation
void Set_Mode_001() {

  Serial.println("In Mode_001");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);
  
  for(int i=0; i < pixelCount; i++) 
  {
      strip_04.SetPixelColor(i,rgbColorToUse);
      strip_05.SetPixelColor(i,rgbColorToUse);
      strip_15.SetPixelColor(i,rgbColorToUse);
      strip_02.SetPixelColor(i,rgbColorToUse);
      strip_14.SetPixelColor(i,rgbColorToUse);
  }
  
  strip_04.Show();
  strip_05.Show(); 
  strip_15.Show();
  strip_02.Show();
  strip_14.Show();
}

//Rotating Single LED - Clockwise
void Set_Mode_002() 
{
  Serial.println("In Mode_002");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData) //set when data has been received.  Interupts the display mode.
   {
    Serial.println("In Mode_002_Loop");
    for(int i=0; i < pixelCount; i++) 
    {    
      int j = LEDOffSet(i); 
      
      strip_04.SetPixelColor(j,rgbColorToUse);
      strip_05.SetPixelColor(j,rgbColorToUse);
      strip_15.SetPixelColor(j,rgbColorToUse);
      strip_02.SetPixelColor(j,rgbColorToUse);
      strip_14.SetPixelColor(j,rgbColorToUse);

        if(j==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbBlack);
          strip_05.SetPixelColor(pixelCount-1,rgbBlack);
          strip_15.SetPixelColor(pixelCount-1,rgbBlack);
          strip_02.SetPixelColor(pixelCount-1,rgbBlack);
          strip_14.SetPixelColor(pixelCount-1,rgbBlack);
        }
        else
        {
          strip_04.SetPixelColor(j-1,rgbBlack);
          strip_05.SetPixelColor(j-1,rgbBlack);
          strip_15.SetPixelColor(j-1,rgbBlack);
          strip_02.SetPixelColor(j-1,rgbBlack);
          strip_14.SetPixelColor(j-1,rgbBlack);
        }
        
        strip_04.Show();
        strip_05.Show(); 
        strip_15.Show();
        strip_02.Show();
        strip_14.Show();
        delay(loopDelay);

        server.handleClient();
    }
  }
      Serial.println("Mode_002 Cancelled");
}


//Rotating Single LED - Anti-Clockwise
void Set_Mode_003() 
{
  Serial.println("In Mode_003");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_003_Loop");
    for(int i=pixelCount-1; i>=0; i--) 
    {     
      int j = LEDOffSet(i); 
      
      strip_04.SetPixelColor(j,rgbColorToUse);
      strip_05.SetPixelColor(j,rgbColorToUse);
      strip_15.SetPixelColor(j,rgbColorToUse);
      strip_02.SetPixelColor(j,rgbColorToUse);
      strip_14.SetPixelColor(j,rgbColorToUse);

        if(j==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);
        }
        else
        {
          strip_04.SetPixelColor(j+1,rgbBlack);
          strip_05.SetPixelColor(j+1,rgbBlack);
          strip_15.SetPixelColor(j+1,rgbBlack);
          strip_02.SetPixelColor(j+1,rgbBlack);
          strip_14.SetPixelColor(j+1,rgbBlack);
        }
        
          strip_04.Show();
          strip_05.Show(); 
          strip_15.Show();
          strip_02.Show();
          strip_14.Show();
          delay(loopDelay);

        server.handleClient();
    }
  }
    Serial.println("Mode_003 Cancelled");
}

//Rotating Single LED Turned Off - Clockwise
void Set_Mode_004() 
{
  Serial.println("In Mode_004");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_004_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
      int j = LEDOffSet(i); 
      
      strip_04.SetPixelColor(j,rgbBlack);
      strip_05.SetPixelColor(j,rgbBlack);
      strip_15.SetPixelColor(j,rgbBlack);
      strip_02.SetPixelColor(j,rgbBlack);
      strip_14.SetPixelColor(j,rgbBlack);
      
        if(j==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_05.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_15.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_02.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_14.SetPixelColor(pixelCount-1,rgbColorToUse);
        }
        else
        {
          strip_04.SetPixelColor(j-1,rgbColorToUse);
          strip_05.SetPixelColor(j-1,rgbColorToUse);
          strip_15.SetPixelColor(j-1,rgbColorToUse);
          strip_02.SetPixelColor(j-1,rgbColorToUse);
          strip_14.SetPixelColor(j-1,rgbColorToUse);
        }
        
          strip_04.Show();
          strip_05.Show(); 
          strip_15.Show();
          strip_02.Show();
          strip_14.Show();
        delay(loopDelay);
  
        server.handleClient();
    }
  }
      Serial.println("Mode_004 Cancelled");
}


//Rotating Single LED Turned Off - Anti-Clockwise
void Set_Mode_005() 
{
  Serial.println("In Mode_005");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_005_Loop");
    for(int i=pixelCount-1; i>=0; i--) 
    {     
        int j = LEDOffSet(i);
         
        strip_04.SetPixelColor(j,rgbBlack);
        strip_05.SetPixelColor(j,rgbBlack);
        strip_15.SetPixelColor(j,rgbBlack);
        strip_02.SetPixelColor(j,rgbBlack);
        strip_14.SetPixelColor(j,rgbBlack);
  
        if(j==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbColorToUse);
          strip_05.SetPixelColor(0,rgbColorToUse);
          strip_15.SetPixelColor(0,rgbColorToUse);
          strip_02.SetPixelColor(0,rgbColorToUse);
          strip_14.SetPixelColor(0,rgbColorToUse);
        }
        else
        {
          strip_04.SetPixelColor(j+1,rgbColorToUse);
          strip_05.SetPixelColor(j+1,rgbColorToUse);
          strip_15.SetPixelColor(j+1,rgbColorToUse);
          strip_02.SetPixelColor(j+1,rgbColorToUse);
          strip_14.SetPixelColor(j+1,rgbColorToUse);
        }
  
          strip_04.Show();
          strip_05.Show(); 
          strip_15.Show();
          strip_02.Show();
          strip_14.Show();
        delay(loopDelay);
  
        server.handleClient();
    }
  }
    Serial.println("Mode_005 Cancelled");
}

//2 Leds moving in anti and clockwise directiions 
void Set_Mode_006() 
{
  Serial.println("In Mode_006");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_006_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
        int j = LEDOffSet(i); 
        //Clockwise
        strip_04.SetPixelColor(j,rgbColorToUse);
        strip_05.SetPixelColor(j,rgbColorToUse);
        strip_15.SetPixelColor(j,rgbColorToUse);
        strip_02.SetPixelColor(j,rgbColorToUse);
        strip_14.SetPixelColor(j,rgbColorToUse);
  
        if(j==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbBlack);
          strip_05.SetPixelColor(pixelCount-1,rgbBlack);
          strip_15.SetPixelColor(pixelCount-1,rgbBlack);
          strip_02.SetPixelColor(pixelCount-1,rgbBlack);
          strip_14.SetPixelColor(pixelCount-1,rgbBlack);
        }
        else
        {
          strip_05.SetPixelColor(j-1,rgbBlack);
  
          strip_04.SetPixelColor(j-1,rgbBlack);
          strip_05.SetPixelColor(j-1,rgbBlack);
          strip_15.SetPixelColor(j-1,rgbBlack);
          strip_02.SetPixelColor(j-1,rgbBlack);
          strip_14.SetPixelColor(j-1,rgbBlack);
        }
  
        //Anticlockwise
        int i180 = LED180(j);
  
        strip_04.SetPixelColor(i180,rgbColorToUse);
        strip_05.SetPixelColor(i180,rgbColorToUse);
        strip_15.SetPixelColor(i180,rgbColorToUse);
        strip_02.SetPixelColor(i180,rgbColorToUse);
        strip_14.SetPixelColor(i180,rgbColorToUse);
  
        if(i180==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);
        }
        else
        {
          strip_04.SetPixelColor(i180+1,rgbBlack);
          strip_05.SetPixelColor(i180+1,rgbBlack);
          strip_15.SetPixelColor(i180+1,rgbBlack);
          strip_02.SetPixelColor(i180+1,rgbBlack);
          strip_14.SetPixelColor(i180+1,rgbBlack);
        }
  
          strip_04.Show();
          strip_05.Show(); 
          strip_15.Show();
          strip_02.Show();
          strip_14.Show();
          delay(loopDelay);
  
          server.handleClient();
    }
  }
      Serial.println("Mode_006 Cancelled");
}


//Clockwise with trails of 4 LEds
void Set_Mode_007() 
{
  Serial.println("In Mode_007");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse_01 = CorrectedColour("colour_001",brightnessModifier);
  RgbColor rgbColorToUse_02 = CorrectedColour("colour_002",brightnessModifier);
  RgbColor rgbColorToUse_03 = CorrectedColour("colour_003",brightnessModifier);
  RgbColor rgbColorToUse_04 = CorrectedColour("colour_004",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

  while(!getNewData)
  {
    Serial.println("In Mode_007_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
        int j = LEDOffSet(i); 
        //Clockwise
        strip_04.SetPixelColor(j,rgbColorToUse_01);
        strip_04.SetPixelColor(ClockMinusOne(j),rgbColorToUse_02);
        strip_04.SetPixelColor(ClockMinusOne(ClockMinusOne(j)),rgbColorToUse_03);
        strip_04.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(j))),rgbColorToUse_04); 
  
        strip_05.SetPixelColor(j,rgbColorToUse_01);
        strip_05.SetPixelColor(ClockMinusOne(j),rgbColorToUse_02);
        strip_05.SetPixelColor(ClockMinusOne(ClockMinusOne(j)),rgbColorToUse_03);
        strip_05.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(j))),rgbColorToUse_04);
  
        strip_15.SetPixelColor(i,rgbColorToUse_01);
        strip_15.SetPixelColor(ClockMinusOne(j),rgbColorToUse_02);
        strip_15.SetPixelColor(ClockMinusOne(ClockMinusOne(j)),rgbColorToUse_03);
        strip_15.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(j))),rgbColorToUse_04); 
  
        strip_02.SetPixelColor(i,rgbColorToUse_01);
        strip_02.SetPixelColor(ClockMinusOne(j),rgbColorToUse_02);
        strip_02.SetPixelColor(ClockMinusOne(ClockMinusOne(j)),rgbColorToUse_03);
        strip_02.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(j))),rgbColorToUse_04); 
  
        strip_14.SetPixelColor(i,rgbColorToUse_01);
        strip_14.SetPixelColor(ClockMinusOne(j),rgbColorToUse_02);
        strip_14.SetPixelColor(ClockMinusOne(ClockMinusOne(j)),rgbColorToUse_03);
        strip_14.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(j))),rgbColorToUse_04);
  
        strip_04.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(j)))),rgbBlack);
        strip_05.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(j)))),rgbBlack);
        strip_15.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(j)))),rgbBlack);
        strip_02.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(j)))),rgbBlack);
        strip_14.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(j)))),rgbBlack);
  
        strip_04.Show();
        strip_05.Show(); 
        strip_15.Show();
        strip_02.Show();
        strip_14.Show();
        delay(loopDelay);
  
        server.handleClient();
    }
  }
  Serial.println("Mode_007 Cancelled");
}


//Both Motors On
void Set_Mode_008() 
{
  Serial.println("In Mode_008");

  digitalWrite(ledPin_12, HIGH);

  digitalWrite(ledPin_13, HIGH);
}

//Both Motors Off
void Set_Mode_009() 
{
  Serial.println("In Mode_009");

  digitalWrite(ledPin_12, LOW);

  digitalWrite(ledPin_13, LOW);
}

//Pulsed motor
void Set_Mode_010() 
{
  Serial.println("In Mode_010");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element
  
  while(!getNewData)
  {
    Serial.println("In Mode_010_Loop");
    
    digitalWrite(ledPin_12, HIGH);
    digitalWrite(ledPin_13, HIGH);
    delay(loopDelay);
    
    digitalWrite(ledPin_12, LOW);
    digitalWrite(ledPin_13, LOW);
    delay(loopDelay);
        
    server.handleClient();
  }
  Serial.println("Mode_010 Cancelled");
}


//2 Leds moving in anti and clockwise directiions with pulsed motors
void Set_Mode_011() 
{
  Serial.println("In Mode_011");

  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

  while(!getNewData)
  {
    Serial.println("In Mode_011_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
        int j = LEDOffSet(i); 
      
        //Clockwise
        strip_04.SetPixelColor(j,rgbColorToUse);
        strip_05.SetPixelColor(j,rgbColorToUse);
        strip_15.SetPixelColor(j,rgbColorToUse);
        strip_02.SetPixelColor(j,rgbColorToUse);
        strip_14.SetPixelColor(j,rgbColorToUse);
    
        if(j==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbBlack);
          strip_05.SetPixelColor(pixelCount-1,rgbBlack);
          strip_15.SetPixelColor(pixelCount-1,rgbBlack);
          strip_02.SetPixelColor(pixelCount-1,rgbBlack);
          strip_14.SetPixelColor(pixelCount-1,rgbBlack);
        }
        else
        {
          strip_05.SetPixelColor(j-1,rgbBlack);
    
          strip_04.SetPixelColor(j-1,rgbBlack);
          strip_05.SetPixelColor(j-1,rgbBlack);
          strip_15.SetPixelColor(j-1,rgbBlack);
          strip_02.SetPixelColor(j-1,rgbBlack);
          strip_14.SetPixelColor(j-1,rgbBlack);
        }
    
        //Anticlockwise
        int i180 = LED180(j);
    
        strip_04.SetPixelColor(i180,rgbColorToUse);
        strip_05.SetPixelColor(i180,rgbColorToUse);
        strip_15.SetPixelColor(i180,rgbColorToUse);
        strip_02.SetPixelColor(i180,rgbColorToUse);
        strip_14.SetPixelColor(i180,rgbColorToUse);
    
        if(i180==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);
        }
        else
        {
          strip_04.SetPixelColor(i180+1,rgbBlack);
          strip_05.SetPixelColor(i180+1,rgbBlack);
          strip_15.SetPixelColor(i180+1,rgbBlack);
          strip_02.SetPixelColor(i180+1,rgbBlack);
          strip_14.SetPixelColor(i180+1,rgbBlack);
        }
  
          strip_04.Show();
          strip_05.Show(); 
          strip_15.Show();
          strip_02.Show();
          strip_14.Show();
    
          Set_Mode_008(); //motors on
          delay(loopDelay/2);
          Set_Mode_009(); //motors off
          delay(loopDelay/2);
        
          server.handleClient();
    }
  }
      Serial.println("Mode_011 Cancelled");
}

//Rotate between rings, not in a ring
void Set_Mode_012() 
{
  Serial.println("In Mode_012");
  
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element
  
   ClearAll();
   while(!getNewData)
   {  
      Serial.println("In Mode_012_Loop");

      strip_05.SetPixelColor(LEDOffSet(3),rgbColorToUse);
      strip_05.Show();
      delay(loopDelay);
      strip_05.SetPixelColor(LEDOffSet(3),rgbBlack);
      strip_05.Show();
      
      strip_04.SetPixelColor(LEDOffSet(3),rgbColorToUse);
      strip_04.Show();
      delay(loopDelay);      
      strip_04.SetPixelColor(LEDOffSet(3),rgbBlack);
      strip_04.Show();
      
      strip_15.SetPixelColor(LEDOffSet(3),rgbColorToUse);
      strip_15.Show();
      delay(loopDelay);
      strip_15.SetPixelColor(LEDOffSet(3),rgbBlack);
      strip_15.Show();
      
      strip_02.SetPixelColor(LEDOffSet(3),rgbColorToUse);
      strip_02.Show();
      delay(loopDelay);
      strip_02.SetPixelColor(LEDOffSet(3),rgbBlack);
      strip_02.Show();
      
      strip_14.SetPixelColor(LEDOffSet(3),rgbColorToUse);
      strip_14.Show();
      delay(loopDelay);
      strip_14.SetPixelColor(LEDOffSet(3),rgbBlack);
      strip_14.Show();
      
      server.handleClient();
  }
      Serial.println("Mode_012 Cancelled");
}

//Twinkle
//Each LED should show a slightly different colour and change over time
//NOT WORKING
void Set_Mode_013() 
{
  Serial.println("In Mode_013");

  int loopDelay = server.arg("loopdelay").toInt();
  int variation = server.arg("variation").toInt();
  int brightnessModifier = server.arg("brightness").toInt();
  
  int rgb[3];
  GetColourAsRGBArray("colour_001", rgb);

   ClearAll();
   while(!getNewData)
   {
      Serial.println("In Mode_013_Loop");
      strip_04.SetPixelColor(LEDOffSet(1),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(2),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(3),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(4),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(5),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(6),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(7),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(8),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(9),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(10),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(11),TwinkleColour(rgb , variation, brightnessModifier));
      strip_04.SetPixelColor(LEDOffSet(12),TwinkleColour(rgb , variation, brightnessModifier));
    
    
      strip_05.SetPixelColor(LEDOffSet(1),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(2),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(3),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(4),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(5),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(6),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(7),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(8),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(9),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(10),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(11),TwinkleColour(rgb , variation, brightnessModifier));
      strip_05.SetPixelColor(LEDOffSet(12),TwinkleColour(rgb , variation, brightnessModifier));
    
    
      strip_02.SetPixelColor(LEDOffSet(1),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(2),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(3),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(4),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(5),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(6),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(7),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(8),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(9),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(10),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(11),TwinkleColour(rgb , variation, brightnessModifier));
      strip_02.SetPixelColor(LEDOffSet(12),TwinkleColour(rgb , variation, brightnessModifier));
    
    
      strip_15.SetPixelColor(LEDOffSet(1),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(2),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(3),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(4),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(5),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(6),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(7),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(8),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(9),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(10),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(11),TwinkleColour(rgb , variation, brightnessModifier));
      strip_15.SetPixelColor(LEDOffSet(12),TwinkleColour(rgb , variation, brightnessModifier));
    
    
      strip_14.SetPixelColor(LEDOffSet(1),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(2),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(3),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(4),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(5),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(6),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(7),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(8),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(9),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(10),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(11),TwinkleColour(rgb , variation, brightnessModifier));
      strip_14.SetPixelColor(LEDOffSet(12),TwinkleColour(rgb , variation, brightnessModifier));
    
      strip_04.Show();
      strip_05.Show();
      strip_02.Show();
      strip_15.Show();
      strip_14.Show(); 

      delay(loopDelay);
      server.handleClient();
   }          
  Serial.println("Mode_013 Cancelled");
}

//All LEDs are set to the same colour
//Colour should change over time but be close to original colour
//Started life as Twinkle tester
void Set_Mode_014() {

  Serial.println("In Mode_014");

  int loopDelay = server.arg("loopdelay").toInt();
  int variation = server.arg("variation").toInt();
  int brightnessModifier = server.arg("brightness").toInt();

  int rgb[3];
  GetColourAsRGBArray("colour_001", rgb);

   while(!getNewData) //set when data has been received.  Interupts the display mode.
   {
      Serial.println("In Mode_014_Loop");
      RgbColor rgbColorToUse = TwinkleColour(rgb , variation, brightnessModifier);

      for(int i=0; i < pixelCount; i++) 
      {
          strip_04.SetPixelColor(i,rgbColorToUse);
          strip_05.SetPixelColor(i,rgbColorToUse);
          strip_15.SetPixelColor(i,rgbColorToUse);
          strip_02.SetPixelColor(i,rgbColorToUse);
          strip_14.SetPixelColor(i,rgbColorToUse);
      }

      strip_04.Show();
      strip_05.Show(); 
      strip_15.Show();
      strip_02.Show();
      strip_14.Show();
    
      delay(loopDelay);

      server.handleClient();
  }
  Serial.println("Mode_014 Cancelled");
}


//Clear all rings
void Set_Mode_015() {

  Serial.println("In Mode_015");

  ClearAll();
}

//All LEDs in a ring are set to the same colour
//Single shot mode with no animation
//Repeated use of the command allows rings to be set without effecting other rings
void Set_Mode_016() {

  Serial.println("In Mode_016");
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);
  int ringID = server.arg("ring").toInt();
     switch (ringID) 
     {
       case 1:
          for(int i=0; i < pixelCount; i++) 
          {
              strip_04.SetPixelColor(i,rgbColorToUse);
          }          
          strip_04.Show();
          break;
       case 2:
          for(int i=0; i < pixelCount; i++) 
          {
              strip_05.SetPixelColor(i,rgbColorToUse);
          }
          strip_05.Show();
          break;
       case 3:
          for(int i=0; i < pixelCount; i++) 
          {
              strip_02.SetPixelColor(i,rgbColorToUse);
          }
          strip_02.Show();
          break;
       case 4:
          for(int i=0; i < pixelCount; i++) 
          {
              strip_15.SetPixelColor(i,rgbColorToUse);
          }
          strip_15.Show();
          break;
       case 5:
          for(int i=0; i < pixelCount; i++) 
          {
              strip_14.SetPixelColor(i,rgbColorToUse);
          }
          strip_14.Show();
          break;
     }
}

//All LEDs in a ring are set to the same colour
//They then animate from 0 brightness to 100 and back and repeat
void Set_Mode_017() {

  Serial.println("In Mode_017");
  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element
  int ringID = server.arg("ring").toInt();

  int brightnessModifier=0;
  String direction="up";
  
  while(!getNewData) //set when data has been received.  Interupts the display mode.
  {
    if(direction=="up")
    {
      brightnessModifier++;
      if(brightnessModifier==100)
      {
        direction="down";
      }
    }
    else
    {
      brightnessModifier--;
      if(brightnessModifier==1)
      {
        direction="up";
      } 
    }
    
    RgbColor rgbColorToUse = CorrectedColour("colour_001",brightnessModifier);
       switch (ringID) 
       {
         case 1:
            for(int i=0; i < pixelCount; i++) 
            {
                strip_04.SetPixelColor(i,rgbColorToUse);
            }          
            strip_04.Show();
            break;
         case 2:
            for(int i=0; i < pixelCount; i++) 
            {
                strip_05.SetPixelColor(i,rgbColorToUse);
            }
            strip_05.Show();
            break;
         case 3:
            for(int i=0; i < pixelCount; i++) 
            {
                strip_02.SetPixelColor(i,rgbColorToUse);
            }
            strip_02.Show();
            break;
         case 4:
            for(int i=0; i < pixelCount; i++) 
            {
                strip_15.SetPixelColor(i,rgbColorToUse);
            }
            strip_15.Show();
            break;
         case 5:
            for(int i=0; i < pixelCount; i++) 
            {
                strip_14.SetPixelColor(i,rgbColorToUse);
            }
            strip_14.Show();
            break;
       }
    delay(loopDelay);

    server.handleClient();
  }
  Serial.println("Mode_017 Cancelled");
}


//leds trickle down to the bottom which slowly fills
void Set_Mode_018() {

  Serial.println("In Mode_018");

  int trickleDelay = server.arg("trickledelay").toInt();
  int endDelay = server.arg("enddelay").toInt();
  int loopDelay = server.arg("loopdelay").toInt();
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse = CorrectedColour("colour_trickle",brightnessModifier);
  RgbColor rgbColorToUseFill_01 = CorrectedColour("colour_fill_mid",brightnessModifier);
  RgbColor rgbColorToUseFill_Final = CorrectedColour("colour_fill_final",brightnessModifier);
  

while(!getNewData)
{
  ClearAll();
  int fillCount=0;  //where we are in the fill
  int fill=3; //delay between fill
  int currentLevel=1;  //the fill level
  int LastDeleteBoundry = 0; //when we cross a currentLevel boundry we still need to delete th elast lit LED before the boundry was crossed
  
  while(currentLevel<=7)
    {
//      Serial.println("In Mode_017_Loop");
      
      for(int i=0; i <= (pixelCount/2)-(currentLevel); i++) 
      {    
        int actualLEDClockwise = LEDOffSet(i);
      
        strip_04.SetPixelColor(actualLEDClockwise,rgbColorToUse);
        strip_05.SetPixelColor(actualLEDClockwise,rgbColorToUse);
        strip_15.SetPixelColor(actualLEDClockwise,rgbColorToUse);
        strip_02.SetPixelColor(actualLEDClockwise,rgbColorToUse);
        strip_14.SetPixelColor(actualLEDClockwise,rgbColorToUse);
  
        int actualLEDCurrentBottomClockwise=0;
  
        if(i==0)//We're at the top so we delete the current bottom as set in the last itteration
        {
            actualLEDCurrentBottomClockwise = LEDOffSet((pixelCount/2)-(currentLevel-LastDeleteBoundry));
            strip_04.SetPixelColor(actualLEDCurrentBottomClockwise,rgbBlack);
            strip_05.SetPixelColor(actualLEDCurrentBottomClockwise,rgbBlack);
            strip_15.SetPixelColor(actualLEDCurrentBottomClockwise,rgbBlack);
            strip_02.SetPixelColor(actualLEDCurrentBottomClockwise,rgbBlack);
            strip_14.SetPixelColor(actualLEDCurrentBottomClockwise,rgbBlack);
        }
  
        int  actualLEDToRemoveClockwise=LEDOffSet((i-1)%12);
        
        strip_04.SetPixelColor(actualLEDToRemoveClockwise,rgbBlack);
        strip_05.SetPixelColor(actualLEDToRemoveClockwise,rgbBlack);
        strip_15.SetPixelColor(actualLEDToRemoveClockwise,rgbBlack);
        strip_02.SetPixelColor(actualLEDToRemoveClockwise,rgbBlack);
        strip_14.SetPixelColor(actualLEDToRemoveClockwise,rgbBlack);
  
//        Serial.println("-------------------------------");
//        Serial.print("i ");
//        Serial.println(i);
//        Serial.print("actualLEDClockwise ");
//        Serial.println(actualLEDClockwise);
//        Serial.print("actualLEDCurrentBottomClockwise ");
//        Serial.println(actualLEDCurrentBottomClockwise);
//        Serial.print("actualLEDToRemoveClockwise ");
//        Serial.println(actualLEDToRemoveClockwise);
//        Serial.println("-------------------------------");   
      
        int actualLEDAntiClockwise=LEDOffSetMirrorVertical(i);
  
        strip_04.SetPixelColor(actualLEDAntiClockwise,rgbColorToUse);
        strip_05.SetPixelColor(actualLEDAntiClockwise,rgbColorToUse);
        strip_15.SetPixelColor(actualLEDAntiClockwise,rgbColorToUse);
        strip_02.SetPixelColor(actualLEDAntiClockwise,rgbColorToUse);
        strip_14.SetPixelColor(actualLEDAntiClockwise,rgbColorToUse);
  
        int actualLEDCurrentBottomAntiClockwise=0;
      
        if(i==0)//We're at the top so we delete the current bottom as set in the last itteration
        {
          int actualLEDCurrentBottomAntiClockwise = LEDOffSetMirrorVertical((pixelCount/2)-(currentLevel-LastDeleteBoundry));
          strip_04.SetPixelColor(actualLEDCurrentBottomAntiClockwise,rgbBlack);
          strip_05.SetPixelColor(actualLEDCurrentBottomAntiClockwise,rgbBlack);
          strip_15.SetPixelColor(actualLEDCurrentBottomAntiClockwise,rgbBlack);
          strip_02.SetPixelColor(actualLEDCurrentBottomAntiClockwise,rgbBlack);
          strip_14.SetPixelColor(actualLEDCurrentBottomAntiClockwise,rgbBlack);
        }
  
        int  actualLEDToRemoveAntiClockwise=LEDOffSetMirrorVertical((i-1)%12);
        
        strip_04.SetPixelColor(actualLEDToRemoveAntiClockwise,rgbBlack);
        strip_05.SetPixelColor(actualLEDToRemoveAntiClockwise,rgbBlack);
        strip_15.SetPixelColor(actualLEDToRemoveAntiClockwise,rgbBlack);
        strip_02.SetPixelColor(actualLEDToRemoveAntiClockwise,rgbBlack);
        strip_14.SetPixelColor(actualLEDToRemoveAntiClockwise,rgbBlack);
  
//        Serial.println("-------------------------------");
//        Serial.print("i ");
//        Serial.println(i);
//        Serial.print("actualLEDAntiClockwise ");
//        Serial.println(actualLEDAntiClockwise);
//        Serial.print("actualLEDCurrentBottomAntiClockwise ");
//        Serial.println(actualLEDCurrentBottomAntiClockwise);
//        Serial.print("actualLEDToRemoveAntiClockwise ");
//        Serial.println(actualLEDToRemoveAntiClockwise);
//        Serial.println("-------------------------------");  
  
        strip_04.Show();
        strip_05.Show(); 
        strip_15.Show();
        strip_02.Show();
        strip_14.Show();
  
        delay(trickleDelay);
       }
  
       //We are at a fill point
       //One day, in the future, I'll work out the code to set the brightness here based on the fill rate
       //For now we use the colours from the command and have a fill rate of 3
       //Think of it as a feature
  
      fillCount++;
  
//      Serial.println("----");
//      Serial.print("fillCount ");
//      Serial.println(fillCount);
//      Serial.println("----");
  
  
  //Set end Conditions
  //so if currentLevel = 1 then we are at the bottom of the ring
  //And if currentLevel = 7? then we are at the top
  //if fillCount = 1 then rgbColorToUse
  //if fillCount = 2 then rgbColorToUseFill_01
  //if fillCount = 3 then rgbColorToUseFill_Final
  
      int Led1=LEDOffSet(1);
      int Led2=LEDOffSet(2);
      int Led3=LEDOffSet(3);
      int Led4=LEDOffSet(4);
      int Led5=LEDOffSet(5);
      int Led6=LEDOffSet(6);
      int Led7=LEDOffSet(7);
      int Led8=LEDOffSet(8);
      int Led9=LEDOffSet(9);
      int Led10=LEDOffSet(10);
      int Led11=LEDOffSet(11);
      int Led12=LEDOffSet(12);    
  
       switch (currentLevel) 
       {
            case 1:
            //Bottom only
            switch (fillCount)
            {
              case 1:
                strip_04.SetPixelColor(Led6,rgbColorToUse);
                strip_05.SetPixelColor(Led6,rgbColorToUse);
                strip_15.SetPixelColor(Led6,rgbColorToUse);
                strip_02.SetPixelColor(Led6,rgbColorToUse);
                strip_14.SetPixelColor(Led6,rgbColorToUse);
              break;
              case 2:
                strip_04.SetPixelColor(Led6,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led6,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led6,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led6,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led6,rgbColorToUseFill_01);
              break;
              case 3:
                strip_04.SetPixelColor(Led6,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led6,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led6,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led6,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led6,rgbColorToUseFill_Final);
              break;
            }
            break;
  
            case 2:
            switch (fillCount)
            {
                case 1:
                strip_04.SetPixelColor(Led5,rgbColorToUse);
                strip_05.SetPixelColor(Led5,rgbColorToUse);
                strip_15.SetPixelColor(Led5,rgbColorToUse);
                strip_02.SetPixelColor(Led5,rgbColorToUse);
                strip_14.SetPixelColor(Led5,rgbColorToUse);
  
                strip_04.SetPixelColor(Led7,rgbColorToUse);
                strip_05.SetPixelColor(Led7,rgbColorToUse);
                strip_15.SetPixelColor(Led7,rgbColorToUse);
                strip_02.SetPixelColor(Led7,rgbColorToUse);
                strip_14.SetPixelColor(Led7,rgbColorToUse);
              break;
              case 2:
                strip_04.SetPixelColor(Led5,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led5,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led5,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led5,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led5,rgbColorToUseFill_01);
  
                strip_04.SetPixelColor(Led7,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led7,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led7,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led7,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led7,rgbColorToUseFill_01); 
              break;
              case 3:
                strip_04.SetPixelColor(Led5,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led5,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led5,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led5,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led5,rgbColorToUseFill_Final);
  
                strip_04.SetPixelColor(Led7,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led7,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led7,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led7,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led7,rgbColorToUseFill_Final);
              break;
            }
            break;
            
            case 3:
            switch (fillCount)
            {
                case 1:
                strip_04.SetPixelColor(Led4,rgbColorToUse);
                strip_05.SetPixelColor(Led4,rgbColorToUse);
                strip_15.SetPixelColor(Led4,rgbColorToUse);
                strip_02.SetPixelColor(Led4,rgbColorToUse);
                strip_14.SetPixelColor(Led4,rgbColorToUse);
  
                strip_04.SetPixelColor(Led8,rgbColorToUse);
                strip_05.SetPixelColor(Led8,rgbColorToUse);
                strip_15.SetPixelColor(Led8,rgbColorToUse);
                strip_02.SetPixelColor(Led8,rgbColorToUse);
                strip_14.SetPixelColor(Led8,rgbColorToUse);
              break;
              case 2:
                strip_04.SetPixelColor(Led4,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led4,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led4,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led4,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led4,rgbColorToUseFill_01);
  
                strip_04.SetPixelColor(Led8,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led8,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led8,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led8,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led8,rgbColorToUseFill_01); 
              break;
              case 3:
                strip_04.SetPixelColor(Led4,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led4,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led4,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led4,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led4,rgbColorToUseFill_Final);
  
                strip_04.SetPixelColor(Led8,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led8,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led8,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led8,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led8,rgbColorToUseFill_Final);
              break;
            }
            break;
            
            case 4:
            switch (fillCount)
            {
                case 1:
                strip_04.SetPixelColor(Led3,rgbColorToUse);
                strip_05.SetPixelColor(Led3,rgbColorToUse);
                strip_15.SetPixelColor(Led3,rgbColorToUse);
                strip_02.SetPixelColor(Led3,rgbColorToUse);
                strip_14.SetPixelColor(Led3,rgbColorToUse);
  
                strip_04.SetPixelColor(Led9,rgbColorToUse);
                strip_05.SetPixelColor(Led9,rgbColorToUse);
                strip_15.SetPixelColor(Led9,rgbColorToUse);
                strip_02.SetPixelColor(Led9,rgbColorToUse);
                strip_14.SetPixelColor(Led9,rgbColorToUse);
              break;
              case 2:
                strip_04.SetPixelColor(Led3,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led3,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led3,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led3,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led3,rgbColorToUseFill_01);
  
                strip_04.SetPixelColor(Led9,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led9,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led9,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led9,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led9,rgbColorToUseFill_01); 
              break;
              case 3:
                strip_04.SetPixelColor(Led3,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led3,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led3,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led3,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led3,rgbColorToUseFill_Final);
  
                strip_04.SetPixelColor(Led9,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led9,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led9,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led9,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led9,rgbColorToUseFill_Final);
              break;
            }
            break;
            
            case 5:
            switch (fillCount)
            {
                case 1:
                strip_04.SetPixelColor(Led2,rgbColorToUse);
                strip_05.SetPixelColor(Led2,rgbColorToUse);
                strip_15.SetPixelColor(Led2,rgbColorToUse);
                strip_02.SetPixelColor(Led2,rgbColorToUse);
                strip_14.SetPixelColor(Led2,rgbColorToUse);
  
                strip_04.SetPixelColor(Led10,rgbColorToUse);
                strip_05.SetPixelColor(Led10,rgbColorToUse);
                strip_15.SetPixelColor(Led10,rgbColorToUse);
                strip_02.SetPixelColor(Led10,rgbColorToUse);
                strip_14.SetPixelColor(Led10,rgbColorToUse);
              break;
              case 2:
                strip_04.SetPixelColor(Led2,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led2,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led2,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led2,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led2,rgbColorToUseFill_01);
  
                strip_04.SetPixelColor(Led10,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led10,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led10,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led10,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led10,rgbColorToUseFill_01); 
              break;
              case 3:
                strip_04.SetPixelColor(Led2,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led2,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led2,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led2,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led2,rgbColorToUseFill_Final);
  
                strip_04.SetPixelColor(Led10,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led10,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led10,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led10,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led10,rgbColorToUseFill_Final);
              break;
            }
            break;
            
            case 6:
            switch (fillCount)
            {
                case 1:
                strip_04.SetPixelColor(Led1,rgbColorToUse);
                strip_05.SetPixelColor(Led1,rgbColorToUse);
                strip_15.SetPixelColor(Led1,rgbColorToUse);
                strip_02.SetPixelColor(Led1,rgbColorToUse);
                strip_14.SetPixelColor(Led1,rgbColorToUse);
  
                strip_04.SetPixelColor(Led11,rgbColorToUse);
                strip_05.SetPixelColor(Led11,rgbColorToUse);
                strip_15.SetPixelColor(Led11,rgbColorToUse);
                strip_02.SetPixelColor(Led11,rgbColorToUse);
                strip_14.SetPixelColor(Led11,rgbColorToUse);
              break;
              case 2:
                strip_04.SetPixelColor(Led1,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led1,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led1,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led1,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led1,rgbColorToUseFill_01);
  
                strip_04.SetPixelColor(Led11,rgbColorToUseFill_01);
                strip_05.SetPixelColor(Led11,rgbColorToUseFill_01);
                strip_15.SetPixelColor(Led11,rgbColorToUseFill_01);
                strip_02.SetPixelColor(Led11,rgbColorToUseFill_01);
                strip_14.SetPixelColor(Led11,rgbColorToUseFill_01); 
              break;
              case 3:
                strip_04.SetPixelColor(Led1,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led1,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led1,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led1,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led1,rgbColorToUseFill_Final);
  
                strip_04.SetPixelColor(Led11,rgbColorToUseFill_Final);
                strip_05.SetPixelColor(Led11,rgbColorToUseFill_Final);
                strip_15.SetPixelColor(Led11,rgbColorToUseFill_Final);
                strip_02.SetPixelColor(Led11,rgbColorToUseFill_Final);
                strip_14.SetPixelColor(Led11,rgbColorToUseFill_Final);
              break;
            }
            break;
       }
        strip_04.Show();
        strip_05.Show(); 
        strip_15.Show();
        strip_02.Show();
        strip_14.Show();
  
        LastDeleteBoundry=0;
        if(fillCount==fill)
        {
          LastDeleteBoundry=1;
          currentLevel++;
          fillCount=0;
        }
//      Serial.println("----");
//      Serial.print("currentLevel ");
//      Serial.println(currentLevel);
//      Serial.println("----");
//  
//      Serial.println("----");
//      Serial.print("LastDeleteBoundry ");
//      Serial.println(LastDeleteBoundry);
//      Serial.println("----");
  
      delay(endDelay);
  
      server.handleClient();
    }
    delay(loopDelay);
}

}

//All LEDs are set to the same colour
//Colours pulse over time
//4 levels of pulse
void Set_Mode_019() {

  Serial.println("In Mode_019");

  int loopDelay = server.arg("loopdelay").toInt();
  int stepDelay = server.arg("stepdelay").toInt();
  int endDelay = server.arg("enddelay").toInt();
  int brightnessModifier = server.arg("brightness").toInt();
  RgbColor rgbColorToUse_start = CorrectedColour("colour_start",brightnessModifier);
  RgbColor rgbColorToUse_mid_01 = CorrectedColour("colour_mid_01",brightnessModifier);
  RgbColor rgbColorToUse_mid_02 = CorrectedColour("colour_mid_02",brightnessModifier);
  RgbColor rgbColorToUse_mid_03 = CorrectedColour("colour_mid_03",brightnessModifier);
  RgbColor rgbColorToUse_mid_04 = CorrectedColour("colour_mid_04",brightnessModifier);
  RgbColor rgbColorToUse_mid_05 = CorrectedColour("colour_mid_05",brightnessModifier);
  RgbColor rgbColorToUse_mid_06 = CorrectedColour("colour_mid_06",brightnessModifier);
  RgbColor rgbColorToUse_mid_07 = CorrectedColour("colour_mid_07",brightnessModifier);
  RgbColor rgbColorToUse_mid_08 = CorrectedColour("colour_mid_08",brightnessModifier);
  RgbColor rgbColorToUse_mid_09 = CorrectedColour("colour_mid_09",brightnessModifier);
  RgbColor rgbColorToUse_mid_10 = CorrectedColour("colour_mid_10",brightnessModifier);
  RgbColor rgbColorToUse_mid_11 = CorrectedColour("colour_mid_11",brightnessModifier);
  RgbColor rgbColorToUse_mid_12 = CorrectedColour("colour_mid_12",brightnessModifier);
  RgbColor rgbColorToUse_end = CorrectedColour("colour_end",brightnessModifier);

  while(!getNewData) //set when data has been received.  Interupts the display mode.
  {
    Serial.println("In Mode_019_Loop");
  
    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_start);
        strip_05.SetPixelColor(i,rgbColorToUse_start);
        strip_15.SetPixelColor(i,rgbColorToUse_start);
        strip_02.SetPixelColor(i,rgbColorToUse_start);
        strip_14.SetPixelColor(i,rgbColorToUse_start);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();

    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_01);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_02);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_03);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_04);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);        

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_05);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_06);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay); 

     for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_07);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_08);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_09);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_10);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);        

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_11);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_12);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay); 
           
    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_end);
        strip_05.SetPixelColor(i,rgbColorToUse_end);
        strip_15.SetPixelColor(i,rgbColorToUse_end);
        strip_02.SetPixelColor(i,rgbColorToUse_end);
        strip_14.SetPixelColor(i,rgbColorToUse_end);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();

    delay(endDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_12);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_12);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_11);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_11);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_10);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_10);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay); 

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_09);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_09);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_08);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_08);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_07);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_07);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);    

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_06);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_06);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay); 

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_05);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_05);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);


    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_04);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_04);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);  

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_03);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_03);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_02);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_02);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_05.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_15.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_02.SetPixelColor(i,rgbColorToUse_mid_01);
        strip_14.SetPixelColor(i,rgbColorToUse_mid_01);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
  
    delay(stepDelay);

    for(int i=0; i < pixelCount; i++) 
    {
        strip_04.SetPixelColor(i,rgbColorToUse_start);
        strip_05.SetPixelColor(i,rgbColorToUse_start);
        strip_15.SetPixelColor(i,rgbColorToUse_start);
        strip_02.SetPixelColor(i,rgbColorToUse_start);
        strip_14.SetPixelColor(i,rgbColorToUse_start);
    }
  
    strip_04.Show();
    strip_05.Show(); 
    strip_15.Show();
    strip_02.Show();
    strip_14.Show();
   
    delay(loopDelay);
  
    server.handleClient();
  }
  Serial.println("Mode_019 Cancelled");
}

/*
 * End Display modes
 */


/*
 * Utilities for display
 */

void ClearAll()
{
  for(int i=0; i < pixelCount; i++) 
  {
      strip_04.SetPixelColor(i,rgbBlack);
      strip_05.SetPixelColor(i,rgbBlack);
      strip_15.SetPixelColor(i,rgbBlack);
      strip_02.SetPixelColor(i,rgbBlack);
      strip_14.SetPixelColor(i,rgbBlack);
  }

  strip_04.Show();
  strip_05.Show(); 
  strip_15.Show();
  strip_02.Show();
  strip_14.Show();
}


//Get the colour values out of the HTML request and adjust for Gamma 
//Not working properly
RgbColor CorrectedColour(String colourParamter, int brightnessModifier)
{
  
//  Serial.println("In CorrectedColour");
//  Serial.print("--");
//  Serial.print(colourParamter);
//  Serial.println("--");
  
  char charColourParamter[colourParamter.length()+1];
  colourParamter.toCharArray(charColourParamter, colourParamter.length()+1);

//  Serial.println("Got char");
//  Serial.print("--");
//  Serial.print(charColourParamter);
//  Serial.println("--");
  
  String rgbStr = server.arg(charColourParamter);

//  Serial.println("brightness");
//  Serial.print("--");
//  Serial.print(brightnessModifier);
//  Serial.println("--");
//
//  Serial.println("Got RGB");
//  Serial.print("--");
//  Serial.print(rgbStr);
//  Serial.println("--");

  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 

//  Serial.println("Red");
//  Serial.print("--");
//  Serial.print(rgb[0]);
//  Serial.println("--");
//
//  Serial.println("Green");
//  Serial.print("--");
//  Serial.print(rgb[1]);
//  Serial.println("--");
//
//  Serial.println("Blue");
//  Serial.print("--");
//  Serial.print(rgb[2]);
//  Serial.println("--");

  int red = round((((rgb[0]*100)/100)*brightnessModifier)/100);
  int green = round((((rgb[1]*100)/100)*brightnessModifier)/100);
  int blue = round((((rgb[2]*100)/100)*brightnessModifier)/100);

//  Serial.println("Red+Brightness");
//  Serial.print("--");
//  Serial.print(red);
//  Serial.println("--");
//
//  Serial.println("Green+Brightness");
//  Serial.print("--");
//  Serial.print(green);
//  Serial.println("--");
//
//  Serial.println("Blue+Brightness");
//  Serial.print("--");
//  Serial.print(blue);
//  Serial.println("--");

//  int redGamma = gammaCorrect[red];
//  int greenGamma = gammaCorrect[green];
//  int blueGamma = gammaCorrect[blue];
//  
//  Serial.println("Red+Brightness+Gamma");
//  Serial.print("--");
//  Serial.print(redGamma);
//  Serial.println("--");
//
//  Serial.println("Green+Brightness+Gamma");
//  Serial.print("--");
//  Serial.print(greenGamma);
//  Serial.println("--");
//
//  Serial.println("Blue+Brightness+Gamma");
//  Serial.print("--");
//  Serial.print(blueGamma);
//  Serial.println("--");
  
  RgbColor rgbColorToUse = RgbColor(red,green,blue); 
  
  return rgbColorToUse;
}

//Randomly change colour by a fixed point around a fixed colour
//Should i adjust for boundry conditions?
RgbColor TwinkleColour(int rgb[], int variation, int brightnessModifier)
{
  int redTwinkle = (rgb[0]+random(variation)-random(variation));
  int greenTwinkle = (rgb[1]+random(variation)-random(variation));
  int blueTwinkle = (rgb[2]+random(variation)-random(variation));
  int red = round((((redTwinkle*100)/100)*brightnessModifier)/100);
  int green = round((((greenTwinkle*100)/100)*brightnessModifier)/100);
  int blue = round((((blueTwinkle*100)/100)*brightnessModifier)/100);

  RgbColor rgbColorToUse = RgbColor(red,green,blue); 
  return rgbColorToUse;
} 

//Needed when we chanage a colour over time
//The RGBColor object is not useful for this 
//easier to do the colour adjustments at the rgb level
void GetColourAsRGBArray(String colourParamter, int *rgb)
{
  char charColourParamter[colourParamter.length()+1];
  colourParamter.toCharArray(charColourParamter, colourParamter.length()+1); 
  String rgbStr = server.arg(charColourParamter);
  rgbStr.replace("%23","#"); //%23 = # in URI                       
  getRGB(rgbStr,rgb); 
} 

int convertToInt(char upper,char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal >64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal >64 ? lVal - 55 : lVal - 48;
  return uVal + lVal;
}

//convert colour from hex to 3 ints
void getRGB(String hexRGB, int *rgb) {
  hexRGB.toUpperCase();
  char c[7]; 
  hexRGB.toCharArray(c,8);
  rgb[0] = convertToInt(c[1],c[2]); //red
  rgb[1] = convertToInt(c[3],c[4]); //green
  rgb[2] = convertToInt(c[5],c[6]); //blue  
}

//Deals with the transition from 0 to maximum pixelCount
//Clockwise
int ClockMinusOne(int value) 
{
  if(value==0)
  {
    return pixelCount-1;
  }
  return value-1;
}

//Deals with the transition from maximum pixelCount to 0
//Clockwise
int ClockPlusOne(int value) 
{
  if(value==pixelCount-1)
  {
    return 0;
  }
  return value+1;
}

//return LED 180 degrees from input 
//NOTE Currently used in an anticlockwise loop generated from clockwise counter
//So it may be a bit specific (that's the -1)
int LED180(int ledNumber) 
{
  int returnLedNumber;
  returnLedNumber= pixelCount-1-ledNumber;
  return returnLedNumber;
}

//Return LED number corrected for position of actual in LED in physical Vibing 
//The LED at the top of the ring is actually LED 1 (the ring starts at LED 0) so we need to offset
//An input of 0 or of 12 returns LED1 which is at the 12 o'clock position
//Which makes inputs of 1 to 11 output as regular clock positions
//This makes designing modes easier
int LEDOffSet(int ledPositionNumber) 
{
  int returnLedNumber;
  returnLedNumber=((ledPositionNumber+1)%12);
  return returnLedNumber;
}




//Same as above but the LED returned is the mirror of the method above
//Mirrored around a vertical line
int LEDOffSetMirrorVertical(int ledPositionNumber) 
{
  int returnLedNumber;
  
  switch (ledPositionNumber) 
  {
      case 0:
      returnLedNumber = LEDOffSet(0);
      break;
      case 1:
      returnLedNumber = LEDOffSet(11);
      break;
     case 2:
      returnLedNumber = LEDOffSet(10);
      break;
    case 3:
      returnLedNumber = LEDOffSet(9);
      break;
    case 4:
      returnLedNumber = LEDOffSet(8);
      break;
    case 5:
      returnLedNumber = LEDOffSet(7);
      break;
    case 6:
      returnLedNumber = LEDOffSet(6);        
      break;
  }
 
  return returnLedNumber;
}



/*
 * End Utilities for display
 */

  

