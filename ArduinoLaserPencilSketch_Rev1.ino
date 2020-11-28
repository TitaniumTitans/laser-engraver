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

#include <PS2Keyboard.h>         //Keyboard
#include <SPI.h>                 //DACs
//#include "MegunoLink.h"          //For testing only
#include "SoftwareSerial.h"      //Serial Monitor
PS2Keyboard keyboard;
 
//Declare User Values
int KeyboardInterStrokeDelay = 200;      //milliseconds
int BeamGrossVerticalOffset = 0;         //out of 1023   Coarse vertical offset
int BeamGrossHorizOffset = 0;            //out of 1023   Coarse horiz offset
int BurnDelayDivisor = 2;                //1023/2 = 512ms max
int CharSizeDivisor = 90;                //1023/90 = 11 max
int CharSpaceDivisor = 90;               //1023/90 = 11 max
int yOffsetDivisor = 16;                 //1023/16 = 64  max Fine vertical offset
int TimeForStageToMoveOneCharacter = 1500;   //set at 1.5 seconds

//Declare General Variables
long x;
int y;
int z;
int i = 0;
byte ii;
double j;
int burn;
byte LetterIndex;        
int CharSizeValue;
int CharSpaceValue;
int yOffsetValue;
int BurnDelayValue = 0;
int pwmValue;    //sent to TTL pin of laser controller
int Kerning;
byte Letter;
byte len;
byte NumberOfLettersToDraw;
byte NumberOfChars;
int VoltageToDAC;
byte HighByte;  //for serial DAC
byte LowByte;   //for serial DAC

//Declare Variables for X-Axis Stepper 
int NumberofSteps;
int StepCntRst;
long StepperCounter = 0;
int StepCounter;
volatile byte TriggerJog = 0;
char FileName[40];
byte time;
char time1;

//Declare Pins
const int XDACSelectPin = 14;
const int YDACSelectPin = 15;
const int DACLatch = 53;
const byte BurnDelayPin = A8;
const byte CharSpacePin = A9;
const byte CharSizePin = A11;
const byte yOffsetPin = A12;        //move beam vertically
const byte pwmOutPin = 23; // (Laser enable pin)
const byte StopButton = 10;
const byte BurnButton = 9;
const byte JogLeftButtonPin = 2;
const byte JogRightButtonPin = 19;
const byte StepperDirPin = 31;
const byte StepperEnablePin = 32;
const byte StepperPulseOutputPin = 33;
const byte BeeperPin = 25;
const byte RightLimitSw = 21;
const byte LeftLimitSw = 20;
const byte HomeButtonPin = 11;
const byte SafetySwitchPin = 48;
//const byte ExhaustFanPin = 49;

//Data out pin = 51, MOSI Pin,    SPI routine specifies it    
//Clock out pin = 52, SCK Pin,    SPI routine specifies it 


//keyboard stuff
//***********************
const int DataPin = 8;  
const int IRQpin =  3;   //if the #include PS2Keyboard library is used, pin 3 must be used for the PS2 keyboard interrupt
char c;                  //or it will not work for all PS-2 keyboards.  Pin 3 = interrupt 1.                       

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

//                                                                                              Key    ASCII                                                              
byte CharacterArray[96][46]={
9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //SPACE  32  
3,1,3,3,3,4,3,5,3,6,3,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //!      33
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //"      34  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //#      35
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //$      36  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //%      37  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //&      38  
3,6,3,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //'      39 
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //(      40  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //)      41  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //*      42
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //+      43  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //,      44
2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //-      45  
3,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //.      46
1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,3,1,2,1,2,3,3,3,4,3,2,5,4,5,9,9,0,0,0,0,  // '/'   47  FACE  
1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,2,3,3,4,4,5,9,9,0,0,0,0,0,0,  //0      48
2,1,3,1,4,1,3,2,3,3,3,4,3,5,3,6,3,7,2,6,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //1      49
1,6,2,7,3,7,4,7,5,6,5,5,4,4,3,4,2,4,1,3,1,2,1,1,2,1,3,1,4,1,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,  //2      50 
1,6,2,7,3,7,4,7,5,6,5,5,3,4,4,4,5,3,5,2,4,1,3,1,2,1,1,2,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //3      51
4,1,4,2,4,3,4,4,4,5,4,6,4,7,3,6,2,5,1,4,1,3,2,3,3,3,5,3,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //4      52
1,2,2,1,3,1,4,1,5,2,5,3,5,4,4,5,3,5,2,5,1,5,1,6,1,7,2,7,3,7,4,7,5,7,9,9,0,0,0,0,0,0,0,0,0,0,  //5      53
2,4,3,4,4,4,5,3,5,2,4,1,3,1,2,1,1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,9,9,0,0,0,0,0,0,0,0,0,0,  //6      54
1,7,2,7,3,7,4,7,5,7,5,6,4,5,3,4,2,3,2,2,2,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //7      55
1,2,1,3,2,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,4,4,3,4,5,3,5,2,4,1,3,1,2,1,9,9,0,0,0,0,0,0,0,0,0,0,  //8      56
1,2,2,1,3,1,4,1,5,2,5,3,5,4,5,5,5,6,4,7,3,7,2,7,1,6,1,5,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,0,0,  //9      57  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //:      58  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //;      59  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //<      60  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //=      61  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //>      62  
1,6,2,7,3,7,4,7,5,6,5,5,4,4,3,3,3,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //?      63
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //@      64
1,1,1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,5,1,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,  //A      65  
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,4,4,5,3,5,2,4,1,3,1,2,1,2,4,3,4,9,9,0,0,0,0,  //B      66
5,2,4,1,3,1,2,1,1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //C      67
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,9,9,0,0,0,0,0,0,0,0,  //D      68
5,1,4,1,3,1,2,1,1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,7,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,  //E      69
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,3,7,4,7,5,7,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //F      70
5,6,4,7,3,7,2,7,1,6,1,5,1,4,1,3,1,2,2,1,3,1,4,1,5,2,5,3,5,4,4,4,3,4,9,9,0,0,0,0,0,0,0,0,0,0,  //G      71
1,7,1,6,1,5,1,4,1,3,1,2,1,1,2,4,3,4,4,4,5,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,  //H      72
2,1,3,1,4,1,3,2,3,3,3,4,3,5,3,6,2,7,3,7,4,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //I      73
1,3,1,2,2,1,3,1,4,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //J      74
1,1,1,2,1,3,1,4,1,5,1,6,1,7,5,7,4,6,3,5,2,4,3,3,4,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //K      75
1,7,1,6,1,5,1,4,1,3,1,2,1,1,2,1,3,1,4,1,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //L      76
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,6,3,5,4,6,5,7,5,6,5,5,5,4,5,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,  //M      77
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,5,3,4,4,3,5,7,5,6,5,5,5,4,5,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,  //N      78
1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,  //O      79
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,4,4,3,4,2,4,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //P      80
1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,3,3,4,2,9,9,0,0,0,0,0,0,0,0,  //Q      81
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,4,4,3,4,2,4,4,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,  //R      82
1,2,2,1,3,1,4,1,5,2,5,3,4,4,3,4,2,4,1,5,1,6,2,7,3,7,4,7,5,6,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //S      83
1,7,2,7,3,7,4,7,5,7,3,6,3,5,3,4,3,3,3,2,3,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //T      84
1,7,1,6,1,5,1,4,1,3,1,2,2,1,3,1,4,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //U      85
1,7,1,6,1,5,1,4,1,3,2,2,3,1,4,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //V      86
1,7,1,6,1,5,1,4,1,3,1,2,1,1,2,2,3,3,4,2,5,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,  //W      87
1,7,1,6,2,5,3,4,4,5,5,6,5,7,1,1,1,2,2,3,4,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //X      88
1,7,1,6,1,5,2,4,3,3,4,4,5,5,5,6,5,7,3,2,3,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //Y      89
1,7,2,7,3,7,4,7,5,7,5,6,4,5,3,4,2,3,1,2,1,1,2,1,3,1,4,1,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //Z      90  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //[      91  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //\      92  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //]      93
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //       94
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //_      95  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //       96
1,1,1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,5,1,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,  //a=A    97  
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,4,4,5,3,5,2,4,1,3,1,2,1,2,4,3,4,9,9,0,0,0,0,  //b=B    98  
5,2,4,1,3,1,2,1,1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //c=C    99
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,9,9,0,0,0,0,0,0,0,0,  //d=D    100
5,1,4,1,3,1,2,1,1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,7,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,  //e=E    101
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,3,7,4,7,5,7,2,4,3,4,4,4,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //f=F    102
5,6,4,7,3,7,2,7,1,6,1,5,1,4,1,3,1,2,2,1,3,1,4,1,5,2,5,3,5,4,4,4,3,4,9,9,0,0,0,0,0,0,0,0,0,0,  //g=G    103
1,7,1,6,1,5,1,4,1,3,1,2,1,1,2,4,3,4,4,4,5,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,  //h=H    104
2,1,3,1,4,1,3,2,3,3,3,4,3,5,3,6,2,7,3,7,4,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //i=I    105
1,3,1,2,2,1,3,1,4,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //j=J    106
1,1,1,2,1,3,1,4,1,5,1,6,1,7,5,7,4,6,3,5,2,4,3,3,4,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //k=K    107
1,7,1,6,1,5,1,4,1,3,1,2,1,1,2,1,3,1,4,1,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //l=L    108
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,6,3,5,4,6,5,7,5,6,5,5,5,4,5,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,  //m=M    109
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,5,3,4,4,3,5,7,5,6,5,5,5,4,5,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,  //n=N    110
1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,  //o=O    111
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,4,4,3,4,2,4,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //p=P    112
1,2,1,3,1,4,1,5,1,6,2,7,3,7,4,7,5,6,5,5,5,4,5,3,5,2,4,1,3,1,2,1,3,3,4,2,9,9,0,0,0,0,0,0,0,0,  //q=Q    113
1,1,1,2,1,3,1,4,1,5,1,6,1,7,2,7,3,7,4,7,5,6,5,5,4,4,3,4,2,4,4,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,  //r=R    114
1,2,2,1,3,1,4,1,5,2,5,3,4,4,3,4,2,4,1,5,1,6,2,7,3,7,4,7,5,6,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //s=S    115
1,7,2,7,3,7,4,7,5,7,3,6,3,5,3,4,3,3,3,2,3,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //t=T    116
1,7,1,6,1,5,1,4,1,3,1,2,2,1,3,1,4,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //u=U    117
1,7,1,6,1,5,1,4,1,3,2,2,3,1,4,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //v=V    118
1,7,1,6,1,5,1,4,1,3,1,2,1,1,2,2,3,3,4,2,5,1,5,2,5,3,5,4,5,5,5,6,5,7,9,9,0,0,0,0,0,0,0,0,0,0,  //w=W    119
1,7,1,6,2,5,3,4,4,5,5,6,5,7,1,1,1,2,2,3,4,3,5,2,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //x=X    120
1,7,1,6,1,5,2,4,3,3,4,4,5,5,5,6,5,7,3,2,3,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //y=Y    121
1,7,2,7,3,7,4,7,5,7,5,6,4,5,3,4,2,3,1,2,1,1,2,1,3,1,4,1,5,1,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //z=Z    122
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //{      123  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //|      124  
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //}      125
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //~      126
0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //del    127 

byte MessageTable[80] = {0};  //declare message table

 
//*********************************************************************************************************
void setup()
{

pinMode(XDACSelectPin, OUTPUT);
pinMode(YDACSelectPin, OUTPUT);
pinMode(DACLatch, OUTPUT);
delay(1);
digitalWrite(XDACSelectPin, HIGH);   //neg CS for SPI writing in data
digitalWrite(YDACSelectPin, HIGH);   //neg CS for SPI writing in data
digitalWrite(DACLatch, HIGH);        //neg pulse to latch data to DAC outputs
delay(10);
pinMode(CharSizePin, INPUT);
pinMode(yOffsetPin, INPUT);
pinMode(BurnDelayPin, INPUT);
pinMode(SparePotPin, INPUT);
pinMode(CharSpacePin, INPUT);
pinMode(StopButton, INPUT);
pinMode(BurnButton, INPUT);
pinMode(RightLimitSw, INPUT);
pinMode(LeftLimitSw, INPUT);
pinMode(pwmOutPin, OUTPUT);
pinMode(JogRightButtonPin, INPUT);
pinMode(JogLeftButtonPin, INPUT);
pinMode(BeeperPin, OUTPUT);
pinMode(HomeButtonPin, INPUT);
pinMode(StepperDirPin, OUTPUT);
pinMode(StepperEnablePin, OUTPUT);
pinMode(StepperPulseOutputPin, OUTPUT);
pinMode(SafetySwitchPin, INPUT);
//pinMode(ExhaustFanPin, OUTPUT);

//Interrupt calls
//****************
//Available interrupt pins: 2,3,21,20,19,18 (int 0,1,2,3,4,5)
//attachInterrupt(Keyboard); //Pin  3 = interrupt 1
//No other interrupts are used due to noise from stepper

SPI.setClockDivider(SPI_CLOCK_DIV128);
SPI.begin();
SPI.setClockDivider(SPI_CLOCK_DIV128);

//keyboard and LCD stuff
 keyboard.begin(DataPin, IRQpin);
 
 delay(100);
 analogWrite(pwmOutPin, 0);  //pre-shuts laser off, double safety
  
 //The LCD is automatically assigned Pin1(Tx) when Serial.begin() is used.
 //The Seetron LCD requires an INVERTED 9600 baud serial signal from the Arduino
 Serial.begin(9600); //for Serial Monitor in the IDE & LCD, not the keyboard
 Serial.write(12);   //clear LCD screen
 Serial.write(4);    //shut off LCD cursor
 
 ResetDACs();
 Beep(2);            //system is booted up and ready
}

//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
void loop(){
  
 Start();
 
}  
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************


//***********************************************************************************
//      FUNCTIONS, EXCEPT FOR X-AXIS STEPPER STUFF WHICH IS AT THE BOTTOM 
//***********************************************************************************

void Start(){
  
Serial.write(12);         //clear LCD screen  
Serial.println("START:TYPE 1,2 OR 3");
Serial.println("1.NEW MESSAGE");  
Serial.println("2.REPEAT BURN");
Serial.println("3.CHARACTER ADJUST"); 

Loop1:
if ((digitalRead(JogLeftButtonPin)) == 0) ManualJogLeftFunc();
if ((digitalRead(JogRightButtonPin)) == 0) ManualJogRightFunc();
if ((digitalRead(HomeButtonPin)) == 0) ManualHome();

if (keyboard.available()){
    c = keyboard.read();
    if (c == 49)         // 1.NEW MESSAGE
      {
       Beep(1);
       ReadPots();
       MessageFormatting();
       TypeNewMessage();
       BurnMessageSequence();
      }
           
    if (c == 50)         // 2. REPEAT BURN
      {
       Serial.write(12);     
       Beep(1);
       BurnMessageSequence();
      }
      
    if (c == 51)        // 3. CHARACTER ADJUST
      {
       Serial.write(12); 
       Beep(1);    
       AdjCharSizes1();
      }
   
    if ((c <=48) || (c >= 52))  //error check for incorrect keyboard choices
      {
      Beep(2); 
      goto Loop1;
      }
    }
    goto Loop1;    
 }

//**********************************************************
void MessageFormatting(){

 int cnt;
 byte ch;
 byte storedChar;
 
 Serial.write(12);   //clear LCD screen 
 Serial.println("-MESSAGE PARAMETERS"); 
 Serial.println(" 40 CHAR MAXIMUM = "); 
 Serial.println(" 2 LINES ON LCD");
 Serial.print(" <ENTER>");
 WaitForEnterKey();                            
 Beep(1);
 
 Serial.write(12);   //clear LCD screen 
 Serial.println("-USE BACKSPACE FOR");
 Serial.println(" CORRECTIONS");
 Serial.print("-DO <ENTER> NOW TO");
 Serial.write(16);
 Serial.write(124);   //move cursor to line 4
 Serial.print(" OPEN TYPING WINDOW");
 WaitForEnterKey();                            
 Beep(1);
 }
  
 //************************************************************ 
 void TypeNewMessage(){
   
 int cnt=0;
 int ch;
 char ch1;
 byte KeyFlag = 0;
 Serial.write(12);   //clear LCD screen 
 Serial.write(6);   //put blinking block cursor on LCD
  
 MessLoop:
 if (keyboard.available())
    {
    ch = keyboard.read();
    if (ch == 13) goto NewMessDone;   // a CR(ENTER) will terminate the table
    if (ch == 127)       //backspace or delete
       {
       if (cnt == 0) goto MessLoop;  
       Serial.write(8);  //ASCII backspace
       Beep(1);
       cnt = cnt - 1;
       goto MessLoop;
       }
       
    //Check for characters NOT IMPLEMENTED YET 
    //by checking for 0,0 as the first dot
    //Also let <ENTER> and DEL go through
     if ((CharacterArray[ch-32][1] == 0) && (ch != 13) && (ch != 127))
     {
     Beep(3);
     goto MessLoop;
     }
          
    Serial.print(char(ch));  
    Beep(1);
    
    delay(KeyboardInterStrokeDelay); //typ: 200ms, the keyboard seems to lockup or 
                                     //something at odd times so I added this delay. 
    MessageTable[cnt] = ch;
    cnt++;
  } 
    
goto MessLoop;
NewMessDone:
Serial.write(4);   //turn off LCD cursor
Beep(2);
delay(300);
NumberOfChars = cnt; 
 }


//*********************************************************

void BurnMessageSequence(){
  
 byte ch; 
 
 Serial.write(12);   //clear LCD screen
 Serial.println("-STANDBY: WAIT FOR");
 Serial.println(" STAGE TO STOP");
 Home();           //move stage to Home(right) position //******************************************************************************************
 Beep(1);
 Serial.write(12);   //clear LCD screen
 Serial.println("-MOUNT PENCIL"); 
 Serial.println("-PUT ON GOOGLES"); 
 Serial.print("-PUSH BURN BUTTON OR");
 Serial.write(16);
 Serial.write(124);   //move cursor to line 4
 Serial.print(" HOLD STOP TO QUIT");
 do
 {}
 while (digitalRead(BurnButton) == 1);
 Beep(1); 
 Serial.write(12);
 
//*******************************
//turn on laser, start burning
//letters and advancing stage
//*******************************
 BurnMessage(); 
    
 Beep(3);
 Serial.write(12);  //clear screen 
 Serial.println("-BURN CYCLE DONE");
 Serial.println("-REMOVE PENCIL");
 Serial.println("<ENTER>");
 WaitForEnterKey();
 Beep(1);
 Start();
}

//********************************************************
void BurnMessage(){
  
  byte bb;
  byte y;
  byte z;
  char aa;
  byte AscCode;
  byte ArrayPosition;
  //LetterIndex is global
  //digitalWrite(ExhaustFanPin, HIGH);
  delay(500);
  y=0;
   
 //Step through MessageTable printing one letter at a time
  for (bb=0;bb<=(NumberOfChars-1);bb++)
    {
    AscCode = MessageTable[bb];             //Message table has a string of ASCII numbers to print, A = 65, a = 97 but it prints A
    if (AscCode == 13) goto BurnMessDone;   //13 = enter key
    //AscLetter = AscCode;
    Serial.print(char(AscCode));
    ArrayPosition = (AscCode - 32);    //offset for CharacterArray (ASCII chars start at 32, CharacterArray starts at 0)
    LetterIndex = ArrayPosition;       //vertical index for CharacterArray
    BurnOneLetter();                   //writing each letter as it is called
    delay(200);
    }
   
BurnMessDone:
delay(500);
ResetDACs();
//digitalWrite(ExhaustFanPin, LOW);
}
  
//************************************************** 
void BurnOneLetter(){
  
  int xArrayVal;
  int yArrayVal;
  int i = 0;
  
TableLoop:
if (digitalRead(StopButton) == LOW)         //check for Stop button
  {
  Beep(3); 
  digitalWrite(pwmOutPin, LOW);  //turn laser off, if not off
  Serial.write(12);  //clear screen 
  Serial.println("BURN HALTED:");
  Serial.print("<ENTER>");
  WaitForEnterKey();
  Beep(1);
  //digitalWrite(ExhaustFanPin, LOW);
  Start();
  }
 xArrayVal = CharacterArray[LetterIndex][i];
 yArrayVal = CharacterArray[LetterIndex][i+1];
 if ((xArrayVal == 9) || (yArrayVal == 9)) goto BurnOneDone;      //"9,9" is the 5x7 "end of message" char 
 
//write data to DACs
//***************************************************
//SERIAL  X DAC 
digitalWrite(XDACSelectPin, LOW);       //X chip select
VoltageToDAC = (((xArrayVal*CharSizeValue)*6) + BeamGrossHorizOffset);   //0 to 1023(measured=2.03mv/count)  
ConvertToTwoBytes();
SPI.transfer(HighByte);
SPI.transfer(LowByte);
digitalWrite(XDACSelectPin, HIGH);

//SERIAL Y DAC 
//You can dynamically adjust vertical position of char on pencil, each time a new char is burned
yOffsetValue = analogRead(yOffsetPin);   //0-1023   1024/16 = 64
yOffsetValue = int(yOffsetValue/yOffsetDivisor);     //need more offset to get beam on pencil, vertically

digitalWrite(YDACSelectPin, LOW);      //Y chip select
VoltageToDAC = ((((yArrayVal*CharSizeValue) + yOffsetValue)*6) + BeamGrossVerticalOffset);
ConvertToTwoBytes(); 
SPI.transfer(HighByte);
SPI.transfer(LowByte);
digitalWrite(YDACSelectPin, HIGH);

//Latch X & Y values to DAC outputs
digitalWrite(DACLatch, LOW);   //10us negative pulse to chips to latch in write
delayMicroseconds(10);
digitalWrite(DACLatch, HIGH);  

if (LetterIndex == 0) goto NoBurn;   //skip one space 

//BURNING ONE DOT ON THE PENCIL
//**************************************************
digitalWrite(pwmOutPin, HIGH);        //LASER ON
delay(BurnDelayValue);                //typ: 100-500 ms. 
digitalWrite(pwmOutPin, LOW);         //LASER OFF
//**************************************************

NoBurn:
i = i + 2;   //advance index two digits, to next x,y point
goto TableLoop;

BurnOneDone:  
i = 0;

MoveStageLeftOneLetterDuringBurn();       //the number of steps is adjusted earlier by the Char Space knob
delay(TimeForStageToMoveOneCharacter);    //Allow time for stage to move over one letter. Increase this 
                                          //delay if the characters are much bigger. Typ for pencil: 1.5 sec
                                          
}

//**************************************************
 void ConvertToTwoBytes()
{
  int DecNum = VoltageToDAC;   //0 to 1023
  
  //HighByte
  //********************************
  HighByte = 112;   //reqd upper nib for DAC = bits 15,14,13,12 = (0111) = 112 dec
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
    LowByte = (DecNum * 4);  //LSB and LSB+1 are both zeros, therefore bit 0 of
    }                        //the Voltage starts at Bit 2, which is a 4 dec 
}

//**************************************************************
void AdjCharSizes1(){

 Beep(1);
 //parameter adjustment screen
 //*****************************
 Serial.write(12);   //clear LCD screen
 Serial.println("ADJUST AS NEEDED:");
 c=0;
 do{
 //display Burn Time so it can be adjusted
 BurnDelayValue = analogRead(BurnDelayPin);   //0-1023   pwm = 0-255   Burn =  0 to 150ms
 BurnDelayValue = int(BurnDelayValue/BurnDelayDivisor);      //512ms max
 Serial.write(16);  //move cursor...
 Serial.write(84);  //to Line 2
 Serial.print("-BURN TIME:");
 Serial.print(BurnDelayValue);
 Serial.println("ms.  ");
 
 //display Character Size so it can be adjusted   
 CharSizeValue = analogRead(CharSizePin);   //0-1023   1024/100 = 10
 CharSizeValue = int(CharSizeValue/CharSizeDivisor);     //
 Serial.print("-CHAR SIZE:");
 Serial.print(CharSizeValue);
 Serial.println("/11 ");
 Serial.print("<ENTER>");
 //WaitForEnterKey();   //cannot use this function because it slows down the refresh rate too much 
 if (keyboard.available())
      {  
      c = keyboard.read();
      }
 }
 while (c != 13);  
 Beep(1);
 delay(100);

//more parameter adjustment 
//**************************
 Serial.write(12);   //clear LCD screen
 Serial.println("ADJUST AS NEEDED:");
 c=0;
 do
 {
 //display Character Spacing so it can be adjusted   
 CharSpaceValue = analogRead(CharSpacePin);   //0-1023   1024/100 = 10
 CharSpaceValue = int(CharSpaceValue/CharSpaceDivisor);     //
 Serial.write(16);  //move cursor...
 Serial.write(84);  //to Line 3
 Serial.print("-CHAR SPACE:");
 Serial.print(CharSpaceValue);
 Serial.println("/11 "); 
 
 //display Beam Vertical position so it can be adjusted  
 yOffsetValue = analogRead(yOffsetPin);   //0-1023   1024/16 = 64
 yOffsetValue = int(yOffsetValue/yOffsetDivisor);     //need more offset to get beam on pencil, vertically
 Serial.print("-BEAM VERT:");
 Serial.print(yOffsetValue);
 Serial.println("/64 ");
 Serial.print("<ENTER>");
 
 //WaitForEnterKey();    //cannot use this function because it slows down the refresh rate too much
 if (keyboard.available())
      {  
      c = keyboard.read();
      }
  }
 while (c != 13);
 
 Beep(1);
 Start();
 } 
 
//*********************************************************
void ResetDACs(){

 VoltageToDAC = 0;
 ConvertToTwoBytes();

 digitalWrite(XDACSelectPin, LOW);       //X chip select
 SPI.transfer(HighByte);
 SPI.transfer(LowByte);
 digitalWrite(XDACSelectPin, HIGH);

 digitalWrite(YDACSelectPin, LOW);      //Y chip select
 SPI.transfer(HighByte);
 SPI.transfer(LowByte);
 digitalWrite(YDACSelectPin, HIGH);

 //Latch X & Y values to DAC outputs
 digitalWrite(DACLatch, LOW);   //10us negative pulse to chips to latch in write
 delayMicroseconds(10);
 digitalWrite(DACLatch, HIGH);    
 }

//********************************************************
void Beep(int y){
for (x=1;x<=y;x++)
  {
digitalWrite(BeeperPin, HIGH);
delay(100);
digitalWrite(BeeperPin, LOW);
delay(100);
  }
}

//*********************************************************
void WaitForEnterKey(){
    
    do
    {
    if (keyboard.available())
      {  
      c = keyboard.read();
      }
    }
    while(c != 13);   // a 13 (CR or ENTER)
    c = 0;
    delay(300);   
}

//**********************************************************
 void ReadPots(){
  
 BurnDelayValue = analogRead(BurnDelayPin);               //0-1023   
 BurnDelayValue = int(BurnDelayValue/BurnDelayDivisor);   //typ 1023/2 = typ 512ms max burn time

 CharSizeValue = analogRead(CharSizePin);                 //typ 1023/90 = 11
 CharSizeValue = int(CharSizeValue/CharSizeDivisor);      //

 CharSpaceValue = analogRead(CharSpacePin);               //typ 1023/90 = 11
 CharSpaceValue = int(CharSpaceValue/CharSpaceDivisor);    
 
 yOffsetValue = analogRead(yOffsetPin);                   //typ 1023/16 = 64
 yOffsetValue = int(yOffsetValue/yOffsetDivisor);         //may need more offset to get beam on pencil, vertically
                                                          //refer to Gross Vert Offset at start of program
 } 
 
//********************************************************************************************************************
//                               FUNCTIONS FOR X-AXIS STAGE STEPPER AND LIMIT SWITCHES
//********************************************************************************************************************

//******************************************
void MoveStageLeftOneLetterDuringBurn()    //jog one space worth of jogs, approx 1100
{
  CharSpaceValue = ((analogRead(CharSpacePin)) * 2);   //0 to 1023 

  SetLeftJogDir();
  for (z=0;z<=CharSpaceValue;z++)    //1100 = Char Space Pot set at 50% = 1023/2 = 512, so multiply x2
    {
    JogLeftDuringBurn();
    } 
  delay(100);
 }
  
//********************************************
void JogLeftDuringBurn(){    //while testing for left limit switch in case there are too many letters,
                             //or they are too large or spacing between them is too much   
   SetLeftJogDir();
   Jog1();
   if (digitalRead(LeftLimitSw) == LOW)  //left limit switch is tripped
     {
      digitalWrite(pwmOutPin, LOW);      //LASER OFF, if ON during burning  
      Beep(6); 
      Serial.write(12);  //clear LCD 
      Serial.println("ERROR: STANDBY");
      Serial.print("BURN STOPPED");
      y = 6000;   //number of backward steps to take to show burn stopped
      SetRightJogDir();
      for (x=0;x<=y;x++)
        {
        Jog1();
        } 
      Beep(3);
      //ERROR message; 
      Serial.write(12);  //clear LCD 
      Serial.println("-STAGE MOVED TOO");
      Serial.println(" FAR LEFT");
      Serial.println("-SHORTEN MESSAGE OR");
      Serial.write(16);  //move cursor...
      Serial.write(124);  //to Line 4
      Serial.print("<ENTER>");
      WaitForEnterKey();
      Beep(1);
      Serial.write(12);  //clear LCD 
      Serial.println("-ADJUST CHAR SIZE");
      Serial.println(" OR SPACING");
      Serial.println("-START AGAIN");
      Serial.print("<ENTER>");
      WaitForEnterKey();
      Beep(1);
      Start();
     }
  } 

//********************************************
void Home(){                 
  
  SetRightJogDir();
  do
  {
   Jog1();
  }
  while(digitalRead(RightLimitSw) == HIGH); 
  y = 1000;   //number of backward steps to move stage off limit sw
  SetLeftJogDir();
  for (x=0;x<=y;x++)
    {
    Jog1();
    } 
  Beep(3);
}

//********************************************
void ManualHome(){
  
  SetRightJogDir();
  do
    {
    Jog1();
    if (digitalRead(RightLimitSw) == LOW)
      {
      y = 1000;   //number of backward steps to move stage off limit sw
      SetLeftJogDir();
      for (x=0;x<=y;x++)
       {
       Jog1();
       } 
      Beep(3);
      do
      {}
      while(digitalRead(HomeButtonPin) == LOW);  //button lockout loop
      Start();
      }
    }   
  while(digitalRead(RightLimitSw) == HIGH);
}

//********************************************
void ManualJogLeftFunc(){
  
  SetLeftJogDir();
  do
   {
   Jog1();
   if (digitalRead(LeftLimitSw) == 0)
     {
     //ExceedLeftLimitMessageAndBackoff
      Serial.write(12);  //clear LCD 
      Serial.println("ERROR:");
      Serial.println("-STAGE MOVED TOO FAR");
      Serial.write(16);  //move cursor...
      Serial.write(104);  //to Line 3
      Serial.println(" LEFT, REVERSE"); 
      Serial.println(" DIRECTION"); 
      y = 800;   //number of backward steps to move stage off limit sw
      SetRightJogDir();
      for (x=0;x<=y;x++)
       {
       Jog1();
       } 
      Beep(3);
      do
      {}
      while(digitalRead(JogLeftButtonPin) == LOW);  //button lockout loop 
      } 
   }
   while(digitalRead(JogLeftButtonPin) == LOW);
  }

//*******************************************
void ManualJogRightFunc(){
  
  SetRightJogDir();
  do
    {
    Jog1();
    if (digitalRead(RightLimitSw) == 0)
      {
     //ExceedRightLimitMessageAndBackoff
      Serial.write(12);  //clear LCD 
      Serial.println("ERROR:");
      Serial.println("-STAGE MOVED TOO FAR");
      Serial.write(16);  //move cursor...
      Serial.write(104);  //to Line 3
      Serial.println(" RIGHT, REVERSE"); 
      Serial.println(" DIRECTION "); 
      y = 800;   //number of backward steps to move stage off limit sw
      SetLeftJogDir();
      for (x=0;x<=y;x++)
       {
       Jog1();
       } 
      Beep(3);
      do
      {}
      while(digitalRead(JogRightButtonPin) == LOW);   //button lockout loop
      } 
    }
  while(digitalRead(JogRightButtonPin) == LOW);
}

//********************************************
void Jog1(){
  
  int cnt;
  for (cnt=1;cnt<=50;cnt++) digitalWrite(StepperPulseOutputPin,HIGH);   //"20" is too short, the motor stalls
  for (cnt=1;cnt<=50;cnt++)digitalWrite(StepperPulseOutputPin,LOW);      //low pulse duration
  }

//********************************************
void SetLeftJogDir(){
  
  digitalWrite(StepperEnablePin, LOW);  //enable stepper controller
  digitalWrite(StepperDirPin, HIGH);
}

//********************************************
void SetRightJogDir(){
  
  digitalWrite(StepperEnablePin, LOW);  //enable stepper controller
  digitalWrite(StepperDirPin, LOW);
} 


//**************************************************************************************************************************************
//**************************************************************************************************************************************
//                                                       END OF PROGRAM
//**************************************************************************************************************************************
//**************************************************************************************************************************************
