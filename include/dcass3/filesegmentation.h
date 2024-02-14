//
// Created by adria on 14/02/2024.
//

#ifndef FILESEGMENTATION_H
#define FILESEGMENTATION_H

#define MEGABYTES_SIZE 24000000 // 24MB

typedef struct
{
    std::string filename; // Original filename
    std::vector<char> data; // Data as bytes
    int bytesize; // Data size in bytes
    int n_partitions; // bytesize / MEGABYTES_SIZE
} FILE_METADATA_T;

FILE_METADATA_T read_file(const char* filename, bool verbose = false);

FILE_METADATA_T read_file_metadata(const char* filename, bool verbose = false);

bool partition_exists(const char* partition_name);

void delete_partitions(const std::vector<std::string>& partitions_names);

const char* save_partitions_metadata(const FILE_METADATA_T& file_metadata);

std::vector<std::string>& create_file_partitions(const FILE_METADATA_T& file_metadata);

void merge_file_partitions(const FILE_METADATA_T& file_metadata);

#endif //FILESEGMENTATION_H
