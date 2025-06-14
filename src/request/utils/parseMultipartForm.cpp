#include "../../../include/request/Post.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

std::vector<Post::FormPart> Post::parseMultipartForm(const std::string &body, const std::string &boundary) {
    std::vector<FormPart> parts;
    std::string delimiter = "--" + boundary;
    std::string endDelimiter = delimiter + "--";
    
    std::cout << "Parsing with delimiter: '" << delimiter << "'" << std::endl;
    std::cout << "Body length: " << body.size() << " bytes" << std::endl;
    
    size_t pos = body.find(delimiter);
    if (pos == std::string::npos) {
        return parts;
    }
    
    size_t currentPos = pos;
    int partNumber = 0;
    
    while (currentPos < body.size()) {
        partNumber++;
        std::cout << "\n=== Processing Part " << partNumber << " ===" << std::endl;
        
        // Find the start of the part after the delimiter
        size_t partStart = currentPos + delimiter.length();
        
        // Check if we've reached the end delimiter
        if (partStart + 2 <= body.size() && body.substr(partStart, 2) == "--") {
            break; // End delimiter found
        }
        
        // Skip CRLF after the boundary
        if (partStart + 2 <= body.size() && body.substr(partStart, 2) == "\r\n") {
            partStart += 2;
        }
        
        // Find the next boundary
        size_t nextBoundaryPos = body.find(delimiter, partStart);
        if (nextBoundaryPos == std::string::npos) {
            break; // No more boundaries found
        }
        
        // Extract part content (including headers)
        std::string partContent = body.substr(partStart, nextBoundaryPos - partStart);
        
        // Remove trailing CRLF before the next boundary
        if (partContent.size() >= 2 && partContent.substr(partContent.size() - 2) == "\r\n") {
            partContent = partContent.substr(0, partContent.size() - 2);
        }
        
        // Find the separator between headers and body
        size_t headerEnd = partContent.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            currentPos = nextBoundaryPos;
            continue; // Invalid part, no header/body separator
        }
        
        // Extract headers and body
        std::string headers = partContent.substr(0, headerEnd);
        
        FormPart part;
        part.body = partContent.substr(headerEnd + 4); // Skip \r\n\r\n
        
        // Parse headers
        std::istringstream headerStream(headers);
        std::string headerLine;
        
        while (std::getline(headerStream, headerLine)) {
            // Remove trailing CR if present
            if (!headerLine.empty() && headerLine[headerLine.length() - 1] == '\r') {
                headerLine = headerLine.substr(0, headerLine.length() - 1);
            }
            
            if (headerLine.find("Content-Disposition:") == 0) {
                // Parse name field
                size_t namePos = headerLine.find("name=\"");
                if (namePos != std::string::npos) {
                    namePos += 6; // Skip 'name="'
                    size_t nameEnd = headerLine.find("\"", namePos);
                    if (nameEnd != std::string::npos) {
                        part.name = headerLine.substr(namePos, nameEnd - namePos);
                        std::cout << "Extracted name: '" << part.name << "'" << std::endl;
                    }
                }
                
                // Parse filename field
                size_t filenamePos = headerLine.find("filename=\"");
                if (filenamePos != std::string::npos) {
                    filenamePos += 10; // Skip 'filename="'
                    size_t filenameEnd = headerLine.find("\"", filenamePos);
                    if (filenameEnd != std::string::npos) {
                        part.filename = headerLine.substr(filenamePos, filenameEnd - filenamePos);
                        part.isFile = !part.filename.empty();
                        std::cout << "Extracted filename: '" << part.filename << "'" << std::endl;
                    }
                }
            } else if (headerLine.find("Content-Type:") == 0) {
                part.contentType = headerLine.substr(13); // Skip "Content-Type:"
                
                // Trim whitespace
                size_t start = part.contentType.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    part.contentType = part.contentType.substr(start);
                    size_t end = part.contentType.find_last_not_of(" \t");
                    if (end != std::string::npos) {
                        part.contentType = part.contentType.substr(0, end + 1);
                    }
                }
                std::cout << "Extracted content type: '" << part.contentType << "'" << std::endl;
            }
        }
        
        // Add part if it has valid data
        if (!part.name.empty() || !part.filename.empty()) {
            parts.push_back(part);
            std::cout << "Added part: name='" << part.name << "', filename='" << part.filename 
                     << "', isFile=" << (part.isFile ? "true" : "false") 
                     << ", body size=" << part.body.size() << std::endl;
        }
        
        // Move to next boundary
        currentPos = nextBoundaryPos;
    }
    
    std::cout << "Total valid parts found: " << parts.size() << std::endl;
    return parts;
}