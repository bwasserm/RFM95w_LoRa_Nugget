#pragma once

#include <SPI.h>
#include <LoRa.h>

#include "cobs.h"
#include "identity.h"
#include "game_state.h"

// SPI pins
#define SCK 15
#define MISO 17
#define MOSI 21

// Define the pins used by the LoRa module
#define ss 34
#define rst 5
#define dio0 16

void init_lora()
{
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
}

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
    Serial.println(serdes.action);
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

uint16_t tx_sequence_num = 0;
int num_packets_received = 0;

void tx_packet(uint8_t action, uint32_t dest, int8_t my_health, uint8_t game_number)
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
