#!/bin/sh

## echo start this scene...

PATCH=$1

if [ ! -e "${PATCH}" ]
then
 PATCH=_main.pd
fi

pd -path ../../rjlib/:../../rjutils:. -open ../../rjutils/rjdj-player.pd -open "${PATCH}"
