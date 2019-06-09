#!/bin/sh

SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"

VERSION_FILE=$SCRIPTDIR/version.txt
VERSION=$(cat $VERSION_FILE)
REVISION_FILE=$SCRIPTDIR/revision.txt
REVISION=$(cat $REVISION_FILE)
echo $VERSION-$REVISION
