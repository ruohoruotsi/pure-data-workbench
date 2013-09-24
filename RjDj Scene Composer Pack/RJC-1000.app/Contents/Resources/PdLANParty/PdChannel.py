import asyncore
import socket

from Channel import Channel

class PdSender(Channel):
	def __init__(self, parent, *args, **kwargs):
		Channel.__init__(self, *args, **kwargs)
		self.parent = parent
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.connect((self.parent.addr[0], 10315))
	
	def Connected(self):
		print 'PdSender connected to', self.parent.addr
		self.Send(["server", "ip", self.parent.addr[0]])
		self.Send(["server", "id", str(self.parent.id)])
	
	def Error(self, error):
		if error[0] in [61, 32]: # connection refused, broken pipe
			self.parent.Die()
		print 'PdSender error:', error

class PdChannel(Channel):
	def __init__(self, *args, **kwargs):
		Channel.__init__(self, *args, **kwargs)
		# initiate connection back to the client
		self.id = self._server.MakeNewID(self)
		#print "MAKE NEW ID:", self.id
		self.outgoing = PdSender(self)
	
	def Incoming(self, data):
		self._server.FromClient(str(self.id), data)
	
	def ToClient(self, data):
		self.outgoing.Send(data)
	
	def Close(self):
		self.Die()
	
	def Pump(self):
		Channel.Pump(self)
		self.outgoing.Pump()
	
	def Die(self):
		self.outgoing.Disconnect()
		self.Disconnect()
		self._server.Remove(self)

