// USB-Nugget LoRa Tag
// USB-Nugget: Use Board "LOLIN S2 Mini"
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// SPI pins
#define SCK 15
#define MISO 17
#define MOSI 21

// Define the pins used by the LoRa module
#define ss 34
#define rst 5
#define dio0 16

// Define NeoPixel settings
#define NEOPIXEL_PIN 12
#define NUMPIXELS 1

// Define buttons (nugget v2.2)
#define BTN_UP 9
#define BTN_DOWN 18
#define BTN_LEFT 11
#define BTN_RIGHT 7

// Define SH1106 display dimensions and address
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C // Adjust if needed

// States
#define STATE_INIT 0
#define STATE_GAME_SEARCH 1
#define STATE_PLAYING 2
#define STATE_TAGGING 3

uint8_t current_state;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN);

#define MAX_NAME_LEN 8

int num_packets_received = 0;
uint32_t my_id = 0;
String my_name;
int8_t my_health;
int my_range = 60;
int last_button = 0;
uint8_t game_number = 0;

struct Player
{
    uint32_t id;
    int8_t health;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
#define MAX_PLAYERS 8
Player players[MAX_PLAYERS];
uint8_t num_players = 0;
uint8_t last_bcast_player_idx = 0;

// Actions
#define HEARTBEAT 0
#define TAG 1
#define ACK 2
#define NACK 3
#define ITEM_POKE 4
#define ITEM_ACK 5

#define PACKET_HEADER_SYNC 0xCAFEB0BACAFEB0BA

struct Packet
{
    uint64_t header_sync;
    uint32_t src_id;
    uint32_t dst_id;
    uint32_t other_id;
    uint16_t sequence_num;
    uint8_t game_number;
    uint8_t action;
    int8_t src_health;
    int8_t other_health;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    char name[MAX_NAME_LEN];
};

union PacketSerdes
{
    Packet packet;
    unsigned char bytes[sizeof(Packet)];
};

void setup()
{
    // Initialize Serial Monitor
    Serial.begin(9600);
    // while (!Serial);
    Serial.println("Nugget LoRa Tag");

    // Initialize SPI with custom pins
    SPI.begin(SCK, MISO, MOSI, ss);

    // Setup LoRa transceiver module
    LoRa.setPins(ss, rst, dio0);
    while (!LoRa.begin(915E6))
    {
        Serial.println(".");
        delay(500);
    }
    LoRa.setSyncWord(0xF3);
    Serial.println("LoRa Initializing OK!");

    // Initialize NeoPixel
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Blue color on boot
    pixels.show();
    delay(1000);

    my_id = get_chip_id();
    my_name = get_name(my_id);

    // Set up game state
    my_health = 3;
    for (int player_idx = 0; player_idx < MAX_PLAYERS; player_idx++)
    {
        players[player_idx].id = 0;
        players[player_idx].health = 0;
        players[player_idx].red = 0;
        players[player_idx].green = 0;
        players[player_idx].blue = 0;
    }

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    last_button = 0;

    // Initialize the SH1106 display
    if (!display.begin(OLED_ADDR, OLED_RESET))
    {
        Serial.println(F("SH1106 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
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
    delay(2000);

    current_state = STATE_INIT;
}

void loop()
{
    // Read buttons
    // if(last_button == 0 && digitalRead(BTN_UP))
}

uint32_t get_chip_id()
{
    uint32_t chip_id = 0;
    // Get unique ID per player
    for (int i = 0; i < 17; i = i + 8)
    {
        chip_id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("This chip has %d cores\n", ESP.getChipCores());
    Serial.print("Chip ID: ");
    Serial.println(chip_id);
    return chip_id;
}

String get_name(uint32_t uuid)
{
    // These two need to be kept in sync!
    String names = "alpha,bravo,charlie,delta,echo";
    int name_start_idx = 0;
    int name_end_idx = 0;
    // Count how many names are defined
    uint8_t num_names = 0;
    while (name_end_idx != -1)
    {
        name_start_idx = name_end_idx;
        name_end_idx = names.indexOf(',', name_start_idx + 1);
        num_names++;
    }
    // Find the name from the list
    uint8_t name_index = uuid % num_names;
    name_start_idx = 0;
    name_end_idx = 0;
    for (uint8_t current_index = 0; current_index <= name_index; current_index++)
    {
        name_start_idx = name_end_idx;
        name_end_idx = names.indexOf(',', name_start_idx + 1);
    }
    if (name_end_idx == -1)
    {
        name_end_idx = names.length();
    }
    // Truncate names that are too long.
    if (name_end_idx - name_start_idx > MAX_NAME_LEN)
    {
        name_end_idx = name_start_idx + MAX_NAME_LEN;
    }
    String name = names.substring(name_start_idx + 1, name_end_idx);
    Serial.print("My name: ");
    Serial.println(name);
    return name;
}

void update_ui(int last_rssi, String last_packet)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE); // Corrected here
    display.setCursor(0, 0);            // col, row
    display.println("***Nugget LoRa Tag***");
    display.print("I am ");
    display.println(my_name);
    display.print("My Health: ");
    for (int heart = 0; heart < my_health; heart++)
    {
        display.print("<3 ");
    }
    display.println("");
    display.print("Last RSSI: ");
    display.println(last_rssi);
    display.println(last_packet);
    display.display();

    if (my_health > 0) // Still alive
    {
        pixels.setPixelColor(0, pixels.Color(0, 32, 0));
    }
    else // dead
    {
        pixels.setPixelColor(0, pixels.Color(32, 0, 0));
    }
    pixels.show();
}

void state_machine_update()
{
    switch (current_state)
    {
    case STATE_INIT:
        break;
    case STATE_GAME_SEARCH:
        break;
    case STATE_PLAYING:
        break;
    case STATE_TAGGING:
        break;
    default:
        break;
    }
}

void update_players(uint32_t other_id, int8_t other_health)
{
}

void rx_packets()
{
    int packetSize = LoRa.parsePacket();
    String lora_data;
    if (packetSize)
    {
        // Red color when a message is received
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.show();
        while (LoRa.available())
        {
            lora_data = LoRa.readString();
            int packet_rssi = LoRa.packetRssi();
            int packet_snr = LoRa.packetSnr();

            num_packets_received++;

            PacketSerdes serdes;
            if (lora_data.length() != sizeof(Packet))
            {
                update_ui(lora_data.length(), "Packet Wrong Size");
                continue;
            }

            lora_data.getBytes(serdes.bytes, sizeof(PacketSerdes));
            if (serdes.packet.header_sync != PACKET_HEADER_SYNC)
            {
                update_ui(0, "Wrong header");
                continue;
            }
            if (game_number != serdes.packet.game_number)
            {
                if (game_number == 0)
                {
                    game_number = serdes.packet.game_number;
                }
                else
                {
                    update_ui(serdes.packet.game_number, "Unknown game number");
                    continue;
                }
            }
        }
    }
}
