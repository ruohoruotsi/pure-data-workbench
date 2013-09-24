#!/bin/sh

# make sure we're up to date with the latest version
bzr up

# generate the documentation for each module
pydoc -w Pd
pydoc -w PdParser

# create the tar file
bzr export PyPd-`bzr revno`.tar.gz .

# export the documentation from our modules into text files
python -c "from Pd import Pd; print Pd.__doc__" > Pd_doc.txt
python -c "from PdParser import PdParser; print PdParser.__doc__" > PdParser_doc.txt

# output our page
infile=`cat web/index.src.html`
echo "${infile//\<\#version\#\>/`bzr revno`}" > index.shtml
