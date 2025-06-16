#!/usr/bin/env python3
"""
Diagnostic Session Manager - Shows exact cookie and session flow
"""

import os
import sys
import uuid
import time
import json
from urllib.parse import parse_qs, unquote_plus

def debug_log(message):
    """Log to file (never stdout)"""
    try:
        with open('/tmp/webserv_debug.log', 'a') as f:
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
            f.write(f"[{timestamp}] {message}\n")
            f.flush()
    except:
        pass

# User database
USERS = {
    'admin': 'password123',
    'user': 'mypass', 
    'john': 'secret'
}

SESSION_FILE = './webserv_sessions.json'
SESSION_TIMEOUT = 3600

class SessionManager:
    def __init__(self):
        self.sessions = self._load_sessions()
    
    def _load_sessions(self):
        try:
            if os.path.exists(SESSION_FILE):
                with open(SESSION_FILE, 'r') as f:
                    return json.load(f)
        except:
            pass
        return {}
    
    def _save_sessions(self):
        try:
            with open(SESSION_FILE, 'w') as f:
                json.dump(self.sessions, f, indent=2)
        except:
            pass
    
    def create_session(self, username):
        session_id = str(uuid.uuid4())
        self.sessions[session_id] = {
            'username': username,
            'created': time.time(),
            'last_access': time.time()
        }
        self._save_sessions()
        debug_log(f"CREATED SESSION: {session_id} for {username}")
        return session_id
    
    def get_session(self, session_id):
        debug_log(f"LOOKING UP SESSION: {session_id}")
        if session_id in self.sessions:
            session = self.sessions[session_id]
            if time.time() - session['created'] < SESSION_TIMEOUT:
                debug_log(f"SESSION VALID: {session_id}")
                return session
            else:
                debug_log(f"SESSION EXPIRED: {session_id}")
                del self.sessions[session_id]
                self._save_sessions()
        debug_log(f"SESSION NOT FOUND: {session_id}")
        return None

def parse_cookies():
    """Parse HTTP_COOKIE environment variable"""
    cookies = {}
    cookie_string = os.environ.get('HTTP_COOKIE', '')
    debug_log(f"RAW HTTP_COOKIE: '{cookie_string}'")
    
    if cookie_string:
        for cookie in cookie_string.split(';'):
            cookie = cookie.strip()
            if '=' in cookie:
                name, value = cookie.split('=', 1)
                cookies[name.strip()] = value.strip()
                debug_log(f"PARSED COOKIE: {name.strip()} = {value.strip()}")
    
    debug_log(f"TOTAL COOKIES: {len(cookies)}")
    return cookies

def parse_post_data():
    """Parse POST form data"""
    fields = {}
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
        if content_length > 0:
            post_data = sys.stdin.read(content_length)
            debug_log(f"RAW POST DATA: '{post_data}'")
            
            for pair in post_data.split('&'):
                if '=' in pair:
                    key, value = pair.split('=', 1)
                    fields[unquote_plus(key)] = unquote_plus(value)
                    debug_log(f"PARSED FORM: {key} = {value}")
    except Exception as e:
        debug_log(f"POST PARSE ERROR: {e}")
    
    return fields

def send_response(body, headers=None, status="200 OK"):
    """Send HTTP response with detailed logging"""
    debug_log(f"SENDING RESPONSE: {status}")
    debug_log(f"RESPONSE BODY LENGTH: {len(body)}")
    
    # Send status
    print(f"Status: {status}")
    debug_log(f"SENT STATUS: {status}")
    
    # Send additional headers
    if headers:
        for header in headers:
            print(header)
            debug_log(f"SENT HEADER: {header}")
    
    # Send content type
    print("Content-Type: text/html; charset=utf-8")
    debug_log("SENT CONTENT-TYPE: text/html; charset=utf-8")
    
    # Empty line
    print()
    debug_log("SENT EMPTY LINE (end of headers)")
    
    # Send body
    print(body)
    debug_log("SENT BODY")

def main():
    try:
        debug_log("=" * 60)
        debug_log("DIAGNOSTIC CGI SESSION MANAGER START")
        debug_log("=" * 60)
        
        # Parse request
        query_string = os.environ.get('QUERY_STRING', '')
        params = parse_qs(query_string)
        action = params.get('action', [''])[0]
        method = os.environ.get('REQUEST_METHOD', 'GET')
        
        debug_log(f"ACTION: '{action}'")
        debug_log(f"METHOD: {method}")
        debug_log(f"QUERY: {query_string}")
        
        # Log all environment variables related to cookies
        debug_log("ENVIRONMENT VARIABLES:")
        for key, value in os.environ.items():
            if 'COOKIE' in key or 'HTTP_' in key:
                debug_log(f"  {key} = '{value}'")
        
        if action == 'login':
            if method == 'POST':
                debug_log("PROCESSING LOGIN FORM")
                
                # Parse form data
                form_data = parse_post_data()
                username = form_data.get('username', '').strip()
                password = form_data.get('password', '').strip()
                
                debug_log(f"USERNAME: '{username}'")
                debug_log(f"PASSWORD LENGTH: {len(password)}")
                
                # Authenticate
                if username in USERS and USERS[username] == password:
                    debug_log("AUTHENTICATION SUCCESS!")
                    
                    # Create session
                    session_manager = SessionManager()
                    session_id = session_manager.create_session(username)
                    
                    # Create cookie
                    cookie_value = f"SESSIONID={session_id}; Path=/; HttpOnly; Max-Age={SESSION_TIMEOUT}"
                    debug_log(f"CREATING COOKIE: {cookie_value}")
                    
                    # Send redirect
                    headers = [
                        f"Set-Cookie: {cookie_value}",
                        "Location: /cgi-bin/session_manager.py?action=profile"
                    ]
                    
                    debug_log("SENDING REDIRECT WITH SET-COOKIE")
                    send_response("", headers, "302 Found")
                    
                else:
                    debug_log("AUTHENTICATION FAILED")
                    # Show login form with error
                    html = f"""<!DOCTYPE html>
<html>
<head><title>Login Failed</title></head>
<body>
    <h1>❌ Login Failed</h1>
    <p>Invalid credentials. Try again.</p>
    <form method="POST" action="/cgi-bin/session_manager.py?action=login">
        <input type="text" name="username" placeholder="Username" required>
        <input type="password" name="password" placeholder="Password" required>  
        <button type="submit">Login</button>
    </form>
    <p><a href="/cgi-bin/session_manager.py">Home</a></p>
</body>
</html>"""
                    send_response(html)
            else:
                debug_log("SHOWING LOGIN FORM")
                # Show login form
                html = """<!DOCTYPE html>
<html>
<head><title>Login</title></head>
<body>
    <h1>🔐 Login</h1>
    <form method="POST" action="/cgi-bin/session_manager.py?action=login">
        <input type="text" name="username" placeholder="Username" required>
        <input type="password" name="password" placeholder="Password" required>
        <button type="submit">Login</button>
    </form>
    <h3>Test Accounts:</h3>
    <ul>
        <li>admin / password123</li>
        <li>user / mypass</li>
        <li>john / secret</li>
    </ul>
    <p><a href="/cgi-bin/session_manager.py">Home</a></p>
</body>
</html>"""
                send_response(html)
        
        elif action == 'profile':
            debug_log("SHOWING PROFILE PAGE")
            
            # Parse cookies
            cookies = parse_cookies()
            session_id = cookies.get('SESSIONID')
            
            debug_log(f"SESSION ID FROM COOKIE: '{session_id}'")
            
            if not session_id:
                debug_log("NO SESSION COOKIE - REDIRECTING TO LOGIN")
                send_response("", [
                    "Location: /cgi-bin/session_manager.py?action=login"
                ], "302 Found")
                return
            
            # Validate session
            session_manager = SessionManager()
            session_data = session_manager.get_session(session_id)
            
            if not session_data:
                debug_log("INVALID SESSION - REDIRECTING TO LOGIN")
                send_response("", [
                    "Location: /cgi-bin/session_manager.py?action=login"
                ], "302 Found")
                return
            
            # Show profile
            username = session_data['username']
            debug_log(f"SHOWING PROFILE FOR USER: {username}")
            
            html = f"""<!DOCTYPE html>
<html>
<head><title>Profile - {username}</title></head>
<body>
    <h1>🎉 Welcome, {username}!</h1>
    <h2>✅ SESSION LOGIN SUCCESS!</h2>
    
    <h3>Session Info:</h3>
    <ul>
        <li>Session ID: {session_id[:16]}...</li>
        <li>Username: {username}</li>
        <li>Login Time: {time.ctime(session_data['created'])}</li>
    </ul>
    
    <h3>Navigation:</h3>
    <p>
        <a href="/cgi-bin/session_manager.py">Home</a> |
        <a href="/cgi-bin/session_manager.py?action=logout">Logout</a>
    </p>
    
    <hr>
    <p><em>This is a protected page - only accessible with valid session!</em></p>
</body>
</html>"""
            send_response(html)
        
        elif action == 'logout':
            debug_log("PROCESSING LOGOUT")
            
            cookies = parse_cookies()
            session_id = cookies.get('SESSIONID')
            
            if session_id:
                session_manager = SessionManager()
                if session_id in session_manager.sessions:
                    del session_manager.sessions[session_id]
                    session_manager._save_sessions()
                    debug_log(f"DESTROYED SESSION: {session_id}")
            
            # Expire cookie
            expire_cookie = "SESSIONID=; Path=/; Max-Age=0"
            debug_log(f"EXPIRING COOKIE: {expire_cookie}")
            
            html = """<!DOCTYPE html>
<html>
<head>
    <title>Logged Out</title>
    <meta http-equiv="refresh" content="3;url=/cgi-bin/session_manager.py">
</head>
<body>
    <h1>👋 Logged Out</h1>
    <p>Session destroyed. Redirecting to home...</p>
    <p><a href="/cgi-bin/session_manager.py">Go Home</a></p>
</body>
</html>"""
            
            send_response(html, [f"Set-Cookie: {expire_cookie}"])
        
        else:
            debug_log("SHOWING HOME PAGE")
            
            # Check current session
            cookies = parse_cookies()
            session_id = cookies.get('SESSIONID')
            current_user = None
            
            if session_id:
                session_manager = SessionManager()
                session_data = session_manager.get_session(session_id)
                if session_data:
                    current_user = session_data['username']
                    debug_log(f"CURRENT USER: {current_user}")
            
            html = f"""<!DOCTYPE html>
<html>
<head><title>Session Manager Diagnostic</title></head>
<body>
    <h1>🚀 Session Manager Diagnostic</h1>
    
    <h2>Current Status:</h2>
    <p><strong>{'✅ Logged in as: ' + current_user if current_user else '❌ Not logged in'}</strong></p>
    
    <h2>Navigation:</h2>
    <p>
        <a href="/cgi-bin/session_manager.py">🏠 Home</a> |
        {f'<a href="/cgi-bin/session_manager.py?action=profile">👤 Profile</a> |' if current_user else ''}
        {f'<a href="/cgi-bin/session_manager.py?action=logout">🚪 Logout</a>' if current_user else '<a href="/cgi-bin/session_manager.py?action=login">🔐 Login</a>'}
    </p>
    
    <h2>🔍 Debug Info:</h2>
    <ul>
        <li>Session ID: {session_id or 'None'}</li>
        <li>Total Cookies: {len(cookies)}</li>
        <li>Raw Cookie String: '{os.environ.get('HTTP_COOKIE', 'None')}'</li>
        <li>Debug Log: /tmp/webserv_debug.log</li>
    </ul>
    
    <h2>🧪 Test Instructions:</h2>
    <ol>
        <li>Click Login and use: <strong>admin / password123</strong></li>
        <li>Watch the debug log: <code>tail -f /tmp/webserv_debug.log</code></li>
        <li>Check browser dev tools → Application → Cookies</li>
    </ol>
    
    <hr>
    <p><em>Check /tmp/webserv_debug.log for detailed session flow</em></p>
</body>
</html>"""
            
            send_response(html)
        
        debug_log("=" * 60)
        debug_log("DIAGNOSTIC CGI SESSION MANAGER END")
        debug_log("=" * 60)
    
    except Exception as e:
        debug_log(f"❌ CRITICAL ERROR: {str(e)}")
        import traceback
        debug_log(f"TRACEBACK: {traceback.format_exc()}")
        
        html = f"""<!DOCTYPE html>
<html>
<head><title>Error</title></head>
<body>
    <h1>❌ Error</h1>
    <p>Error: {str(e)}</p>
    <p>Check: /tmp/webserv_debug.log</p>
    <p><a href="/cgi-bin/session_manager.py">Home</a></p>
</body>
</html>"""
        send_response(html, status="500 Internal Server Error")

if __name__ == "__main__":
    main()