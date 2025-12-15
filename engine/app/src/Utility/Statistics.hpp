#pragma once

#include <string>
#include "DataModel/ContentProvider.hpp"
#include "Utility/HttpAsync.hpp"

class SimpleJSON;

// This is a hack but used from Aya Server that disables onRemoteSysStats, client ticket authentication, and trustcheck/any sort of other validations.
void SetInsecureMode(bool mode);
const bool IsInsecureMode();

void SetVerboseLogging(bool verbose);
const bool IsVerboseLogging();

void SetMasterServerURL(const std::string& baseUrl);
const std::string& GetMasterServerURL();

void SetUsingMasterServer(bool useMasterServer);
const bool IsUsingMasterServer();

void SetMasterServerKey(const std::string& key);
const std::string& GetMasterServerKey();

// This is for instances now.
void SetBaseURL(const std::string& baseUrl);
const std::string& GetBaseURL();

void SetInstanceAccessKey(const std::string& accessKey);
const std::string& GetInstanceAccessKey();

void SetUsingInstance(bool useInstance);
const bool IsUsingInstance();

// All fetched from metadata
void FetchInstanceMetadata();
const std::string& GetInstanceCurrency();
const std::string& GetInstanceName();
const std::string& GetInstanceMotd();

void SetTrustCheckURL(const std::string& url);
const std::string& GetTrustCheckURL();

void SetUsingTrustCheck(bool useTrustCheck);
const bool IsUsingTrustCheck();

void SetContentPath(const std::string& path);
const std::string& GetContentPath(); // never used lol

void ReportStatisticWithMessage(const std::string& baseUrl, const std::string& id, const std::string& simpleMessage,
    const char* secondaryFilterName = NULL, const char* secondaryFilterValue = NULL);

void ReportStatistic(const std::string& baseUrl, const std::string& id, const std::string& primaryFilterName, const std::string& primaryFilterValue,
    const std::string& secondaryFilterName, const std::string& secondaryFilterValue);

void ReportStatisticPost(const std::string& baseUrl, const std::string& id, const std::string& postData, const char* secondaryFilterName,
    const char* secondaryFilterValue);


std::string UploadLogFile(const std::string& baseUrl, const std::string& data);

void SetFetchLocalClientSettings(bool status);
bool FetchLocalClientSettingsData(const char* group, SimpleJSON* dest);
void LoadClientSettingsFromString(const char* group, const std::string& settingsData, SimpleJSON* dest);
bool FetchClientSettingsData(const char* group, const char* apiKey, SimpleJSON* dest);
void FetchClientSettingsData(const char* group, const char* apiKey, std::string* dest);
Aya::HttpFuture FetchABTestDataAsync(const std::string& url);
std::string LoadABTestFromString(const std::string& responseData);