// USB-Nugget LoRa Tag
// USB-Nugget: Use Board "LOLIN S2 Mini"

#include "identity.h"
#include "ui.h"
#include "packets.h"
#include "game_state.h"

// States
#define STATE_INIT 0
#define STATE_GAME_SEARCH 1
#define STATE_PLAYING 2
#define STATE_TAGGING 3


void setup()
{
    // Initialize Serial Monitor
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Nugget LoRa Tag");

    init_lora();

    delay(1000);

    my_id = get_chip_id();
    my_name = get_name(my_id);


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
    // game_number++;
    update_ui(0, "Lets go!", my_health, game_number);
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
                    tx_packet(ACTION_ACK, serdes.src_id, my_health, game_number);
                    String message = String("Tagged by ");
                    message.concat(src_name);
                    Serial.println(message);
                    update_ui(packet_rssi, message, my_health, game_number);
                } else
                {
                    tx_packet(ACTION_NACK, serdes.src_id, my_health, game_number);
                    String message = src_name;
                    message.concat(" is close");
                    Serial.println(message);
                    update_ui(packet_rssi, message, my_health, game_number);
                }
            }
        }
    }
}

void send_tag(uint32_t target)
{
    tx_packet(ACTION_TAG, target, my_health, game_number);
}

int tx_counter = 0;
int tx_decimator = 10;

void loop()
{
    // Read buttons
    // if(last_button == 0 && digitalRead(BTN_UP))
    if (tx_counter % tx_decimator == 0)
    {
        tx_packet(ACTION_HEARTBEAT, 0, my_health, game_number);
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
