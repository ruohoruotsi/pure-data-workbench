from os import listdir, path
import zipfile
from sys import argv
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer

port = 8314

def zipall(d, zip):
	for f in listdir(d):
		try:
			f.index(".svn")
		except ValueError:
			z = path.join(d, f)
			if path.isdir(z):
				zipall(z, zip)
			else:
				zip.write(z)

def zipdir(rjdir):
	zip = zipfile.ZipFile(rjdir + "z", 'w')
	zipall(rjdir, zip)
	zip.close()

class RjzHandler(BaseHTTPRequestHandler):
	def do_GET(self):
		if self.path.endswith(".rjz"):
			self.send_response(200)
			self.send_header('Content-type', 'application/zip')
			
			rjzfile = path.basename(self.path)
			zipdir(rjzfile[:-1])
			f = open(rjzfile, mode='rb')
			rjz = f.read()
			
			self.send_header('Content-length', len(rjz))
			self.end_headers()
			
			self.wfile.write(rjz)
			f.close()
			return
		else:
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			self.end_headers()
			self.wfile.write("<html><head><title>Rjz Server</title><style> pre { width: 80%; white-space: -moz-pre-wrap; white-space: -pre-wrap; white-space: -o-pre-wrap; white-space: pre-wrap; word-wrap: break-word; }\nbody { margin-left: auto; margin-right: auto; width: 80%; margin-top: 10%; } </style><meta name='viewport' content='width=device-width; initial-scale=1.0; minimum-scale=1.0; maximum-scale=1.0; user-scalable=0;'/></head><body><pre><h1 style='border-bottom: 1px solid grey;'>Rjz Server</h1>\nContact <a href='mailto:chrism@rjdj.me'>Chris McCormick</a> for support, bug reports, etc.\n\n")
			self.wfile.write("Set your iPhone proxy to: <strong>" + self.address_string() + ":" + str(port) + "</strong> and browse to http://rjdj.me/\n\n")
			rjzs = ["<a href='rjdj://rjdj.me/%sz' style='font: +2;'>%sz</a> (<a href='%sz'>download</a>)" % (d, d, d) for d in listdir(".") if d[-3:] == ".rj"]
			self.wfile.write("\n".join(rjzs))
			self.wfile.write("</pre></body></html>")
			return
		return

if __name__ == '__main__':
	if len(argv) == 2:
		try:
			server = HTTPServer((argv[1], port), RjzHandler)
			print 'Started rjz server'
			server.serve_forever()
		except KeyboardInterrupt:
			print 'Shutting down server'
			server.socket.close()
	else:
		print "Usage:"
		print argv[0] + " ip-address"


