#!/usr/bin/env python

import http.server
import socketserver
import cgi

PORT = 5000

data = b"""1500,1.5,2.5,2.5,21.5,90001.5,1001.5,20.5,1.5,2.5,3.5,4.5,0.5,1.5,2.5,3.5,4.5,2.25,10.5,81058000,515911000,999.5,270.5,45.5,1581358875
2000,7.5,6.5,6.5,20.5,89000.5,990.5,21.5,4.5,3.5,2.5,1.5,4.5,3.5,2.5,1.5,0.5,2.75,11.5,81059000,516911000,1000.5,260.5,40.5,1581358880"""

class Handler(http.server.SimpleHTTPRequestHandler):
    STREAMS = ["SecretStream1", "SecretStream2"]

    def do_POST(self):
        form = cgi.FieldStorage(
            fp=self.rfile, 
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST','CONTENT_TYPE':self.headers['Content-Type']}
        )
        
        if self.path == "/" and form.getvalue("name") in self.STREAMS:
            self.send_response(200)
        else:
            self.send_error(404)
        self.end_headers()


with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print("serving at port", PORT)
    httpd.serve_forever()