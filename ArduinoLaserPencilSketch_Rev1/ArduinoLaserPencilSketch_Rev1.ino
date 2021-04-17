/*
*********************************************************************************************************************************************
*********************************************************************************************************************************************
                                   ARDUINO PROGRAM(SKETCH) FOR LASER PENCIL ENGRAVER BREADBOARD
*********************************************************************************************************************************************
*********************************************************************************************************************************************
File: ArduinoLaserPencilSketch_Rev1 (.ino) 
Date: 11/11/18
This is an experimental project, not a plug and play unit. It works great. I have successfully burned hundreds of pencils and other materials.
As you construct and integrate the various mechanical, optical and electrical components, please take the time to test the laser, galvos and
x-axis stage by themselves before integrating them into a system. The DAC electrical drives for the galvos also need to be tested prior
to hooking the signals to the galvos, to prevent the galvos from being overdriven and damaged.
The laser is not eye-safe so please use the googles. 
Disclaimer:
I am a laser guy, not a software guy. This Arduino program may not be pretty but it works. If the system goes off track, push the START button
and try again. 
******************************************************
Project Title:
LASERS, MIRRORS AND NO.2 PENCILS
or Building a Breadboard on a Breadboard
******************************************************
Microcontroller:
IDE Microcontroller: Tools/Board/Arduino Mega 2560
IDE Programmer: Tools/Programmer/Arduino as ISP
IDE Port: Tools/Serial Port/As offered by host computer
Power Header on Arduino Mega board "5V" pin is connected to breadboard +5V power supply 
Power Header on Arduino Mega board: "Vin" pin should never be powered
Power Barrel Connector on Arduino Mega board: Should never be powered
USB Programming Connector on Arduino Mega board: Must use modified USB cable, with +5V wire snipped or bad things will happen
Program memory space: 258K bytes
Program memory used: 16K bytes
Available interrupt pins: 2,3,21,20,19,18 (int0,1,2,3,4,5)
40 characters = ~4 inches on a pencil.
*/

#include <PS2Keyboard.h> //Keyboard
#include <SPI.h>         //DACs
#include <LiquidCrystal_I2C.h>
PS2Keyboard keyboard;

// LCD over I²C screen:
#define lcd_width 20
// Must be 2 or more:
#define lcd_height 4

uint8_t lcd_y = 0; // which line
uint8_t lcd_x = 0;

char lcd_screen[lcd_height][lcd_width + 1]; // (lcd_width+1, for the \0 at the line end)
uint8_t lcd_screen_wrap = 0;

// LCD text editing:
bool lcd_editMode = false;
int lcd_editMode_i = 0; // The position of the text input caret. Equals lcd_editMode_len if the caret is at the end of the currently inputted text.
int lcd_editMode_len = 0; // The current length of inputted text.
int lcd_editMode_maxLen = 0; // The maximum length of lcd_editMode_input.
// Internally set to true while using the print functions during edit mode.
bool lcd_editMode_printOverride = false;
// SoME commenT
const char *lcd_editMode_prompt;
char *lcd_editMode_input;

LiquidCrystal_I2C lcd(0x27, lcd_width, lcd_height); // set the LCD address to 0x27 for a 20 chars and 4 line display

//Declare Pins
#define X_SEL 6          //EE-1003 X-SEL
#define Y_SEL 7          //EE-1003 Y-SEL
#define MISO 53          //EE-1003 MISO
#define BRN_PIN A8       //BRN
#define CSP_PIN A9       //CSP
#define CHR_PIN A11      //CHR
#define YOFF_PIN A12     //YOFF
#define LASER_EN_PIN 23  //LASER EN
#define STP_BRN_PIN 38   //STP BRN
#define BURN_PIN 40      //BURN
#define STG_LEFT_PIN 34  //STG LEFT
#define STG_RGHT_PIN 32  //STG RIGHT
#define STG_HOME_PIN 36  //STG HOME
#define DIR_PIN 13       //DIR_PIN on EE-1003
#define EN_PIN 12        //EN on EE-1003
#define STEP_PIN 11      //STEP on EE-1003
#define BEEP_PIN 27      //BEEP on EE-1003
#define LIM1_RGHT_PIN 19 //LIM1 on EE-1003
#define LIM2_LEFT_PIN 18 //LIM2 on EE-1003
#define MS1_PIN 10       //MS1 on EE-1003
#define MS2_PIN 9        //MS2 on EE-1003
#define SAFETY_PIN 48    //Unused.
#define KB_DATA_PIN 8    //DATA on EE-1003
#define KB_CLK_PIN 2     //CLK on EE-1003

//Declare User Values
#define BeamGrossVerticalOffset 0           //out of 1023   Coarse vertical offset
#define BeamGrossHorizOffset 0              //out o3 1023   Coarse horiz offset
#define BurnDelayDivisor 3                  //1023/3 = 341ms max
#define CharSizeDivisor 90                  //1023/90 = 11 max
#define CharSpaceDivisor 90                 //1023/90 = 11 max
#define yOffsetDivisor 16                   //1023/16 = 64  max Fine vertical offset
#define HomeBackOffSteps 100               // number of steps to move off of the limit switch during homing.

#define IDLE_DELAY 100 // The amount of milliseconds delayed repeatedly in idle loops

//Declare General Variables
long x;
int y;
//int z; (unused)
int i = 0;
byte ii;
double j;
int burn;
byte LetterIndex;
int CharSizeValue;
int CharSpaceValue;
int yOffsetValue;
int BurnDelayValue = 0;
int pwmValue; //sent to TTL pin of laser controller
//int Kerning; unused
byte Letter;
byte len;
byte NumberOfLettersToDraw;
byte NumberOfChars;
int VoltageToDAC;
byte HighByte; //for serial DAC
byte LowByte;  //for serial DAC

//Declare Variables for X-Axis Stepper (from Sam: none of these appear to be used.)
//int NumberofSteps;
//int StepCntRst;
//long StepperCounter = 0;
//int StepCounter;
//volatile byte TriggerJog = 0;
//char FileName[40]; //unused? confirm?
//byte time; //unused?
//char time1; //unused?

//Data out pin = 51, MOSI Pin,    SPI routine specifies it
//Clock out pin = 52, SCK Pin,    SPI routine specifies it

//if the #include PS2Keyboard library is used, pin 2 must be used for the PS2 keyboard interrupt
char c; //or it will not work for all PS-2 keyboards.  Pin 2 = interrupt 0.


//CHARACTER ARRAY
//Array reads DOWN and ACROSS, CharacterArray[DOWN][ACROSS], [96]DOWN [46]ACROSS, CharacterArray[ASCII letter to be burned][x,y burn dot coordinates]
//Each row contains the XY dots that form the letters. Each letter can have up to 46/2 = 23 dots. Although there are 5 x 7 = 35 dots that can be
//burned. The array would have to be extended to accommodate more dots.

//All rows MUST BE COMPLETELY FILLED, using zeros if needed.
//A 9,9 is the "end of character" indicator in this array.

//THE CHARACTER ROWS THAT START WITH 0,0 HAVE NOT BEEN IMPLEMENTED YET. HERE'S YOUR CHANCE TO DO SOME SERIOUS PROGRAMMING!!!!
//Dot coordinates for special characters like emoji's can be inserted in any unused row and called up by using that key. Except
//your special character will not show up on the LCD, only during the pencil burn.
//The lower case letters(a-z) have the same dot coordinates as upper case letters(A-Z), so you can type in either case and still
//have the upper case letter burned in the pencil. However, if you are ambitious and want to modify the XY coordinates of a-z
//to form lower case letters, go for it.
//The function keys(F1-F12) are not recognized at present.

//                                                                                                                                               Key    ASCII
byte CharacterArray[96][46] = {
    9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //SPACE  32
    3, 1, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //!      33
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //"      34
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //#      35
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //$      36
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //%      37
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //&      38
    3, 6, 3, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //'      39
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //(      40
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //)      41
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //*      42
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //+      43
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //,      44
    2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //-      45
    3, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //.      46
    1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 3, 1, 2, 1, 2, 3, 3, 3, 4, 3, 2, 5, 4, 5, 9, 9, 0, 0, 0, 0,  // '/'   47  FACE
    1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 2, 3, 3, 4, 4, 5, 9, 9, 0, 0, 0, 0, 0, 0,  //0      48
    2, 1, 3, 1, 4, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 2, 6, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //1      49
    1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 4, 2, 4, 1, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //2      50
    1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 3, 4, 4, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 1, 2, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //3      51
    4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 3, 6, 2, 5, 1, 4, 1, 3, 2, 3, 3, 3, 5, 3, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //4      52
    1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 4, 5, 3, 5, 2, 5, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //5      53
    2, 4, 3, 4, 4, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //6      54
    1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 5, 6, 4, 5, 3, 4, 2, 3, 2, 2, 2, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //7      55
    1, 2, 1, 3, 2, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //8      56
    1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 4, 7, 3, 7, 2, 7, 1, 6, 1, 5, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //9      57
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //:      58
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //;      59
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //<      60
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //=      61
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //>      62
    1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 3, 3, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //?      63
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //@      64
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 5, 1, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //A      65
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 2, 4, 3, 4, 9, 9, 0, 0, 0, 0,  //B      66
    5, 2, 4, 1, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //C      67
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //D      68
    5, 1, 4, 1, 3, 1, 2, 1, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //E      69
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 3, 7, 4, 7, 5, 7, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //F      70
    5, 6, 4, 7, 3, 7, 2, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 4, 4, 3, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //G      71
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 1, 1, 2, 4, 3, 4, 4, 4, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //H      72
    2, 1, 3, 1, 4, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 2, 7, 3, 7, 4, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //I      73
    1, 3, 1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //J      74
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 5, 7, 4, 6, 3, 5, 2, 4, 3, 3, 4, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //K      75
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //L      76
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 6, 3, 5, 4, 6, 5, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //M      77
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 5, 3, 4, 4, 3, 5, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //N      78
    1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //O      79
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 4, 2, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //P      80
    1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 3, 3, 4, 2, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //Q      81
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 4, 2, 4, 4, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //R      82
    1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 4, 4, 3, 4, 2, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //S      83
    1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 3, 6, 3, 5, 3, 4, 3, 3, 3, 2, 3, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //T      84
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //U      85
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 2, 2, 3, 1, 4, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //V      86
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 1, 1, 2, 2, 3, 3, 4, 2, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //W      87
    1, 7, 1, 6, 2, 5, 3, 4, 4, 5, 5, 6, 5, 7, 1, 1, 1, 2, 2, 3, 4, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //X      88
    1, 7, 1, 6, 1, 5, 2, 4, 3, 3, 4, 4, 5, 5, 5, 6, 5, 7, 3, 2, 3, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //Y      89
    1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //Z      90
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //[      91
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //\      92
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //]      93
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //       94
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //_      95
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //       96
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 5, 1, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //a=A    97
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 2, 4, 3, 4, 9, 9, 0, 0, 0, 0,  //b=B    98
    5, 2, 4, 1, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //c=C    99
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //d=D    100
    5, 1, 4, 1, 3, 1, 2, 1, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //e=E    101
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 3, 7, 4, 7, 5, 7, 2, 4, 3, 4, 4, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //f=F    102
    5, 6, 4, 7, 3, 7, 2, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 4, 4, 3, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //g=G    103
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 1, 1, 2, 4, 3, 4, 4, 4, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //h=H    104
    2, 1, 3, 1, 4, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 2, 7, 3, 7, 4, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //i=I    105
    1, 3, 1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //j=J    106
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 5, 7, 4, 6, 3, 5, 2, 4, 3, 3, 4, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //k=K    107
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //l=L    108
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 6, 3, 5, 4, 6, 5, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //m=M    109
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 5, 3, 4, 4, 3, 5, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //n=N    110
    1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //o=O    111
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 4, 2, 4, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //p=P    112
    1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 5, 4, 5, 3, 5, 2, 4, 1, 3, 1, 2, 1, 3, 3, 4, 2, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //q=Q    113
    1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 2, 7, 3, 7, 4, 7, 5, 6, 5, 5, 4, 4, 3, 4, 2, 4, 4, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0,  //r=R    114
    1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 4, 4, 3, 4, 2, 4, 1, 5, 1, 6, 2, 7, 3, 7, 4, 7, 5, 6, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //s=S    115
    1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 3, 6, 3, 5, 3, 4, 3, 3, 3, 2, 3, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //t=T    116
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 2, 1, 3, 1, 4, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //u=U    117
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 2, 2, 3, 1, 4, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //v=V    118
    1, 7, 1, 6, 1, 5, 1, 4, 1, 3, 1, 2, 1, 1, 2, 2, 3, 3, 4, 2, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //w=W    119
    1, 7, 1, 6, 2, 5, 3, 4, 4, 5, 5, 6, 5, 7, 1, 1, 1, 2, 2, 3, 4, 3, 5, 2, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //x=X    120
    1, 7, 1, 6, 1, 5, 2, 4, 3, 3, 4, 4, 5, 5, 5, 6, 5, 7, 3, 2, 3, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //y=Y    121
    1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //z=Z    122
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //{      123
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //|      124
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //}      125
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //~      126
    0, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //del    127

byte MessageTable[80] = {0}; //declare message table

//*********************************************************************************************************
void setup()
{

  pinMode(X_SEL, OUTPUT);
  pinMode(Y_SEL, OUTPUT);
  pinMode(MISO, OUTPUT);
  pinMode(CHR_PIN, INPUT);
  pinMode(YOFF_PIN, INPUT);
  pinMode(BRN_PIN, INPUT);
  pinMode(CSP_PIN, INPUT);
  pinMode(STP_BRN_PIN, INPUT);
  pinMode(BURN_PIN, INPUT);
  pinMode(LIM1_RGHT_PIN, INPUT);
  pinMode(LIM2_LEFT_PIN, INPUT);
  pinMode(LASER_EN_PIN, OUTPUT);
  pinMode(STG_RGHT_PIN, INPUT);
  pinMode(STG_LEFT_PIN, INPUT);
  pinMode(BEEP_PIN, OUTPUT);
  pinMode(STG_HOME_PIN, INPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(SAFETY_PIN, INPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);


  digitalWrite(X_SEL, HIGH); //neg CS for SPI writing in data
  digitalWrite(Y_SEL, HIGH); //neg CS for SPI writing in data
  delay(5);
  digitalWrite(MISO, HIGH);  //neg pulse to latch data to DAC outputs

  digitalWrite(MS1_PIN, LOW);
  digitalWrite(MS2_PIN, LOW);

  //Interrupt calls
  //****************
  //Available interrupt pins: 2,3,21,20,19,18 (int 0,1,2,3,4,5)
  //attachInterrupt(Keyboard); //Pin  2 = interrupt 0
  //No other interrupts are used due to noise from stepper

  SPI.setClockDivider(SPI_CLOCK_DIV128);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128); //does this need to be here twice

  keyboard.begin(KB_DATA_PIN, KB_CLK_PIN);

  delay(100);
  analogWrite(LASER_EN_PIN, 0); //pre-shuts laser off, double safety

  lcd_init();

  ResetDACs();
  Beep(2); //system is booted up and ready
}

//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
void loop()
{

  Start();
}
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************

//***********************************************************************************
//      FUNCTIONS, EXCEPT FOR X-AXIS STEPPER STUFF WHICH IS AT THE BOTTOM
//***********************************************************************************

void Start()
{
  printStartMenu();
  for (;;) {
    if ((digitalRead(STG_LEFT_PIN)) == 0)
      ManualJogFunc(true);
    if ((digitalRead(STG_RGHT_PIN)) == 0)
      ManualJogFunc(false);
    if ((digitalRead(STG_HOME_PIN)) == 0)
      ManualHome();

    if (keyboard.available())
    {
      c = keyboard.read();
      if (c == 49) // 1.NEW MESSAGE
      {
        Beep(1);
        ReadPots();
        MessageFormatting();
        TypeNewMessage();
        BurnMessageSequence();
        printStartMenu();
      }

      if (c == 50) // 2. REPEAT BURN (comment from sam: what happens if you press this when you hadn't burned already)
      {
        lcd_clear();
        Beep(1);
        BurnMessageSequence();
        printStartMenu();
      }

      if (c == 51) // 3. CHARACTER ADJUST
      {
        lcd_clear();
        Beep(1);
        AdjCharSizes();
        printStartMenu();
      }

      if ((c <= 48) || (c >= 52)) //error check for incorrect keyboard choices
      {
        Beep(2);
        continue;
      }
    }
    
    delay(IDLE_DELAY);
  }
}

void printStartMenu(){
  lcd_clear();
  lcd_println("START:TYPE 1,2 OR 3");
  lcd_println("1.NEW MESSAGE");
  lcd_println("2.REPEAT BURN");
  lcd_print("3.CHARACTER ADJUST");
}

void MessageFormatting()
{

  int cnt;
  byte ch;
  byte storedChar;

  lcd_clear();
  lcd_println("-MESSAGE PARAMETERS");
  lcd_println(" 40 CHAR MAXIMUM = ");
  lcd_println(" 2 LINES ON LCD");
  lcd_print(" <ENTER>");
  WaitForEnterKey();
  Beep(1);

  lcd_clear();
  lcd_println("-USE BACKSPACE FOR");
  lcd_println(" CORRECTIONS");
  lcd_println("-DO <ENTER> NOW TO"); 
  lcd_print(" OPEN TYPING WINDOW");
  WaitForEnterKey();
  Beep(1);
}

//************************************************************
void TypeNewMessage()
{
  lcd_clear();
  
  char c;
  bool lastErrorWasUnsupportedChar = false;
  
  lcd_editMode_prompt = "Enter message:";
  lcd_editMode_init(MessageTable, 40);
  
  lcd_editMode = true;
  lcd_repaint();
  
  for (;;) {
    while (!keyboard.available()) delay(10);
    
    c = keyboard.read();
    /*PS2_KC_BREAK
      PS2_KC_ENTER
      PS2_KC_ESC
      PS2_KC_KPLUS
      PS2_KC_KMINUS
      PS2_KC_KMULTI
      PS2_KC_NUM
      PS2_KC_BKSP*/
    switch (c) {
    case PS2_ENTER:
      Beep(2);
      NumberOfChars = lcd_editMode_off();
      return;
    case PS2_DELETE: // Backspace key or Delete key.
      if (lcd_editMode_backsp()) {
        Beep(1);
      }
      break;
    case PS2_LEFTARROW:
      if (lcd_editMode_setCursor(lcd_editMode_i - 1)) {
        Beep(1);
      }
      break;
    case PS2_RIGHTARROW:
      if (lcd_editMode_setCursor(lcd_editMode_i + 1)) {
        Beep(1);
      }
      break;
    case PS2_UPARROW:
      if (lcd_editMode_setCursor(lcd_editMode_i - lcd_width)) {
        Beep(1);
      }
      break;
    case PS2_DOWNARROW:
      if (lcd_editMode_setCursor(lcd_editMode_i + lcd_width)) {
        Beep(1);
      }
      break;
    default: // Any other character.
      if (c >= 32 && c <= 127) { // 32 = ' ' and 127 = '~'. The ASCII printables.
        if (CharacterArray[c - 32][1] != 0) { // If the character is in the character array:
          if (lcd_editMode_type(c)) {
            Beep(1);
            lcd_clear();
            lcd_print("40 CHARACTER MAXIMUM");
            
            lastErrorWasUnsupportedChar = false;
          }
        } else {
          Beep(1);
          if (!lastErrorWasUnsupportedChar) {
            lcd_clear();
          } else {
            lcd_lineBreak();
          }
          lcd_print(String(c));
          lcd_print(" NOT SUPPORTED");
          
          // This might help out users who don't understand the concept of printing and scrolling yet:
          if (lastErrorWasUnsupportedChar) {
            lcd_print(" TOO");
          }
          
          lastErrorWasUnsupportedChar = true;
        }
      } else { // Unused keyboard button:
        Beep(1);
      }
      break;
    }
  }
}
/*void TypeNewMessage()
{

  int cnt = 0;
  int ch;
  char ch1;
  
  lcd_clear();
  lcd.blink();
  ch = keyboard.read();
  while (ch != 13)  //the only thing here is backspace doesnt work. I'm assuming you are working on that. Rewrote without goto.
  {
    if (keyboard.available())
    {
      ch = keyboard.read();
      if (ch != 13 && ch != 127 && CharacterArray[ch - 32][1] != 0)
      { //not enter, not bksp, character exists
        lcd_print(String(char(ch)));
        MessageTable[cnt] = ch;
        cnt++;
      }
      else if (ch != 13 && ch != 127 && CharacterArray[ch - 32][1] == 0)
      { //character doesnt exist, still not enter or bksp (foolproofing ig?)
        Beep(3);
      }
      else if (ch == 127)
      { //if you hit backspace
        if (cnt > 0)
        {
          MessageTable[cnt - 1] = {0};
          //lcd_editMode_backsp(); this function didnt really work so up 2 u ig
          cnt--;
        }
        else
        {
          Beep(3);
        }
      }
    }
  }
  Beep(2);
  NumberOfChars = cnt;
}*/

//*********************************************************

void BurnMessageSequence()
{

  byte ch;

  lcd_clear();
  lcd_println("-STANDBY: WAIT FOR");
  lcd_println(" STAGE TO STOP");
  Home();
  Beep(1);
  lcd_clear();
  lcd_println("-MOUNT PENCIL");
  lcd_println("-PUT ON GOOGLES");
  lcd_println("-PUSH BURN BUTTON OR");
  lcd_print(" HOLD STOP TO QUIT");
  do
  {
  } while (digitalRead(BURN_PIN) == HIGH && digitalRead(STP_BRN_PIN) == HIGH);
  if (digitalRead(STP_BRN_PIN) != LOW)
  {
    Beep(1);
    lcd_clear();
    ReadPots();
    BurnMessage();

    Beep(3);
    lcd_clear();
    lcd_println("-BURN CYCLE DONE");
    lcd_println("-REMOVE PENCIL");
    lcd_println("<ENTER>");
    WaitForEnterKey();
    Beep(1);
  }else {
    lcd_clear();
    lcd_println("-Burn Cycle Stop");
    lcd_println("-Return to Menu");
    lcd_println("<ENTER>");
    WaitForEnterKey();
    Beep(1);
  }
}

//********************************************************
void BurnMessage()
{
  byte AscCode;
  byte ArrayPosition;

  //Step through MessageTable printing one letter at a time
  for (byte bb = 0; bb <= (NumberOfChars - 1); bb++)
  {
    AscCode = MessageTable[bb]; //Message table has a string of ASCII numbers to print, A = 65, a = 97 but it prints A
    if (AscCode == 13)
      break; //13 = enter key
    lcd_print(String(char(AscCode)));
    ArrayPosition = (AscCode - 32); //offset for CharacterArray (ASCII chars start at 32, CharacterArray starts at 0)
    LetterIndex = ArrayPosition;    //vertical index for CharacterArray
    BurnOneLetter();                //writing each letter as it is called
  }
  ResetDACs();
  //digitalWrite(ExhaustFanPin, LOW);
}

//**************************************************
void BurnOneLetter()
{

  int xArrayVal;
  int yArrayVal;
  int i = 0;

TableLoop:
  if (digitalRead(STP_BRN_PIN) == LOW)
  { //check for Stop button
    Beep(3);
    digitalWrite(LASER_EN_PIN, LOW); //turn laser off, if not off
    lcd_clear();                     //clear screen
    lcd_println("BURN STOPPED:");
    lcd_print("<ENTER>");
    WaitForEnterKey();
    Beep(1);
  }
  else //used if/else here to replace a recursive call to Start()
  {
    xArrayVal = CharacterArray[LetterIndex][i];
    yArrayVal = CharacterArray[LetterIndex][i + 1];
    if ((xArrayVal == 9) || (yArrayVal == 9))
      goto BurnOneDone; //"9,9" is the 5x7 "end of message" char

    //write data to DACs
    //***************************************************
    //SERIAL  X DAC
    digitalWrite(X_SEL, LOW);                                                  //X chip select
    VoltageToDAC = (((xArrayVal * CharSizeValue) * 7) + BeamGrossHorizOffset); //0 to 1023(measured=2.03mv/count)
    ConvertToTwoBytes();
    SPI.transfer(HighByte);
    SPI.transfer(LowByte);
    digitalWrite(X_SEL, HIGH);

    //SERIAL Y DAC
    //You can dynamically adjust vertical position of char on pencil, each time a new char is burned
    //yOffsetValue = analogRead(YOFF_PIN);               //0-1023   1024/16 = 64
    //yOffsetValue = int(yOffsetValue / yOffsetDivisor); //need more offset to get beam on pencil, vertically

    digitalWrite(Y_SEL, LOW); //Y chip select
    VoltageToDAC = (((((9-yArrayVal) * CharSizeValue) + yOffsetValue) * 7) + BeamGrossVerticalOffset);
    ConvertToTwoBytes();
    SPI.transfer(HighByte);
    SPI.transfer(LowByte);
    digitalWrite(Y_SEL, HIGH);

    //Latch X & Y values to DAC outputs
    digitalWrite(MISO, LOW); //10us negative pulse to chips to latch in write
    delayMicroseconds(10);
    digitalWrite(MISO, HIGH);

    if (LetterIndex == 0)
      goto NoBurn; //skip one space

    //BURNING ONE DOT ON THE PENCIL
    //**************************************************
    digitalWrite(LASER_EN_PIN, HIGH); //LASER ON
    delay(BurnDelayValue);            //typ: 100-500 ms.
    digitalWrite(LASER_EN_PIN, LOW);  //LASER OFF
    //**************************************************

  NoBurn:
    i = i + 2; //advance index two digits, to next x,y point
    goto TableLoop;

  BurnOneDone:
    i = 0;

    moveStage(-50 + (CharSpaceValue * -50));               //move left one letter during burn
  }
}

//**************************************************
void ConvertToTwoBytes()
{
  int DecNum = VoltageToDAC; //0 to 1023

  //HighByte
  //********************************
  HighByte = 112; //reqd upper nib for DAC = bits 15,14,13,12 = (0111) = 112 dec
  if (DecNum >= 512)
  {
    HighByte = HighByte + 8;
    DecNum = DecNum - 512;
  }
  if (DecNum >= 256)
  {
    HighByte = HighByte + 4;
    DecNum = DecNum - 256;
  }
  if (DecNum >= 128)
  {
    HighByte = HighByte + 2;
    DecNum = DecNum - 128;
  }
  if (DecNum >= 64)
  {
    HighByte = HighByte + 1;
    DecNum = DecNum - 64;
  }

  //LowByte
  //********************************
  if (DecNum >= 0)
  {
    LowByte = (DecNum * 4); //LSB and LSB+1 are both zeros, therefore bit 0 of
  }                         //the Voltage starts at Bit 2, which is a 4 dec
}

//**************************************************************
void AdjCharSizes()
{

  Beep(1);
  c = 0;
  do
  {
    lcd_clear();
    BurnDelayValue = analogRead(BRN_PIN);                    //0-1023   pwm = 0-255   Burn =  0 to 150ms
    BurnDelayValue = int(BurnDelayValue / BurnDelayDivisor); //341ms max
    //move cursor to line 3 (whatever that means?)
    lcd_print("-BURN TIME:");
    lcd_print(String(BurnDelayValue));
    lcd_println("ms.  ");

    CharSizeValue = analogRead(CHR_PIN);                  //0-1023   1024/100 = 10
    CharSizeValue = int(CharSizeValue / CharSizeDivisor); //
    lcd_print("-CHAR SIZE:");
    lcd_println(String(CharSizeValue) + "/11");
    lcd_print("<ENTER>");
    if (keyboard.available())
    {
      c = keyboard.read();
    }
    delay(500);
  } while (c != 13);
  Beep(1);
  delay(100);
  c = 0;
  do
  {
    lcd_clear();
    //display Character Spacing so it can be adjusted
    CharSpaceValue = analogRead(CSP_PIN);                    //0-1023   1024/100 = 10
    CharSpaceValue = int(CharSpaceValue / CharSpaceDivisor); //
    lcd_print("-CHAR SPACE:");
    lcd_println(String(CharSpaceValue) + "/11");

    //display Beam Vertical position so it can be adjusted
    yOffsetValue = analogRead(YOFF_PIN);               //0-1023   1024/16 = 64
    yOffsetValue = int(yOffsetValue / yOffsetDivisor); //need more offset to get beam on pencil, vertically
    lcd_print("-BEAM VERT:");
    lcd_println(String(yOffsetValue) + "/64");
    lcd_print("<ENTER>");
    if (keyboard.available())
    {
      c = keyboard.read();
    }
    delay(500);
  } while (c != 13);
  Beep(1);
}

//*********************************************************
void ResetDACs()
{

  VoltageToDAC = 0;
  ConvertToTwoBytes();

  digitalWrite(X_SEL, LOW); //X chip select
  SPI.transfer(HighByte);
  SPI.transfer(LowByte);
  digitalWrite(X_SEL, HIGH);

  digitalWrite(Y_SEL, LOW); //Y chip select
  SPI.transfer(HighByte);
  SPI.transfer(LowByte);
  digitalWrite(Y_SEL, HIGH);

  //Latch X & Y values to DAC outputs
  digitalWrite(MISO, LOW); //10us negative pulse to chips to latch in write
  delayMicroseconds(10);
  digitalWrite(MISO, HIGH);
}

//********************************************************
void Beep(int beeps)
{
  /*for (x = 1; x <= y; x++)
  {
    digitalWrite(BEEP_PIN, HIGH);
    delay(100);
    digitalWrite(BEEP_PIN, LOW);
    delay(100);
  }*/
  
  if (beeps == 0) return;
  
  for (int i = 0; ;) { // loop-and-a-half
    digitalWrite(BEEP_PIN, HIGH);
    delay(100);
    digitalWrite(BEEP_PIN, LOW);
    
    i++;
    if (i == beeps) break;
    
    delay(100);
  }
}

//*********************************************************
void WaitForEnterKey()
{
  char c;
  do
  {
    if (keyboard.available())
    {
      c = keyboard.read();
    }
    delay(IDLE_DELAY);
  } while (c != PS2_ENTER);
  delay(300); // TODO: why is this here?
}

//**********************************************************
void ReadPots()
{

  BurnDelayValue = analogRead(BRN_PIN);                    //0-1023
  BurnDelayValue = int(BurnDelayValue / BurnDelayDivisor); //typ 1023/2 = typ 512ms max burn time

  CharSizeValue = analogRead(CHR_PIN);                  //typ 1023/90 = 11
  CharSizeValue = int(CharSizeValue / CharSizeDivisor); //

  CharSpaceValue = analogRead(CSP_PIN); //typ 1023/90 = 11
  CharSpaceValue = int(CharSpaceValue / CharSpaceDivisor);

  yOffsetValue = analogRead(YOFF_PIN);               //typ 1023/16 = 64
  yOffsetValue = int(yOffsetValue / yOffsetDivisor); //may need more offset to get beam on pencil, vertically
                                                     //refer to Gross Vert Offset at start of program
}

//********************************************************************************************************************
//                               FUNCTIONS FOR X-AXIS STAGE STEPPER AND LIMIT SWITCHES
//********************************************************************************************************************

/**
 * @brief Moves the stage according to DIR_PINectional input and number of steps.
 * 
 * @param steps Number of steps. Positive for right movement, negative for left movement.
 * 
 */
void moveStage(int steps) //Forward means stage is moving to the right
{
  if (steps > 0){
    digitalWrite(DIR_PIN,LOW);
  }else{
    digitalWrite(DIR_PIN,HIGH);
  }
  for (x = 0; x < abs(steps); x++) //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STEP_PIN, HIGH); //Trigger one step forward
    delayMicroseconds(700);
    digitalWrite(STEP_PIN, LOW); //Pull step pin low so it can be triggered again
    delayMicroseconds(700);
    if (digitalRead(LIM2_LEFT_PIN) == LOW or digitalRead(LIM1_RGHT_PIN) == LOW) {
      if (digitalRead(LIM1_RGHT_PIN) == LOW){
        digitalWrite(DIR_PIN,HIGH);
        for (x=0;x<50;x++){
          digitalWrite(STEP_PIN, HIGH); //Trigger one step forward
          delay(1);
          digitalWrite(STEP_PIN, LOW); //Pull step pin low so it can be triggered again
          delay(1);
        }
      }
      if (digitalRead(LIM2_LEFT_PIN) == LOW){
        digitalWrite(DIR_PIN,LOW);
        for (x=0;x<50;x++){
          digitalWrite(STEP_PIN, HIGH); //Trigger one step forward
          delay(1);
          digitalWrite(STEP_PIN, LOW); //Pull step pin low so it can be triggered again
          delay(1);
        }
      }
      break;
    }
  }
}

/**
 * First, this function moves the stage all the way to the right.
 * Then, the function moves the stage an arbitrary value of steps to the left.
 */
void Home()
{
  moveStage(10000);
  moveStage(-1 * HomeBackOffSteps);
  Beep(3);
}

/**
 * @brief Invoked when home button is pressed.
 * Moves stage all the way to the right, then back left an arbitrary number of steps.
 */
void ManualHome()
{
  moveStage(10000);
  moveStage(-1 * HomeBackOffSteps);
  Beep(3);
  /* do
  {
    moveStage(10000);
    if (digitalRead(LIM2_LEFT_PIN) == LOW || digitalRead(LIM1_RGHT_PIN))   //check limit switch. if it doesnt hit then keep looping.
    {
      moveStage(-1 * HomeBackOffSteps, true);
      Beep(3);
      do
      {
      } while (digitalRead(STG_HOME_PIN) == LOW); //button lockout loop
      //Start(); Originally, this function called Start(); again. I'm not sure why exactly.
    }
  } while (digitalRead(LIM1_RGHT_PIN) == HIGH); */
}
/**
 * @brief Invoked when a manual jog function is called.
 * Moves the stage continuously either left or right. Checks for limit switches.
 * @param left Set this to true for left movement, false for right movement.
 */
void ManualJogFunc(bool left)
{ // I rewrote these since i felt that doing it this way will make the movement feel less stuttery.
  int buttonPin;
  if (left)
  {
    buttonPin = STG_LEFT_PIN;
    digitalWrite(DIR_PIN, HIGH);
  }
  else
  {
    buttonPin = STG_RGHT_PIN;
    digitalWrite(DIR_PIN, LOW);
  }
  do
  {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(600); //delay should be fine to use here, since its only 50ms and the motor only moved 1 step.
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(600);
    if (digitalRead(LIM2_LEFT_PIN) == LOW || digitalRead(LIM1_RGHT_PIN) == LOW)
    {
      if (digitalRead(LIM1_RGHT_PIN) == LOW){
        digitalWrite(DIR_PIN,HIGH);
        for (x=0;x<50;x++){
          digitalWrite(STEP_PIN, HIGH); //Trigger one step forward
          delay(1);
          digitalWrite(STEP_PIN, LOW); //Pull step pin low so it can be triggered again
          delay(1);
        }
      }
      if (digitalRead(LIM2_LEFT_PIN) == LOW){
        digitalWrite(DIR_PIN,LOW);
        for (x=0;x<50;x++){
          digitalWrite(STEP_PIN, HIGH); //Trigger one step forward
          delay(1);
          digitalWrite(STEP_PIN, LOW); //Pull step pin low so it can be triggered again
          delay(1);
        }
      }
      do {} while (digitalRead(buttonPin) == LOW);
      break;
    }
  } while (digitalRead(buttonPin) == LOW); //button lockout loop
}




//**************************************************************************************************************************************
//                                                       LCD Functions
//**************************************************************************************************************************************
// *Important:*
// There are two screens with their own buffers, the "console screen" and the "input screen".
// The console screen is where text printing goes.
// The input screen, used for text input, has a one-line prompt, and a caller-supplied 3-line text buffer.
// These functions switch the screen automatically (i.e. print functions switch to the console screen, and edit mode functions switch to the input screen).
// The console screen scrolls when text exceeds the bottom of the screen, and scrollback would be somewhat easy to program.
// 
// TODO:
//   Program line editing. Some primitive work on this is done, such as there being separate lcd_editMode_i and lcd_editMode_len variables.
//   Test code, then integrate into the laser engraver code.

/**
 * @brief Initializes the LCD. Should be called during startup.
 * 
 */
void lcd_init()
{
  lcd.init();
  lcd.backlight();
  lcd_clear();
  /*for (int i = 0; i < lcd_width*lcd_height) {
        lcd_screen[i] = '\0';
    }*/
}
/**
 * @brief Prints a message to the LCD, then moves cursor to next line.
 * 
 * @param text The message to be printed. (String is used for ease of conversion)
 */
void lcd_println(String text)
{
  lcd_print(text);
  lcd_lineBreak();
}
/**
 * @brief Prints a message to the LCD. '\n' is the only supported control character.
 * 
 * @param text The message to be printed. (String is used for ease of conversion)
 */
void lcd_print(String text)
{
  if (lcd_editMode) {
    lcd_editMode = false;
    lcd_repaint();
  }
  
  int len = text.length();
  char currentChar;
  for (int i = 0; i < len; i++)
  {
    currentChar = text.charAt(i);

    if (currentChar == '\n')
    {
      lcd_lineBreak();
      continue;
    }

    // Register the new character in lcd_screen:
    lcd_screen[(lcd_y + lcd_screen_wrap) % lcd_height][lcd_x] = currentChar;

    lcd.write(currentChar);
    lcd_x++;

    // If at right end of screen:
    if (lcd_x == lcd_width)
    {
      lcd_lineBreak();
    }
  }
}
/**
 * @brief lcd_print(), except without scrolling, switching screens, and more.
 * 
 */
void lcd_print_direct(char *text, uint8_t lcd_x, uint8_t lcd_y) {
  lcd.setCursor(lcd_x, lcd_y);
  
  char currentChar;
  for (int i = 0; ; i++)
  {
    currentChar = text[i];

    if (currentChar == '\n')
    {
      // NEWLINE:
      lcd_x = 0;
      lcd_y++;
      if (lcd_y == lcd_height)
      {
        break; // Out of screen
      } else {
        lcd.setCursor(lcd_x, lcd_y);
      }
      continue;
    }
    else if (currentChar == '\0')
    {
      break;
    }

    lcd.write(currentChar);
    lcd_x++;

    // If at right end of screen:
    if (lcd_x == lcd_width)
    {
      // NEWLINE:
      lcd_x = 0;
      lcd_y++;
      if (lcd_y == lcd_height)
      {
        break; // Out of screen
      } else {
        lcd.setCursor(lcd_x, lcd_y);
      }
    }
  }
}

/**
 * @brief Clears the console screen.
 * 
 */
void lcd_clear()
{
  if (!lcd_editMode) {
    lcd.clear();
  }
  
  lcd_x = 0;
  lcd_y = 0;
  
  // Clear the text buffer:
  for (uint8_t y = 0; y < lcd_height; y++) {
    for (uint8_t x = 0; x < lcd_width+1; x++)
    {
      lcd_screen[y][x] = '\0';
    }
  }
}
/**
 * @brief Moves to next LCD line. Resets cursor to beginning of line.
 * 
 */
void lcd_lineBreak()
{
  if (lcd_editMode) {
    lcd_editMode = false;
    lcd_repaint();
  }
  
  lcd_x = 0;
  lcd_y++;
  if (lcd_y == lcd_height)
  {
    // SOMEONE ANYONE HELP WE'RE OUT OF SCREEN 😱😱😱😱
    // I KNOW!! LCD_SCROLL TO THE RESCUE!!!!!!!!!1
    lcd_scroll();
  } else {
    lcd.setCursor(lcd_x, lcd_y);
  }
}
/**
 * @brief Scrolls the LCD down one line. For the console screen.
 * 
 */
void lcd_scroll()
{
  if (lcd_editMode) {
    lcd_editMode = false;
    lcd_repaint();
  }
  
  // Clear the (soon to be) last line:
  for (uint8_t x = 0; x < lcd_width; x++)
  {
    lcd_screen[lcd_screen_wrap][x] = '\0';
  }
  // Scroll:
  lcd_screen_wrap = (lcd_screen_wrap + 1) % lcd_height;
  lcd_y--;

  // Update LCD:
  lcd_repaint();
}

/**
 * @brief Repaints the screen and resets the cursor appearance and position.
 * 
 */
void lcd_repaint()
{
  lcd.clear(); // LATER: clear manually with spaces as you print the content of a screen.
  if (lcd_editMode)
  {
    lcd.print(lcd_editMode_prompt);
    
    /*if (lcd_editMode_maxLen != 0) {
		  uint8_t ixw = 0; // i times lcd_width
		  char tempChar; // Certain characters will need to be temporarily replaced with '\0' while printing lines.
		  // Print the lines of input, stopping when there are no more lines or a line is empty:
		  for (uint8_t i = 0; (i <= lcd_height-1) && (lcd_editMode_input[ixw] != '\0'); i++) {
		    ixw = i*lcd_width;
		    
		    lcd.setCursor(0, i + 1);
		    
		    if (ixw+lcd_width > lcd_editMode_maxLen) { // If this line is the last, don't add the null char.
		      lcd.print(lcd_editMode_input[ixw]);
		    } else {
		      tempChar = lcd_editMode_input[ixw];
		      lcd_editMode_input[ixw] = '\0';
		      
		      lcd.print(lcd_editMode_input[ixw]);
		      
		      lcd_editMode_input[ixw] = tempChar;
		    }
		  }
    }*/
    // Phew that ^ was complicated. And didn't work. So here's a much easier way that is slightly slower:
    lcd_print_direct(lcd_editMode_input, 0, 1);
    
    // Set the position of the cursor:
    lcd_editMode_posCur();
    
    lcd.cursor();
    lcd.blink();
  } else {
    // LATER: it's inefficient to have these calls in the repaint function, they could be called redundantly.
    lcd.noCursor();
    lcd.noBlink();
    
    for (uint8_t y = 0; y < lcd_height; y++)
    {
      lcd.setCursor(0, y);
      lcd.print(lcd_screen[(y + lcd_screen_wrap) % lcd_height]);
    }
    
    lcd.setCursor(lcd_x, lcd_y);
  }
}

/**
 * @brief Prepares edit mode.
 * Set the lcd_editMode_prompt variable to the prompt string.
 * 
 * @param input The input buffer to use. The length MUST BE THIS LENGTH OR SHORTER: lcd_width*(lcd_height-1)+1-1. (One row less, for the prompt. One element more and one element less, for '\0' and physical space for the cursor). The input buffer is allowed to already have text in it, but all other characters must be '\0'.
 * @param maxLen Then maximum length of supplied input buffer, so 1 less than the buffer's length due to '\0'. See @param input
 * 
 */
void lcd_editMode_init(char *input, int maxLen)
{
  lcd_editMode_input = input;
  lcd_editMode_maxLen = maxLen;
  
  lcd_editMode_len = 0;
  lcd_editMode_i = 0;
}

/**
 * @brief Switch to the console screen:
 * The text that was previously on the screen will be displayed again.
 * Use the input buffer supplied to lcd_editMode_on(), which contains the inputted text.
 * 
 * 
 */
int lcd_editMode_off()
{
  lcd_editMode = false;
  lcd_repaint();
  
  return lcd_editMode_len;
}

bool lcd_editMode_type(char c)
{
  if (!lcd_editMode) {
    lcd_editMode = true;
    lcd_repaint();
  }
  
  if (lcd_editMode_len == lcd_editMode_maxLen) {
    return true;
  }
  
  // (Part of text editing) If the caret is not at the end of the input, shift characters 1 to the right:
  if (lcd_editMode_i != lcd_editMode_len) {
    for (int i = lcd_editMode_len - 1; i >= lcd_editMode_i; i--) {
      lcd_editMode_input[i+1] = lcd_editMode_input[i];
    }
  }
  
  lcd_editMode_input[lcd_editMode_i] = c;
  lcd_editMode_len++;
  lcd_editMode_i++;
  
  if (lcd_editMode_i != lcd_editMode_len) {
    lcd_repaint();
  } else {
    lcd.write(c);
    
    // Reposition the cursor if it's at the end of the line:
    if (lcd_editMode_i % lcd_width == 0) {
      // Set the position of the cursor:
      lcd_editMode_posCur();
    }
  }
  
  return false;
}

/**
 * @brief Backspace. Returns true if the cursor is all the way to the left, false otherwise.
 * 
 */
bool lcd_editMode_backsp()
{
  // The usual `if (!lcd_editMode)` part is somewhere below. One advantage is that this way prevents lcd_redraw() from being called twice, which would happen if lcd_editMode==false and the caret is not at the end.
  
  if (lcd_editMode_i == 0) {
    return true;
  }
  
  // Delete the character:
  lcd_editMode_len--;
  lcd_editMode_i--;
  lcd_editMode_input[lcd_editMode_i] = '\0';
  
  // (Part of text editing) If the caret is not at the end of the input, shift characters 1 to the left:
  if (lcd_editMode_i != lcd_editMode_len) {
    // TODO: do the shifting.
    for (int i = lcd_editMode_i; i < lcd_editMode_len + 1; i++) {
      lcd_editMode_input[i] = lcd_editMode_input[i+1];
    }
    lcd_editMode = true; // In case it wasn't (which is possible).
    lcd_repaint();
  } else {
    // Clear the backspace-d character:
    if (!lcd_editMode) {
      lcd_editMode = true;
      lcd_repaint();
    } else {
      lcd_editMode_posCur();
      lcd.write(' ');
      lcd_editMode_posCur();
    }
  }
  
  return false;
}

/**
 * @brief Reposition the LCD cursor in editMode. Returns true if the cursor cannot be positioned there.
 */
bool lcd_editMode_setCursor(int index)
{
  if (!lcd_editMode) {
    lcd_editMode = true;
    lcd_repaint();
  }
  
  if (index < 0 || index > lcd_editMode_len) {
    return true;
  }
  
  lcd_editMode_i = index;
  lcd_editMode_posCur();
  return false;
}

/**
 * @brief Reposition the LCD cursor in editMode.
 */
void lcd_editMode_posCur()
{
  lcd.setCursor(lcd_editMode_i % lcd_width, (lcd_editMode_i / lcd_width) + 1);
}
