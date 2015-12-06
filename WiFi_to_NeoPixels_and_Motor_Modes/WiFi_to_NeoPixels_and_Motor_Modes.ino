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
 
int ledPin_12 = 12;  // Vibrating Motor connected to digital pin 12
int ledPin_13 = 13;  // Vibrating Motor connected to digital pin 13     

//***************Set Outputs************************//


NeoPixelBus strip_04 = NeoPixelBus(pixelCount, 04);  //GPIO 4 Note 4 and 5 have their markings swapped on the vertical break out board
NeoPixelBus strip_05 = NeoPixelBus(pixelCount, 05); 
NeoPixelBus strip_15 = NeoPixelBus(pixelCount, 15); 
NeoPixelBus strip_02 = NeoPixelBus(pixelCount, 02);
NeoPixelBus strip_14 = NeoPixelBus(pixelCount, 14);

//const char* ssid     = "Hippy-Laptop";
//const char* password = "12345678";

//const char* ssid     = "Hippy-Apple";
//const char* password = "1wabcdiaatcws13";

const char* ssid     = "Orange-21DF";
const char* password = "429E5C7F";

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
  delay(500);
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
        case 1:
          Set_Mode_01();
          break;
        case 2:
          Set_Mode_02();
          break;
        case 3:
          Set_Mode_03();
          break;
        case 4:
          Set_Mode_04();
          break;
        case 5:
          Set_Mode_05();
          break;
        case 6:
          Set_Mode_06();
          break;
        case 7:
          Set_Mode_07();
          break;
        case 8:
          Set_Mode_08();
          break;
        case 9:
          Set_Mode_09();
          break;
        case 10:
          Set_Mode_10();
          break;
        case 11:
          Set_Mode_11();
          break;
    }
  }
}


/*
 * WiFi
 */
void handle_command() {
  Serial.println("handle_command: ");
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
   
  RgbColor rgbColorToUse = RgbColor(red, green, blue); 

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

//All LEDs set to the same colour
//Single shot mode with no animation
void Set_Mode_01() {

  Serial.println("In Mode_01");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb);
    
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]); 
 
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
void Set_Mode_02() 
{
  Serial.println("In Mode_02");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData) //set when data has been received.  Interupts the display mode.
   {
    Serial.println("In Mode_02_Loop");
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
      Serial.println("Mode_02 Cancelled");
}


//Rotating Single LED - Anti-Clockwise
void Set_Mode_03() 
{
  Serial.println("In Mode_03");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_03_Loop");
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
      Serial.println("Mode_03 Cancelled");
}

//Rotating Single LED Turned Off - Clockwise
void Set_Mode_04() 
{
  Serial.println("In Mode_04");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_04_Loop");
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
      Serial.println("Mode_04 Cancelled");
}


//Rotating Single LED Turned Off - Anti-Clockwise
void Set_Mode_05() 
{
  Serial.println("In Mode_05");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_05_Loop");
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
      Serial.println("Mode_05 Cancelled");
}

//2 Leds moving in anti and clockwise directiions 
void Set_Mode_06() 
{
  Serial.println("In Mode_06");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_06_Loop");
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

              Serial.print("Mode_06 Set");
              Serial.println(i180);

        if(i180==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);

          Serial.print("Mode_06 Clear");
          Serial.println(0);
        }
        else
        {
          strip_04.SetPixelColor(i180+1,rgbBlack);
          strip_05.SetPixelColor(i180+1,rgbBlack);
          strip_15.SetPixelColor(i180+1,rgbBlack);
          strip_02.SetPixelColor(i180+1,rgbBlack);
          strip_14.SetPixelColor(i180+1,rgbBlack);

          Serial.print("Mode_06 Clear");
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
      Serial.println("Mode_06 Cancelled");
}


//Clockwise with trails of 4 LEds
void Set_Mode_07() 
{
  Serial.println("In Mode_07");

  String rgbStr_01 = server.arg("color01");  //get value from color element
  rgbStr_01.replace("%23","#"); //%23 = # in URI
  int rgb_01[3];                           
  getRGB(rgbStr_01,rgb_01); 
  RgbColor rgbColorToUse_01 = RgbColor(rgb_01[0], rgb_01[1], rgb_01[2]);

  String rgbStr_02 = server.arg("color02");  //get value from color element
  rgbStr_02.replace("%23","#"); //%23 = # in URI
  int rgb_02[3];                           
  getRGB(rgbStr_02,rgb_02); 
  RgbColor rgbColorToUse_02 = RgbColor(rgb_02[0], rgb_02[1], rgb_02[2]);

  String rgbStr_03 = server.arg("color03");  //get value from color element
  rgbStr_03.replace("%23","#"); //%23 = # in URI
  int rgb_03[3];                           
  getRGB(rgbStr_03,rgb_03); 
  RgbColor rgbColorToUse_03 = RgbColor(rgb_03[0], rgb_03[1], rgb_03[2]);

  String rgbStr_04 = server.arg("color04");  //get value from color element
  rgbStr_04.replace("%23","#"); //%23 = # in URI
  int rgb_04[3];                           
  getRGB(rgbStr_04,rgb_04); 
  RgbColor rgbColorToUse_04 = RgbColor(rgb_04[0], rgb_04[1], rgb_04[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_07_Loop");
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

        Serial.print("Mode_07 Set-");
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

          Serial.print("Mode_07 Clear-");
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
      Serial.println("Mode_07 Cancelled");
}


//Both Motors On
void Set_Mode_08() 
{
  Serial.println("In Mode_08");

  digitalWrite(ledPin_12, HIGH);

  digitalWrite(ledPin_13, HIGH);
}

//Both Motors Off
void Set_Mode_09() 
{
  Serial.println("In Mode_09");

  digitalWrite(ledPin_12, LOW);

  digitalWrite(ledPin_13, LOW);
}

//Pulsed motor
void Set_Mode_10() 
{
  Serial.println("In Mode_10");

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_10_Loop");
   
    digitalWrite(ledPin_12, HIGH);
    digitalWrite(ledPin_13, HIGH);
    delay(loopDelay);

    digitalWrite(ledPin_12, LOW);
    digitalWrite(ledPin_13, LOW);
    delay(loopDelay);
        
    server.handleClient();
  }
      Serial.println("Mode_02 Cancelled");
}


//2 Leds moving in anti and clockwise directiions with pulsed motors
void Set_Mode_11() 
{
  Serial.println("In Mode_11");

  String rgbStr = server.arg("color");  //get value from color element
  rgbStr.replace("%23","#"); //%23 = # in URI
  int rgb[3];                           
  getRGB(rgbStr,rgb); 
  RgbColor rgbColorToUse = RgbColor(rgb[0], rgb[1], rgb[2]);

  int loopDelay = server.arg("loopdelay").toInt();  //get value from loopdelay element

   while(!getNewData)
   {
    Serial.println("In Mode_11_Loop");
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

              Serial.print("Mode_11 Set");
              Serial.println(i180);

        if(i180==pixelCount-1)
        {
          strip_04.SetPixelColor(0,rgbBlack);
          strip_05.SetPixelColor(0,rgbBlack);
          strip_15.SetPixelColor(0,rgbBlack);
          strip_02.SetPixelColor(0,rgbBlack);
          strip_14.SetPixelColor(0,rgbBlack);

          Serial.print("Mode_11 Clear");
          Serial.println(0);
        }
        else
        {
          strip_04.SetPixelColor(i180+1,rgbBlack);
          strip_05.SetPixelColor(i180+1,rgbBlack);
          strip_15.SetPixelColor(i180+1,rgbBlack);
          strip_02.SetPixelColor(i180+1,rgbBlack);
          strip_14.SetPixelColor(i180+1,rgbBlack);

          Serial.print("Mode_11 Clear");
          Serial.println(i180+1);
        }
        
        
          strip_04.Show();
          strip_05.Show(); 
          strip_15.Show();
          strip_02.Show();
          strip_14.Show();

          Set_Mode_08(); //motors on
          delay(loopDelay/2);
          Set_Mode_09(); //motors off
          delay(loopDelay/2);
        
          server.handleClient();
    }
  }
      Serial.println("Mode_11 Cancelled");
}

/*
 * End Display modes
 */


/*
 * Utilities for display
 */


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

  

