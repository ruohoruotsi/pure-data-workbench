from time import sleep
from sys import version
import asynchat
import asyncore

# monkey patch older versions to support maps in asynchat. Yuck.
if float(version[:3]) < 2.6:
	def asynchat_monkey_init(self, conn=None, map=None):
		self.ac_in_buffer = ''
		self.ac_out_buffer = ''
		self.producer_fifo = asynchat.fifo()
		asyncore.dispatcher.__init__ (self, sock=conn, map=map)
	asynchat.async_chat.__init__ = asynchat_monkey_init

if float(version[:3]) < 2.5:
	from asyncore import poll2 as poll
else:
	from asyncore import poll

from PdServer import PdServer
from PdChannel import PdChannel
from PdLANPoller import PdLANPoller
from PdUDPListener import PdUDPListener

class PdLANParty(PdServer):
	def __init__(self):
		"""
		Creates the lanparty server. Call the Launch() method to start it.
		Use GetMessages() and PostMessage(data) to send stuff to and from connected clients. 
		
		# two way Pd style connection working
		# run server first
		# emulate ip address broadcast catcher = nc -lu 10314
		# emulate Pd listener = nc -l 192.168.2.113 10315
		# emulate Pd sender = nc localhost 10314 -s 192.168.2.113
		"""
		# tell external process when we're done
		self.exitFlag = False
		
		# set this up as the actual two way server
		PdServer.__init__(self, channelClass=PdChannel, localaddr=("0.0.0.0", 10314), listeners=50)
		
		# broadcasts to all devices on the network every two seconds and handles UDP sends
		self.poller = PdLANPoller(self)
		
		# listens for UDP packets from the network
		self.udplistener = PdUDPListener()
	
	def Pump(self):
		if not self.exitFlag:
			poll()
			self.poller.Pump()
			self.udplistener.Pump()
			PdServer.Pump(self)
		return not self.exitFlag
	
	def Exit(self):
		""" Make PdLANParty sockets close. """
		self.exitFlag = True
		self.close()
		self.poller.close()
		self.udplistener.close()
	
	def PostUDPMessage(self, data):
		""" Send a broadcast UDP message to all connected clients. 'data' should be an array. """
		self.poller.Post(data)
	
	def GetUDPMessages(self):
		""" Get all of the UDP messages from all connected clients. """
		return self.udplistener.GetMessages()

