#include <stdio.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1) // Disable structure padding

typedef struct {
    uint32_t signature;           // 0x04034b50
    uint16_t version_needed;      // 20
    uint16_t general_purpose;     // 0
    uint16_t compression;         // 0 = store
    uint16_t mod_time;            // dummy
    uint16_t mod_date;            // dummy
    uint32_t crc32;
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint16_t fname_len;
    uint16_t extra_len;
} LocalFileHeader;

typedef struct {
    uint32_t signature;           // 0x02014b50
    uint16_t version_made;
    uint16_t version_needed;
    uint16_t flag;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint16_t fname_len;
    uint16_t extra_len;
    uint16_t comment_len;
    uint16_t disk_num_start;
    uint16_t internal_attr;
    uint32_t external_attr;
    uint32_t local_header_offset;
} CentralDirHeader;

typedef struct {
    uint32_t signature;           // 0x06054b50
    uint16_t disk_num;
    uint16_t start_disk;
    uint16_t entries_disk;
    uint16_t entries_total;
    uint32_t cd_size;
    uint32_t cd_offset;
    uint16_t comment_len;
} EndOfCentralDir;

#pragma pack(pop)

uint32_t crc32_calc(const unsigned char *buf, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320 & (-(int)(crc & 1)));
    }
    return ~crc;
}

int main() {
    const char *filename = "hello.txt";
    const unsigned char data[] = "Hello, ZIP!";
    const size_t data_len = sizeof(data) - 1;

    FILE *fp = fopen("minimal.zip", "wb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    uint32_t crc = crc32_calc(data, data_len);

    // --- Local File Header ---
    LocalFileHeader lfh = {
        .signature = 0x04034b50,
        .version_needed = 20,
        .general_purpose = 0,
        .compression = 0,  // stored
        .mod_time = 0,
        .mod_date = 0,
        .crc32 = crc,
        .comp_size = data_len,
        .uncomp_size = data_len,
        .fname_len = strlen(filename),
        .extra_len = 0
    };

    long local_header_offset = ftell(fp);
    fwrite(&lfh, sizeof(lfh), 1, fp);
    fwrite(filename, strlen(filename), 1, fp);
    fwrite(data, data_len, 1, fp);

    long central_dir_offset = ftell(fp);

    // --- Central Directory Entry ---
    CentralDirHeader cdh = {
        .signature = 0x02014b50,
        .version_made = 20,
        .version_needed = 20,
        .flag = 0,
        .compression = 0,
        .mod_time = 0,
        .mod_date = 0,
        .crc32 = crc,
        .comp_size = data_len,
        .uncomp_size = data_len,
        .fname_len = strlen(filename),
        .extra_len = 0,
        .comment_len = 0,
        .disk_num_start = 0,
        .internal_attr = 0,
        .external_attr = 0,
        .local_header_offset = local_header_offset
    };

    fwrite(&cdh, sizeof(cdh), 1, fp);
    fwrite(filename, strlen(filename), 1, fp);

    long central_dir_end = ftell(fp);
    uint32_t central_dir_size = central_dir_end - central_dir_offset;

    // --- End of Central Directory ---
    EndOfCentralDir eocd = {
        .signature = 0x06054b50,
        .disk_num = 0,
        .start_disk = 0,
        .entries_disk = 1,
        .entries_total = 1,
        .cd_size = central_dir_size,
        .cd_offset = central_dir_offset,
        .comment_len = 0
    };

    fwrite(&eocd, sizeof(eocd), 1, fp);
    fclose(fp);

    printf("✅ Created minimal.zip successfully!\n");
    return 0;
}
