from Server import Server

class PdServer(Server):
	def __init__(self, **kwargs):
		Server.__init__(self, **kwargs)
		self.IDs = {}
		self.inQueue = []
		self.outQueue = []
	
	def Connected(self, channel, addr):
		print "PdServer connection from", addr, channel
		self.outQueue.append(["connected", str(channel.id)])
	
	def MakeNewID(self, what):
		ID = 1
		IDs = self.IDs.keys()
		while ID in IDs:
			ID += 1
		self.IDs[ID] = what
		return ID
	
	def Pump(self):
		# send all queued up messages to all clients
		while self.inQueue:
			sendparts = self.inQueue.pop(0)
			[c.ToClient(sendparts[1:]) for c in self.channels if str(sendparts[0]) == "*" or str(c.id) == str(sendparts[0])]
		Server.Pump(self)
	
	# internal callbacks
	def FromClient(self, source, data):
		self.outQueue.append([source] + data)
	
	def Remove(self, channel):
		del self.IDs[channel.id]
		Server.Remove(self, channel)
		self.outQueue.append(["disconnected", str(channel.id)])
		del channel
	
	### Public methods ###
	
	def PostMessage(self, data):
		""" Send a Pd message to all connected clients. """
		self.inQueue.append(["*"] + data)
	
	def PostMessageTo(self, id, data):
		self.inQueue.append([id] + data)
	
	# check the queue and return any messages from clients
	def GetMessages(self):
		""" Get any pending Pd messages from connected clients. """
		messages = self.outQueue[:]
		self.outQueue = []
		return messages

