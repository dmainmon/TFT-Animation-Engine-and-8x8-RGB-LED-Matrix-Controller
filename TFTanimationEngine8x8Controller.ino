/////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                 //
//              TFT Touch Screen Animation Engine and 8x8 RGB LED Controller                       //
//                                                                                                 //
//                                   version 1.00                                                  //
//                                                                                                 //
//                             programmer: Damon Borgnino                                          //
//                                                                                                 //
//                                   February 19, 2016                                             //
//                                                                                                 //
//                        http://www.youtube.com/surfdmountain                                     //
//                                                                                                 //
//                     http://www.instructables.com/member/dmainmon/                               //
//                                                                                                 //
/////////////////////////////////////////////////////////////////////////////////////////////////////

// debug mode will make playback much slower but give you valuable raw coordinates to set calibration
//#define DEBUG
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <SD.h>


/* default adafruit settings
  #define LCD_CS A3 // Chip Select goes to Analog 3
  #define LCD_CD A2 // Command/Data goes to Analog 2
  #define LCD_WR A1 // LCD Write goes to Analog 1
  #define LCD_RD A0 // LCD Read goes to Analog 0

  #define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
*/

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0

#define LCD_RESET A4

/* default adafruit settings
  #define YP A1  // must be an analog pin, use "An" notation!
  #define XM A2  // must be an analog pin, use "An" notation!
  #define YM 7   // can be a digital pin
  #define XP 6   // can be a digital pin
*/

// use this on the UNO R3 TFT
#define YP A3
#define XM A2
#define YM 9
#define XP 8

// get these numbers in DEBUB mode
// the serial data will show the raw x and y values
// press/swipe each edge of the screen and find min max values
#define TS_MINX 206
#define TS_MAXX 920
#define TS_MINY 188
#define TS_MAXY 932

#define MINPRESSURE 15
#define MAXPRESSURE 1000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Assign names to some common 16-bit color values:
#define BLACK   0x0000 // 1
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define ROZ     0xFBE0
#define GRI     0xBDF7

/////////////////////////////////////////////////////////////////
// LED 8x8 Setup                                           /////
/////////////////////////////////////////////////////////////////

// setup your digital pins
int DATA = 22;
int DATA_OUTPUT_ENABLE = 24;
int LATCH = 26;
int CLOCK = 28;
int RESET = 30;


// must have this Screen array, it's the default to show each time on the 8x8 matrix
byte Screen[9][8] = {{0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0} // this is actually an extra "dummy line" that is always off, needed to prevent last row from glowing
};

// menu buttons - defining the buttons globally makes it easier to handle touch events and drawing
// you can change the coordinates (position) of a button here without losing integrity of touch and draw events
const int msgArea[] = {42, 195, 45, 125};

const int saveBtn[] = {94, 290, 25, 25};
const int delAllBtn[] = {125, 290, 25, 25};
const int playAllBtn[] = {156, 290, 25, 25};
const int fileMgrBtn[] = {187, 290, 25, 25};

const int speedUp[] =  {165, 220, 26, 26};
const int speedDown[] =  {197, 220, 26, 26};
const int loopUp[] =  {90, 220, 26, 26};
const int loopDown[] =  {122, 220, 26, 26};

const int confirmBtn[] = {58, 290, 20, 20};
const int cancelBtn[] = {58, 230, 20, 20};

const int fileMgrCloseBtn[] = {5, 170 , 25, 25};
const int fileNewBtn[] = {45, 160 , 20, 20};

const int file0Btn[] = {71, 33 , 15, 150};
const int file1Btn[] = {86, 33 , 15, 150};
const int file2Btn[] = {101, 33 , 15, 150};
const int file3Btn[] = {116, 33 , 15, 150};
const int file4Btn[] = {131, 33 , 15, 150};
const int file5Btn[] = {146, 33 , 15, 150};
const int file6Btn[] = {161, 33 , 15, 150};
const int file7Btn[] = {176, 33 , 15, 150};
const int file8Btn[] = {191, 33 , 15, 150};
const int file9Btn[] = {206, 33 , 15, 150};


const int rndBtnRad = 3;  //rounded rectangle corner radius
const int YSTART = 45; // start position of cursor
const int lineSpacing = 10;
const int gridBox = 23;  // size of the 8x8 matrix grid box on tft

// two event modes require confirmation touch
bool deletePop = false;
bool clearScrPop = false;
// file pop up event
bool fileMgrPop = false;

// default variables
///////////////////////////////////////////////////
// 0=Black(off)                                 //
// 1=Green                                      //
// 2=Blue               Color                   //
// 3=Cyan                reference             ///
// 4=Red                                        //
// 5=Yellow                                   ////
// 6=Magenta                                    //
// 7=White                                      //
//////////////////////////////////////////////////
// you can change these to suit your desires
int currentLEDcolor = 4; //same as tft default: red // see color reference
int frameDelay = 300; // miliseconds
int playLoops = 1;

// don't change these
int numFiles = 0;
char *fileName = "/TFT8X8/DATA0.TXT"; // first file
char *dirName = "TFT8X8";



int oldcolor, currentcolor;

////////////////////////////////////////////////////////////////////
// SD Setup
////////////////////////////////////////////////////////////////////

#if defined __AVR_ATmega2560__
#define SD_SCK 13
#define SD_MISO 12
#define SD_MOSI 11
#endif
#define SD_CS 10     // Set the chip select line to whatever you use (10 doesnt conflict with the library)


Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);;

#define BOXSIZE 40

//////////////////////////////////////////////////////////////////////////
//            setup routine                                            //
/////////////////////////////////////////////////////////////////////////

void setup(void) {


#ifdef DEBUG
  Serial.begin(9600);
  Serial.println(F("TFT 8x8 Controller!"));
#endif // DEBUG


  tft.reset();

  uint16_t identifier = tft.readID();


  if (identifier == 0x9325) {
#ifdef DEBUG
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if (identifier == 0x9328) {

    Serial.println(F("Found ILI9328 LCD driver"));
  } else if (identifier == 0x7575) {

    Serial.println(F("Found HX8347G LCD driver"));
  } else if (identifier == 0x9341) {

    Serial.println(F("Found ILI9341 LCD driver"));
  } else if (identifier == 0x8357) {

    Serial.println(F("Found HX8357D LCD driver"));
#endif // DEBUG
  } else {
#ifdef DEBUG
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.print(F("Trying ILI9341 LCD driver "));
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
#endif // DEBUG
    identifier = 0x9341;
  }


  tft.begin(identifier);

  tft.fillScreen(BLACK);
  tft.setRotation(3);
  tft.setCursor(30, 100);
  tft.setTextColor(RED);  tft.setTextSize(3);
  tft.println("LCD driver chip: ");
  tft.setCursor(100, 150);
  tft.setTextColor(BLUE);
  tft.println(identifier, HEX);

  delay(700);

  tft.fillScreen(BLACK);

  tft.setRotation(0);
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
  tft.fillRect(0, BOXSIZE, BOXSIZE, BOXSIZE, YELLOW);
  tft.fillRect(0, BOXSIZE * 2, BOXSIZE, BOXSIZE, GREEN);
  tft.fillRect(0, BOXSIZE * 3, BOXSIZE, BOXSIZE, CYAN);
  tft.fillRect(0, BOXSIZE * 4, BOXSIZE, BOXSIZE, BLUE);
  tft.fillRect(0, BOXSIZE * 5, BOXSIZE, BOXSIZE, MAGENTA);
  tft.fillRect(0, BOXSIZE * 6, BOXSIZE, BOXSIZE, GRI);
  tft.fillRect(0, BOXSIZE * 7, BOXSIZE, BOXSIZE,  ROZ);

  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE);
  currentcolor = RED;


  setColorBtnNum (WHITE);

  /////////////////////////////
  ////        8x8 setup      //
  /////////////////////////////

  pinMode(DATA, OUTPUT);
  pinMode(DATA_OUTPUT_ENABLE, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(RESET, OUTPUT);

  digitalWrite(DATA_OUTPUT_ENABLE, LOW);
  digitalWrite(RESET, HIGH);
  digitalWrite(CLOCK, LOW);
  digitalWrite(LATCH, LOW);
  digitalWrite(DATA, LOW);

  draw8x8Grid();
  drawMenuButtons();

  initSD();  //start the SD

}

///////////////////////////////////////////////////////////////////////////////
//                   LOOP Routine                                     ///////
///////////////////////////////////////////////////////////////////////////////


void loop()
{

  TSPoint p = ts.getPoint();


  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);  // default Adafruit setting
  //pinMode(YM, OUTPUT); // default Adafruit setting
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  ////////////////////////////////////////////////////////////////
  //           CHECK FOR TOUCH EVENT                          ////
  //        we have a valid pressure event                   ////
  //          or pressure below min (no press)                ////
  ////////////////////////////////////////////////////////////////

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {

#ifdef DEBUG
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);
#endif // DEBUG


    // scale from 0->1023 to tft.width
    //  p.x = tft.width()-(map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
    //  p.y = tft.height()-(map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
    // remapped for UNO 3 TFT:
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);


    // first check for delete button popUp
    if (deletePop)
    {
      if (p.x >= confirmBtn[0] && p.x <= confirmBtn[0] + confirmBtn[2] && p.y >= confirmBtn[1] && p.y <= confirmBtn[1] + confirmBtn[3])
      {
        deleteAll();
        deletePop = false;
      }
      if (p.x >= cancelBtn[0] && p.x <= cancelBtn[0] + cancelBtn[2] && p.y >= cancelBtn[1] && p.y <= cancelBtn[1] + cancelBtn[3])
      {
        deletePop = false;
        clearMsgArea();
      }

    }

    // check for fileMgr button popUp
    if (fileMgrPop)
    {
      if (p.x >= fileMgrCloseBtn[0] && p.x <= fileMgrCloseBtn[0] + fileMgrCloseBtn[2] && p.y >= fileMgrCloseBtn[1] && p.y <= fileMgrCloseBtn[1] + fileMgrCloseBtn[3])
      {
        fileMgrPop = false;
        tft.fillRoundRect(5, 5 , tft.width() - 10, tft.height() - 130, rndBtnRad, BLACK);

        clear8x8();
        resetScreen();
        draw8x8Grid();
        p.x = 400; // so that the blue color picker doesn't get set when closing the file manager
        p.y = 400;
      }


      if (p.x >= fileNewBtn[0] && p.x <= fileNewBtn[0] + fileNewBtn[2] && p.y >= fileNewBtn[1] && p.y <= fileNewBtn[1] + fileNewBtn[3])
      {
        createNewFile();
      }


      if (numFiles > 0)
        if (p.x >= file0Btn[0] && p.x <= file0Btn[0] + file0Btn[2] && p.y >= file0Btn[1] && p.y <= file0Btn[1] + file0Btn[3])
        {
          fileName = "/TFT8x8/DATA0.TXT";
          showFileManager();
        }

      if (numFiles > 1)
        if (p.x >= file1Btn[0] && p.x <= file1Btn[0] + file1Btn[2] && p.y >= file1Btn[1] && p.y <= file1Btn[1] + file1Btn[3])
        {
          fileName = "/TFT8x8/DATA1.TXT";
          showFileManager();
        }

      if (numFiles > 2)
        if (p.x >= file2Btn[0] && p.x <= file2Btn[0] + file2Btn[2] && p.y >= file2Btn[1] && p.y <= file2Btn[1] + file2Btn[3])
        {
          fileName = "/TFT8x8/DATA2.TXT";
          showFileManager();
        }

      if (numFiles > 3)
        if (p.x >= file3Btn[0] && p.x <= file3Btn[0] + file3Btn[2] && p.y >= file3Btn[1] && p.y <= file3Btn[1] + file3Btn[3])
        {
          fileName = "/TFT8x8/DATA3.TXT";
          showFileManager();
        }

      if (numFiles > 4)
        if (p.x >= file4Btn[0] && p.x <= file4Btn[0] + file4Btn[2] && p.y >= file4Btn[1] && p.y <= file4Btn[1] + file4Btn[3])
        {
          fileName = "/TFT8x8/DATA4.TXT";
          showFileManager();
        }

      if (numFiles > 5)
        if (p.x >= file5Btn[0] && p.x <= file5Btn[0] + file5Btn[2] && p.y >= file5Btn[1] && p.y <= file5Btn[1] + file5Btn[3])
        {
          fileName = "/TFT8x8/DATA5.TXT";
          showFileManager();
        }

      if (numFiles > 6)
        if (p.x >= file6Btn[0] && p.x <= file6Btn[0] + file6Btn[2] && p.y >= file6Btn[1] && p.y <= file6Btn[1] + file6Btn[3])
        {
          fileName = "/TFT8x8/DATA6.TXT";
          showFileManager();
        }

      if (numFiles > 7)
        if (p.x >= file7Btn[0] && p.x <= file7Btn[0] + file7Btn[2] && p.y >= file7Btn[1] && p.y <= file7Btn[1] + file7Btn[3])
        {
          fileName = "/TFT8x8/DATA7.TXT";
          showFileManager();
        }

      if (numFiles > 8)
        if (p.x >= file8Btn[0] && p.x <= file8Btn[0] + file8Btn[2] && p.y >= file8Btn[1] && p.y <= file8Btn[1] + file8Btn[3] )
        {
          fileName = "/TFT8x8/DATA8.TXT";
          showFileManager();
        }

      if (numFiles > 9)
        if (p.x >= file9Btn[0] && p.x <= file9Btn[0] + file9Btn[2] && p.y >= file9Btn[1] && p.y <= file9Btn[1] + file9Btn[3] )
        {
          fileName = "/TFT8x8/DATA9.TXT";
          showFileManager();
        }


    }

    //  check for clearScreen button popUp
    if (clearScrPop)
    {
      if (p.x >= confirmBtn[0] && p.x <= confirmBtn[0] + confirmBtn[2] && p.y >= confirmBtn[1] && p.y <= confirmBtn[1] + confirmBtn[3])
      {
        clear8x8();
        resetScreen();
        draw8x8Grid();
        // currentLEDcolor = 4; // red
        clearScrPop = false;
      }
      if (p.x >= cancelBtn[0] && p.x <= cancelBtn[0] + cancelBtn[2] && p.y >= cancelBtn[1] && p.y <= cancelBtn[1] + cancelBtn[3])
      {
        clearScrPop = false;
        //  currentLEDcolor = 4; // red
        clearMsgArea();
      }
    }

    if (deletePop || clearScrPop || fileMgrPop)  // don't do anything else until confirm/cancel touched
      return;


    //filesMgr button pressed
    if (p.x >= fileMgrBtn[0] && p.x <= fileMgrBtn[0] + fileMgrBtn[2] && p.y >= fileMgrBtn[1] && p.y <= fileMgrBtn[1] + fileMgrBtn[3])
    {
      fileMgrPop = true;
      showFileManager();
    }

    //delete button pressed
    if (p.x >= delAllBtn[0] && p.x <= delAllBtn[0] + delAllBtn[2] && p.y >= delAllBtn[1] && p.y <= delAllBtn[1] + delAllBtn[3])
    {
      deletePop = true;
      showConfirmPop();
    }

    // save button pressed
    if (p.x >= saveBtn[0] && p.x <= saveBtn[0] + saveBtn[2] && p.y >= saveBtn[1] && p.y <= saveBtn[1] + saveBtn[3])
    {
      saveToSD();
    }

    //play button pressed
    if (p.x >= playAllBtn[0] && p.x <= playAllBtn[0] + playAllBtn[2] && p.y >= playAllBtn[1] && p.y <= playAllBtn[1] + playAllBtn[3])
    {
      playAll();
    }

    //speed up button pressed
    if (p.x >= speedUp[0] && p.x <= speedUp[0] + speedUp[2] && p.y >= speedUp[1] && p.y <= speedUp[1] + speedUp[3])
    {
      if (frameDelay >= 100)
        frameDelay = frameDelay + 100;
      else if (frameDelay < 10)
        frameDelay = frameDelay + 1;
      else
        frameDelay = frameDelay + 10;


      clearMsgArea();
      int yStart = YSTART;
      tft.setTextColor(WHITE);
      tft.setCursor(0, yStart);
      tft.setRotation(3);
      tft.setTextSize(2);
      tft.print(F("Delay:"));
      tft.setTextSize(1);
      tft.setCursor(70, 50);
      tft.println("(millis)");/// tft.println(" "); tft.print(" ");
      tft.setTextSize(2);
      tft.setCursor(15, yStart = yStart + 20);
      tft.setTextColor(YELLOW);
      tft.println(frameDelay);
      tft.setTextSize(1);

      tft.setRotation(0);

    }

    //speed down button pressed
    if (p.x >= speedDown[0] && p.x <= speedDown[0] + speedDown[2] && p.y >= speedDown[1] && p.y <= speedDown[1] + speedDown[3])
    {

      if (frameDelay > 0) // at 10 go down in increments of one unti one is reached
      {
        if (frameDelay > 100)
          frameDelay = frameDelay - 100;
        else if (frameDelay <= 10)
          frameDelay = frameDelay - 1;
        else
          frameDelay = frameDelay - 10;
      }

      clearMsgArea();

      tft.setTextColor(WHITE);
      tft.setCursor(0, YSTART);
      tft.setRotation(3);
      tft.setTextSize(2);
      tft.print(F("Delay:"));
      tft.setTextSize(1);
      tft.setCursor(70, 50);
      tft.println("(millis)");/// tft.println(" "); tft.print(" ");
      tft.setTextSize(2);
      tft.setCursor(15, YSTART + 20);
      tft.setTextColor(YELLOW);
      tft.println(frameDelay);
      tft.setTextSize(1);

      tft.setRotation(0);
    }

    //loops up button pressed
    if (p.x >= loopUp[0] && p.x <= loopUp[0] + loopUp[2] && p.y >= loopUp[1] && p.y <= loopUp[1] + loopUp[3])
    {
      playLoops = playLoops + 1;

      clearMsgArea();
      tft.setTextColor(WHITE);
      tft.setCursor(0, YSTART);
      tft.setRotation(3);
      tft.setTextSize(2);
      tft.println(F("Loops: "));
      tft.setCursor(30, YSTART + 20);
      tft.setTextColor(YELLOW);
      tft.println(playLoops);
      tft.setRotation(0);
    }

    //loops down button pressed
    if (p.x >= loopDown[0] && p.x <= loopDown[0] + loopDown[2] && p.y >= loopDown[1] && p.y <= loopDown[1] + loopDown[3])
    {
      if (playLoops > 1) // don't go lower than one
        playLoops = playLoops - 1;

      clearMsgArea();
      tft.setTextColor(WHITE);
      tft.setCursor(0, YSTART);
      tft.setRotation(3);
      tft.setTextSize(2);
      tft.println(F("Loops: "));
      tft.setCursor(30, YSTART + 20);
      tft.setTextColor(YELLOW);
      tft.println(playLoops);
      tft.setRotation(0);
    }


#ifdef DEBUG
    Serial.print("("); Serial.print(p.x);
    Serial.print(", "); Serial.print(p.y);
    Serial.println(")");
#endif // DEBUG


    // reset color picker boxes
    if (p.x < BOXSIZE) {
      oldcolor = currentcolor;

      // 0=Black(off)
      // 1=Green
      // 2=Blue
      // 3=Cyan
      // 4=Red
      // 5=Yellow
      // 6=Magenta
      // 7=White

      if (p.y < BOXSIZE) {
        currentcolor = RED;
        currentLEDcolor = 4;
        tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 2) {
        currentcolor = YELLOW;
        currentLEDcolor = 5;
        tft.drawRect(0, BOXSIZE, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 3) {
        currentcolor = GREEN;
        currentLEDcolor = 1;
        tft.drawRect(0, BOXSIZE * 2, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 4) {
        currentcolor = CYAN;
        currentLEDcolor = 3;
        tft.drawRect(0, BOXSIZE * 3, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 5) {

        currentcolor = BLUE;
        currentLEDcolor = 2;
        tft.drawRect(0, BOXSIZE * 4, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 6) {
        currentcolor = MAGENTA;
        currentLEDcolor = 6;
        tft.drawRect(0, BOXSIZE * 5, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 7) {
        currentcolor = GRI;
        currentLEDcolor = 0;
        tft.drawRect(0, BOXSIZE * 6, BOXSIZE, BOXSIZE, WHITE);

      } else if (p.y < BOXSIZE * 8) {
        // currentcolor = ROZ;
        tft.drawRect(0, BOXSIZE * 7, BOXSIZE, BOXSIZE, WHITE);

        clearScrPop = true;
        showConfirmPop();
      }


      if (oldcolor != currentcolor) {
        if (oldcolor == RED) tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
        if (oldcolor == YELLOW) tft.fillRect(0, BOXSIZE, BOXSIZE, BOXSIZE, YELLOW);
        if (oldcolor == GREEN) tft.fillRect(0, BOXSIZE * 2, BOXSIZE, BOXSIZE, GREEN);
        if (oldcolor == CYAN) tft.fillRect(0, BOXSIZE * 3, BOXSIZE, BOXSIZE, CYAN);
        if (oldcolor == BLUE) tft.fillRect(0, BOXSIZE * 4, BOXSIZE, BOXSIZE, BLUE);
        if (oldcolor == MAGENTA) tft.fillRect(0, BOXSIZE * 5, BOXSIZE, BOXSIZE, MAGENTA);
        if (oldcolor == GRI) tft.fillRect(0, BOXSIZE * 6, BOXSIZE, BOXSIZE, GRI);
        if (oldcolor == ROZ) tft.fillRect(0, BOXSIZE * 7, BOXSIZE, BOXSIZE, ROZ);

        setColorBtnNum (currentcolor);
      }

    }

    // this determines which box has been pressed and turns the tft box the current color
    // and sets a RGB LED on the 8x8 matrix to the current color
    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        if (p.x > 50 + gridBox * j && p.y > 10 + gridBox * i && p.x < gridBox + 50 + gridBox * j && p.y < gridBox + 10 + gridBox * i)
        {
          tft.fillRect(50 + gridBox * j, 10 + gridBox * i, gridBox, gridBox, currentcolor);
          Screen[j][i] = currentLEDcolor;
        }
      }
    }


    draw8x8Grid();
    drawMenuButtons();

  }

  sendDataFromArray();

}


////////////////////////////////////////////////
//             Create New File Function       //
////////////////////////////////////////////////


void createNewFile()
{
  clearMsgArea();

  int yStart = 40;

  tft.setRotation(3);

  tft.setTextColor(WHITE);
  tft.setTextSize(1);


  if (numFiles == 10)
  {
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("Maximum 10 files. "));
    tft.setRotation(0);
    return;
  }

  char *temp = "/TFT8X8/DATAx.TXT";
  temp[12] = intToChar(numFiles);


  File myFile;

  myFile = SD.open(temp, FILE_WRITE);
  if (!myFile.available())
  {
    clearMsgArea();
    tft.setTextColor(RED);
    tft.setTextSize(1);
    tft.setCursor(0, yStart = YSTART);
    tft.println(F("Error!"));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("No File Exists!"));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("Check SD card!"));



  }
  else
  {


    myFile.close();

    fileName = temp; // set curent file to the new file
    File dir;

    dir = SD.open(dirName);

    tft.setRotation(0);
    showFileManager(); // redraws file manager window to show new file list
    tft.setRotation(3);
    dir.close();

    tft.setTextColor(WHITE);
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("File Size: "));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.print(myFile.size()); tft.println(" bytes");

    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("Num Files "));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(temp);
  }
  tft.setRotation(0);
}

////////////////////////////////////////
// show FileManager                 ////
////////////////////////////////////////

void  showFileManager()
{
  clearMsgArea();
  int yStart = 0;
  tft.fillRoundRect(5, 5 , tft.width() - 10, tft.height() - 130, rndBtnRad, WHITE);
  tft.drawRoundRect(5, 5 , tft.width() - 10, tft.height() - 130, rndBtnRad, RED);

  tft.fillRoundRect(fileMgrCloseBtn[0], fileMgrCloseBtn[1] , fileMgrCloseBtn[2], fileMgrCloseBtn[3], rndBtnRad, RED);
  tft.drawRoundRect(fileMgrCloseBtn[0], fileMgrCloseBtn[1] , fileMgrCloseBtn[2], fileMgrCloseBtn[3], rndBtnRad, BLACK);

  tft.drawLine(5 + 3, 170 + 3, 30 - 3, 195 - 3, BLACK); // draw '/' of X to cancel mgr
  tft.drawLine(5 + 3, 195 - 3, 30 - 3, 170 + 3, BLACK); // draw '\' or X to cancel mgr

  tft.fillRoundRect(fileNewBtn[0], fileNewBtn[1] , fileNewBtn[2], fileNewBtn[3], rndBtnRad, GREEN);
  tft.drawRoundRect(fileNewBtn[0], fileNewBtn[1] , fileNewBtn[2], fileNewBtn[3], rndBtnRad, BLACK);


  tft.setRotation(3);

  tft.setCursor(160, yStart = yStart + lineSpacing);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.println(F("File Manager "));

  tft.setCursor(165, yStart = yStart + 39);
  tft.println(F("New File "));


  tft.setTextSize(1);


  File dir = SD.open("/TFT8X8");

  if (dir.available())
  {
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.setCursor(0, YSTART);
    tft.println(F("File Manager"));
    printDirectory(dir, 0, yStart);


  }
  else
  {
    tft.setTextColor(RED);
    tft.setTextSize(1);
    tft.setCursor(0, yStart = YSTART);
    tft.println(F("Error!"));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("No File Exists!"));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("Check SD card!"));
  }

  dir.close();

  tft.setRotation(0);

}

/////////////////////////////////////////////////////////////
//            Print Directory Function                     //
//                                                         //
//   Lists out files in the file manager pop up window     //
/////////////////////////////////////////////////////////////

void printDirectory(File dir, int numTabs, int yStart) {

  tft.setTextColor(BLACK);
  tft.setCursor(140, yStart = yStart + 15);

  numFiles = 0 ;
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    tft.setCursor(139, yStart = yStart + 15);
    char *curFile = entry.name();

    if (curFile[4] == fileName[12]) {

      tft.fillRect(137, yStart - 4, 165, 15, YELLOW); // fill file select btn region

    }

    tft.print(" "); tft.print(curFile);
    //    tft.print(" "); tft.print(entry.size(), DEC); tft.print(" bytes "); // shows bytes
    tft.print(" frames: "); tft.print(entry.size() / 66);  // shows number of frames

    numFiles = numFiles + 1;

    tft.drawRect(137, yStart - 4, 165, 15, BLACK); // file select btn region

    entry.close();  // always close any object you open to avoid memory and file IO issues

  }
}

//////////////////////////////////////////////////////////////////
//  sets the number and color of the color picker buttons       //
//////////////////////////////////////////////////////////////////

void setColorBtnNum (int btnColor)
{
  tft.setRotation(3);
  tft.setTextColor(btnColor);
  tft.setTextSize(3);
  tft.drawLine(0, 0, 40, 40, btnColor);
  tft.drawLine(0, 40, 40, 0, btnColor);

  tft.setCursor(55, 10);
  tft.println("1");
  tft.setCursor(95, 10);
  tft.println("2");
  tft.setCursor(135, 10);
  tft.println("3");
  tft.setCursor(175, 10);
  tft.println("4");
  tft.setCursor(215, 10);
  tft.println("5");
  tft.setCursor(255, 10);
  tft.println("6");
  tft.setCursor(295, 10);
  tft.println("7");

  tft.setRotation(0);
}

/////////////////////////////////////////////////////////////////////
//       Sends data to control the 8x8 LED Matrix                 ///
/////////////////////////////////////////////////////////////////////


void sendBit(uint8_t aData)
{

  digitalWrite(DATA, aData);
  digitalWrite(CLOCK, HIGH);
  digitalWrite(CLOCK, LOW);
  digitalWrite(DATA, LOW);
}

/////////////////////////////////////////////////////////////////////
//     turns on/off the appropriate LED color                      //
//      going line by line                                         //
//      one LED at a time                                          //
/////////////////////////////////////////////////////////////////////

void sendDataFromArray()
{

  byte lineIndex;
  byte rowIndex;

  for (lineIndex = 0; lineIndex < 9; lineIndex++)
  {

    //Set Anode Line
    for (rowIndex = 0; rowIndex < 8; rowIndex++)
    {
      if (lineIndex == rowIndex)
      {
        sendBit(HIGH);
      } else {
        sendBit(LOW);
      }
    }

    //Set Green Row
    for (rowIndex = 0; rowIndex < 8; rowIndex++)
    {
      if ((Screen[lineIndex][rowIndex] & 1 ) == 0)
      {
        sendBit(HIGH);
      } else {
        sendBit(LOW);
      }
    }

    //Set Blue Row
    for (rowIndex = 0; rowIndex < 8; rowIndex++)
    {
      if ((Screen[lineIndex][rowIndex] & 2 ) == 0)
      {
        sendBit(HIGH);
      } else {
        sendBit(LOW);
      }
    }

    //Set Red Row
    for (rowIndex = 0; rowIndex < 8; rowIndex++)
    {
      if ((Screen[lineIndex][rowIndex] & 4 ) == 0)
      {
        sendBit(HIGH);
      } else {
        sendBit(LOW);
      }
    }


    //Send data to output
    digitalWrite(LATCH, HIGH);
    digitalWrite(LATCH, LOW);

  }

}


///////////////////////////////////////////////////////////
//            Start SD                                 ////
///////////////////////////////////////////////////////////

void initSD()
{

  int yStart = YSTART;

  tft.setTextColor(WHITE);
  tft.setCursor(0, yStart);
  tft.setRotation(3);
  tft.setTextSize(1);
  tft.println(F("Initializing SD..."));


#if defined __AVR_ATmega2560__
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCK )) {
    tft.setTextColor(RED);
    tft.println(F("failed!"));
    tft.println(F("Check SD card!"));
    tft.setRotation(0);
    return;
  }
#else
  if (!SD.begin(SD_CS)) {
    tft.println(F("failed!"));
    tft.setRotation(0);
    return;
  }
#endif

  if (!SD.exists(fileName))
  {
    SD.mkdir("TFT8X8");
  }
  File myFile;


  myFile = SD.open(fileName);

  tft.setCursor(0, yStart = yStart + lineSpacing);
  tft.println(fileName);
  tft.setCursor(0, yStart = yStart + lineSpacing);
  tft.print(F("Size: "));  tft.print(myFile.size()); tft.println(" bytes");
  tft.print(F("Frames: "));  tft.print(myFile.size() / 66);

  myFile.close();

  tft.setRotation(0);
}

///////////////////////////////////////////////////////////////////
//               play all frames of current file               ////
///////////////////////////////////////////////////////////////////

void playAll()
{

  clearMsgArea();

  int yStart = YSTART;
  tft.setTextColor(WHITE);
  tft.setCursor(0, yStart);
  tft.setRotation(3);
  tft.setTextSize(1);

  tft.println(F("Playing File: "));
  tft.setCursor(0, yStart = yStart + lineSpacing);
  tft.println(fileName);


  File myFile;

  int numFrames = 0;

  myFile = SD.open(fileName, FILE_READ);


#ifdef DEBUG
  Serial.println("File Contents: ");
#endif // DEBUG


  if (myFile) {
    // 66 because there are two extra characters at the end of each line
    numFrames = (myFile.size() / 66);
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.print(F("Size: "));
    tft.print(myFile.size()); tft.println(" bytes");

  }
  else // file doesn't exist
  {
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.setTextColor(RED);
    tft.println(F("No Data Saved!"));
    tft.setRotation(0);
    return;
  }
  myFile.close();

  tft.setCursor(0, yStart = yStart + lineSpacing);

  for (int l = 0; l < playLoops; l++) //controls the number of playback loops
  {

    tft.setRotation(3);
    tft.setCursor(0, yStart);
    tft.fillRect(28, yStart - 2, 18, 10, BLACK); // clear old loop count to draw a new one
    tft.print(F("Loop ")); tft.print(l + 1);
    tft.setRotation(0);

    showFrames(yStart);

  }

  clear8x8();
  resetScreen();

}

//////////////////////////////////////////////////////////////////
//    Opens current file and starts reading in the data         //
//    Drawing the frames to the tft and LED Matrix              //
//  You can try some different drawing modes by uncommenting    //
//  some lines of code in the function (not here in the box):   //
//                                                              //
//    sendDataFromArray(); // display box by box                //
//                                                              //
//                 and / or                                     //
//                                                              //
//    sendDataFromArray(); // display row by row                //
//                                                              //
//////////////////////////////////////////////////////////////////

void showFrames(int yStart)
{
  char inData[64];
  int index = 0;
  char inchar ;

  File myFile;

  myFile = SD.open(fileName);

  if (myFile) {

    int frameCount = 0;
    while (myFile.available()) {

      if (index < 64)
      {
        inchar = myFile.read(); //reads one character at a time
        inData[index] = inchar;
      }
      else
      {
        inchar =  myFile.read(); // read next two non data characters but do nothing // just to advance to next frame
      }

      if (index == 63) // last character in this frame
      {
        frameCount = frameCount + 1;
        tft.setRotation(3);
        tft.setCursor(45, yStart);
        tft.fillRect(85, yStart - 2, 18, 10, BLACK); // clear old frame count to draw a new one
        tft.print(" Frame "); tft.print(frameCount  );
        tft.setRotation(0);

        int cCnt = 0;
        for (int i = 0; i < 8; i++)
        {
          for (int j = 0; j < 8; j++)
          {

            Screen[i][j] = inData[cCnt];

            ////     sendDataFromArray(); // display box by box

            tft.fillRect(50 + gridBox * i, 10 + gridBox * j, gridBox, gridBox, colorToNum( inData[cCnt] ) );

            cCnt++;
          }

          //   sendDataFromArray(); // display row by row

        }


        long mils = millis();
        while (mils + frameDelay > millis() )
        {
          sendDataFromArray(); // display frame by frame
#ifdef DEBUG
          for (int i = 0; i < strlen(inData) - 2; i++)
          {
            Serial.print("inData["); Serial.print(i); Serial.print("]: ");
            Serial.println(inData[i]);
          }
          Serial.print("inData[] Size: ");
          Serial.println(strlen(inData));
#endif

        }

        if (frameDelay == 0) // needed if using the display frame by frame or the 8x8 will not turn on
        {
          sendDataFromArray();
#ifdef DEBUG
          for (int i = 0; i < strlen(inData) - 2; i++)
          {
            Serial.print("inData["); Serial.print(i); Serial.print("]: ");
            Serial.println(inData[i]);
          }
          Serial.print("inData[] Size: ");
          Serial.println(strlen(inData));
#endif

        }

        tft.setRotation(3);

      }  ///end if

      index++;
      if (index == 66) // last character reached
        index = 0;

    } // end while file available loop

    // close the file
    myFile.close();




  } else {
    // if the file didn't open, print an error:
    tft.println("error opening ");
    tft.println(fileName);
  }

  tft.setRotation(0);

}

////////////////////////////////////////////////////////
// delete all stored frames of current file          ///
////////////////////////////////////////////////////////

void deleteAll()
{

  clearMsgArea();

  int yStart = YSTART;

  tft.setRotation(3);
  tft.setTextColor(WHITE);
  tft.setCursor(0, yStart);
  tft.setTextSize(1);
  // delete file if it exists
  if (SD.exists(fileName))
  {

    tft.println(F("File:"));

    SD.remove(fileName);
    File f;
    f = SD.open(fileName, FILE_WRITE); // recreate the file
    f.close(); // always close any door you open

    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(fileName);
    tft.println("Contents deleted!");
    tft.print("Size: "); tft.print(f.size()); tft.println(" bytes");


  } else
  {
    tft.setTextColor(RED);
    tft.println(F("Error: Delete Failed!"));
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(F("No File Exists!"));
  }

  tft.setRotation(0);

}

////////////////////////////////////////////////////////////////////////
//      Show Confirmation Message which requires a touch to           //
//        confirm or cancel the touch event                           //
//         Prevents accidental deletion of the file                   //
//         and accidental screen clearing                             //
////////////////////////////////////////////////////////////////////////

void showConfirmPop()
{
  clearMsgArea();
  int yStart = YSTART;

  tft.fillRect(msgArea[0], msgArea[1], msgArea[2], msgArea[3], RED);
  tft.drawRect(msgArea[0], msgArea[1], msgArea[2], msgArea[3], GREEN);

  tft.fillRect(confirmBtn[0], confirmBtn[1], confirmBtn[2], confirmBtn[3], GREEN);
  tft.drawRect(confirmBtn[0], confirmBtn[1], confirmBtn[2], confirmBtn[3], YELLOW);

  tft.fillRect(cancelBtn[0], cancelBtn[1], cancelBtn[2], cancelBtn[3], MAGENTA);
  tft.drawRect(cancelBtn[0], cancelBtn[1], cancelBtn[2], cancelBtn[3], YELLOW);


  tft.setTextColor(WHITE);
  tft.setCursor(20, yStart);
  tft.setRotation(3);
  tft.setTextSize(1);
  tft.println(F("Are You Sure!"));

  tft.setCursor(35, 63);
  tft.print(F("YES"));
  tft.setCursor(95, 63);
  tft.print(F("NO"));
  tft.setRotation(0);
}

///////////////////////////////////////////////////////////////////////////////////
//      Saves the currently visible frame to the SD Card                       ////
///////////////////////////////////////////////////////////////////////////////////

void  saveToSD()
{

  tft.fillRect(msgArea[0], msgArea[1], msgArea[2], msgArea[3], GREEN);
  delay(50); // just a flash of green to visually indicate the file has been saved.
  clearMsgArea();

  int yStart = YSTART;

  tft.setTextColor(WHITE);
  tft.setCursor(0, yStart);
  tft.setRotation(3);
  tft.setTextSize(1);

  File myFile;

  myFile = SD.open(fileName, FILE_WRITE);

  // if the file opened, start writing data to the file using myFile.print()
  if (myFile) {

    tft.println("Saved to file: ");
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println(fileName);

    // write the new image data to SD
    for (int i = 0; i < 8; i++)
    {

      for (int j = 0; j < 8; j++)
      {
        myFile.print(Screen[i][j]);
      }

    }
    myFile.println("");

    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.print("Size: "); tft.print(myFile.size()); tft.println(" bytes");
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.print("Frames: "); tft.print(myFile.size() / 66);
    // close the file
    myFile.close();

  } else {
    // if the file didn't open, print an error
    tft.setTextColor(RED);
    tft.println("Error: Save Failed! "); //tft.println(fileName);
    tft.setCursor(0, yStart = yStart + lineSpacing);
    tft.println("No File Exists! "); //tft.println(fileName);
  }

  // re-open the file for reading if in DEBUG mode

  ///////////////////////////////////////////////////////////
  // reads the data from SD                             ////
  //////////////////////////////////////////////////////////
#ifdef DEBUG
  myFile = SD.open(fileName);
  if (myFile) {

    Serial.println("File Contents: ");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {

      Serial.write(myFile.read());
    }


    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    tft.setCursor(0, yStart = yStart + lineSpacing);
    //  tft.println("error opening "); tft.println(fileName);
    Serial.print("error opening "); Serial.println(fileName);
  }

#endif

  // reset the rotation so funky things don't happen
  tft.setRotation(0);

}

////////////////////////////////////////////////////////////////////////////////////////////
//              draws the 64 box 8x8 matrix grid on the tft                              ///
////////////////////////////////////////////////////////////////////////////////////////////

void draw8x8Grid()
{
  if ( !fileMgrPop) // don't draw the grid if the file manager is open
  {
    tft.drawRect(50, 10, 184, 184, WHITE); // main box

    // now draw out each box on the grid 64 total
    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        tft.drawRect(50 + gridBox * j, 10 + gridBox * i, gridBox, gridBox, WHITE);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////
//    clears the LED Matrix setting all 64 values to zero (OFF)  //
///////////////////////////////////////////////////////////////////

void clear8x8()
{
  byte lineIndex;
  byte rowIndex;

  for (lineIndex = 0; lineIndex < 8; lineIndex++)
  {
    for (rowIndex = 0; rowIndex < 8; rowIndex++)
    {
      Screen[lineIndex][rowIndex] = 0;
    }
  }
}


////////////////////////////////////////////////////////
//   resets the tft screen                            //
////////////////////////////////////////////////////////

void resetScreen ()
{
#ifdef DEBUG
  Serial.println("erase");
#endif

  tft.fillRect(0, 0, 240, 320, BLACK);
  tft.setRotation(0);

  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
  tft.fillRect(0, BOXSIZE, BOXSIZE, BOXSIZE, YELLOW);
  tft.fillRect(0, BOXSIZE * 2, BOXSIZE, BOXSIZE, GREEN);
  tft.fillRect(0, BOXSIZE * 3, BOXSIZE, BOXSIZE, CYAN);
  tft.fillRect(0, BOXSIZE * 4, BOXSIZE, BOXSIZE, BLUE);
  tft.fillRect(0, BOXSIZE * 5, BOXSIZE, BOXSIZE, MAGENTA);
  tft.fillRect(0, BOXSIZE * 6, BOXSIZE, BOXSIZE, GRI);
  tft.fillRect(0, BOXSIZE * 7, BOXSIZE, BOXSIZE,  ROZ);

  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE);

  setColorBtnNum (WHITE);

}


///////////////////////////////////////////////////////////////////////////////////////
//         clears he message data in the upper left corner of the tft screen         //
///////////////////////////////////////////////////////////////////////////////////////

void clearMsgArea()
{
  // Draw Message Area
  tft.fillRect(msgArea[0], msgArea[1], msgArea[2], msgArea[3], BLACK);
  // uncomment the line below to see the defined message area
  //  tft.drawRect(msgArea[0], msgArea[1], msgArea[2], msgArea[3], RED);
}

///////////////////////////////////////////////////////////
//    converts a color char into a uint16_t              //
//                                                       //
///////////////////////////////////////////////////////////

uint16_t  colorToNum(char color)
{
  // 0=Black(off)
  // 1=Green
  // 2=Blue
  // 3=Cyan
  // 4=Red
  // 5=Yellow
  // 6=Magenta
  // 7=White

  uint16_t returnVal = BLACK;

  switch (color) {

    case '1':
      returnVal = GREEN;
      break;
    case '2':
      returnVal = BLUE;
      break;
    case '3':
      returnVal = CYAN;
      break;
    case '4':
      returnVal = RED;
      break;
    case '5':
      returnVal = YELLOW;
      break;
    case '6':
      returnVal = MAGENTA;
      break;

  }

  return returnVal;
}


////////////////////////////////////////////////////////////////
//    converts and integer into a char                       ///
////////////////////////////////////////////////////////////////

char intToChar(int num)
{

  char retStr = '0';

  switch (num)
  {

    case 1:
      retStr = '1';
      break;
    case 2:
      retStr = '2';
      break;
    case 3:
      retStr = '3';
      break;
    case 4:
      retStr = '4';
      break;
    case 5:
      retStr = '5';
      break;
    case 6:
      retStr = '6';
      break;
    case 7:
      retStr = '7';
      break;
    case 8:
      retStr = '8';
      break;
    case 9:
      retStr = '9';
      break;

  }
  return retStr;
}

//////////////////////////////////////////////////////////////////////////////////
//          draws out all the menu buttons on the tft                          //
/////////////////////////////////////////////////////////////////////////////////

void drawMenuButtons()
{
  int textDrop = 7;

  // Save Button
  tft.fillRoundRect(saveBtn[0], saveBtn[1], saveBtn[2], saveBtn[3], rndBtnRad, GREEN);
  tft.drawRoundRect(saveBtn[0], saveBtn[1], saveBtn[2], saveBtn[3], rndBtnRad, WHITE);

  tft.setRotation(3);
  tft.drawChar(13, saveBtn[0] + 4, 'S', BLACK, BLACK, 2);
  tft.setCursor(35, saveBtn[0] + textDrop);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.println("Save");
  tft.setRotation(0);


  //delAllBtn
  tft.fillRoundRect(delAllBtn[0], delAllBtn[1], delAllBtn[2], delAllBtn[3], rndBtnRad, RED);
  tft.drawRoundRect(delAllBtn[0], delAllBtn[1], delAllBtn[2], delAllBtn[3], rndBtnRad, WHITE);

  tft.setRotation(3);
  tft.drawChar(13, delAllBtn[0] + 4, 'D', BLACK, BLACK, 2);
  tft.setCursor(35, delAllBtn[0] + textDrop);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.println("Delete");
  tft.setRotation(0);

  //playAllBtn
  tft.fillRoundRect(playAllBtn[0], playAllBtn[1], playAllBtn[2], playAllBtn[3], rndBtnRad, GREEN);
  tft.drawRoundRect(playAllBtn[0], playAllBtn[1], playAllBtn[2], playAllBtn[3], rndBtnRad, WHITE);

  tft.setRotation(3);
  tft.drawChar(13, playAllBtn[0] + 4, 'P', BLACK, BLACK, 2);
  tft.setCursor(35, playAllBtn[0] + textDrop);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.println("Play");
  tft.setRotation(0);


  //fileMgrBtn
  tft.fillRoundRect(fileMgrBtn[0], fileMgrBtn[1], fileMgrBtn[2], fileMgrBtn[3], rndBtnRad, GREEN);
  tft.drawRoundRect(fileMgrBtn[0], fileMgrBtn[1], fileMgrBtn[2], fileMgrBtn[3], rndBtnRad, WHITE);

  tft.setRotation(3);
  tft.drawChar(13, fileMgrBtn[0] + 4, 'F', BLACK, BLACK, 2);
  tft.setCursor(35, fileMgrBtn[0] + textDrop);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.println("Files");
  tft.setRotation(0);


  //speedUp
  tft.fillTriangle(speedUp[0] + speedUp[2], speedUp[1] + speedUp[3], speedUp[0], speedUp[1] + (speedUp[3] / 2), speedUp[0] + speedUp[3], speedUp[1], GREEN);
  // tft.drawRect(speedUp[0], speedUp[1], speedUp[2], speedUp[3], WHITE);

  //speedDown
  tft.fillTriangle(speedDown[0], speedDown[1], speedDown[0] + speedDown[2], speedDown[1] + (speedDown[3] / 2), speedDown[0], speedDown[1] + speedDown[3], GREEN);
  // tft.drawRect(speedDown[0], speedDown[1], speedDown[2], speedDown[3], WHITE); //

  tft.fillRect(speedUp[0] + 5, speedUp[1] - 20, 80, 15, BLACK);
  tft.setCursor( speedUp[0] + 9, speedUp[1] - 15);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("Delay "); tft.println(frameDelay);

  //loopUp

  //left arrow
  //  tft.fillTriangle(loopUp[0], loopUp[1], loopUp[0]+(loopUp[2]/2), loopUp[1]+(loopUp[3]/2), loopUp[0]+loopUp[2], loopUp[1], GREEN);
  // up arrow
  tft.fillTriangle(loopUp[0] + loopUp[2], loopUp[1] + loopUp[3], loopUp[0], loopUp[1] + (loopUp[3] / 2), loopUp[0] + loopUp[3], loopUp[1], GREEN);
  //  tft.drawRect(loopUp[0], loopUp[1], loopUp[2], loopUp[3], WHITE);

  tft.fillRect(loopUp[0] + 5, loopUp[1] - 20, 75, 15, BLACK);
  tft.setCursor( loopUp[0] + 9, loopUp[1] - 15);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("Loops "); tft.println(playLoops);

  //loopDown

  // down arrow
  tft.fillTriangle(loopDown[0], loopDown[1], loopDown[0] + loopDown[2], loopDown[1] + (loopDown[3] / 2), loopDown[0], loopDown[1] + loopDown[3], GREEN);
  //  tft.drawRect(loopDown[0], loopDown[1], loopDown[2], loopDown[3], WHITE);


}

