import asynchat
import sys, traceback
import re

# regular expression for extracting Pd style messages
element_re = re.compile("((.*?)[^\\\]);[\r\n\ ]{0,1}", re.MULTILINE | re.DOTALL)

class Channel(asynchat.async_chat):
	endchars = '\n'
	def __init__(self, conn=None, addr=(), server=None):
		asynchat.async_chat.__init__(self, conn)
		self.addr = addr
		self._server = server
		self._ibuffer = ""
		self.set_terminator(self.endchars)
		self.sendqueue = []
	
	def collect_incoming_data(self, data):
		self._ibuffer += data
	
	def found_terminator(self):
		# split up into pd messages, ignoring the last message (whitespace or carriage return)
		for found in element_re.finditer(self._ibuffer):
			# trim off the semicolon and split up the data by space
			data = found.group(1).split(" ")
			if hasattr(self, "Incoming"):
				self.Incoming(data)
			else:
				print "Dropped data", data
		self._ibuffer = ""
	
	def Pump(self):
		[asynchat.async_chat.push(self, d) for d in self.sendqueue]
		self.sendqueue = []
	
	def Send(self, data):
		self.sendqueue.append(" ".join(data) + ";\n")
	
	def Disconnect(self):
		self.close()
	
	def handle_connect(self):
		if hasattr(self, "Connected"):
			self.Connected()
		else:
			print "Unhandled Connected() at", self.addr
	
	def handle_error(self):
		try:
			self.close()
		except:
			pass
		if hasattr(self, "Error"):
			self.Error(sys.exc_info()[1])
		else:
			asynchat.async_chat.handle_error(self)
	
	def handle_expt(self):
		pass
	
	def handle_close(self):
		if hasattr(self, "Close"):
			self.Close()
		asynchat.async_chat.handle_close(self)

