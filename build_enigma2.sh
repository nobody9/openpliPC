#!/bin/sh

#define enigma2 ref and enigma2 patch
REF="ffebefa30b56fac2c114f4b10e3cb32ca33c8b92"
PATCH="openpliPC_20111214.patch"
#Build and install enigma2:
echo "--------------------------------------"
echo "downloading OpenPli sources, please wait..."
echo "--------------------------------------"
PKG="enigma2"
if [ -d $PKG ]; then
	echo "Erasing older build dir"
	rm -Rf $PKG
	rm -f $PKG*
fi
git clone git://openpli.git.sourceforge.net/gitroot/openpli/enigma2
cd enigma2
git checkout $REF
 
echo "--------------------------------------"
echo "configuring OpenPli sources"
echo "--------------------------------------"
patch -p1 < ../patches/$PATCH
autoreconf -i
./configure --prefix=/usr/local --with-xlib --with-debug
 
echo "--------------------------------------"
echo "build OpenPli, please wait..."
echo "--------------------------------------"
make
 
echo "--------------------------------------"
echo "installing OpenPli in /usr/local/"
echo "--------------------------------------"
sudo make install
cd ..
 
echo "--------------------------------------"
echo "final changes"
echo "--------------------------------------"
sudo strip /usr/local/bin/enigma2
#remove compiled py-files
sudo find /usr/local/lib/enigma2/python/ -name '*.pyo' -exec rm {} \;
sudo find /usr/local/lib/enigma2/python/ -name '*.pyc' -exec rm {} \;
#copy needed files
sudo mkdir -p /usr/local/etc/enigma2
sudo mkdir -p /usr/local/etc/tuxbox
sudo cp share/fonts/* /usr/local/share/fonts
sudo cp -rf etc/* /usr/local/etc
 
echo "**********************<END>**********************"