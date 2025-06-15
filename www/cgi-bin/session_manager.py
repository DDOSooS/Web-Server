#!/usr/bin/env python3
"""
Clean session_manager.py without emojis to avoid encoding issues
"""

import os
import sys
import cgi
import uuid
import time
import json
import hashlib
from urllib.parse import parse_qs

def debug_log(message):
    try:
        with open('/tmp/webserv_debug.log', 'a') as f:
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
            f.write(f"[{timestamp}] {message}\n")
            f.flush()
    except Exception as e:
        print(f"Debug log error: {e}", file=sys.stderr)

# Simple user database
USERS = {
    'admin': 'password123',
    'user': 'mypass',
    'john': 'secret'
}

SESSION_FILE = '/tmp/webserv_sessions.json'
SESSION_TIMEOUT = 3600  # 1 hour

class SessionManager:
    def __init__(self):
        self.sessions = self.load_sessions()
    
    def load_sessions(self):
        try:
            if os.path.exists(SESSION_FILE):
                with open(SESSION_FILE, 'r') as f:
                    sessions = json.load(f)
                    debug_log(f"Loaded {len(sessions)} sessions from file")
                    return sessions
        except Exception as e:
            debug_log(f"Error loading sessions: {e}")
        debug_log("No existing sessions found, starting fresh")
        return {}
    
    def save_sessions(self):
        try:
            with open(SESSION_FILE, 'w') as f:
                json.dump(self.sessions, f, indent=2)
            debug_log(f"Saved {len(self.sessions)} sessions to file")
        except Exception as e:
            debug_log(f"Error saving sessions: {e}")
    
    def cleanup_expired_sessions(self):
        current_time = time.time()
        expired = [sid for sid, data in self.sessions.items() 
                  if current_time - data.get('created', 0) > SESSION_TIMEOUT]
        for sid in expired:
            del self.sessions[sid]
        if expired:
            debug_log(f"Cleaned up {len(expired)} expired sessions")
            self.save_sessions()
    
    def create_session(self, username):
        session_id = str(uuid.uuid4())
        self.sessions[session_id] = {
            'username': username,
            'created': time.time(),
            'last_access': time.time()
        }
        self.save_sessions()
        debug_log(f"Created new session {session_id} for user {username}")
        return session_id
    
    def get_session(self, session_id):
        debug_log(f"Looking up session: {session_id}")
        if session_id in self.sessions:
            session = self.sessions[session_id]
            if time.time() - session.get('created', 0) > SESSION_TIMEOUT:
                del self.sessions[session_id]
                self.save_sessions()
                debug_log(f"Session {session_id} expired and removed")
                return None
            session['last_access'] = time.time()
            self.save_sessions()
            debug_log(f"Session {session_id} valid for user {session['username']}")
            return session
        debug_log(f"Session {session_id} not found")
        return None
    
    def destroy_session(self, session_id):
        if session_id in self.sessions:
            username = self.sessions[session_id].get('username', 'unknown')
            del self.sessions[session_id]
            self.save_sessions()
            debug_log(f"Destroyed session {session_id} for user {username}")

def get_cookies():
    cookies = {}
    cookie_string = os.environ.get('HTTP_COOKIE', '')
    debug_log(f"Raw HTTP_COOKIE environment: '{cookie_string}'")
    
    if cookie_string:
        for cookie in cookie_string.split(';'):
            cookie = cookie.strip()
            if '=' in cookie:
                name, value = cookie.split('=', 1)
                name = name.strip()
                value = value.strip()
                cookies[name] = value
                debug_log(f"Parsed cookie: {name} = {value}")
    
    debug_log(f"Total cookies parsed: {len(cookies)}")
    return cookies

def set_cookie(name, value, max_age=None, path='/', http_only=True):
    cookie = f"{name}={value}; Path={path}"
    if max_age:
        cookie += f"; Max-Age={max_age}"
    if http_only:
        cookie += "; HttpOnly"
    debug_log(f"Generated Set-Cookie: {cookie}")
    return cookie

def authenticate_user(username, password):
    result = username in USERS and USERS[username] == password
    debug_log(f"Authentication for '{username}': {'SUCCESS' if result else 'FAILED'}")
    return result

def get_current_user():
    debug_log("Getting current user...")
    cookies = get_cookies()
    session_id = cookies.get('SESSIONID')
    debug_log(f"Looking for SESSIONID cookie: {session_id}")
    
    if session_id:
        debug_log(f"Found SESSIONID: {session_id}")
        session_manager = SessionManager()
        session_manager.cleanup_expired_sessions()
        session = session_manager.get_session(session_id)
        if session:
            username = session['username']
            debug_log(f"Valid session found for user: {username}")
            return username
        else:
            debug_log(f"Session {session_id} is invalid/expired")
    else:
        debug_log("No SESSIONID cookie found")
    
    debug_log("No valid session found")
    return None

def send_response(content, headers=None, status="200 OK"):
    debug_log(f"Sending response: {status}")
    print(f"Status: {status}")
    if headers:
        for header in headers:
            print(header)
            debug_log(f"Response header: {header}")
    print("Content-Type: text/html; charset=utf-8")
    print()  # Empty line to end headers
    print(content)
    debug_log(f"Response sent: {status}, content length: {len(content)}")

def login_page(error_message=""):
    debug_log("Displaying login page")
    current_user = get_current_user()
    if current_user:
        debug_log(f"User {current_user} already logged in, redirecting to profile")
        send_response("", [
            "Status: 302 Found",
            "Location: /cgi-bin/session_manager.py?action=profile"
        ])
        return

    cookies = get_cookies()

    html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Login - Webserv Debug</title>
        <meta charset="utf-8">
        <style>
            body {{ font-family: Arial, sans-serif; max-width: 600px; margin: 20px auto; padding: 20px; }}
            .error {{ color: red; margin: 10px 0; padding: 10px; background: #ffe6e6; border: 1px solid #ffcccc; }}
            .debug {{ background: #f0f0f0; padding: 15px; margin: 10px 0; font-family: monospace; font-size: 12px; border: 1px solid #ccc; }}
            input {{ display: block; width: 100%; margin: 10px 0; padding: 8px; box-sizing: border-box; }}
            button {{ background: #007cba; color: white; padding: 10px 20px; border: none; cursor: pointer; }}
            button:hover {{ background: #005a8a; }}
        </style>
    </head>
    <body>
        <h1>LOGIN (Debug Mode)</h1>
        
        {f'<div class="error">ERROR: {error_message}</div>' if error_message else ''}
        
        <form method="POST" action="/cgi-bin/session_manager.py?action=login">
            <label><strong>Username:</strong></label>
            <input type="text" name="username" required placeholder="Enter username">
            <label><strong>Password:</strong></label>
            <input type="password" name="password" required placeholder="Enter password">
            <button type="submit">LOGIN</button>
        </form>
        
        <div class="debug">
            <h4>DEBUG: Environment Info</h4>
            <p><strong>Query String:</strong> {os.environ.get('QUERY_STRING', 'None')}</p>
            <p><strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD', 'None')}</p>
            <p><strong>HTTP Cookie Header:</strong> {os.environ.get('HTTP_COOKIE', 'None')}</p>
        </div>
        
        <div class="debug">
            <h4>DEBUG: Parsed Cookies ({len(cookies)})</h4>
            {chr(10).join([f'<p><strong>{name}:</strong> {value}</p>' for name, value in cookies.items()]) if cookies else '<p><em>No cookies found</em></p>'}
        </div>
        
        <div class="debug">
            <h4>DEBUG: Session Status</h4>
            <p><strong>Current User:</strong> {get_current_user() or '<em>Not logged in</em>'}</p>
            <p><strong>SESSIONID Cookie:</strong> {cookies.get('SESSIONID', '<em>Not found</em>')}</p>
        </div>
        
        <hr>
        <h3>Test Accounts:</h3>
        <ul>
            <li><strong>admin</strong> / password123</li>
            <li><strong>user</strong> / mypass</li>
            <li><strong>john</strong> / secret</li>
        </ul>
        
        <p><a href="/cgi-bin/session_manager.py">Back to Home</a></p>
    </body>
    </html>
    """
    send_response(html)

def main():
    try:
        debug_log("=== CGI EXECUTION START ===")
        
        # Get action from query string
        query_string = os.environ.get('QUERY_STRING', '')
        params = parse_qs(query_string)
        action = params.get('action', [''])[0]
        debug_log(f"Action extracted: '{action}'")
        
        request_method = os.environ.get('REQUEST_METHOD', 'GET')
        debug_log(f"Request method: {request_method}")
        
        if action == 'login':
            if request_method == 'POST':
                debug_log("Processing POST login request")
                form = cgi.FieldStorage()
                username = form.getvalue('username', '').strip()
                password = form.getvalue('password', '').strip()
                debug_log(f"Login attempt: username='{username}', password_length={len(password)}")
                
                if authenticate_user(username, password):
                    debug_log("Authentication successful!")
                    session_manager = SessionManager()
                    session_manager.cleanup_expired_sessions()
                    session_id = session_manager.create_session(username)
                    
                    cookie_header = set_cookie('SESSIONID', session_id, max_age=SESSION_TIMEOUT)
                    debug_log(f"CRITICAL: About to send Set-Cookie header: {cookie_header}")
                    
                    headers = [
                        f"Set-Cookie: {cookie_header}",
                        "Location: /cgi-bin/session_manager.py?action=profile"
                    ]
                    debug_log("CRITICAL: Redirecting to profile page with session cookie")
                    send_response("", headers, "302 Found")
                else:
                    debug_log("Authentication failed - invalid credentials")
                    login_page("Invalid username or password")
            else:
                debug_log("Showing login form (GET request)")
                login_page()
        
        elif action == 'logout':
            debug_log("Processing logout request")
            cookies = get_cookies()
            session_id = cookies.get('SESSIONID')
            if session_id:
                session_manager = SessionManager()
                session_manager.destroy_session(session_id)
            
            expire_cookie = set_cookie('SESSIONID', '', max_age=0)
            headers = [f"Set-Cookie: {expire_cookie}"]
            
            html = """
            <!DOCTYPE html>
            <html>
            <head>
                <title>Logged Out</title>
                <meta charset="utf-8">
                <meta http-equiv="refresh" content="3;url=/cgi-bin/session_manager.py">
                <style>
                    body { font-family: Arial, sans-serif; max-width: 400px; margin: 50px auto; padding: 20px; text-align: center; }
                </style>
            </head>
            <body>
                <h1>Logged Out</h1>
                <p>You have been successfully logged out.</p>
                <p>Redirecting to home page in 3 seconds...</p>
                <p><a href="/cgi-bin/session_manager.py">Go to Home Page</a></p>
            </body>
            </html>
            """
            send_response(html, headers)
        
        elif action == 'profile':
            debug_log("Processing profile request")
            current_user = get_current_user()
            if not current_user:
                debug_log("No valid session, redirecting to login")
                send_response("", [
                    "Status: 302 Found", 
                    "Location: /cgi-bin/session_manager.py?action=login"
                ])
                return

            cookies = get_cookies()
            session_id = cookies.get('SESSIONID')
            session_manager = SessionManager()
            session = session_manager.get_session(session_id)
            
            created_time = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(session['created']))
            last_access = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(session['last_access']))

            html = f"""
            <!DOCTYPE html>
            <html>
            <head>
                <title>Profile - {current_user}</title>
                <meta charset="utf-8">
                <style>
                    body {{ font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }}
                    .user-info {{ background: #f0f8ff; padding: 15px; border-radius: 5px; margin: 20px 0; }}
                    .session-info {{ background: #f5f5f5; padding: 15px; border-radius: 5px; margin: 20px 0; }}
                    a {{ color: #007cba; text-decoration: none; margin-right: 15px; }}
                </style>
            </head>
            <body>
                <h1>Welcome, {current_user}!</h1>
                
                <div class="user-info">
                    <h3>User Information</h3>
                    <p><strong>Username:</strong> {current_user}</p>
                    <p><strong>Status:</strong> Logged In</p>
                </div>
                
                <div class="session-info">
                    <h3>Session Information</h3>
                    <p><strong>Session ID:</strong> {session_id[:16]}...</p>
                    <p><strong>Login Time:</strong> {created_time}</p>
                    <p><strong>Last Access:</strong> {last_access}</p>
                </div>
                
                <h3>Navigation</h3>
                <p>
                    <a href="/cgi-bin/session_manager.py">Home</a>
                    <a href="/cgi-bin/session_manager.py?action=profile">Refresh Profile</a>
                    <a href="/cgi-bin/session_manager.py?action=logout">Logout</a>
                </p>
                
                <hr>
                <p><em>This is a protected page. Only logged-in users can see this content.</em></p>
            </body>
            </html>
            """
            send_response(html)
        
        else:
            debug_log("Displaying home page")
            current_user = get_current_user()
            
            html = f"""
            <!DOCTYPE html>
            <html>
            <head>
                <title>Webserv - Session Demo</title>
                <meta charset="utf-8">
                <style>
                    body {{ font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }}
                    .status {{ padding: 15px; border-radius: 5px; margin: 20px 0; }}
                    .logged-in {{ background: #d4edda; color: #155724; }}
                    .logged-out {{ background: #f8d7da; color: #721c24; }}
                    a {{ color: #007cba; text-decoration: none; margin-right: 15px; }}
                    .demo-info {{ background: #e7f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }}
                    .debug {{ background: #f0f0f0; padding: 10px; margin: 10px 0; font-family: monospace; font-size: 12px; }}
                </style>
            </head>
            <body>
                <h1>Webserv Cookie Session Demo</h1>
                
                <div class="status {'logged-in' if current_user else 'logged-out'}">
                    {'Logged in as: ' + current_user if current_user else 'Not logged in'}
                </div>
                
                <h3>Navigation</h3>
                <p>
                    <a href="/cgi-bin/session_manager.py">Home</a>
                    {f'<a href="/cgi-bin/session_manager.py?action=profile">Profile</a>' if current_user else ''}
                    {f'<a href="/cgi-bin/session_manager.py?action=logout">Logout</a>' if current_user else '<a href="/cgi-bin/session_manager.py?action=login">Login</a>'}
                </p>
                
                <div class="demo-info">
                    <h3>Cookie Session Features</h3>
                    <ul>
                        <li>Session-based authentication using HTTP cookies</li>
                        <li>Automatic session expiry (1 hour timeout)</li>
                        <li>Protected pages (profile requires login)</li>
                        <li>Session data stored server-side</li>
                        <li>Secure session ID generation</li>
                    </ul>
                </div>
                
                <div class="debug">
                    <h4>DEBUG Information</h4>
                    <p><strong>Current User:</strong> {current_user or 'None'}</p>
                    <p><strong>SESSIONID Cookie:</strong> {get_cookies().get('SESSIONID', 'Not found')}</p>
                    <p><strong>Debug Log:</strong> /tmp/webserv_debug.log</p>
                </div>
                
                <h3>Test Instructions</h3>
                <ol>
                    <li>Clear all browser cookies for localhost:8080</li>
                    <li>Click "Login" and use: <strong>admin / password123</strong></li>
                    <li>Check browser dev tools for cookies</li>
                    <li>Visit protected "Profile" page</li>
                </ol>
                
                <hr>
                <p><em>Debug mode enabled - check /tmp/webserv_debug.log for logs</em></p>
            </body>
            </html>
            """
            send_response(html)
            
        debug_log("=== CGI EXECUTION END ===")
            
    except Exception as e:
        debug_log(f"ERROR in CGI execution: {str(e)}")
        send_response(f"""
        <html>
        <body>
        <h1>Error</h1>
        <p>An error occurred: {str(e)}</p>
        <p>Check debug log: /tmp/webserv_debug.log</p>
        <p><a href="/cgi-bin/session_manager.py">Go to Home</a></p>
        </body>
        </html>
        """, status="500 Internal Server Error")

if __name__ == "__main__":
    main()