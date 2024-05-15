#pragma once

// Actions
#define ACTION_UNKNOWN 0
#define ACTION_HEARTBEAT 1
#define ACTION_TAG 2
#define ACTION_ACK 3
#define ACTION_NACK 4
#define ACTION_ITEM_POKE 5
#define ACTION_ITEM_ACK 6

uint8_t current_state;
int8_t my_health;
#define FULL_HEALTH 3
int my_range = 40;
uint8_t game_number = 0;
uint8_t my_red = 0;
uint8_t my_green = 0;
uint8_t my_blue = 0;

struct Player
{
    uint32_t id;
    uint16_t ttl;
    int8_t health;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
#define MAX_PLAYERS 8
Player players[MAX_PLAYERS];
uint8_t num_players = 0;
uint8_t last_bcast_player_idx = 0;

void init_game_state()
{
    // Set up game state
    for (int player_idx = 0; player_idx < MAX_PLAYERS; player_idx++)
    {
        players[player_idx].id = 0;
        players[player_idx].ttl = 0;
        players[player_idx].health = 0;
        players[player_idx].red = 0;
        players[player_idx].green = 0;
        players[player_idx].blue = 0;
    }
}


void update_players(uint32_t other_id, int8_t other_health)
{
}
