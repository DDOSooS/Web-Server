#!/usr/bin/env python3

import sys
import time
import os
import cgi

def main():
    # CGI content type header
    print("Content-Type: text/html")
    print()  # Empty line required to end headers
    
    # HTML start
    print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Streaming Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { color: #333; border-bottom: 2px solid #333; padding-bottom: 10px; }
        .info { background: #f0f0f0; padding: 15px; margin: 10px 0; border-radius: 5px; }
        .stream { background: #e8f5e8; padding: 10px; margin: 5px 0; border-left: 4px solid #4CAF50; }
        .error { background: #ffe8e8; padding: 10px; margin: 5px 0; border-left: 4px solid #f44336; }
        .footer { margin-top: 30px; padding-top: 20px; border-top: 1px solid #ccc; color: #666; }
    </style>
</head>
<body>""")
    
    print('<h1 class="header">🚀 CGI Direct Streaming Test</h1>')
    
    # Show environment info
    print('<div class="info">')
    print('<h3>📋 Request Information:</h3>')
    print(f'<p><strong>Method:</strong> {os.environ.get("REQUEST_METHOD", "Unknown")}</p>')
    print(f'<p><strong>Script:</strong> {os.environ.get("SCRIPT_NAME", "Unknown")}</p>')
    print(f'<p><strong>Query String:</strong> {os.environ.get("QUERY_STRING", "None")}</p>')
    print(f'<p><strong>Content Length:</strong> {os.environ.get("CONTENT_LENGTH", "0")}</p>')
    print(f'<p><strong>Server:</strong> {os.environ.get("SERVER_SOFTWARE", "Unknown")}</p>')
    print(f'<p><strong>Client IP:</strong> {os.environ.get("REMOTE_ADDR", "Unknown")}</p>')
    print('</div>')
    
    # Handle POST data if present
    if os.environ.get("REQUEST_METHOD") == "POST":
        try:
            content_length = int(os.environ.get("CONTENT_LENGTH", 0))
            if content_length > 0:
                post_data = sys.stdin.read(content_length)
                print('<div class="info">')
                print('<h3>📨 POST Data Received:</h3>')
                print(f'<pre>{post_data}</pre>')
                
                # Parse form data
                form = cgi.FieldStorage()
                if form:
                    print('<h4>Parsed Form Fields:</h4>')
                    for key in form.keys():
                        value = form[key].value
                        print(f'<p><strong>{key}:</strong> {value}</p>')
                print('</div>')
        except Exception as e:
            print(f'<div class="error">Error reading POST data: {e}</div>')
    
    # Streaming demonstration
    print('<h3>🌊 Streaming Demonstration:</h3>')
    print('<p>Watch this content appear progressively (should start immediately):</p>')
    
    # Force output of everything so far
    sys.stdout.flush()
    
    # Stream content with delays
    for i in range(8):
        print(f'<div class="stream">')
        print(f'<strong>Stream Chunk #{i+1}</strong><br>')
        print(f'Timestamp: {time.strftime("%Y-%m-%d %H:%M:%S")}<br>')
        print(f'Process ID: {os.getpid()}<br>')
        if i == 0:
            print('✅ <em>If you see this immediately, streaming is working!</em><br>')
        elif i == 3:
            print('🔄 <em>Halfway through the streaming test...</em><br>')
        elif i == 7:
            print('🎉 <em>Streaming test completed successfully!</em><br>')
        
        # Simulate some processing time
        processing_time = 0.5 + (i * 0.1)  # Gradually increase delay
        print(f'Processing for {processing_time:.1f} seconds...')
        print('</div>')
        
        # Force immediate output
        sys.stdout.flush()
        
        # Sleep to demonstrate streaming
        time.sleep(processing_time)
    
    # Performance test section
    print('<h3>⚡ Performance Test:</h3>')
    print('<div class="info">')
    
    # Generate some data quickly
    start_time = time.time()
    data_size = 0
    
    for i in range(50):
        line = f'<p>Performance line {i+1}: {"Data " * 20}</p>'
        print(line)
        data_size += len(line)
        
        # Flush every 10 lines
        if i % 10 == 9:
            sys.stdout.flush()
    
    end_time = time.time()
    processing_time = end_time - start_time
    
    print(f'<p><strong>Generated {data_size} bytes in {processing_time:.3f} seconds</strong></p>')
    print(f'<p><strong>Rate: {data_size/processing_time:.0f} bytes/second</strong></p>')
    print('</div>')
    
    # Final summary
    print('<div class="footer">')
    print('<h3>📊 Test Summary:</h3>')
    print('<p>✅ <strong>Headers sent:</strong> Immediately at start</p>')
    print('<p>✅ <strong>Content streaming:</strong> Progressive delivery</p>')
    print('<p>✅ <strong>POST handling:</strong> Working (if POST data sent)</p>')
    print('<p>✅ <strong>Environment variables:</strong> Properly set</p>')
    print('<p>✅ <strong>Performance:</strong> No temporary files used</p>')
    print('<p><em>If you can see this, CGI direct streaming is working perfectly!</em></p>')
    print(f'<p><small>Test completed at: {time.strftime("%Y-%m-%d %H:%M:%S")}</small></p>')
    print('</div>')
    
    print('</body></html>')
    
    # Final flush
    sys.stdout.flush()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("Content-Type: text/html")
        print()
        print(f"<html><body><h1>CGI Error</h1><p>Error: {e}</p></body></html>")
        sys.stderr.write(f"CGI Error: {e}\n")