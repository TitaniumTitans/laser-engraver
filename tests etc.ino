#include <PS2Keyboard.h>         //Keyboard
#include <SPI.h>                 //DACs

// LCD over I²C:
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Keyboard:
PS2Keyboard keyboard;

const uint8_t lcd_width = 20;
const uint8_t lcd_height = 4;
LiquidCrystal_I2C lcd(0x27, lcd_width, lcd_height);  // set the LCD address to 0x27 for a 20 chars and 4 line display

void setup() {
    lcd_init();
    lcd.blink();

    /*lcd_println("START:TYPE 1,2 OR 3");
    lcd_println("1.NEW MESSAGE");
    lcd_println("2.REPEAT BURN");
    lcd_println("3.CHARACTER ADJUST");*/
    
    /*lcd.clear();
    lcd.println("START:TYPE 1,2 OR 3");
    lcd.println("1.NEW MESSAGE");
    lcd.println("2.REPEAT BURN");
    lcd.println("3.CHARACTER ADJUST");*/
    
    /*lcd.home();
    lcd.print("Hello, world!");
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("Let's burn some");
    lcd.setCursor(0, 2);
    lcd.print(" pencils!");
    delay(2000);
    lcd.noBlink();*/
}

char foo[2] = {'a', '\0'};
void loop() {
    lcd_print(foo);
    if (foo[0] == 'z') foo[0] = 'a'-1;
    foo[0]++;
    delay(100);
    /*lcd_println("b");
    delay(1000);
    lcd_println("c");
    delay(1000);
    lcd_println("d");
    delay(1000);
    lcd_println("e");
    delay(1000);*/
}

// Word-wrap LCD:
uint8_t lcd_y = 0; // which line
uint8_t lcd_x = 0;

char lcd_screen[lcd_height][lcd_width+1]; // (lcd_width+1, for the \0 at the line end)
uint8_t lcd_screen_wrap = 0;

void lcd_init() {
    lcd.init();
    lcd.backlight();
    /*for (int i = 0; i < lcd_width*lcd_height) {
        lcd_screen[i] = '\0';
    }*/
}
void lcd_println(const char text[]) {
    lcd_print(text);
    lcd_lineBreak();
}
void lcd_print(const char text[]) {
    char currentChar;
    for (int i = 0; ; i++) {
        currentChar = text[i];
        if (currentChar == '\0') break;
        
        if (currentChar == '\n') {
            lcd_lineBreak();
            continue;
        }

        // Register the new character in lcd_screen:
        lcd_screen[(lcd_y+lcd_screen_wrap) % lcd_height][lcd_x] = currentChar;
        
        lcd.write(currentChar);
        lcd_x++;

        // If at right end of screen:
        if (lcd_x == lcd_width) {
            lcd_lineBreak();
        }
    }
}
void lcd_clear() {
    lcd.clear();
    lcd_x = 0;
    lcd_y = 0;
}
void lcd_lineBreak() {
    lcd_x = 0;
    lcd_y++;
    if (lcd_y == lcd_height) {
        // SOMEONE ANYONE HELP WE'RE OUT OF SCREEN 😱😱😱😱
        // I KNOW!! LCD_SCROLL TO THE RESCUE!!!!!!!!!1
        lcd_scroll();
    }
}
// Scrolls the display down one line
void lcd_scroll() {
    lcd.clear();

    // Clear the (soon to be) last line:
    lcd_screen[lcd_screen_wrap][0] = '\0';
    // Scroll:
    lcd_screen_wrap = (lcd_screen_wrap+1) % lcd_height;

    // Update LCD:
    for (uint8_t y = 0; y < lcd_height; y++) {
        lcd.setCursor(0, y);
        lcd.print(lcd_screen[(y+lcd_screen_wrap) % lcd_height]);
    }
    lcd_y--;
    lcd.setCursor(lcd_x, lcd_y);
}
