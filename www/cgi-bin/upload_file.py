#!/usr/bin/env python3
"""
Fixed File Upload Script - Compatible with Python 3.8+
No deprecated cgi module required
"""

import os
import sys
import traceback
from urllib.parse import unquote_plus

def debug_log(message):
    """Log debug messages to file"""
    try:
        with open('./tmp/webserv_upload_debug.log', 'a') as f:
            import time
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
            f.write(f"[{timestamp}] {message}\n")
            f.flush()
    except:
        pass

class MultipartParser:
    """
    Parse multipart/form-data without using deprecated cgi module
    """
    def __init__(self, data, boundary):
        self.data = data
        self.boundary = boundary.encode() if isinstance(boundary, str) else boundary
        self.files = {}
        self.fields = {}
        self._parse()
    
    def _parse(self):
        """Parse multipart data into fields and files"""
        try:
            # Split data by boundary
            boundary_separator = b'--' + self.boundary
            end_boundary = b'--' + self.boundary + b'--'
            
            parts = self.data.split(boundary_separator)
            
            for part in parts:
                if not part or part == b'\r\n' or part == b'\n' or part.startswith(b'--'):
                    continue
                
                # Remove leading/trailing whitespace
                part = part.strip()
                if not part:
                    continue
                
                # Find headers/body separator
                if b'\r\n\r\n' in part:
                    headers_data, body = part.split(b'\r\n\r\n', 1)
                elif b'\n\n' in part:
                    headers_data, body = part.split(b'\n\n', 1)
                else:
                    continue
                
                # Parse headers
                headers = self._parse_headers(headers_data)
                content_disposition = headers.get('content-disposition', '')
                
                # Extract field name
                field_name = self._extract_field_name(content_disposition)
                if not field_name:
                    continue
                
                # Check if it's a file field
                filename = self._extract_filename(content_disposition)
                
                if filename:
                    # It's a file upload
                    debug_log(f"Found file field: {field_name}, filename: {filename}")
                    self.files[field_name] = {
                        'filename': filename,
                        'content': body,
                        'content_type': headers.get('content-type', 'application/octet-stream')
                    }
                else:
                    # It's a regular form field
                    try:
                        field_value = body.decode('utf-8')
                        self.fields[field_name] = field_value
                        debug_log(f"Found form field: {field_name} = {field_value}")
                    except:
                        debug_log(f"Failed to decode form field: {field_name}")
        
        except Exception as e:
            debug_log(f"Error parsing multipart data: {e}")
    
    def _parse_headers(self, headers_data):
        """Parse HTTP headers"""
        headers = {}
        try:
            headers_str = headers_data.decode('utf-8', errors='ignore')
            for line in headers_str.strip().split('\n'):
                if ':' in line:
                    key, value = line.split(':', 1)
                    headers[key.strip().lower()] = value.strip()
        except Exception as e:
            debug_log(f"Error parsing headers: {e}")
        return headers
    
    def _extract_field_name(self, content_disposition):
        """Extract field name from Content-Disposition header"""
        try:
            for part in content_disposition.split(';'):
                part = part.strip()
                if part.startswith('name='):
                    name = part.split('=', 1)[1].strip()
                    if name.startswith('"') and name.endswith('"'):
                        name = name[1:-1]
                    return name
        except:
            pass
        return None
    
    def _extract_filename(self, content_disposition):
        """Extract filename from Content-Disposition header"""
        try:
            for part in content_disposition.split(';'):
                part = part.strip()
                if part.startswith('filename='):
                    filename = part.split('=', 1)[1].strip()
                    if filename.startswith('"') and filename.endswith('"'):
                        filename = filename[1:-1]
                    return filename if filename else None
        except:
            pass
        return None

def parse_multipart_from_file(file_path, content_type):
    """Parse multipart data from a file (for streaming uploads)"""
    try:
        debug_log(f"Parsing multipart from file: {file_path}")
        
        with open(file_path, 'rb') as f:
            data = f.read()
        
        debug_log(f"Read {len(data)} bytes from file")
        
        # Extract boundary from content type
        boundary = None
        if 'boundary=' in content_type:
            boundary = content_type.split('boundary=')[1].strip()
            if boundary.startswith('"') and boundary.endswith('"'):
                boundary = boundary[1:-1]
        
        if not boundary:
            return None, "No boundary found in Content-Type"
        
        debug_log(f"Using boundary: {boundary}")
        
        # Parse multipart data
        parser = MultipartParser(data, boundary)
        
        # Look for file fields
        for field_name, file_info in parser.files.items():
            filename = file_info['filename']
            content = file_info['content']
            
            if filename and content:
                debug_log(f"Found file: {filename}, size: {len(content)} bytes")
                return filename, content
        
        return None, "No file field found"
        
    except Exception as e:
        debug_log(f"Error parsing multipart file: {e}")
        return None, f"Error parsing multipart data: {str(e)}"

def parse_multipart_from_stdin(content_type, content_length):
    """Parse multipart data from stdin (traditional CGI)"""
    try:
        debug_log(f"Parsing multipart from stdin, length: {content_length}")
        
        # Read data from stdin
        data = sys.stdin.buffer.read(int(content_length))
        debug_log(f"Read {len(data)} bytes from stdin")
        
        # Extract boundary
        boundary = None
        if 'boundary=' in content_type:
            boundary = content_type.split('boundary=')[1].strip()
            if boundary.startswith('"') and boundary.endswith('"'):
                boundary = boundary[1:-1]
        
        if not boundary:
            return None, "No boundary found in Content-Type"
        
        debug_log(f"Using boundary: {boundary}")
        
        # Parse multipart data
        parser = MultipartParser(data, boundary)
        
        # Look for file fields (usually named 'filename' or 'file')
        for field_name in ['filename', 'file', 'upload']:
            if field_name in parser.files:
                file_info = parser.files[field_name]
                filename = file_info['filename']
                content = file_info['content']
                
                if filename and content:
                    debug_log(f"Found file: {filename}, size: {len(content)} bytes")
                    return filename, content
        
        # If no standard field names, try the first file found
        if parser.files:
            field_name = list(parser.files.keys())[0]
            file_info = parser.files[field_name]
            filename = file_info['filename']
            content = file_info['content']
            
            if filename and content:
                debug_log(f"Found file in field {field_name}: {filename}, size: {len(content)} bytes")
                return filename, content
        
        return None, "No file field found"
        
    except Exception as e:
        debug_log(f"Error parsing multipart from stdin: {e}")
        return None, f"Error parsing form data: {str(e)}"

def sanitize_filename(filename):
    """Sanitize filename for security"""
    if not filename:
        return None
    
    # Remove path components for security
    safe_filename = os.path.basename(filename)
    
    # Remove potentially dangerous characters
    safe_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
    safe_filename = ''.join(c for c in safe_filename if c in safe_chars)
    
    # Ensure it's not empty and doesn't start with a dot
    if not safe_filename or safe_filename.startswith('.'):
        safe_filename = 'uploaded_file'
    
    return safe_filename

def send_response(message, status="200 OK"):
    """Send HTTP response"""
    debug_log(f"Sending response: {status}")
    print(f"Status: {status}")
    print("Content-Type: text/html; charset=utf-8")
    print()  # Empty line to end headers
    
    html = f"""<!DOCTYPE html>
<html>
<head>
    <title>File Upload Result</title>
    <meta charset="utf-8">
    <style>
        body {{ font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }}
        .success {{ color: green; background: #e6ffe6; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .error {{ color: red; background: #ffe6e6; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .info {{ color: blue; background: #e6f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        a {{ color: #007cba; text-decoration: none; }}
    </style>
</head>
<body>
    <h1>📁 File Upload Result</h1>
    
    <div class="{'success' if 'successfully' in message else 'error' if 'Error' in message or 'error' in message else 'info'}">
        <p>{message}</p>
    </div>
    
    <p><a href="/cgi-bin/">← Back to upload form</a></p>
    
    <hr>
    <p><em>Debug log: ./tmp/webserv_upload_debug.log</em></p>
</body>
</html>"""
    
    print(html)
    debug_log("Response sent")

def main():
    """Main CGI script entry point"""
    try:
        debug_log("=" * 50)
        debug_log("FILE UPLOAD CGI START")
        debug_log("=" * 50)
        
        # Read environment variables
        request_method = os.environ.get('REQUEST_METHOD', '')
        content_type = os.environ.get('CONTENT_TYPE', '')
        content_length = os.environ.get('CONTENT_LENGTH', '0')
        
        debug_log(f"Request method: {request_method}")
        debug_log(f"Content type: {content_type}")
        debug_log(f"Content length: {content_length}")
        
        # Validate request
        if request_method != 'POST':
            send_response('❌ Error: Only POST method is supported')
            return
        
        if not content_type.startswith('multipart/form-data'):
            send_response('❌ Error: Only multipart/form-data is supported')
            return
        
        if content_length == '0':
            send_response('❌ Error: No data received')
            return
        
        # Check for streaming upload marker
        try:
            # Try to read from stdin first to check for streaming marker
            stdin_data = sys.stdin.read()
            debug_log(f"Stdin data: {stdin_data[:100]}...")  # Log first 100 chars
            
            if stdin_data.startswith('__STREAMING_UPLOAD_FILE:'):
                # Handle streaming upload
                debug_log("Detected streaming upload")
                temp_file_path = stdin_data.replace('__STREAMING_UPLOAD_FILE:', '').strip()
                debug_log(f"Streaming file path: {temp_file_path}")
                
                filename, file_content = parse_multipart_from_file(temp_file_path, content_type)
                
                if filename and file_content:
                    # Sanitize filename
                    safe_filename = sanitize_filename(filename)
                    debug_log(f"Sanitized filename: {safe_filename}")
                    
                    # Save the file
                    output_path = os.path.join('./tmp', safe_filename)
                    with open(output_path, 'wb') as f:
                        f.write(file_content)
                    
                    debug_log(f"File saved to: {output_path}")
                    send_response(f'✅ The file "{safe_filename}" was uploaded successfully ({len(file_content)} bytes at ({output_path})')
                    
                    # Clean up the temporary file
                    try:
                        os.unlink(temp_file_path)
                        debug_log(f"Cleaned up temp file: {temp_file_path}")
                    except:
                        debug_log(f"Failed to clean up temp file: {temp_file_path}")
                else:
                    error_msg = file_content if isinstance(file_content, str) else "Invalid file data"
                    send_response(f'❌ Error processing streaming upload: {error_msg}')
            
            else:
                # Handle traditional form upload
                debug_log("Processing traditional form upload")
                
                # Reset stdin and parse multipart data
                sys.stdin = sys.__stdin__  # Reset stdin
                
                filename, file_content = parse_multipart_from_stdin(content_type, content_length)
                
                if filename and file_content:
                    # Sanitize filename
                    safe_filename = sanitize_filename(filename)
                    debug_log(f"Sanitized filename: {safe_filename}")
                    
                    # Save the file
                    output_path = os.path.join('./tmp', safe_filename)
                    with open(output_path, 'wb') as f:
                        f.write(file_content)
                    
                    debug_log(f"File saved to: {output_path}")
                    send_response(f'✅ The file "{safe_filename}" was uploaded successfully ({len(file_content)} bytes at ({output_path})')
                else:
                    error_msg = file_content if isinstance(file_content, str) else "No file data found"
                    send_response(f'❌ Error processing upload: {error_msg}')
        
        except Exception as e:
            debug_log(f"Error in upload processing: {e}")
            debug_log(f"Traceback: {traceback.format_exc()}")
            send_response(f'❌ Error processing upload: {str(e)}')
    
    except Exception as e:
        debug_log(f"Unexpected error: {e}")
        debug_log(f"Traceback: {traceback.format_exc()}")
        send_response(f'❌ Unexpected error: {str(e)}', "500 Internal Server Error")
    
    finally:
        debug_log("=" * 50)
        debug_log("FILE UPLOAD CGI END")
        debug_log("=" * 50)

if __name__ == "__main__":
    main()