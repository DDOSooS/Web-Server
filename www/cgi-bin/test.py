#!/usr/bin/env python3

import sys
import os
import urllib.parse
import json
import datetime

def parse_form_data():
    """Parse GET/POST form data"""
    method = os.environ.get('REQUEST_METHOD', 'GET')
    form_data = {}
    raw_data = ""
    
    try:
        if method == 'POST':
            # Read POST data from stdin
            content_length = int(os.environ.get('CONTENT_LENGTH', 0))
            if content_length > 0:
                raw_data = sys.stdin.read(content_length)
                # Parse URL-encoded data
                parsed = urllib.parse.parse_qs(raw_data)
                for key, values in parsed.items():
                    form_data[key] = values[0] if values else ''
        
        elif method == 'GET':
            # Parse query string
            query_string = os.environ.get('QUERY_STRING', '')
            raw_data = query_string
            if query_string:
                parsed = urllib.parse.parse_qs(query_string)
                for key, values in parsed.items():
                    form_data[key] = values[0] if values else ''
    
    except Exception as e:
        form_data['error'] = str(e)
    
    return form_data, raw_data, method

def main():
    """Main CGI function"""
    
    # Parse form data
    form_data, raw_data, method = parse_form_data()
    
    # Output HTTP headers
    print("Content-Type: text/html")
    print("Access-Control-Allow-Origin: *")
    print("Access-Control-Allow-Methods: GET, POST, OPTIONS")
    print("Access-Control-Allow-Headers: Content-Type")
    print()  # Empty line - CRITICAL!
    
    # Generate HTML response
    print(f"""<!DOCTYPE html>
<html>
<head>
    <title>CGI POST Test Results</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
        }}
        .container {{
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        }}
        .success {{
            background: #d4edda;
            color: #155724;
            padding: 15px;
            border-radius: 8px;
            margin: 20px 0;
            border: 1px solid #c3e6cb;
        }}
        .info {{
            background: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            margin: 15px 0;
            border-left: 4px solid #007bff;
        }}
        .method-badge {{
            background: #007bff;
            color: white;
            padding: 4px 12px;
            border-radius: 15px;
            font-size: 14px;
            font-weight: bold;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }}
        th, td {{
            border: 1px solid #ddd;
            padding: 12px;
            text-align: left;
        }}
        th {{
            background: #f5f5f5;
            font-weight: bold;
        }}
        .json-box {{
            background: #2d3748;
            color: #e2e8f0;
            padding: 20px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            overflow-x: auto;
        }}
        .raw-data {{
            background: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            font-family: monospace;
            border: 1px solid #e9ecef;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 CGI POST Test Results</h1>
        
        <div class="success">
            <strong>✅ CGI Script Working Successfully!</strong><br>
            Your POST data has been received and parsed.
        </div>
        
        <div class="info">
            <strong>Request Method:</strong> <span class="method-badge">{method}</span><br>
            <strong>Timestamp:</strong> {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}<br>
            <strong>Python Version:</strong> {sys.version.split()[0]}<br>
            <strong>Content Length:</strong> {os.environ.get('CONTENT_LENGTH', '0')} bytes
        </div>""")
    
    # Display form data
    if form_data and 'error' not in form_data:
        print(f"""
        <h2>📝 Form Data Received ({len(form_data)} fields)</h2>
        <table>
            <thead>
                <tr>
                    <th>Field Name</th>
                    <th>Value</th>
                    <th>Length</th>
                </tr>
            </thead>
            <tbody>""")
        
        for key, value in form_data.items():
            print(f"""
                <tr>
                    <td><strong>{key}</strong></td>
                    <td>{value}</td>
                    <td>{len(str(value))} chars</td>
                </tr>""")
        
        print("""
            </tbody>
        </table>
        
        <h3>📄 JSON Format</h3>
        <div class="json-box">""")
        
        print(json.dumps(form_data, indent=2))
        
        print("""</div>""")
    
    elif 'error' in form_data:
        print(f"""
        <h2>❌ Error Parsing Form Data</h2>
        <div class="info">
            <strong>Error:</strong> {form_data['error']}
        </div>""")
    
    else:
        print("""
        <h2>📝 Form Data</h2>
        <div class="info">
            ℹ️ No form data received. Try sending POST data with -d parameter.
        </div>""")
    
    # Show raw data
    if raw_data:
        print(f"""
        <h3>🔍 Raw POST Data</h3>
        <div class="raw-data">{raw_data}</div>""")
    
    # Environment info
    print(f"""
        <h2>🔧 Environment Information</h2>
        <table>
            <tr><th>Variable</th><th>Value</th></tr>
            <tr><td>REQUEST_METHOD</td><td>{os.environ.get('REQUEST_METHOD', 'N/A')}</td></tr>
            <tr><td>CONTENT_TYPE</td><td>{os.environ.get('CONTENT_TYPE', 'N/A')}</td></tr>
            <tr><td>CONTENT_LENGTH</td><td>{os.environ.get('CONTENT_LENGTH', 'N/A')}</td></tr>
            <tr><td>QUERY_STRING</td><td>{os.environ.get('QUERY_STRING', 'N/A')}</td></tr>
            <tr><td>HTTP_HOST</td><td>{os.environ.get('HTTP_HOST', 'N/A')}</td></tr>
            <tr><td>HTTP_USER_AGENT</td><td>{os.environ.get('HTTP_USER_AGENT', 'N/A')}</td></tr>
            <tr><td>REMOTE_ADDR</td><td>{os.environ.get('REMOTE_ADDR', 'N/A')}</td></tr>
            <tr><td>SERVER_NAME</td><td>{os.environ.get('SERVER_NAME', 'N/A')}</td></tr>
        </table>
        
        <div style="margin-top: 30px; text-align: center; color: #6c757d; font-size: 14px;">
            <em>Generated by test.py CGI script</em>
        </div>
    </div>
</body>
</html>""")
    
    # Force output
    sys.stdout.flush()

if __name__ == "__main__":
    main()