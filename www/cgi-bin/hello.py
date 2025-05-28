#!/usr/bin/env python3
import cgi
import cgitb
cgitb.enable()  # Enable CGI error reporting
import os
# Set the content type to HTML



# body
print("Content-Type: text/html")  # HTML is following
print()  # Blank line to end headers
print("<html>")
print("<head>")
print("<title>Hello CGI</title>")
print("</head>")
print("<body>")
print("<h1>Hello, CGI!</h1>")
print("<p>This is a simple CGI script.</p>")
print("<p>Environment Variables:</p>")
print("<ul>")
# Print environment variables
for key, value in os.environ.items():
    print(f"<li><strong>{key}:</strong> {value}</li>")
print("</ul>")
print("</body>")
print("</html>")
# This is a simple CGI script that prints "Hello, CGI!" and some environment variables.
# It uses the cgitb module to enable error reporting for CGI scripts.
# The script outputs HTML content and lists environment variables in an unordered list.