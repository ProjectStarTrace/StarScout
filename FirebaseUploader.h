// FirebaseUploader.h

#ifndef FIREBASE_UPLOADER_H
#define FIREBASE_UPLOADER_H

#include "external_libraries/json.hpp" // Include the path to the nlohmann/json.hpp library

using json = nlohmann::json;
#include <string>

class FirebaseUploader {
public:
    FirebaseUploader(const std::string& projectId, const std::string& collection);
    void uploadData(const json& data, const std::string& accessToken);

private:
    std::string _projectId;
    std::string _collection;
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);
};

#endif // FIREBASE_UPLOADER_H
