#include "../../../include/request/Post.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

std::vector<Post::FormPart> Post::parseMultipartForm(const std::string &body, const std::string &boundary) {
    std::vector<FormPart> parts;
    
    std::cout << "Parsing multipart form with boundary: '" << boundary << "'" << std::endl;
    std::cout << "Body length: " << body.length() << " bytes" << std::endl;
    
    // DEBUG: Print the first 200 characters of the body to see the structure
    std::cout << "=== BODY PREVIEW (first 200 chars) ===" << std::endl;
    std::string preview = body.substr(0, std::min((size_t)200, body.size()));
    for (size_t i = 0; i < preview.length(); ++i) {
        char c = preview[i];
        if (c == '\r') std::cout << "\\r";
        else if (c == '\n') std::cout << "\\n";
        else if (c >= 32 && c <= 126) std::cout << c;
        else std::cout << "[" << (int)c << "]";
    }
    std::cout << std::endl << "=== END BODY PREVIEW ===" << std::endl;
    
    // Construct the boundary delimiters
    std::string startBoundary = "--" + boundary;
    std::string endBoundary = "--" + boundary + "--";
    
    std::cout << "Looking for start boundary: '" << startBoundary << "'" << std::endl;
    
    // Find the first boundary
    size_t pos = body.find(startBoundary);
    if (pos == std::string::npos) {
        std::cout << "Error: Could not find start boundary in body" << std::endl;
        
        // DEBUG: Try to find any boundary-like patterns
        std::cout << "Searching for any '--' patterns in first 500 chars:" << std::endl;
        std::string searchArea = body.substr(0, std::min((size_t)500, body.size()));
        size_t dashPos = 0;
        while ((dashPos = searchArea.find("--", dashPos)) != std::string::npos) {
            size_t lineEnd = searchArea.find('\n', dashPos);
            if (lineEnd == std::string::npos) lineEnd = searchArea.length();
            std::string line = searchArea.substr(dashPos, std::min((size_t)50, lineEnd - dashPos));
            std::cout << "Found '--' at pos " << dashPos << ": '" << line << "'" << std::endl;
            dashPos++;
        }
        return parts;
    }
    
    std::cout << "Found first boundary at position: " << pos << std::endl;
    
    // Skip the first boundary and any following CRLF
    pos += startBoundary.length();
    if (pos < body.length() && body[pos] == '\r') pos++;
    if (pos < body.length() && body[pos] == '\n') pos++;
    
    int partNumber = 1;
    
    while (pos < body.length()) {
        std::cout << "\n=== Processing Part " << partNumber << " ===" << std::endl;
        
        // Find the next boundary (could be start or end boundary)
        size_t nextBoundaryPos = body.find("\r\n--" + boundary, pos);
        size_t altNextBoundaryPos = body.find("\n--" + boundary, pos);
        
        // Use whichever boundary we find first
        if (nextBoundaryPos == std::string::npos || 
            (altNextBoundaryPos != std::string::npos && altNextBoundaryPos < nextBoundaryPos)) {
            nextBoundaryPos = altNextBoundaryPos;
            if (nextBoundaryPos != std::string::npos) {
                nextBoundaryPos += 1; // Skip \n
            }
        } else if (nextBoundaryPos != std::string::npos) {
            nextBoundaryPos += 2; // Skip \r\n
        }
        
        if (nextBoundaryPos == std::string::npos) {
            std::cout << "Could not find next boundary, breaking" << std::endl;
            break;
        }
        
        // Extract this part's content
        std::string partContent = body.substr(pos, nextBoundaryPos - pos);
        std::cout << "Part content length: " << partContent.length() << " bytes" << std::endl;
        
        // Find headers/body separator
        size_t headerEndPos = partContent.find("\r\n\r\n");
        bool foundCRLF = true;
        if (headerEndPos == std::string::npos) {
            headerEndPos = partContent.find("\n\n");
            foundCRLF = false;
            if (headerEndPos == std::string::npos) {
                std::cout << "Could not find header/body separator in part" << std::endl;
                pos = nextBoundaryPos + boundary.length() + 2;
                partNumber++;
                continue;
            }
        }
        
        // Extract headers and body
        std::string headers = partContent.substr(0, headerEndPos);
        std::string partBody = partContent.substr(headerEndPos + (foundCRLF ? 4 : 2));
        
        std::cout << "Headers: " << headers << std::endl;
        std::cout << "Body length: " << partBody.length() << " bytes" << std::endl;
        
        // Parse the Content-Disposition header
        FormPart part;
        part.body = partBody;
        part.isFile = false;
        
        // Process headers line by line
        std::istringstream headerStream(headers);
        std::string headerLine;
        
        while (std::getline(headerStream, headerLine)) {
            // Remove trailing \r if present
            if (!headerLine.empty() && headerLine.back() == '\r') {
                headerLine.pop_back();
            }
            
            std::cout << "Processing header: '" << headerLine << "'" << std::endl;
            
            if (headerLine.find("Content-Disposition:") == 0) {
                // Extract name
                size_t namePos = headerLine.find("name=\"");
                if (namePos != std::string::npos) {
                    namePos += 6; // Skip name="
                    size_t nameEnd = headerLine.find("\"", namePos);
                    if (nameEnd != std::string::npos) {
                        part.name = headerLine.substr(namePos, nameEnd - namePos);
                        std::cout << "Extracted name: '" << part.name << "'" << std::endl;
                    }
                }
                
                // Extract filename
                size_t filenamePos = headerLine.find("filename=\"");
                if (filenamePos != std::string::npos) {
                    filenamePos += 10; // Skip filename="
                    size_t filenameEnd = headerLine.find("\"", filenamePos);
                    if (filenameEnd != std::string::npos) {
                        part.filename = headerLine.substr(filenamePos, filenameEnd - filenamePos);
                        part.isFile = !part.filename.empty();
                        std::cout << "Extracted filename: '" << part.filename << "'" << std::endl;
                    }
                }
            } 
            else if (headerLine.find("Content-Type:") == 0) {
                part.contentType = headerLine.substr(13); // Skip "Content-Type:"
                
                // Trim whitespace
                size_t start = part.contentType.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    part.contentType = part.contentType.substr(start);
                    size_t end = part.contentType.find_last_not_of(" \t\r\n");
                    if (end != std::string::npos) {
                        part.contentType = part.contentType.substr(0, end + 1);
                    }
                }
                std::cout << "Extracted content type: '" << part.contentType << "'" << std::endl;
            }
        }
        
        // Only add parts that have a name or filename
        if (!part.name.empty() || !part.filename.empty()) {
            parts.push_back(part);
            std::cout << "Added part: name='" << part.name << "', filename='" << part.filename 
                     << "', isFile=" << (part.isFile ? "true" : "false") 
                     << ", body size=" << part.body.size() << std::endl;
        } else {
            std::cout << "Skipping part with no name or filename" << std::endl;
        }
        
        // Move to next part
        pos = nextBoundaryPos + 2 + boundary.length(); // Skip the boundary
        
        // Check if this is the end boundary
        std::string potentialEndBoundary = body.substr(nextBoundaryPos, endBoundary.length());
        if (potentialEndBoundary == endBoundary) {
            std::cout << "Found end boundary, stopping parsing" << std::endl;
            break;
        }
        
        // Skip any CRLF after boundary
        if (pos < body.length() && body[pos] == '\r') pos++;
        if (pos < body.length() && body[pos] == '\n') pos++;
        
        partNumber++;
        
        // Safety check to prevent infinite loops
        if (partNumber > 100) {
            std::cout << "Too many parts, breaking to prevent infinite loop" << std::endl;
            break;
        }
    }
    
    std::cout << "Total valid parts found: " << parts.size() << std::endl;
    return parts;
}