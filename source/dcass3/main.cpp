#include <curl/curl.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

#define WEBHOOK "YOUR_DISCORD_WEBHOOK_URL"
#define MEGABYTES_SIZE 24000000 // 24MB

typedef struct
{
    std::string filename; // Original filename
    std::vector<char> data; // Data as bytes
    int bytesize; // Data size in bytes
    int n_partitions; // bytesize / MEGABYTES_SIZE
} FILE_METADATA_T;

void delete_partitions(const std::vector<std::string>& partitions_names)
{
    for (const auto& filename : partitions_names)
        remove(filename.c_str());
}

void save_partitions_metadata(const FILE_METADATA_T& file_metadata)
{
    FILE* fichero = fopen((file_metadata.filename + ".meta").c_str(), "wb");

    // Save filename length and filename value
    int filename_length = file_metadata.filename.length();
    fwrite(&filename_length, 4, 1, fichero);
    fwrite(file_metadata.filename.data(), filename_length, 1, fichero);

    // Save bytesize and n_partitions
    int bytesize_n_partitions[2] = {file_metadata.bytesize, file_metadata.n_partitions};
    fwrite(bytesize_n_partitions, 4, 2, fichero);

    fclose(fichero);
}

FILE_METADATA_T& read_file(const char* filename)
{
    // Create file metadata
    auto* file_data = new FILE_METADATA_T;
    file_data->filename = std::string(filename);
    file_data->bytesize = 0;
    file_data->n_partitions = 0;

    // Read file data
    FILE* fichero = fopen(filename, "rb");

    // Get file size by moving the cursor to the end of the file and then getting the cursor position
    fseek(fichero, 0, SEEK_END);
    file_data->bytesize = ftell(fichero);
    rewind(fichero);

    // Calculate the number of partitions
    file_data->n_partitions = file_data->bytesize / MEGABYTES_SIZE;

    // Read file data
    char byte = 0;
    while (fread(&byte, 1, 1, fichero)) file_data->data.push_back(byte);

    return *file_data;
}

std::vector<std::string>& create_file_partitions(const FILE_METADATA_T& file_metadata)
{
    // Create filenames vector and save partitions metadata
    auto* filenames = new std::vector<std::string>;
    save_partitions_metadata(file_metadata);
    filenames->push_back(file_metadata.filename + ".meta");

    // Create file partitions
    for (int i = 0; i < file_metadata.n_partitions; i++)
    {
        std::string filename = file_metadata.filename + std::to_string(i);
        filenames->push_back(filename);

        FILE* fichero = fopen(filename.c_str(), "wb");
        for (int j = 0; j < MEGABYTES_SIZE; j++) fwrite(&file_metadata.data[i * MEGABYTES_SIZE + j], 1, 1, fichero);
        fclose(fichero);
    }

    // Create last partition (rest of the data)
    std::string filename = file_metadata.filename + std::to_string(file_metadata.n_partitions);
    filenames->push_back(filename);

    FILE* fichero = fopen(filename.c_str(), "wb");
    for (int i = 0; i < file_metadata.bytesize % MEGABYTES_SIZE; i++) fwrite(&file_metadata.data[file_metadata.n_partitions * MEGABYTES_SIZE + i], 1, 1, fichero);
    fclose(fichero);

    return *filenames;
}


int main()
{
    // Read file and create partitions
    FILE_METADATA_T file_data = read_file("airbnb.json");
    const std::vector<std::string> partitions_names = create_file_partitions(file_data);

    // Send partitions to Discord Webhook
    int actual_partition = 0;
    for (const auto& partition_name : partitions_names)
    {
        // Initialize curl
        CURL* curl = curl_easy_init();
        if (!curl) break;

        curl_global_init(CURL_GLOBAL_ALL);

        // Configure headers, important for Discord to accept the request
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Create a MIME form
        curl_mime* mime = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(mime);
        curl_mime_name(part, ("file" + std::to_string(actual_partition++)).c_str());
        curl_mime_filedata(part, partition_name.c_str());

        // Configure POST request with MIME form
        curl_easy_setopt(curl, CURLOPT_URL, WEBHOOK);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

        // Realize the request
        if (const auto res = curl_easy_perform(curl); res != CURLE_OK) std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

        // Free resources
        curl_easy_cleanup(curl);
        curl_mime_free(mime);
        curl_global_cleanup();
    }

    // Delete partitions
    delete_partitions(partitions_names);

    return 0;
}
