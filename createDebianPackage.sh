#!/bin/bash
if test -z version.txt; then
    echo "Please create a version file."
    exit 0;
fi
if test -z revision.txt; then
    echo "Please create a revision file."
    exit 0;
fi
if test -z $1; then
    echo "Please specify a distribution (i. e. wheezy)."
    exit 0;
fi
version=$(cat version.txt)
revision=$(cat revision.txt)

rm -Rf python3-homegear*
mkdir python3-homegear-$version
cp -R *.cpp *.h *.py *.in revision.txt version.txt LICENSE README.md debian python3-homegear-$version
date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
echo "python3-homegear ($version-$revision) $1; urgency=low

  * See https://forum.homegear.eu

 -- Sathya Laufer <sathya@laufers.net>  $date" > python3-homegear-$version/debian/changelog
tar -zcpf python3-homegear_$version.orig.tar.gz python3-homegear-$version
cd python3-homegear-$version
debuild -us -uc
cd ..
rm -Rf python3-homegear-$version
