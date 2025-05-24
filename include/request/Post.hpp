#ifndef POST_HPP
#define POST_HPP

#include "./RequestHandler.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm> // For std::swap

class Post : public RequestHandler {
public:
    Post();
    ~Post();

    bool CanHandle(std::string method);
    void ProccessRequest(HttpRequest *request);

    static const size_t MAX_MEMORY_CHUNK = 1000000; // 1MB

private:
    struct FormPart {
        std::map<std::string, std::string> headers;
        std::string name;
        std::string filename;
        std::string body;
        bool isFile;
        std::string uploadId;
    };

    struct FileUpload {
        FileUpload() : fileStream(NULL), bytesReceived(0), complete(false) {}

        ~FileUpload() {
            if (fileStream) {
                if (fileStream->is_open()) {
                    fileStream->close();
                }
                delete fileStream;
            }
        }

        void swap(FileUpload& other) {
            std::swap(filename, other.filename);
            std::swap(tempPath, other.tempPath);
            std::swap(finalPath, other.finalPath);
            std::swap(fileStream, other.fileStream);
            std::swap(bytesReceived, other.bytesReceived);
            std::swap(complete, other.complete);
        }

        std::string filename;
        std::string tempPath;
        std::string finalPath;
        std::ofstream* fileStream; // Pointer to avoid copy issues
        long bytesReceived;
        bool complete;

    // Both copy constructor and assignment operator need to be public for std::map
    public:
        FileUpload(const FileUpload&) {} // Simple implementation
        FileUpload& operator=(const FileUpload&) { return *this; } // Simple implementation
    };

    // Private methods
    std::string extractBoundary(const std::string& contentType);
    void handleUrlEncodedForm(HttpRequest *request, const std::string &contentType);
    std::map<std::string, std::string> parseUrlEncodedForm(const std::string &body);
    std::string urlDecode(const std::string &encoded);
    void handleMultipartForm(HttpRequest *request, const std::string &contentType, const std::string &boundary);
    std::vector<FormPart> parseMultipartForm(const std::string &body, const std::string &boundary);
    void processFormPart(const FormPart &part, HttpRequest *request);
    void processChunkedFormPart(const FormPart &part, HttpRequest *request, bool isFinalChunk);
    void handleChunkedTransfer(HttpRequest *request);
    std::string getUploadsDirectory();
    std::string initializeFileUpload(const std::string &originalFilename);
    bool writeFileChunk(const std::string &uploadId, const std::string &chunk);
    bool finalizeFileUpload(const std::string &uploadId);
    std::string generateUniqueFilename(const std::string &originalName);
    bool saveUploadedFile(const std::string &content, const std::string &filename);
    void handleRawData(HttpRequest *request, const std::string &contentType);

    std::map<std::string, FileUpload> activeUploads;
};

#endif // POST_HPP