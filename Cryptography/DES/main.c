#include "des.h"
#include <stdlib.h>

#define ACTION_ENCRYPT "-e"
#define ACTION_DECRYPT "-d"

extern void generate_keys (
    const unsigned char* const main_key, 
    key_set* key_sets
);

extern void feistel_function (
    const unsigned char* const message_piece, 
    unsigned char processed_piece[], 
    key_set* key_sets, 
    const unsigned char mode
);

static void encryptDecrypt (
    const unsigned char mode,
    unsigned char des_key[], 
    FILE* const input_file, 
    FILE* const output_file
);

static void read_key (unsigned char des[], const char* const key);
static void get_error (const char* const error);
static void check_args (const int argc, const char* const argv[]);
static void check_file (const char* const filename, const char* const mode);

int main (const int argc, const char* const argv[]) {
    check_args(argc, argv);

    unsigned char des_key[8];
    read_key(des_key, argv[2]);

    FILE* const input_file = fopen(argv[3], "rb");
    FILE* const output_file = fopen(argv[4], "wb");

    unsigned char mode = (strcmp(argv[1], ACTION_ENCRYPT)) ? DECRYPTION_MODE : ENCRYPTION_MODE;
    encryptDecrypt(mode, des_key, input_file, output_file);

    fclose(input_file);
    fclose(output_file);

    return 0;
}

static void encryptDecrypt (
    const unsigned char mode,
    unsigned char des_key[], 
    FILE* const input_file, 
    FILE* const output_file
) {
    unsigned long length, end_block, block_count = 0;
    unsigned char data_block[8], final_block[8], remainder;

    key_set* key_sets = (key_set*)malloc(17 * sizeof(key_set));
    generate_keys(des_key, key_sets);

    fseek(input_file, 0L, SEEK_END);
    length = ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);

    remainder = 8 - length % 8;
    end_block = length / 8 + ((length % 8) ? 1 : 0);

    while(fread(data_block, 1, 8, input_file)) {
        if (++block_count == end_block) {
            if (mode == ENCRYPTION_MODE) {
                if (remainder < 8)
                    memset((data_block + 8 - remainder), remainder, remainder);

                feistel_function(data_block, final_block, key_sets, mode);
                fwrite(final_block, 1, 8, output_file);

                if (remainder == 8) {
                    memset(data_block, remainder, 8);
                    feistel_function(data_block, final_block, key_sets, mode);
                    fwrite(final_block, 1, 8, output_file);
                }
            } else {
                feistel_function(data_block, final_block, key_sets, mode);
                remainder = final_block[7];

                if (remainder < 8)
                    fwrite(final_block, 1, 8 - remainder, output_file);
            }
        } else {
            feistel_function(data_block, final_block, key_sets, mode);
            fwrite(final_block, 1, 8, output_file);
        }
    }
}

static void check_args (const int argc, const char* const argv[]) {
    if (argc != 5) get_error("len(argc) != 5");
    if (strlen(argv[2]) < 8) get_error("len(key) < 8");
    if ((strcmp(argv[1], ACTION_ENCRYPT) != 0) && (strcmp(argv[1], ACTION_DECRYPT) != 0)) 
        get_error("argc[1] != [e / d]");
    check_file(argv[3], "rb");
    check_file(argv[4], "wb");
}

static void check_file (const char* const filename, const char* const mode) {
    FILE* const file = fopen(filename, mode);
    if (file == NULL) get_error("can't [read | write] file");
    fclose(file);
}

static void read_key (unsigned char des[], const char* const key) {
    unsigned char i;
    for (i = 0; i < 8; i++) des[i] = key[i];
}

static void get_error(const char* const error) {
    printf("Error: %s\n", error);
    exit(1);
}