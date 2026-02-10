import http.server
import socketserver
import json
import subprocess
import os
import sys

PORT = 3000
DIRECTORY = "."

class NovaHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_POST(self):
        if self.path == '/compile':
            try:
                content_length = int(self.headers['Content-Length'])
                post_data = self.rfile.read(content_length)
                data = json.loads(post_data.decode('utf-8'))
                code = data.get('code', '')


                if not code or not code.strip():
                     response_data = {
                        'success': False,
                        'stdout': '',
                        'stderr': 'No code entered',
                        'asm': ''
                    }
                     self._send_json_response(response_data)
                     return


                if not os.path.exists('nova.exe'):
                     response_data = {
                        'success': False,
                        'stdout': '',
                        'stderr': 'Internal Error: Compiler executable (nova.exe) not found. Please build the project.',
                        'asm': ''
                    }
                     self._send_json_response(500, response_data)
                     return


                asm_file = 'output.asm'
                if os.path.exists(asm_file):
                    try:
                        os.remove(asm_file)
                    except Exception as e:
                        print(f"Warning: Could not remove stale output.asm: {e}")

                temp_file_path = 'temp.no'
                with open(temp_file_path, 'w') as f:
                    f.write(code)

                cmd = ['nova.exe']
                
                process = subprocess.Popen(
                    cmd, 
                    stdin=subprocess.PIPE, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.PIPE,
                    text=True,
                    cwd=os.getcwd()
                )
                
                stdout, stderr = process.communicate(input=code)
                
                asm_content = ""
                
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
                
                self._send_json_response(response_data)

            except Exception as e:
                err_resp = {'stderr': str(e), 'stdout': '', 'asm': ''}
                self._send_json_response(500, err_resp)
        else:
            self.send_error(404)

    def _send_json_response(self, status_code=200, data=None):
        if data is None:
            data = status_code
            status_code = 200
        
        response_bytes = json.dumps(data).encode('utf-8')
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(response_bytes)


if __name__ == '__main__':
    print(f"Starting server at http://localhost:{PORT}")
    try:
        with socketserver.TCPServer(("", PORT), NovaHandler) as httpd:
            httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
