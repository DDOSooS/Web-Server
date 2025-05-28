#!/bin/bash
# This script is a CGI script that returns a simple HTML page with a message.
echo "<html>"
echo "<head><title>Test CGI Script</title></head>"
echo "<body>"
echo "<h1>Test CGI Script</h1>"
echo "<p>This is a test CGI script running on the server.</p>"
echo "<p>Current date and time: $(date)</p>"
echo "<p>Server name: $(hostname)</p>"clear
echo "<p>Server IP address: $(hostname -I | awk '{print $1}')</p>"
echo "<p>Server software: $(cat /etc/os-release | grep PRETTY_NAME | cut -d '=' -f2)</p>"
echo "<p>CGI script executed successfully!</p>"
echo "</body>"
echo "</html>"
# End of script
# Make sure to set the correct permissions for this script to be executable:
# chmod 755 /path/to/test.cgi