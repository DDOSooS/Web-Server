#!/bin/bash

# Test script for HTTP redirects with POST data
# This script will send POST requests to test the redirect functionality

echo "=== HTTP Redirect Test Script ==="
echo "This script will test different types of redirects with POST data"
echo

# Define colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test data
TEST_DATA="name=Test+User&message=This+is+a+test+message"
CONTENT_TYPE="application/x-www-form-urlencoded"
SERVER="http://localhost:8080"

# Function to test a specific redirect type
test_redirect() {
    local redirect_type=$1
    local endpoint="/test/redirect${redirect_type}"
    local full_url="${SERVER}${endpoint}"
    
    echo -e "${BLUE}Testing ${redirect_type} redirect${NC}"
    echo "POST ${full_url}"
    echo "Data: ${TEST_DATA}"
    echo
    
    # Use curl with -v for verbose output and -L to follow redirects
    # -i includes response headers in the output
    curl -v -L -i -X POST \
        -H "Content-Type: ${CONTENT_TYPE}" \
        -d "${TEST_DATA}" \
        "${full_url}" 2>&1 | grep -E '(< HTTP|< Location:|< Content-Type:|< Content-Length:|POST|GET)'
    
    echo -e "${GREEN}Test completed${NC}"
    echo "------------------------------------------------"
    echo
}

# Main test execution
echo -e "${YELLOW}Starting web server with test configuration...${NC}"
echo "Make sure your web server is running with the redirect_test.config configuration"
echo

# Run tests for different redirect types
test_redirect "307"
test_redirect "308"
test_redirect "302"

echo -e "${GREEN}All tests completed!${NC}"
echo "Check the output above to verify if the POST data was preserved during redirects."
echo "For 307 and 308 redirects, the method should remain POST and the body should be preserved."
echo "For 302 redirects, the method typically changes to GET and the body is discarded."
