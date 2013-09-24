========================
RjDj Scene Composer Pack
========================

The RjDj scene composers pack includes a template you should use for writing
your Scenes and a copy of the RjDj utilities to be installed in your Pd's
"extra" directory. 

How to install
==============

Put the content of the "pd" directory into your Pd search path. You can change
the search path of Pd via the menu entry File->Path on Windows/Linux or through
the "Preferences" panel on OS-X. Check the Pd-html-manual rsp. the FlossManual
on Pd for details. In the Flossmanual setting the search path is described here:
http://en.flossmanuals.net/PureData/AdvancedConfig

The end result should be that you are able to create [soundinput] and
[soundoutput] objects in any Pd patch afterwards. 

Using the template
==================

To write a new scene, make a copy of the directory "SceneTemplate.rj".

Give it a name ending in ".rj", preferably without spaces.

Examples for possible names: 

 myfirstscene.rj
 rock_the_disco.rj
 FeelTheHeat.rj

or similar.

Then you have to edit the file "Info.plist".

- Replace "YOUR NAME" with your name
- Replace "SCENE TEMPLATE" with a nice name for your scene
- Replace "PLEASE DESCRIBE YOUR SCENE HERE!" with any description you like.

Save the file.

You probably also want to give your scene a different image. Replace
"image.jpg" with an image of size 320x320 pixels and "thumb.jpg" with an image
of size 55x55 pixels.

Finally open and edit _main.pd to write your scene. Do not remove the [declare]
objects there, unless you know what you're doing.

Testing a scene
===============

Using rjzserver
---------------

Ideally you can use the rjzserver programm to start a local webserver on your
computer and download to the device from there. rjzserver is a Python programm,
whose source code is in the rjzserver directory. Linux users can run the
program from there provided they have Python installed and the wxwidgets Python
bindings.  For Windows and OS-X-users we include binary packages in the
"rjzserver.win" rsp. "rjzserver.app" directories. 

To use the rjzserver, fire up the respective program for your platform and set
the directory where your scenes are with the "File -> Set Scene directory" menu
entry. You can try with the example scenes first. 

A webpage will open that displays an rjzserver webpage, with instructions and a list of your scenes. The adress will be something like http://192.168.1.1:8314/, where the IP number is the adress of your computer on the local network.

Additionally you need to change the proxy settings on your iPhone/iPod to let
RjDj install scenes from your own computer instead of rjdj.me

Go to "Settings/Wifi/" on the iPhone and pick a network's blue arrow. Scroll
down to the proxy settings and change them to:

    * HTTP Proxy on
    * Manual
    * Server: your IP address as reported by rjzserver (like "192.168.1.1")
    * Port: 8314 

After that, any website you try to open with Safari on the phone should give
you the rjzserver page instead. If you want to browse normally later, just turn
the "HTTP Proxy off" again.

Now navigate to http://rjdj.me on the Phone and you should see the rjzserver
page. If you click the Scene names, the Scene will be installed to your RjDj
application.  The links labelled "download" do not install the scenes, they can
be used to download the scene archive and save it somewhere for example to
share the scene with others.


Testing without rjzserver
-------------------------

Once you have finished your scene you have to zip the scene directory and
rename the file so it ends in ".rjz". If you have Python installed, the
rjdj-zip script in "rjutils" does it for you and also checks for common errors
like missing Info.plist or so.

Then you can go to http://www.rjdj.me/sharescene/ to upload it to the rjdj.me
webserver and download it to the iPod/iPhone client from there.

This method is very simple, but for every version you need to upload a new file and you get a new URL to install the scene from. If you do extensive scene development and testing, we really recommend getting Rjzserver running!

Utilities
=========

In rjutils, you will find some useful files to use when developing Scenes on your
computer. E.g. the accelerometerdata includes some recorded accelerometer data.

Example Scenes
==============

This is a growing collection of example scenes doing common tasks. 

Doc
====

With kind permission by Andy Farnell and his publisher we are happy to include
an excerpt of: 

"Designing Sound - Practical synthetic sound design for film, games and
interactive media using dataflow"

here. It gives a detailed introduction into working with the RjDj composer's
software (i.e. Pd). Please consider buying the full book, which includes many
more examples and theory. Check out http://aspress.co.uk/ds/
