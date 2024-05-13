#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Define NeoPixel settings
#define NEOPIXEL_PIN 12
#define NUMPIXELS 1

// Define SH1106 display dimensions and address
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C // Adjust if needed

// Define buttons (nugget v2.2)
#define BTN_UP 9
#define BTN_DOWN 18
#define BTN_LEFT 11
#define BTN_RIGHT 7

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN);
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void init_neopixel()
{
    // Initialize NeoPixel
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Blue color on boot
    pixels.show();
}

void init_display()
{
    // Initialize the SH1106 display
    if (!display.begin(OLED_ADDR, OLED_RESET))
    {
        Serial.println(F("SH1106 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
}

void show_boot_display(String my_name)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE); // Corrected here
    display.setCursor(0, 0);            // col, row
    display.println("Nugget LoRa Tag");
    display.println("Initalized.");
    display.print("My name: ");
    display.println(my_name);
    display.println("Standing by...");
    display.display();
}

void init_buttons()
{
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
}


void update_ui(int last_rssi, String display_message, int8_t health, uint8_t game_number)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE); // Corrected here
    display.setCursor(0, 0);            // col, row
    display.println("***Nugget LoRa Tag***");
    display.print("I am ");
    display.println(my_name);
    display.print("My Health: ");
    for (int heart = 0; heart < health; heart++)
    {
        display.print("<3 ");
    }
    if (health == 0)
    {
        display.print("dead!");
    }
    display.println("");
    display.print("Game num: ");
    display.print(game_number);
    display.print(" RSSI: ");
    display.println(last_rssi);
    display.println("<: End game  >: Tag  ");
    display.println("^:           v:      ");
    display.println(display_message);
    display.display();
}
