####################################################################################
#
#	RjzServer
#	By Chris McCormick
# 	GPLv3
#
#	See the files README and COPYING for details.
#
#       Modified by Christian Haudum for Reality Jockey Ltd.
#       2012
#
#
#       -*- coding: utf-8 -*-
#       vim: set fileencodings=utf-8
#
####################################################################################


__docformat__ = "reStructuredText"

def run():
    from server import RjzServer
    def Output(txt):
        print txt
    server = RjzServer(outputfn=Output)
    try:
        server.Launch()
    except KeyboardInterrupt:
        server.Output("Shutting down RjzServer")

def gui():
    import threading
    import ctypes
    from time import sleep
    from sys import exit

    from server import RjzServer
    from gui import RjzGUI

    # set up the GUI
    gui = RjzGUI()

    # rjzserver's output should go via an event into the GUI
    def Output(txt):
        gui.PostMessage(txt + "\n")

    # set up the server
    server = RjzServer(outputfn=Output)

    # start the background server thread
    threadSrv = threading.Thread(target=server.Launch)
    threadSrv.start()

    # run the app loop in this thread
    gui.MainLoop()
    exit(0)

if __name__ == '__main__':
    gui()
