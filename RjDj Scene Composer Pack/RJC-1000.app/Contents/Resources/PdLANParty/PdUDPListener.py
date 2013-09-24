import sys
import socket
from time import sleep

from Channel import Channel

class PdUDPListener(Channel):
	"""
	To test this run:
	$ python PdUDPListener.py
	
	In a different terminal:
	$ nc -u 127.0.0.1 10316
	zing;
	this is my test;
	this is; another test; hello;
	"""
	def __init__(self):
		listen = ("0.0.0.0", 10316)
		Channel.__init__(self)
		self.create_socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.bind(listen)
		self.incoming = []
	
	def Incoming(self, data):
		self.incoming.append(data)
	
	def Connected(self):
		# ignore connect event
		pass
	
	def GetMessages(self):
		# output everything in our Queue
		messages = self.incoming[:]
		self.incoming = []
		return messages

if __name__ == "__main__":
	print PdUDPListener.__doc__
	if float(sys.version[:3]) < 2.5:
		from asyncore import poll2 as poll
	else:
		from asyncore import poll
	p = PdUDPListener()
	while 1:
		poll()
		p.Pump()
		all = p.GetMessages()
		if all:
			print all
		sleep(0.001)

