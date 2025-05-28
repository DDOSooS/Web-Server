#write a script that will read the post data from a form and print it to the screen & append it to a json file
#!/usr/bin/env python3
import cgi
import cgitb
import json
import os
cgitb.enable()  # Enable CGI error reporting
# Set the content type to HTML
print("Content-Type: text/html")  # HTML is following               
print()  # Blank line required to end headers
# body
print("<html>")
print("<head>")
print("<title>Test POST Data</title>")
print("</head>")
print("<body>")
print("<h1>Received POST Data</h1>")
# Create a FieldStorage instance to read the form data
form = cgi.FieldStorage()
# Initialize a dictionary to store the form data
data = {}
# Iterate through the form data and store it in the dictionary
for key in form.keys():
    value = form.getvalue(key)
    data[key] = value
    print(f"<p><strong>{key}:</strong> {value}</p>")
# Append the data to a JSON file
json_file_path = 'data.json'
# Check if the file exists, if not create it
if not os.path.exists(json_file_path):
    with open(json_file_path, 'w') as json_file:
        json.dump([], json_file)  # Initialize with an empty list
# Read the existing data from the JSON file
with open(json_file_path, 'r') as json_file:
    existing_data = json.load(json_file)
# Append the new data to the existing data
existing_data.append(data)
# Write the updated data back to the JSON file
with open(json_file_path, 'w') as json_file:
    json.dump(existing_data, json_file, indent=4)
print("<h2>Data has been saved to data.json</h2>")
print("</body>")
print("</html>")
# This script reads POST data from a form, prints it to the screen, and appends it to a JSON file.
# It uses the cgi module to handle form data and the json module to read/write JSON files.
# The script also uses cgitb to enable error reporting for CGI scripts.
# The data is displayed in a simple HTML format, and the JSON file is updated with the new data.
# The JSON file is initialized with an empty list if it does not exist.
# The script ensures that the data is stored in a structured format for future use.
# The script outputs HTML content to confirm that the data has been saved successfully.
# The script is designed to be run as a CGI script on a web server.
# The script can be tested by submitting a form with various fields to see how the data is processed and stored.            
