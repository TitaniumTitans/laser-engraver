#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
    lcd.init();                      // initialize the lcd
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
