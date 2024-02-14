#include <curl/curl.h>
#include <argparse/argparse.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "../../include/dcass3/filesegmentation.h"

#define WEBHOOK "YOUR_DISCORD_WEBHOOK_URL"

int main(const int argc, char** argv)
{
    // Parse arguments
    argparse::ArgumentParser program("dcass3", "1.0");

    program.add_argument("filename")
           .help("Filename to send to Discord Webhook or partitions metadata");

    program.add_argument("-m", "--merge-partitions")
           .help("Merge partitions into a single file and save it to the original filename")
           .default_value(false)
           .implicit_value(true);

    program.add_argument("-s", "--just-segment")
           .help("Just segment the file into partitions and save them")
           .default_value(false)
           .implicit_value(true);

    program.add_argument("-k", "--keep-partitions")
           .help("Keep partitions after sending them to Discord Webhook")
           .default_value(false)
           .implicit_value(true);

    program.add_argument("-w", "--webhook")
           .help("Discord Webhook URL")
           .default_value(WEBHOOK)
           .nargs(1);

    try
    {
        program.parse_args(argc, argv);
    }

    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // Variables
    const char* filename = program.get("filename").c_str();

    // Delete partitions
    if (program["--merge-partitions"] == true)
    {
        merge_file_partitions(read_file_metadata(filename));
        return 0;
    }

    // Read file and create partitions
    const FILE_METADATA_T file_data = read_file(filename);
    const std::vector<std::string> partitions_names = create_file_partitions(file_data);

    if (program["--just-segment"] == true) return 0;

    // Send partitions to Discord Webhook
    int actual_partition = 0, exit_code = 0;
    for (const auto& partition_name : partitions_names)
    {
        // Initialize curl
        CURL* curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "curl_easy_init() failed" << std::endl;
            exit_code = 2;
            break;
        }

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
        curl_easy_setopt(curl, CURLOPT_URL, program.get("--webhook").c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

        // Realize the request
        if (const auto res = curl_easy_perform(curl); res != CURLE_OK) std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

        // Free resources
        curl_easy_cleanup(curl);
        curl_mime_free(mime);
        curl_global_cleanup();
    }

    // Delete partitions
    if (program["--keep-partitions"] == false)
        delete_partitions(partitions_names);

    return exit_code;
}
