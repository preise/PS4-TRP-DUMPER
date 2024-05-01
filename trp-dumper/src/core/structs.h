#pragma once

// Struct from https://www.psdevwiki.com/ps4/Trophy00.trp
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned long long file_size;
    unsigned int entry_num;
    unsigned int entry_size;
    unsigned int dev_flag;
    unsigned char digest[20];
    unsigned int key_index;
    unsigned char padding[44];
} trp_header;

typedef struct {
    signed char entry_name[32];
    unsigned long long entry_pos;
    unsigned long long entry_len;
    unsigned int flag;
    unsigned char padding[12];
} trp_entry;

typedef struct {
    signed char entry_name[32];
    unsigned long long entry_pos;
    unsigned long long entry_len;
    unsigned int flag;
    std::vector<uint8_t> data;
} trp_entry_data;