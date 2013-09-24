#!/bin/bash

bzr export PdLANParty-`bzr revno`.tar.gz .
markdown README > README.html
infile=`cat web/index.src.html`
echo "${infile//\<\#version\#\>/`bzr revno`}" > index.shtml
