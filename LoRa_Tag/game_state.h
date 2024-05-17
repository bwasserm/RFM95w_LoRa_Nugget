#pragma once

// Actions
#define ACTION_UNKNOWN 0
#define ACTION_HEARTBEAT 1
#define ACTION_TAG 2
#define ACTION_ACK 3
#define ACTION_NACK 4
#define ACTION_ITEM_POKE 5
#define ACTION_ITEM_ACK 6

// States
#define STATE_INIT 0
#define STATE_GAME_SEARCH 1
#define STATE_PLAYING 2
#define STATE_TAGGING 3

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
uint8_t next_bcast_player_idx = 0;


void reset_players()
{
    for (int player_idx = 0; player_idx < MAX_PLAYERS; player_idx++)
    {
        players[player_idx].id = 0;
        players[player_idx].ttl = 0;
        players[player_idx].health = 0;
        players[player_idx].red = 0;
        players[player_idx].green = 0;
        players[player_idx].blue = 0;
    }
    num_players = 0;
    next_bcast_player_idx = 0;
}

void update_players(uint32_t player_id, int8_t player_health)
{
    bool found = false;
    if(!player_id){
        return;
    }
    for (int player_idx = 0; player_idx < num_players; player_idx++)
    {
        if (player_id == players[player_idx].id)
        {
            found = true;
            players[player_idx].health = player_health;
            Serial.print("Updating player idx ");
            Serial.print(player_idx);
            Serial.print(" id ");
            Serial.print(player_id);
            Serial.print(" health ");
            Serial.print(player_health);
        }
    }
    if (!found)
    {
        players[num_players].id = player_id;
        players[num_players].health = player_health;
        num_players++;
        Serial.print("Adding player idx ");
        Serial.print(num_players - 1);
        Serial.print(" id ");
        Serial.print(player_id);
        Serial.print(" health ");
        Serial.print(player_health);
    }
    Serial.print(" of ");
    Serial.print(num_players);
    Serial.println(" players");
}

bool get_next_player(Player *player)
{
    if (!num_players)
    {
        player->id = 0;
        return false;
    }
    player->id = players[next_bcast_player_idx].id;
    player->health = players[next_bcast_player_idx].health;
    next_bcast_player_idx = (next_bcast_player_idx + 1) % num_players;
    return true;
}