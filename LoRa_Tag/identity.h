/**
 * Identity of this nugget, both ID number and Name.
 */

#define MAX_NAME_LEN 8

uint32_t my_id = 0;
String my_name;

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
    String names = ",alpha,bravo,charlie,delta,echo";
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
