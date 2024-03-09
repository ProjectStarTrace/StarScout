// FirebaseUploader.cpp

#include "FirebaseUploader.h"
#include <curl/curl.h>
#include <iostream>
#include <cstddef>

FirebaseUploader::FirebaseUploader(const std::string& projectId, const std::string& collection, const std::string& scoutID, const std::string& currentDateTime)
    : _projectId(projectId), _collection(collection), _scoutID(scoutID),  _currentDateTime(currentDateTime) {} // Assume _scoutID is declared in the header

size_t FirebaseUploader::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void FirebaseUploader::uploadData(const json& data, const std::string& accessToken) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    // Constructing Firestore REST API URL for documents in a collection
    std::string url = "https://firestore.googleapis.com/v1/projects/" + _projectId + "/databases/(default)/documents/" + _collection;
    std::cout << "Request URL: " << url << std::endl;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + accessToken; // Authorization header with the access token
    headers = curl_slist_append(headers, authHeader.c_str());
    std::string jsonPayload = data.dump(); //to get into the correct format

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST"); // Use POST to add a new document
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FirebaseUploader::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << '\n';
        } else {
            std::cout << "Upload successful: " << readBuffer << '\n';
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}
