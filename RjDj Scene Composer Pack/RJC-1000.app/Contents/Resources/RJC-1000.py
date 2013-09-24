# yuck. monkeypatch python2.5 to support various things (see monkey.py for details)
import src.monkey
from sys import platform, path
from os import chdir, sep, getcwd
from os import path as ospath

if __name__ == "__main__":
    print "Start directory:", getcwd()
    # run RJC1000
    # change to the correct startup directory so relative paths etc. are right
    if platform == "win32":
        # one level up from library.zip
        #chdir(sep.join(path[0].split(sep)[:-1]))
	# change this for builds in windows!!!
        chdir(path[0])
    else:
        chdir(path[0])

    from src.controller.RjcController import RjcController
    from src.model.config import config
    config.defaultSection = "rjc"
    config.SetFilename("RJC-1000.cfg")
    controller = RjcController()
    controller.MainLoop()
