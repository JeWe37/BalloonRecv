#!/usr/bin/env python

import http.server
import socketserver
from time import time

PORT = 5000

data = f"""1500,1.5,2.5,2.5,21.5,90001.5,1001.5,20.5,1.5,2.5,3.5,4.5,0.5,1.5,2.5,3.5,4.5,2.25,10.5,81058000,515911000,999.5,270.5,45.5,{time()-5}
2000,7.5,6.5,6.5,20.5,89000.5,990.5,21.5,4.5,3.5,2.5,1.5,4.5,3.5,2.5,1.5,0.5,2.75,11.5,81059000,516911000,1000.5,260.5,40.5,{time()}""".encode("utf-8")

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        """Serve a GET request."""
        if self.path == "/getData":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.send_header("Content-Length", len(data))
            self.end_headers()
            self.wfile.write(data)
        else:
            f = http.server.SimpleHTTPRequestHandler.send_head(self)
            if f:
                try:
                    self.copyfile(f, self.wfile)
                finally:
                    f.close()

    def do_HEAD(self):
        """Serve a HEAD request."""
        if self.path == "/getData":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.send_header("Content-Length", len(data))
            self.end_headers()
        else:
            f = http.server.SimpleHTTPRequestHandler.send_head(self)
            if f:
                f.close()


with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print("serving at port", PORT)
    httpd.serve_forever()