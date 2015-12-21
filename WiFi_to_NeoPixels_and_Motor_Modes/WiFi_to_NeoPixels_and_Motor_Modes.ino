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

//***************Set WiFi connecion************************//
//const char* ssid     = "Hippy-Laptop";
//const char* password = "12345678";

//const char* ssid     = "Hippy-Apple";
//const char* password = "1wabcdiaatcws13";

const char* ssid     = "Hippy-Vibing";
const char* password = "12345678";

//Returns a form with a colour picker.  Not is use but handy for trying out colours
const char* html = "<html><head><style></style></head>"
                   "<body><form action='/' method='GET'><input type='color' name='color' value='#000000'/>"
                   "<input type='submit' name='submit' value='Update RGB Strip'/></form></body></html>";

//Returns a conformation message
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
  Serial.println("Gamma Live Demo 3.001");
  Serial.println();

  Serial.print("LED Count : ");
  Serial.println(pixelCount);
    
 
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

  server.on("/", handle_command);             //root page
  server.begin();
  Serial.println("HTTP server started");

pinMode(ledPin_12, OUTPUT);
pinMode(ledPin_13, OUTPUT);

  strip_04.Begin();
  strip_05.Begin();
  strip_15.Begin();
  strip_02.Begin();
  strip_14.Begin(); 

  //Flash LEDs to show connection made
  DebugLED(254,0,0);
  delay(500);
  DebugLED(0,254,0);
  delay(500);
  DebugLED(0,0,254);
  delay(500);
  DebugLED(100,100,100);
}

void loop() {
  server.handleClient(); //if no mode is running, this is where we receive data

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
    }
  }
}


/*
 * WiFi
 */
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

//All LEDs set to the same colour
//Used to indicate application progress while debugging
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
//Used to help define new modes although can be used as normal mode
//LEDs are just ON.  No loops
//There are 60 LEDS
void Set_Mode_000() 
{
  Serial.println("In Mode_000");

  int loopDelay = server.arg("loopdelay").toInt();
  int ringID = server.arg("ring").toInt();
  Serial.print("RingID ");
  Serial.println(ringID);
  
  RgbColor rgbColorToUse_01 = CorrectedColour("colour_01");
  RgbColor rgbColorToUse_02 = CorrectedColour("colour_02");
  RgbColor rgbColorToUse_03 = CorrectedColour("colour_03");
  RgbColor rgbColorToUse_04 = CorrectedColour("colour_04");
  RgbColor rgbColorToUse_05 = CorrectedColour("colour_05");
  RgbColor rgbColorToUse_06 = CorrectedColour("colour_06");
  RgbColor rgbColorToUse_07 = CorrectedColour("colour_07");
  RgbColor rgbColorToUse_08 = CorrectedColour("colour_08");
  RgbColor rgbColorToUse_09 = CorrectedColour("colour_09");
  RgbColor rgbColorToUse_10 = CorrectedColour("colour_10");
  RgbColor rgbColorToUse_11 = CorrectedColour("colour_11");
  RgbColor rgbColorToUse_12 = CorrectedColour("colour_12");


  //Switch mode
     switch (ringID) 
     {
         case 1:
                    Serial.println("In Case 1 ");
            strip_04.SetPixelColor(0,rgbColorToUse_01);
            strip_04.SetPixelColor(1,rgbColorToUse_02);
            strip_04.SetPixelColor(2,rgbColorToUse_03);
            strip_04.SetPixelColor(3,rgbColorToUse_04);
            strip_04.SetPixelColor(4,rgbColorToUse_05);
            strip_04.SetPixelColor(5,rgbColorToUse_06);
            strip_04.SetPixelColor(6,rgbColorToUse_07);
            strip_04.SetPixelColor(7,rgbColorToUse_08);
            strip_04.SetPixelColor(8,rgbColorToUse_09);
            strip_04.SetPixelColor(9,rgbColorToUse_10);
            strip_04.SetPixelColor(10,rgbColorToUse_11);
            strip_04.SetPixelColor(11,rgbColorToUse_12);
            strip_04.Show();
          break;
        case 2:
                    Serial.println("In Case 1 ");
            strip_05.SetPixelColor(0,rgbColorToUse_01);
            strip_05.SetPixelColor(1,rgbColorToUse_02);
            strip_05.SetPixelColor(2,rgbColorToUse_03);
            strip_05.SetPixelColor(3,rgbColorToUse_04);
            strip_05.SetPixelColor(4,rgbColorToUse_05);
            strip_05.SetPixelColor(5,rgbColorToUse_06);
            strip_05.SetPixelColor(6,rgbColorToUse_07);
            strip_05.SetPixelColor(7,rgbColorToUse_08);
            strip_05.SetPixelColor(8,rgbColorToUse_09);
            strip_05.SetPixelColor(9,rgbColorToUse_10);
            strip_05.SetPixelColor(10,rgbColorToUse_11);
            strip_05.SetPixelColor(11,rgbColorToUse_12);
            strip_05.Show();
          break;
        case 3:
                    Serial.println("In Case 1 ");
            strip_02.SetPixelColor(0,rgbColorToUse_01);
            strip_02.SetPixelColor(1,rgbColorToUse_02);
            strip_02.SetPixelColor(2,rgbColorToUse_03);
            strip_02.SetPixelColor(3,rgbColorToUse_04);
            strip_02.SetPixelColor(4,rgbColorToUse_05);
            strip_02.SetPixelColor(5,rgbColorToUse_06);
            strip_02.SetPixelColor(6,rgbColorToUse_07);
            strip_02.SetPixelColor(7,rgbColorToUse_08);
            strip_02.SetPixelColor(8,rgbColorToUse_09);
            strip_02.SetPixelColor(9,rgbColorToUse_10);
            strip_02.SetPixelColor(10,rgbColorToUse_11);
            strip_02.SetPixelColor(11,rgbColorToUse_12);
            strip_02.Show();
          break;
        case 4:
                    Serial.println("In Case 1 ");
            strip_15.SetPixelColor(0,rgbColorToUse_01);
            strip_15.SetPixelColor(1,rgbColorToUse_02);
            strip_15.SetPixelColor(2,rgbColorToUse_03);
            strip_15.SetPixelColor(3,rgbColorToUse_04);
            strip_15.SetPixelColor(4,rgbColorToUse_05);
            strip_15.SetPixelColor(5,rgbColorToUse_06);
            strip_15.SetPixelColor(6,rgbColorToUse_07);
            strip_15.SetPixelColor(7,rgbColorToUse_08);
            strip_15.SetPixelColor(8,rgbColorToUse_09);
            strip_15.SetPixelColor(9,rgbColorToUse_10);
            strip_15.SetPixelColor(10,rgbColorToUse_11);
            strip_15.SetPixelColor(11,rgbColorToUse_12);
            strip_15.Show();
          break;
        case 5:
                    Serial.println("In Case 1 ");
            strip_14.SetPixelColor(0,rgbColorToUse_01);
            strip_14.SetPixelColor(1,rgbColorToUse_02);
            strip_14.SetPixelColor(2,rgbColorToUse_03);
            strip_14.SetPixelColor(3,rgbColorToUse_04);
            strip_14.SetPixelColor(4,rgbColorToUse_05);
            strip_14.SetPixelColor(5,rgbColorToUse_06);
            strip_14.SetPixelColor(6,rgbColorToUse_07);
            strip_14.SetPixelColor(7,rgbColorToUse_08);
            strip_14.SetPixelColor(8,rgbColorToUse_09);
            strip_14.SetPixelColor(9,rgbColorToUse_10);
            strip_14.SetPixelColor(10,rgbColorToUse_11);
            strip_14.SetPixelColor(11,rgbColorToUse_12);
            strip_14.Show();           
          break;
    }


}

//All LEDs in a ring are set to the same colour
//Single shot mode with no animation
//does not clear any LEDS so can be called more than once to 
//set different rings without affecting any other ring
void Set_Mode_001() {

  Serial.println("In Mode_001");

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

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

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData) //set when data has been received.  Interupts the display mode.
   {
    Serial.println("In Mode_002_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
      strip_04.SetPixelColor(i,rgbColorToUse);
      strip_05.SetPixelColor(i,rgbColorToUse);
      strip_15.SetPixelColor(i,rgbColorToUse);
      strip_02.SetPixelColor(i,rgbColorToUse);
      strip_14.SetPixelColor(i,rgbColorToUse);

        if(i==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbBlack);
          strip_05.SetPixelColor(pixelCount-1,rgbBlack);
          strip_15.SetPixelColor(pixelCount-1,rgbBlack);
          strip_02.SetPixelColor(pixelCount-1,rgbBlack);
          strip_14.SetPixelColor(pixelCount-1,rgbBlack);
        }
        else
        {
          strip_04.SetPixelColor(i-1,rgbBlack);
          strip_05.SetPixelColor(i-1,rgbBlack);
          strip_15.SetPixelColor(i-1,rgbBlack);
          strip_02.SetPixelColor(i-1,rgbBlack);
          strip_14.SetPixelColor(i-1,rgbBlack);
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

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_003_Loop");
    for(int i=pixelCount-1; i>=0; i--) 
    {     
      strip_04.SetPixelColor(i,rgbColorToUse);
      strip_05.SetPixelColor(i,rgbColorToUse);
      strip_15.SetPixelColor(i,rgbColorToUse);
      strip_02.SetPixelColor(i,rgbColorToUse);
      strip_14.SetPixelColor(i,rgbColorToUse);

        if(i==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);
        }
        else
        {
          strip_04.SetPixelColor(i+1,rgbBlack);
          strip_05.SetPixelColor(i+1,rgbBlack);
          strip_15.SetPixelColor(i+1,rgbBlack);
          strip_02.SetPixelColor(i+1,rgbBlack);
          strip_14.SetPixelColor(i+1,rgbBlack);
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

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_004_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
      strip_04.SetPixelColor(i,rgbBlack);
      strip_05.SetPixelColor(i,rgbBlack);
      strip_15.SetPixelColor(i,rgbBlack);
      strip_02.SetPixelColor(i,rgbBlack);
      strip_14.SetPixelColor(i,rgbBlack);
      
        if(i==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_05.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_15.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_02.SetPixelColor(pixelCount-1,rgbColorToUse);
          strip_14.SetPixelColor(pixelCount-1,rgbColorToUse);
        }
        else
        {
          strip_04.SetPixelColor(i-1,rgbColorToUse);
          strip_05.SetPixelColor(i-1,rgbColorToUse);
          strip_15.SetPixelColor(i-1,rgbColorToUse);
          strip_02.SetPixelColor(i-1,rgbColorToUse);
          strip_14.SetPixelColor(i-1,rgbColorToUse);
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

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_005_Loop");
    for(int i=pixelCount-1; i>=0; i--) 
    {     
        strip_04.SetPixelColor(i,rgbBlack);
        strip_05.SetPixelColor(i,rgbBlack);
        strip_15.SetPixelColor(i,rgbBlack);
        strip_02.SetPixelColor(i,rgbBlack);
        strip_14.SetPixelColor(i,rgbBlack);

        if(i==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbColorToUse);
          strip_05.SetPixelColor(0,rgbColorToUse);
          strip_15.SetPixelColor(0,rgbColorToUse);
          strip_02.SetPixelColor(0,rgbColorToUse);
          strip_14.SetPixelColor(0,rgbColorToUse);
        }
        else
        {
          strip_04.SetPixelColor(i+1,rgbColorToUse);
          strip_05.SetPixelColor(i+1,rgbColorToUse);
          strip_15.SetPixelColor(i+1,rgbColorToUse);
          strip_02.SetPixelColor(i+1,rgbColorToUse);
          strip_14.SetPixelColor(i+1,rgbColorToUse);
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

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_006_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
        //Clockwise
        strip_04.SetPixelColor(i,rgbColorToUse);
        strip_05.SetPixelColor(i,rgbColorToUse);
        strip_15.SetPixelColor(i,rgbColorToUse);
        strip_02.SetPixelColor(i,rgbColorToUse);
        strip_14.SetPixelColor(i,rgbColorToUse);

        if(i==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbBlack);
          strip_05.SetPixelColor(pixelCount-1,rgbBlack);
          strip_15.SetPixelColor(pixelCount-1,rgbBlack);
          strip_02.SetPixelColor(pixelCount-1,rgbBlack);
          strip_14.SetPixelColor(pixelCount-1,rgbBlack);
        }
        else
        {
          strip_05.SetPixelColor(i-1,rgbBlack);

          strip_04.SetPixelColor(i-1,rgbBlack);
          strip_05.SetPixelColor(i-1,rgbBlack);
          strip_15.SetPixelColor(i-1,rgbBlack);
          strip_02.SetPixelColor(i-1,rgbBlack);
          strip_14.SetPixelColor(i-1,rgbBlack);
        }

        //Anticlockwise
        int i180 = LED180(i);

        Serial.print("i = ");
        Serial.println(i);

        strip_04.SetPixelColor(i180,rgbColorToUse);
        strip_05.SetPixelColor(i180,rgbColorToUse);
        strip_15.SetPixelColor(i180,rgbColorToUse);
        strip_02.SetPixelColor(i180,rgbColorToUse);
        strip_14.SetPixelColor(i180,rgbColorToUse);

              Serial.print("Mode_006 Set");
              Serial.println(i180);

        if(i180==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);

          Serial.print("Mode_006 Clear");
          Serial.println(0);
        }
        else
        {
          strip_04.SetPixelColor(i180+1,rgbBlack);
          strip_05.SetPixelColor(i180+1,rgbBlack);
          strip_15.SetPixelColor(i180+1,rgbBlack);
          strip_02.SetPixelColor(i180+1,rgbBlack);
          strip_14.SetPixelColor(i180+1,rgbBlack);

          Serial.print("Mode_006 Clear");
          Serial.println(i180+1);
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

  RgbColor rgbColorToUse_01 = CorrectedColour("colour_001");
  RgbColor rgbColorToUse_02 = CorrectedColour("colour_002");
  RgbColor rgbColorToUse_03 = CorrectedColour("colour_003");
  RgbColor rgbColorToUse_04 = CorrectedColour("colour_004");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_007_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
        //Clockwise
        strip_04.SetPixelColor(i,rgbColorToUse_01);
        strip_04.SetPixelColor(ClockMinusOne(i),rgbColorToUse_02);
        strip_04.SetPixelColor(ClockMinusOne(ClockMinusOne(i)),rgbColorToUse_03);
        strip_04.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(i))),rgbColorToUse_04); 

        strip_05.SetPixelColor(i,rgbColorToUse_01);
        strip_05.SetPixelColor(ClockMinusOne(i),rgbColorToUse_02);
        strip_05.SetPixelColor(ClockMinusOne(ClockMinusOne(i)),rgbColorToUse_03);
        strip_05.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(i))),rgbColorToUse_04);

        strip_15.SetPixelColor(i,rgbColorToUse_01);
        strip_15.SetPixelColor(ClockMinusOne(i),rgbColorToUse_02);
        strip_15.SetPixelColor(ClockMinusOne(ClockMinusOne(i)),rgbColorToUse_03);
        strip_15.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(i))),rgbColorToUse_04); 

        strip_02.SetPixelColor(i,rgbColorToUse_01);
        strip_02.SetPixelColor(ClockMinusOne(i),rgbColorToUse_02);
        strip_02.SetPixelColor(ClockMinusOne(ClockMinusOne(i)),rgbColorToUse_03);
        strip_02.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(i))),rgbColorToUse_04); 

        strip_14.SetPixelColor(i,rgbColorToUse_01);
        strip_14.SetPixelColor(ClockMinusOne(i),rgbColorToUse_02);
        strip_14.SetPixelColor(ClockMinusOne(ClockMinusOne(i)),rgbColorToUse_03);
        strip_14.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(i))),rgbColorToUse_04);

        Serial.print("Mode_007 Set-");
        Serial.print(i);
        Serial.print(" - ");
        Serial.println(ClockMinusOne(i));
//        Serial.println(i-2);
//        Serial.println(i-3);

          strip_04.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(i)))),rgbBlack);
          strip_05.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(i)))),rgbBlack);
          strip_15.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(i)))),rgbBlack);
          strip_02.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(i)))),rgbBlack);
          strip_14.SetPixelColor(ClockMinusOne(ClockMinusOne(ClockMinusOne(ClockMinusOne(i)))),rgbBlack);

          Serial.print("Mode_007 Clear-");
          Serial.println(ClockMinusOne(ClockMinusOne(i)));

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

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_011_Loop");
    for(int i=0; i < pixelCount; i++) 
    {     
        //Clockwise
        strip_04.SetPixelColor(i,rgbColorToUse);
        strip_05.SetPixelColor(i,rgbColorToUse);
        strip_15.SetPixelColor(i,rgbColorToUse);
        strip_02.SetPixelColor(i,rgbColorToUse);
        strip_14.SetPixelColor(i,rgbColorToUse);

        if(i==0)
        {
          strip_04.SetPixelColor(pixelCount-1,rgbBlack);
          strip_05.SetPixelColor(pixelCount-1,rgbBlack);
          strip_15.SetPixelColor(pixelCount-1,rgbBlack);
          strip_02.SetPixelColor(pixelCount-1,rgbBlack);
          strip_14.SetPixelColor(pixelCount-1,rgbBlack);
        }
        else
        {
          strip_05.SetPixelColor(i-1,rgbBlack);

          strip_04.SetPixelColor(i-1,rgbBlack);
          strip_05.SetPixelColor(i-1,rgbBlack);
          strip_15.SetPixelColor(i-1,rgbBlack);
          strip_02.SetPixelColor(i-1,rgbBlack);
          strip_14.SetPixelColor(i-1,rgbBlack);
        }

        //Anticlockwise
        int i180 = LED180(i);

        Serial.print("i = ");
        Serial.println(i);

        strip_04.SetPixelColor(i180,rgbColorToUse);
        strip_05.SetPixelColor(i180,rgbColorToUse);
        strip_15.SetPixelColor(i180,rgbColorToUse);
        strip_02.SetPixelColor(i180,rgbColorToUse);
        strip_14.SetPixelColor(i180,rgbColorToUse);

              Serial.print("Mode_011 Set");
              Serial.println(i180);

        if(i180==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);

          Serial.print("Mode_011 Clear");
          Serial.println(0);
        }
        else
        {
          strip_04.SetPixelColor(i180+1,rgbBlack);
          strip_05.SetPixelColor(i180+1,rgbBlack);
          strip_15.SetPixelColor(i180+1,rgbBlack);
          strip_02.SetPixelColor(i180+1,rgbBlack);
          strip_14.SetPixelColor(i180+1,rgbBlack);

          Serial.print("Mode_011 Clear");
          Serial.println(i180+1);
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
//Remember channels 4 and 5 are marked incorrectly on the chip
void Set_Mode_012() 
{
  Serial.println("In Mode_012");

  RgbColor rgbColorToUse = CorrectedColour("colour_001");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element
  
   ClearAll();
   while(!getNewData)
   {
      Serial.println("In Mode_012_Loop");

      strip_05.SetPixelColor(1,rgbColorToUse);
      strip_05.Show();
      delay(loopDelay);
      strip_05.SetPixelColor(1,rgbBlack);
      strip_05.Show();
      
      strip_04.SetPixelColor(1,rgbColorToUse);
      strip_04.Show();
      delay(loopDelay);      
      strip_04.SetPixelColor(1,rgbBlack);
      strip_04.Show();
      
      strip_15.SetPixelColor(1,rgbColorToUse);
      strip_15.Show();
      delay(loopDelay);
      strip_15.SetPixelColor(1,rgbBlack);
      strip_15.Show();
      
      strip_02.SetPixelColor(1,rgbColorToUse);
      strip_02.Show();
      delay(loopDelay);
      strip_02.SetPixelColor(1,rgbBlack);
      strip_02.Show();
      
      strip_14.SetPixelColor(1,rgbColorToUse);
      strip_14.Show();
      delay(loopDelay);
      strip_14.SetPixelColor(1,rgbBlack);
      strip_14.Show();
      
      server.handleClient();
  }
      Serial.println("Mode_012 Cancelled");
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
RgbColor CorrectedColour(String colourParamter)
{
  /*
  Serial.println("In CorrectedColour");
  Serial.print("--");
  Serial.print(colourParamter);
  Serial.println("--");
   */

  char charColourParamter[colourParamter.length()+1];
  colourParamter.toCharArray(charColourParamter, colourParamter.length()+1);

  /*
  Serial.println("Got char");
  Serial.print("--");
  Serial.print(charColourParamter);
  Serial.println("--");;
   */

  String rgbStr = server.arg(charColourParamter);
  int brightnessModifier = server.arg("brightness").toInt();
  Serial.println("brightness");
  Serial.print("--");
  Serial.print(brightnessModifier);
  Serial.println("--");


  Serial.println("Got RGB");
  Serial.print("--");
  Serial.print(rgbStr);
  Serial.println("--");


  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 

  Serial.println("Red");
  Serial.print("--");
  Serial.print(rgb[0]);
  Serial.println("--");

  Serial.println("Red+Gamma");
  Serial.print("--");
  Serial.print(gammaCorrect[rgb[0]]);
  Serial.println("--");

    Serial.println("Red+Gamma+bright");
  Serial.print("--");
  Serial.print(((gammaCorrect[rgb[0]])/100)*brightnessModifier);
  Serial.println("--");
 
  RgbColor rgbColorToUse = RgbColor(((gammaCorrect[rgb[0]])/100)*brightnessModifier, ((gammaCorrect[rgb[1]])/100)*brightnessModifier, ((gammaCorrect[rgb[2]])/100)*brightnessModifier); 
  return rgbColorToUse;
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
//NOTE Current;t used in an anticlockwise loop generated from clockwise counter
//So it may be a bit specific (that's the -1)
int LED180(int ledNumber) 
{
  int returnLedNumber;
  returnLedNumber= pixelCount-1-ledNumber;
  return returnLedNumber;
}

/*
 * End Utilities for display
 */

  

