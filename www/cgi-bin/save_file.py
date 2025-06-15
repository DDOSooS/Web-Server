#!/usr/bin/python3

import cgi
import cgitb
import os
import sys
import tempfile
import email
from email.message import EmailMessage

# Enable CGI error reporting
cgitb.enable()

def parse_multipart_from_file(file_path, content_type):
    """Parse multipart data from a file"""
    try:
        with open(file_path, 'rb') as f:
            # Read the multipart data
            data = f.read()
        
        # Extract boundary from content type
        boundary = None
        if 'boundary=' in content_type:
            boundary = content_type.split('boundary=')[1].strip()
            if boundary.startswith('"') and boundary.endswith('"'):
                boundary = boundary[1:-1]
        
        if not boundary:
            return None, "No boundary found in Content-Type"
        
        # Parse multipart data manually
        boundary_bytes = ('--' + boundary).encode()
        end_boundary_bytes = ('--' + boundary + '--').encode()
        
        parts = data.split(boundary_bytes)
        
        for part in parts:
            if not part or part == b'--\r\n' or part == b'--':
                continue
                
            # Skip the end boundary
            if part.startswith(b'--'):
                continue
            
            # Find the headers and content separator
            if b'\r\n\r\n' in part:
                headers_bytes, content = part.split(b'\r\n\r\n', 1)
            elif b'\n\n' in part:
                headers_bytes, content = part.split(b'\n\n', 1)
            else:
                continue
            
            # Parse headers
            headers_str = headers_bytes.decode('utf-8', errors='ignore')
            headers = {}
            
            for line in headers_str.strip().split('\n'):
                if ':' in line:
                    key, value = line.split(':', 1)
                    headers[key.strip().lower()] = value.strip()
            
            # Check if this is a file upload field
            content_disposition = headers.get('content-disposition', '')
            if 'name="filename"' in content_disposition and 'filename=' in content_disposition:
                # Extract filename
                filename = None
                for param in content_disposition.split(';'):
                    param = param.strip()
                    if param.startswith('filename='):
                        filename = param.split('=', 1)[1].strip()
                        if filename.startswith('"') and filename.endswith('"'):
                            filename = filename[1:-1]
                        break
                
                if filename:
                    # Remove trailing \r\n from content
                    if content.endswith(b'\r\n'):
                        content = content[:-2]
                    elif content.endswith(b'\n'):
                        content = content[:-1]
                    
                    return filename, content
        
        return None, "No file field found"
        
    except Exception as e:
        return None, f"Error parsing multipart data: {str(e)}"

def main():
    try:
        # Read environment variables
        request_method = os.environ.get('REQUEST_METHOD', '')
        content_type = os.environ.get('CONTENT_TYPE', '')
        content_length = os.environ.get('CONTENT_LENGTH', '0')
        
        message = ''
        
        if request_method != 'POST':
            message = 'Only POST method is supported'
        elif not content_type.startswith('multipart/form-data'):
            message = 'Only multipart/form-data is supported'
        else:
            # Check if this is a streaming upload
            # Read from stdin to see if there's a streaming marker
            try:
                stdin_data = sys.stdin.read()
                
                if stdin_data.startswith('__STREAMING_UPLOAD_FILE:'):
                    # This is a streaming upload
                    temp_file_path = stdin_data.replace('__STREAMING_UPLOAD_FILE:', '').strip()
                    
                    # Parse the temporary file
                    filename, file_content = parse_multipart_from_file(temp_file_path, content_type)
                    
                    if filename and file_content:
                        # Strip leading path from filename for security
                        safe_filename = os.path.basename(filename)
                        
                        # Save the file
                        output_path = '/tmp/' + safe_filename
                        with open(output_path, 'wb') as f:
                            f.write(file_content)
                        
                        message = f'The file "{safe_filename}" was uploaded successfully ({len(file_content)} bytes)'
                        
                        # Clean up the temporary file
                        try:
                            os.unlink(temp_file_path)
                        except:
                            pass
                    else:
                        message = f'Error processing upload: {file_content if isinstance(file_content, str) else "Invalid file data"}'
                else:
                    # Traditional CGI approach
                    form = cgi.FieldStorage()
                    
                    # Get filename field
                    if 'filename' in form:
                        fileitem = form['filename']
                        
                        # Test if the file was uploaded
                        if fileitem.filename:
                            # Strip leading path from file name for security
                            safe_filename = os.path.basename(fileitem.filename)
                            output_path = '/tmp/' + safe_filename
                            
                            with open(output_path, 'wb') as f:
                                f.write(fileitem.file.read())
                            
                            message = f'The file "{safe_filename}" was uploaded successfully'
                        else:
                            message = 'No file was uploaded'
                    else:
                        message = 'No filename field found in form data'
                        
            except Exception as e:
                message = f'Error processing upload: {str(e)}'
    
    except Exception as e:
        message = f'Unexpected error: {str(e)}'
    
    # Output the response
    print("Content-Type: text/html\n")
    print("""<!DOCTYPE html>
<html>
<head>
    <title>File Upload Result</title>
</head>
<body>
    <h2>Upload Result</h2>
    <p>{}</p>
    <p><a href="/cgi-bin/">Back to upload form</a></p>
</body>
</html>""".format(message))

if __name__ == "__main__":
    main()