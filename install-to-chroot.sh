#!/bin/bash
# 
# Ideas and some parts from the original dgl-create-chroot (by joshk@triplehelix.org, modifications by jilles@stack.nl)
# More by <paxed@alt.org>
# More by Michael Andrew Streib <dtype@dtype.org>
# Licensed under the MIT License
# https://opensource.org/licenses/MIT

# autonamed chroot directory. Can rename.
DATESTAMP=`date +%Y%m%d-%H%M%S`
NAO_CHROOT=/opt/nethack/hardfought.org
#NAO_CHROOT=/opt/nethack/chroot
# config outside of chroot
DGL_CONFIG="/opt/nethack/dgamelaunch.conf"
# already compiled versions of dgl and nethack
DGL_GIT="/home/build/dgamelaunch"
NETHACK_GIT="/home/build/NetHack"
GRUNTHACK_GIT="/home/build/GruntHack"
# the user & group from dgamelaunch config file.
USRGRP="games:games"
# COMPRESS from include/config.h; the compression binary to copy. leave blank to skip.
COMPRESSBIN="/bin/gzip"
# fixed data to copy (leave blank to skip)
NH_GIT="/home/build/NetHack"
GH_GIT="/home/build/GruntHack"
NH_BRANCH="3.4.3"
GH_BRANCH="0.2.0"
# HACKDIR from include/config.h; aka nethack subdir inside chroot
NHSUBDIR="nh343"
GHSUBDIR="gh020"
# VAR_PLAYGROUND from include/unixconf.h
NH_VAR_PLAYGROUND="/nh343/var/"
GH_VAR_PLAYGROUND="/gh020/var/"
# only define this if dgl was configured with --enable-sqlite
SQLITE_DBFILE="/dgldir/dgamelaunch.db"
# END OF CONFIG
##############################################################################

errorexit()
{
    echo "Error: $@" >&2
    exit 1
}

findlibs()
{
  for i in "$@"; do
      if [ -z "`ldd "$i" | grep 'not a dynamic executable'`" ]; then
         echo $(ldd "$i" | awk '{ print $3 }' | egrep -v ^'\(' | grep lib)
         echo $(ldd "$i" | grep 'ld-linux' | awk '{ print $1 }')
      fi
  done
}

set -e

umask 022

echo "Creating inprogress and userdata directories"
mkdir -p "$NAO_CHROOT/dgldir/inprogress-gh020"
chown "$USRGRP" "$NAO_CHROOT/dgldir/inprogress-gh020"

echo "Making $NAO_CHROOT/$GHSUBDIR"
mkdir -p "$NAO_CHROOT/$GHSUBDIR"

GRUNTHACKBIN="$GRUNTHACK_GIT/src/grunthack"
if [ -n "$GRUNTHACKBIN" -a ! -e "$GRUNTHACKBIN" ]; then
  errorexit "Cannot find GruntHack binary $GRUNTHACKBIN"
fi

if [ -n "$GRUNTHACKBIN" -a -e "$GRUNTHACKBIN" ]; then
  echo "Copying $GRUNTHACKBIN"
  cd "$NAO_CHROOT/$GHSUBDIR"
  GHBINFILE="`basename $GRUNTHACKBIN`-$DATESTAMP"
  cp "$GRUNTHACKBIN" "$GHBINFILE"
  ln -s "$GHBINFILE" grunthack
  LIBS="$LIBS `findlibs $GRUNTHACKBIN`"
  cd "$NAO_CHROOT"
fi

echo "Copying NetHack playground stuff"
cp "$GRUNTHACK_GIT/dat/ghdat" "$NAO_CHROOT/$GHSUBDIR"
chmod 644 "$NAO_CHROOT/$GHSUBDIR/ghdat"

echo "Creating NetHack variable dir stuff."
mkdir -p "$NAO_CHROOT/$GHSUBDIR/var"
chown -R "$USRGRP" "$NAO_CHROOT/$GHSUBDIR/var"
mkdir -p "$NAO_CHROOT/$GHSUBDIR/var/save"
chown -R "$USRGRP" "$NAO_CHROOT/$GHSUBDIR/var/save"

touch "$NAO_CHROOT/$GHSUBDIR/var/logfile"
chown -R "$USRGRP" "$NAO_CHROOT/$GHSUBDIR/var/logfile"
touch "$NAO_CHROOT/$GHSUBDIR/var/perm"
chown -R "$USRGRP" "$NAO_CHROOT/$GHSUBDIR/var/perm"
touch "$NAO_CHROOT/$GHSUBDIR/var/record"
chown -R "$USRGRP" "$NAO_CHROOT/$GHSUBDIR/var/record"
touch "$NAO_CHROOT/$GHSUBDIR/var/xlogfile"
chown -R "$USRGRP" "$NAO_CHROOT/$GHSUBDIR/var/xlogfile"

LIBS=`for lib in $LIBS; do echo $lib; done | sort | uniq`
echo "Copying libraries:" $LIBS
for lib in $LIBS; do
        mkdir -p "$NAO_CHROOT`dirname $lib`"
        if [ -f "$NAO_CHROOT$lib" ]
	then
		echo "$NAO_CHROOT$lib already exists - skipping."
	else
		cp $lib "$NAO_CHROOT$lib"
	fi
done

echo "Finished."


