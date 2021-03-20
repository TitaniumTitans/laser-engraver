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
int lcd_editMode_i = 0; // The position of the text input caret.
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
#define DIR_PIN 13       //DIR on EE-1003
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
#define KeyboardInterStrokeDelay 200        //milliseconds
#define BeamGrossVerticalOffset 0           //out of 1023   Coarse vertical offset
#define BeamGrossHorizOffset 0              //out of 1023   Coarse horiz offset
#define BurnDelayDivisor 2                  //1023/2 = 512ms max
#define CharSizeDivisor 90                  //1023/90 = 11 max
#define CharSpaceDivisor 90                 //1023/90 = 11 max
#define yOffsetDivisor 16                   //1023/16 = 64  max Fine vertical offset
#define TimeForStageToMoveOneCharacter 1500 //set at 1.5 seconds
#define HomeBackOffSteps 1000               // number of steps to move off of the limit switch during homing.

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

void moveStage(int, bool = true, bool = false); //damn you cpp

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

  digitalWrite(X_SEL, HIGH); //neg CS for SPI writing in data
  digitalWrite(Y_SEL, HIGH); //neg CS for SPI writing in data
  delay(5);
  digitalWrite(MISO, HIGH);  //neg pulse to latch data to DAC outputs

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
  
  // ---
  
  Serial.begin(9600);
}

#define IN keyboard
//#define IN Serial

void loop()
{
  char mode;
  // Choose test mode:
  lcd_clear();
  lcd_print("[MENU] 1=POT VIEW\n2=STAGE TEST\n3=LCD & KBD TEST\n4=BUTTONS");
  
  while (!IN.available()) delay(100);
  
  mode = IN.read();
  
  if (mode == '4') {
    lcd_clear();
    
    #define PIN_LIST_LEN 5
    int pinList[PIN_LIST_LEN] = {
      BURN_PIN, // I don't think that this should be BRN_PIN. I just checked and it is A8.
      STG_LEFT_PIN,
      STG_RGHT_PIN,
      STG_HOME_PIN,
      STP_BRN_PIN
    };
    String nameList[PIN_LIST_LEN] = {
      "BURN",
      "LEFT",
      "RIGHT"
      "HOME",
      "STOP",
    };
    
    while (!IN.available()) {
      for (int i = 0; i < PIN_LIST_LEN; i++) {
        if (digitalRead(pinList[i]) == LOW) {
          lcd_lineBreak();
          lcd_print(nameList[i]);
          
          // Wait for button to be released.
          for (;;) {
            if (digitalRead(pinList[i]) == HIGH) {
              break;
            } else if (IN.available()) {
              IN.read();
              goto loop_break_3;
            }
            delay(10);
          }
          
          lcd_print(" good.");
        }
      }
      
      delay(10);
    }
    loop_break_3: {}
  } else if (mode == '3') {
    int inputLen = lcd_width*(lcd_height-1);
    char input[inputLen];
    for (int i = 0; i < inputLen; i++) input[i] = '\0';
    lcd_editMode_prompt = "ENTER DATA:";
    lcd_editMode_init(input, inputLen-1);
    
    char c;
    
    for (;;) {
      if (!IN.available()) {
        delay(100);
        continue;
      }
      
      c = IN.read();
      
      if (c == '\r') { // Enter key
        break;
      } else if (c == 127) { // Backspace or delete key
        lcd_editMode_backsp();
        continue;
      } else if (c == '-' || c == '=') { // AS A SPECIAL CASE FOR TESTING
        lcd_println(String(c));
        continue;
      }
      
      lcd_editMode_type(c);
    }
    
    lcd_editMode_off();
    Serial.println(input);
  } else if (mode == '2') {
    char sel; // Selection
    int steps = 1;
    
    stageTest_redrawMenu();
    
    for (;;) {
      loop_continue_1:
      while (!IN.available()) delay(100);
      
      sel = IN.read();
      
      switch (sel) {
      case 's':
        moveStage(-steps);
      break;
      case 'f':
        moveStage(steps);
      break;
      case 'j':
        if (steps == 1) break;
        steps--;
        stageTest_printSteps(steps);
      break;
      case 'l':
        steps++;
        stageTest_printSteps(steps);
      break;
      case 'r':
        stageTest_redrawMenu();
      break;
      
      case 'q': goto loop_break_1;
      default: goto loop_continue_1;
      }
    }
    loop_break_1: {}
  } else if (mode == '1') {
    char sel; // Selection
    int potPin;
    // Choose pot:
    for (;;) {
      loop_continue_2:
      lcd_clear();
      lcd_println("[CHOOSE POT] q=exit");
      lcd_print("b=BRN    |   s=CSP\nh=CHR    |   y=YOFF");
      
      while (!IN.available()) delay(100);
      
      sel = IN.read();
      
      switch (sel) {
      case 'b': potPin = BRN_PIN;  break;
      case 's': potPin = CSP_PIN;  break;
      case 'h': potPin = CHR_PIN;  break;
      case 'y': potPin = YOFF_PIN; break;
      
      case 'q': goto loop_break_2;
      default: goto loop_continue_2;
      }
      
      // Read pot:
      lcd_clear();
      while (!IN.available()) {
        lcd_lineBreak();
        lcd_print(String(analogRead(potPin)));
        delay(1000);
      }
      IN.read();
    }
    loop_break_2: {}
  }
}

void stageTest_redrawMenu() {
  lcd_clear();
  lcd_println("[STAGE TEST] q=exit");
  lcd_print("s=left   |   f=right");
  lcd_print("j=step-  |   l=step+");
  lcd_print("r=redraw menu");
}
void stageTest_printSteps(int steps) {
  lcd_lineBreak();
  lcd_print("Steps: ");
  lcd_print(String(steps));
}

//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************

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
void Beep(int y)
{
  for (x = 1; x <= y; x++)
  {
    digitalWrite(BEEP_PIN, HIGH);
    Serial.println("BEEP");
    delay(100);
    digitalWrite(BEEP_PIN, LOW);
    delay(100);
  }
}

//*********************************************************
void WaitForEnterKey()
{

  do
  {
    if (keyboard.available())
    {
      c = keyboard.read();
    }
  } while (c != 13); // a 13 (CR or ENTER)
  c = 0;
  delay(300);
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

/**
 * @brief Displays an error message if the stage is moved too far.
 * 
 * @param limits Input the return value from checkLimits()
 */
void limitErrorMessage(int limits)
{
  if (limits != 0)
  {
    digitalWrite(LASER_EN_PIN, LOW); //disable laser output if on
    lcd_clear();
    lcd_println("ERROR:");
    lcd_println("STAGE MOVED TOO FAR");
    String direction = ((limits > 0) ? "LEFT" : "RIGHT");
    lcd_println(direction + ", REVERSE"); //pick what error message to use
    lcd_println("DIRECTION");
    moveStage(800 * limits, false, true); //since checkLimits returns -1 if right switch tripped, it will move -800 steps (or left 800 steps). If left is tripped, then it will move right!
    Beep(3);
  }
}

/**
 * @brief Checks values of limit switches.
 * 
 * @return int Returns -1 if right switch is tripped, 1 if left and 0 if normal.
 */
int checkLimits()
{
  if (digitalRead(LIM1_RGHT_PIN) == 0)
  {
    return -1;
  }
  if (digitalRead(LIM2_LEFT_PIN) == 0)
  {
    return 1;
  }
  return 0;
}

/**
 * @brief Moves the stage according to directional input and number of steps.
 * 
 * @param steps Number of steps. Positive for right movement, negative for left movement.
 * @param requireErrorMessage Normally true, set to false if tripping a switch shouldn't send a message (useful during homing)
 * @param ignoreLimits Normally false, set to true if movement should ignore limit switches (useful for backing off a switch)
 */
void moveStage(int steps, bool requireErrorMessage = true, bool ignoreLimits = false)
{
  (steps > 0) ? digitalWrite(DIR_PIN, LOW) : digitalWrite(DIR_PIN, HIGH); //select direction
  digitalWrite(EN_PIN, LOW);                                              // enable stepper controller
  for (int i = 0; i <= steps; i++)
  {
    digitalWrite(STEP_PIN, HIGH);
    delay(50); //delay should be fine to use here, since its only 50ms and the motor only moved 1 step.
    digitalWrite(STEP_PIN, LOW);
    delay(50);
    int limits = checkLimits();
    if (limits != 0 && ignoreLimits == false)
    {
      if (requireErrorMessage)
      {
        limitErrorMessage(limits);
      }
      break;
    }
  }
  digitalWrite(EN_PIN, HIGH); // disable stepper when we are done.
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
    
    uint8_t ixw; // i times lcd_width
    char tempChar; // Certain characters will need to be temporarily replaced with '\0' while printing lines.
    // Print the lines of input, stopping when there are no more lines or a line is empty:
    for (uint8_t i = 0; i <= lcd_height-1; i++) {
      ixw = i*lcd_width;
      
      lcd.setCursor(0, i + 1);
      
      if (ixw+lcd_width > lcd_editMode_maxLen) { // If this line is the last, don't add the null char.
        lcd.print(&(lcd_editMode_input[ixw]));
      } else {
        tempChar = lcd_editMode_input[ixw];
        lcd_editMode_input[ixw] = '\0';
        
        lcd.print(&(lcd_editMode_input[ixw]));
        
        lcd_editMode_input[ixw] = tempChar;
      }
    }
    
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
 * @brief Turns edit mode on, switching to the input screen and prompting the user for text input.
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
  
  lcd_editMode = true;
  lcd_repaint();
}

/**
 * @brief Switch to the console screen.
 * The text that was previously on the screen will be displayed again.
 * Use the input buffer supplied to lcd_editMode_on() contains the inputted text.
 * 
 * 
 */
int lcd_editMode_off()
{
  lcd_editMode = false;
  lcd_repaint();
  
  return lcd_editMode_len;
}

void lcd_editMode_type(char c)
{
  if (!lcd_editMode) {
    lcd_editMode = true;
    lcd_repaint();
  }
  
  if (lcd_editMode_len == lcd_editMode_maxLen) {
    Beep(1);
    return;
  }
  
  // If the cursor has been moved from the end of the input, shift characters 1 to the right:
  if (lcd_editMode_i != lcd_editMode_len) {
    // TODO: do the shifting. Also, line editing.
  }
  
  lcd_editMode_input[lcd_editMode_i] = c;
  lcd_editMode_len++;
  lcd_editMode_i++;
  
  lcd.write(c);
  
  // Reposition the cursor if it's at the end of the line:
  if (lcd_editMode_i % lcd_width == 0) {
    // Set the position of the cursor:
    lcd_editMode_posCur();
  }
}

/**
 * @brief Backspace.
 * 
 */
void lcd_editMode_backsp()
{
  if (!lcd_editMode) {
    lcd_editMode = true;
    lcd_repaint();
  }
  
  if (lcd_editMode_i == 0) {
    Beep(1);
    return;
  }
  
  if (lcd_editMode_i != lcd_editMode_len) {
    // TODO: do the shifting. Also, line editing.
 	}
  
  lcd_editMode_i--;
  lcd_editMode_input[lcd_editMode_i] = '\0';
  
  // Clear the backspace-d character
  lcd_editMode_posCur();
  lcd.write(' ');
  lcd_editMode_posCur();
}

/**
 * @brief Reposition the LCD cursor in editMode.
 */
void lcd_editMode_posCur()
{
  lcd.setCursor(lcd_editMode_i % lcd_width, (lcd_editMode_i / lcd_width) + 1);
}
