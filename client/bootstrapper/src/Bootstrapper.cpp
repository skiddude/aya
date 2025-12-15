#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#endif
#include "Bootstrapper.hpp"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include <archive.h>
#include <archive_entry.h>
#include <openssl/sha.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

size_t WriteToString(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t total = size * nmemb;
    auto* out = static_cast<std::string*>(userp);
    out->append(static_cast<const char*>(contents), total);
    return total;
}

size_t WriteToOStream(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t total = size * nmemb;
    auto* os = static_cast<std::ostream*>(userp);
    os->write(static_cast<const char*>(contents), static_cast<std::streamsize>(total));
    return total;
}

Bootstrapper::Bootstrapper(const std::string& mode, bool showUI, bool forceSkipUpdates, bool isUsingInstance, const std::string& instanceUrl,
    const std::string& instanceAccessKey)
    : mode(mode)
    , showUI(showUI)
    , forceSkipUpdates(forceSkipUpdates)
    , isUsingInstance(isUsingInstance)
    , instanceUrl(instanceUrl)
    , instanceAccessKey(instanceAccessKey)
{
}
std::string Bootstrapper::httpGet(const std::string& path)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("CURL init failed for " + path);

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, (this->instanceUrl + path).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    if (!this->instanceAccessKey.empty())
    {
        std::string authHeader = "Authorization: " + this->instanceAccessKey;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);

    if (headers)
        curl_slist_free_all(headers);

    if (res != CURLE_OK)
    {
        std::string msg = "CURL GET failed: ";
        msg += curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error(msg);
    }

    curl_easy_cleanup(curl);
    return response;
}

int Bootstrapper::downloadFile(const std::string& path, const std::string& outputPath)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("CURL init failed for " + path);

    std::ofstream ofs(outputPath, std::ios::binary);
    if (!ofs)
    {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to open output file: " + outputPath);
    }

    curl_easy_setopt(curl, CURLOPT_URL, (this->instanceUrl + path).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToOStream);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&ofs));

    struct curl_slist* headers = nullptr;
    if (!this->instanceAccessKey.empty())
    {
        std::string authHeader = "Authorization: " + this->instanceAccessKey;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);

    curl_off_t downloadSize = 0;
    curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &downloadSize);

    if (headers)
        curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error("CURL download failed: " + std::string(curl_easy_strerror(res)));

    return static_cast<int>(downloadSize);
}

bool Bootstrapper::verifySHA256(const std::string& filePath, const std::string& expectedHex)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file for SHA256: " + filePath);

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    std::vector<char> buffer(1 << 16);
    while (file.good())
    {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::streamsize r = file.gcount();
        if (r > 0)
            SHA256_Update(&ctx, buffer.data(), static_cast<size_t>(r));
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    std::ostringstream oss;
    for (unsigned char b : hash)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str() == expectedHex;
}

void Bootstrapper::extractTarZst(const std::string& archivePath, const std::string& outputDir)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_zstd(a);

    if (archive_read_open_filename(a, archivePath.c_str(), 10240) != ARCHIVE_OK)
    {
        std::string err = archive_error_string(a);
        archive_read_free(a);
        throw std::runtime_error("Failed to open tar.zst: " + err);
    }

    std::filesystem::create_directories(outputDir);

    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const char* pathname = archive_entry_pathname(entry);
        std::filesystem::path outPath = std::filesystem::path(outputDir) / pathname;

        if (archive_entry_filetype(entry) == AE_IFDIR)
        {
            std::filesystem::create_directories(outPath);
        }
        else
        {
            std::filesystem::create_directories(outPath.parent_path());
            std::ofstream ofs(outPath, std::ios::binary);
            const void* buff;
            size_t size;
            la_int64_t offset;
            while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK)
            {
                ofs.write(static_cast<const char*>(buff), static_cast<std::streamsize>(size));
            }
        }
    }

    archive_read_free(a);
}

rapidjson::Document Bootstrapper::parseJson(const std::string& jsonStr)
{
    rapidjson::Document doc;

    doc.Parse(jsonStr.c_str());
    if (doc.HasParseError())
        throw std::runtime_error("Failed to parse JSON");

    return doc;
}

rapidjson::Document Bootstrapper::fetchLatestManifest()
{
    return parseJson(httpGet("api/aya/updater/manifest"));
}

rapidjson::Document Bootstrapper::fetchCachedManifest()
{
    if (std::filesystem::exists("data/manifest.json"))
    {
        std::ifstream ifs("data/manifest.json");
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        return parseJson(buffer.str());
    }
    else
    {
        // return empty data
        if (!std::filesystem::exists("data"))
        {
            std::filesystem::create_directories("data");
        }

        std::string filePath = "data/manifest.json";
        std::string emptyData = "{}";
        std::ofstream ofs(filePath);
        ofs << emptyData;
        ofs.close();

        return parseJson(emptyData);
    }
}

void Bootstrapper::updateCachedManifest(const rapidjson::Document& manifest)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    manifest.Accept(writer);

    std::ofstream ofs("data/manifest.json");
    ofs << buffer.GetString();
    ofs.close();
}

void Bootstrapper::checkForUpdates()
{
    rapidjson::Document latestManifest = fetchLatestManifest();
    rapidjson::Document cachedManifest = fetchCachedManifest();

    if (!cachedManifest.HasMember("version") || cachedManifest["version"].GetString() != latestManifest["version"].GetString())
    {
        std::string downloadUrl = latestManifest["download_url"].GetString();
        std::string expectedSha256 = latestManifest["sha256"].GetString();

        std::cout << "New version available: " << latestManifest["version"].GetString() << std::endl;
        std::cout << "Downloading from: " << downloadUrl << std::endl;

        std::string tempFilePath = "data/update_temp.tar.zst";
        downloadFile(downloadUrl, tempFilePath);

        if (!verifySHA256(tempFilePath, expectedSha256))
        {
            throw std::runtime_error("Downloaded file SHA256 does not match expected value.");
        }

        extractTarZst(tempFilePath, ".");

        std::filesystem::remove(tempFilePath);

        updateCachedManifest(latestManifest);

        std::cout << "Update applied successfully to version " << latestManifest["version"].GetString() << std::endl;
    }
    else
    {
        std::cout << "No updates available." << std::endl;
    }
}

void launchProcess(const std::string& appName, const std::string& commandLine)
{
    std::string fullCommand = appName + " " + commandLine;
    int result = std::system(fullCommand.c_str());
    if (result != 0)
    {
        throw std::runtime_error("Failed to launch process: " + fullCommand);
    }
}

void Bootstrapper::start(const std::string& commandLine)
{
    if (isUsingInstance)
        this->checkForUpdates();

    if (mode == "player")
        launchProcess("Aya.Player", commandLine);
    else if (mode == "studio")
        launchProcess("Aya.Studio", commandLine);
    else if (mode == "server")
        launchProcess("Aya.Server", commandLine);
}