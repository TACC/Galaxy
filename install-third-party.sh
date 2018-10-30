#! /bin/bash

echo INSTALL-THIRD-PARTY $*
tpdir=$1/third-party
shift

if test ! -e $tpdir ; then
  mkdir $tpdir
fi

for i in $* ; do
  for j in lib lib64 ; do
    if test -e $i/$j ; then
      cp -R $i/$j/*  $tpdir
    fi
  done
done
