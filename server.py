import http.server
import socketserver
import json
import subprocess
import os
import sys

PORT = 3000
DIRECTORY = "public"

class NovaHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        # Python 3.7+ supports directory argument
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_POST(self):
        if self.path == '/compile':
            try:
                content_length = int(self.headers['Content-Length'])
                post_data = self.rfile.read(content_length)
                data = json.loads(post_data.decode('utf-8'))
                code = data.get('code', '')

                # Write temp file in the ROOT directory (not public)
                temp_file_path = 'temp.no'
                with open(temp_file_path, 'w') as f:
                    f.write(code)

                # Run nova.exe < temp.no
                # We assume nova.exe is in the current directory (project root)
                cmd = ['nova.exe']
                
                # We need to run subprocess
                process = subprocess.Popen(
                    cmd, 
                    stdin=subprocess.PIPE, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.PIPE,
                    text=True,
                    cwd=os.getcwd() # Run in current dir
                )
                
                stdout, stderr = process.communicate(input=code)
                
                asm_content = ""
                asm_file = 'output.asm'
                
                if os.path.exists(asm_file):
                    try:
                        with open(asm_file, 'r') as f:
                            asm_content = f.read()
                    except Exception as e:
                        asm_content = f"Error reading output.asm: {e}"

                response_data = {
                    'success': True,
                    'stdout': stdout,
                    'stderr': stderr,
                    'asm': asm_content
                }
                
                response_bytes = json.dumps(response_data).encode('utf-8')

                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(response_bytes)

            except Exception as e:
                err_resp = json.dumps({'stderr': str(e), 'stdout': '', 'asm': ''}).encode('utf-8')
                self.send_response(500)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(err_resp)
        else:
            self.send_error(404)

print(f"Starting server at http://localhost:{PORT}")
try:
    with socketserver.TCPServer(("", PORT), NovaHandler) as httpd:
        httpd.serve_forever()
except KeyboardInterrupt:
    print("\nServer stopped.")
