from time import sleep

from PdLANParty import PdLANParty

if __name__ == "__main__":
	p = PdLANParty()
	while p.Pump():
		[p.PostMessage(m) for m in p.GetMessages()]
		[p.PostUDPMessage(m) for m in p.GetUDPMessages()]
		sleep(0.001)

