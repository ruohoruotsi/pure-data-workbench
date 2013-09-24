import socket, sys
from time import sleep, time

from Channel import Channel

# ew...
def getIP():
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.connect(('91.121.94.180', 80)) # doesn't matter if it fails
	addr = s.getsockname()[0] 
	s.close()
	return addr

class PdLANPoller(Channel):
	"""
		Broadcasts our IP address every two seconds.
		It's the poor GNU programmer's Bonjour! :)
		Also has the ability to broadcast arrays of Pd data, queued up with Post()
		
		Test like this:
		$ nc -l -u 10314
		
		Or in Pd:
		[netreceive 10314 1]
		 |
		[print]
		
		You should see:
		ip xxx.xxx.xxx.xx;
		this is just a test;
		...etc.
	"""
	def __init__(self, parent):
		Channel.__init__(self)
		self.parent = parent
		self.queue = []
		self.create_socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
		self.set_reuse_addr()
		self.dest = ('255.255.255.255', 10314)
		self.connect(self.dest)
		# elapsed time
		self.last = 0
		
		# what IP address the server is listening on
		if len(sys.argv) == 2:
			self.addr = sys.argv[1]
		else:
			self.addr = getIP()
		print "LANParty Listening on", self.addr
		print "(You can specify this with an argument)"
	
	def Connected(self):
		print "LANParty UDP broadcast socket connected to", self.dest
	
	def Pump(self):
		# don't send anything if there's been a socket error
		# broadcast the server IP address every two seconds
		if self.last < time() - 2:
			#print 'broadcasting ip...'
			self.Send(["ip", self.addr])
			self.last = time()
		# if we have UDP messages queued, then send them too
		while self.queue:
			self.Send(self.queue.pop(-1))
		Channel.Pump(self)
	
	def handle_error(self):
		""" Do something sensible on socket errors. """
		error = sys.exc_info()[1]
		if error[0] in (9, 65): # No route to host, bad file descriptor
			print "Exiting because PdLANPoller got the following error:"
			print error[1]
			self.parent.exitFlag = True
			self.close()
		else:
			print "Socket error in PdLANPoller:", error
	
	def Post(self, data):
		""" Broadcast this array to all clients on the LAN. """
		self.queue.append(data)

if __name__ == "__main__":
	print PdLANPoller.__doc__
	if float(sys.version[:3]) < 2.5:
		from asyncore import poll2 as poll
	else:
		from asyncore import poll
	
	poller = PdLANPoller()
	poller.Post(["my", "first", "test"])
	start = time()
	while 1:
		poll()
		poller.Pump()
		if start < time() - 2.5:
			poller.Post(["this", "is", "just", "a", "test"])
			start = time()
		sleep(0.01)

