#! /bin/bash

echo INSTALL-THIRD-PARTY $*
tpdir=$1/third-party
shift

echo XXXXXXXXXXXXXXX $* XCCCCCCCCCCCCCCCC

if test ! -e $tpdir ; then
  mkdir $tpdir
fi

for i in $* ; do
  cp -R $i/lib/* $tpdir
done
