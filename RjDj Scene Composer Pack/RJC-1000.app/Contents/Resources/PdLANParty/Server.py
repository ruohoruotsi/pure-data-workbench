import socket
import asyncore
import sys

from Channel import Channel

class Server(asyncore.dispatcher):
	channelClass = Channel
	
	def __init__(self, channelClass=None, localaddr=("127.0.0.1", 31425), listeners=5):
		if channelClass:
			self.channelClass = channelClass
		self.channels = []
		asyncore.dispatcher.__init__(self)
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.set_reuse_addr()
		self.bind(localaddr)
		self.listen(listeners)
	
	def handle_accept(self):
		try:
			conn, addr = self.accept()
		except socket.error:
			print 'warning: server accept() threw an exception'
			return
		except TypeError:
			print 'warning: server accept() threw EWOULDBLOCK'
			return
		
		newChannel = self.channelClass(conn, addr, self)
		self.channels.append(newChannel)
		newChannel.Send(["server", "connected"])
		if hasattr(self, "Connected"):
			self.Connected(newChannel, addr)
	
	def Remove(self, channel):
		self.channels.remove(channel)
	
	def Pump(self):
		[c.Pump() for c in self.channels]

#########################
#	Test stub	#
#########################

if __name__ == "__main__":
	class ServerChannel(Channel):
		def Incoming(self, data):
			print "*Server* received:", data
	
	class EndPointChannel(Channel):
		def Connected(self):
			print "*EndPoint* Connected()"
		
		def Incoming(self, data):
			print "*EndPoint* Incoming(", data, ")"
			outgoing.Send(["hello", "hallo", "pingo"])
	
	def Connected(channel, addr):
		print "*Server* Connected() ", channel, "connected on", addr
	
	server = Server(channelClass=ServerChannel)
	server.Connected = Connected
	
	sender = asyncore.dispatcher()
	sender.create_socket(socket.AF_INET, socket.SOCK_STREAM)
	sender.connect(("localhost", 31425))
	outgoing = EndPointChannel(sender)
	
	from time import sleep
	
	if float(sys.version[:3]) < 2.5:
		from asyncore import poll2 as poll
	else:
		from asyncore import poll
	
	print "*** polling for half a second"
	for x in range(50):
		server.Pump()
		outgoing.Pump()
		sleep(0.001)
		poll()
