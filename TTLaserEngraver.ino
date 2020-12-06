#include <PS2Keyboard.h>         //Keyboard
#include <SPI.h>                 //DACs

// LCD over I²C:
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Keyboard:
PS2Keyboard keyboard;

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 20 chars and 4 line display

void setup() {
    lcd.begin();                      // initialize the lcd
    lcd.blink_on();
    lcd.backlight();
    
    lcd.home();
    
    lcd.print("Hello, world!");
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("Let's burn some");
    lcd.setCursor(0, 2);
    lcd.print(" pencils!");
    delay(2000);
    lcd.blink_off();
}

void loop() {
    delay(1000);
}
