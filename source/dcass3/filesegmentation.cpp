//
// Created by adria on 14/02/2024.
//

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

#include "../../include/dcass3/filesegmentation.h"

FILE_METADATA_T read_file(const char* filename, const bool verbose)
{
    // Create file metadata
    FILE_METADATA_T file_data;
    file_data.filename = std::string(filename);
    file_data.bytesize = 0;
    file_data.n_partitions = 0;

    // Read file data
    FILE* file = fopen(filename, "rb");

    // Get file size by moving the cursor to the end of the file and then getting the cursor position
    fseek(file, 0, SEEK_END);
    file_data.bytesize = ftell(file);
    rewind(file);

    // Calculate the number of partitions
    file_data.n_partitions = file_data.bytesize / MEGABYTES_SIZE;

    // Read file data
    char byte = 0;
    while (fread(&byte, 1, 1, file)) file_data.data.push_back(byte);

    if (verbose)
    {
        std::cout << "Filename: " << file_data.filename << std::endl;
        std::cout << "Bytesize: " << file_data.bytesize << std::endl;
        std::cout << "Partitions: " << file_data.n_partitions << std::endl;
    }

    fclose(file);

    return file_data;
}

FILE_METADATA_T read_file_metadata(const char* filename, const bool verbose)
{
    FILE* file = fopen(filename, "rb");
    FILE_METADATA_T file_metadata{};

    int filename_length = 0;
    fread(&filename_length, 4, 1, file);

    char filename_value[filename_length + 1];
    fread(filename_value, filename_length, 1, file);
    for (int i = 0; i < filename_length; i++) file_metadata.filename.push_back(filename_value[i]);

    fread(&file_metadata.bytesize, 4, 1, file);
    fread(&file_metadata.n_partitions, 4, 1, file);

    fclose(file);

    if (verbose)
    {
        std::cout << "Filename: " << file_metadata.filename << std::endl;
        std::cout << "Bytesize: " << file_metadata.bytesize << std::endl;
        std::cout << "Partitions: " << file_metadata.n_partitions << std::endl;
    }

    return file_metadata;
}

bool partition_exists(const char* partition_name)
{
    bool ret = false;

    FILE* file = fopen(partition_name, "rb");
    if (file) ret = true;
    fclose(file);

    return ret;
}

void delete_partitions(const std::vector<std::string>& partitions_names)
{
    for (const auto& filename : partitions_names)
        remove(filename.c_str());
}

const char* save_partitions_metadata(const FILE_METADATA_T& file_metadata)
{
    const char* filename = (file_metadata.filename + ".meta").c_str();
    FILE* file = fopen(filename, "wb");

    // Save filename length and filename value
    const int filename_length = file_metadata.filename.length();
    fwrite(&filename_length, 4, 1, file);
    fwrite(file_metadata.filename.data(), filename_length, 1, file);

    // Save bytesize and n_partitions
    const int bytesize_n_partitions[2] = {file_metadata.bytesize, file_metadata.n_partitions};
    fwrite(bytesize_n_partitions, 4, 2, file);

    fclose(file);

    return filename;
}

std::vector<std::string>& create_file_partitions(const FILE_METADATA_T& file_metadata)
{
    // Create filenames vector and save partitions metadata
    auto* filenames = new std::vector<std::string>;
    filenames->push_back(save_partitions_metadata(file_metadata));

    // Create file partitions
    for (int i = 0; i < file_metadata.n_partitions; i++)
    {
        std::string filename = file_metadata.filename + std::to_string(i);
        filenames->push_back(filename);

        FILE* file = fopen(filename.c_str(), "wb");
        for (int j = 0; j < MEGABYTES_SIZE; j++) fwrite(&file_metadata.data[i * MEGABYTES_SIZE + j], 1, 1, file);
        fclose(file);
    }

    // Create last partition (rest of the data)
    const std::string filename = file_metadata.filename + std::to_string(file_metadata.n_partitions);
    filenames->push_back(filename);

    FILE* file = fopen(filename.c_str(), "wb");
    for (int i = 0; i < file_metadata.bytesize % MEGABYTES_SIZE; i++) fwrite(&file_metadata.data[file_metadata.n_partitions * MEGABYTES_SIZE + i], 1, 1, file);
    fclose(file);

    return *filenames;
}

void merge_file_partitions(const FILE_METADATA_T& file_metadata)
{
    FILE* file = fopen(file_metadata.filename.c_str(), "wb");

    for (int i = 0; i <= file_metadata.n_partitions; i++)
    {
        const auto partition_name = (file_metadata.filename + std::to_string(i)).c_str();

        if (!partition_exists(partition_name))
        {
            std::cerr << "Partition " << partition_name << " does not exist. Aborting..." << std::endl;
            break;
        }

        FILE_METADATA_T partition_metadata = read_file(partition_name);
        fwrite(partition_metadata.data.data(), 1, partition_metadata.bytesize, file);
    }

    fclose(file);
}
