// USB-Nugget LoRa Tag
// USB-Nugget: Use Board "LOLIN S2 Mini"
#include <SPI.h>
#include <LoRa.h>

#include "cobs.h"
#include "identity.h"
#include "ui.h"

// SPI pins
#define SCK 15
#define MISO 17
#define MOSI 21

// Define the pins used by the LoRa module
#define ss 34
#define rst 5
#define dio0 16

// States
#define STATE_INIT 0
#define STATE_GAME_SEARCH 1
#define STATE_PLAYING 2
#define STATE_TAGGING 3

uint8_t current_state;

int num_packets_received = 0;
int8_t my_health;
#define FULL_HEALTH 3
int my_range = 60;
int last_button = 0;
uint8_t game_number = 0;
uint16_t tx_sequence_num = 0;
uint8_t my_red = 0;
uint8_t my_green = 0;
uint8_t my_blue = 0;

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
#define ACTION_UNKNOWN 0
#define ACTION_HEARTBEAT 1
#define ACTION_TAG 2
#define ACTION_ACK 3
#define ACTION_NACK 4
#define ACTION_ITEM_POKE 5
#define ACTION_ITEM_ACK 6

#define PACKET_HEADER_SYNC 0xCAFEB0BADEADBEEF

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
    byte name[MAX_NAME_LEN];
    char null_term;
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


    delay(1000);

    my_id = get_chip_id();
    my_name = get_name(my_id);

    Serial.print("Tag packet size: ");
    Serial.println(sizeof(Packet));

    // Set up game state
    for (int player_idx = 0; player_idx < MAX_PLAYERS; player_idx++)
    {
        players[player_idx].id = 0;
        players[player_idx].health = 0;
        players[player_idx].red = 0;
        players[player_idx].green = 0;
        players[player_idx].blue = 0;
    }

    last_button = 0;

    init_buttons();
    init_display();
    show_boot_display(my_name);

    delay(2000);
    reset_health();

    current_state = STATE_INIT;
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

void serialPrintPacket(Packet serdes)
{
    Serial.print("Header: ");
    Serial.println(serdes.header_sync, HEX);
    Serial.print("Src ID: ");
    Serial.println(serdes.src_id);
    Serial.print("Dst ID: ");
    Serial.println(serdes.dst_id);
    Serial.print("Oth ID: ");
    Serial.println(serdes.other_id);
    Serial.print("Seq num: ");
    Serial.println(serdes.sequence_num);
    Serial.print("Game num: ");
    Serial.println(serdes.game_number);
    Serial.print("Action: ");
    switch (serdes.action)
    {
    case ACTION_UNKNOWN:
        Serial.println("UNKNOWN");
        break;
    case ACTION_HEARTBEAT:
        Serial.println("HEARTBEAT");
        break;
    case ACTION_TAG:
        Serial.println("TAG");
        break;
    case ACTION_ACK:
        Serial.println("ACK");
        break;
    case ACTION_NACK:
        Serial.println("NACK");
        break;
    case ACTION_ITEM_POKE:
        Serial.println("ITEM_POKE");
        break;
    case ACTION_ITEM_ACK:
        Serial.println("ITEM_ACK");
        break;
    default:
        Serial.println(serdes.action);
    }
    Serial.print("Src health: ");
    Serial.println(serdes.src_health);
    Serial.print("Oth health: ");
    Serial.println(serdes.other_health);
    Serial.print("Red: ");
    Serial.println(serdes.red);
    Serial.print("Grn: ");
    Serial.println(serdes.green);
    Serial.print("Blu: ");
    Serial.println(serdes.blue);
    Serial.print("Source name: ");
    Serial.println(String((char *)(serdes.name)));
    Serial.println("");
}

void take_hit()
{
    my_red = min(0xFF, my_red + 85);
    my_green = max(0, my_green - 85);
    my_health--;
    pixels.setPixelColor(0, pixels.Color(my_red, my_green, my_blue));
    pixels.show();
}

void reset_health()
{
    my_health = FULL_HEALTH;
    my_red = 0;
    my_green = 0xFF;
    my_blue = 0;
    pixels.setPixelColor(0, pixels.Color(my_red, my_green, my_blue));
    pixels.show();
    game_number++;
    update_ui(0, "Lets go!", my_health, game_number);
}

void tx_packet(uint8_t action, uint32_t dest)
{
    Packet serdes;
    serdes.header_sync = PACKET_HEADER_SYNC;
    serdes.src_id = my_id;
    serdes.dst_id = dest;
    serdes.other_id = 0xFACECAFE;
    serdes.sequence_num = tx_sequence_num++;
    serdes.game_number = game_number;
    serdes.action = action;
    serdes.src_health = my_health;
    serdes.other_health = -1;
    serdes.red = 0xFF;
    serdes.green = 0xFF;
    serdes.blue = 0xFF;
    my_name.getBytes((byte *)(serdes.name), MAX_NAME_LEN);
    serdes.null_term = '\0';
    uint8_t buffer[sizeof(Packet) + 2];
    int num_bytes_encoded = cobsEncode((void *)(&serdes), sizeof(Packet), buffer);
    if (num_bytes_encoded != sizeof(Packet) + 1)
    {
        Serial.print("COBS encoded to ");
        Serial.print(num_bytes_encoded);
        Serial.print(" bytes, expected ");
        Serial.println(sizeof(Packet));
    }
    // Null-terminate the packet
    buffer[sizeof(Packet) + 1] = '\0';
    String message = String((char *)buffer);

    // Send packet via LoRa
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
}

void rx_packets()
{
    int packetSize = LoRa.parsePacket();
    String lora_data;
    if (packetSize)
    {
        // Red color when a message is received
        // pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        // pixels.show();
        while (LoRa.available())
        {
            lora_data = LoRa.readString();
            int packet_rssi = LoRa.packetRssi();
            int packet_snr = LoRa.packetSnr();

            num_packets_received++;

            Serial.println("Got a packet!");
            // for (int c = 0; c < lora_data.length(); c++)
            // {
            //     Serial.print(lora_data[c], HEX);
            // }
            // Serial.println("");
            Serial.print("RSSI: ");
            Serial.println(packet_rssi);
            // Serial.print("Length: ");
            // Serial.println(lora_data.length());

            if (lora_data.length() != sizeof(Packet) + 1)
            {
                continue;
            }

            uint8_t buffer[sizeof(Packet) + 2];
            lora_data.getBytes(buffer, sizeof(Packet) + 2);
            Packet serdes;
            int num_bytes_decoded = cobsDecode(buffer, sizeof(Packet) + 1, (void *)(&serdes));
            // Serial.print("Decoded ");
            // Serial.print(num_bytes_decoded);
            // Serial.println(" bytes");

            if (serdes.header_sync != PACKET_HEADER_SYNC)
            {
                continue;
            }
            if (my_id == serdes.src_id)
            {
                // Ignore my own packets
                continue;
            }
            if (game_number != serdes.game_number)
            {
                if (game_number == 0)
                {
                    game_number = serdes.game_number;
                }
                else
                {
                    continue;
                }
            }

            String src_name = String((char *)(serdes.name));

            if (serdes.action != ACTION_HEARTBEAT)
            {
                serialPrintPacket(serdes);
            }
            if (serdes.action == ACTION_TAG)
            {
                if (packet_rssi > -1 * my_range)
                {
                    // I've been tagged!
                    take_hit();
                    tx_packet(ACTION_ACK, serdes.src_id);
                    String message = String("Tagged by ");
                    message.concat(src_name);
                    Serial.println(message);
                    update_ui(packet_rssi, message, my_health, game_number);
                }
            }
        }
    }
}

void send_tag(uint32_t target)
{
    tx_packet(ACTION_TAG, target);
}

int tx_counter = 0;
int tx_decimator = 10;

void loop()
{
    // Read buttons
    // if(last_button == 0 && digitalRead(BTN_UP))
    if (tx_counter % tx_decimator == 0)
    {
        tx_packet(ACTION_HEARTBEAT, 0);
    }
    tx_counter++;

    if (was_right_pressed() && my_health > 0)
    {
        send_tag(123456789);
    }
    if (was_left_pressed())
    {
        reset_health();
    }

    rx_packets();
    delay(100);
}
