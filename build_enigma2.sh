#!/bin/sh

#define enigma2 ref and enigma2 patch
REF="40eea73ce3269c031d231e8cf4ef307cfbc1466c"
PATCH="openpliPC_20111216.patch"
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
./configure --prefix=/usr/local/e2 --with-xlib --with-debug
 
echo "--------------------------------------"
echo "build OpenPli, please wait..."
echo "--------------------------------------"
make
 
echo "--------------------------------------"
echo "installing OpenPli in /usr/local/e2"
echo "--------------------------------------"
sudo make install
cd ..
 
echo "--------------------------------------"
echo "final changes"
echo "--------------------------------------"
sudo strip /usr/local/e2/bin/enigma2
#remove compiled py-files
sudo find /usr/local/e2/lib/enigma2/python/ -name '*.pyo' -exec rm {} \;
sudo find /usr/local/e2/lib/enigma2/python/ -name '*.pyc' -exec rm {} \;
#copy needed files
sudo mkdir -p /usr/local/e2/etc/enigma2
sudo mkdir -p /usr/local/e2/etc/tuxbox
sudo cp share/fonts/* /usr/local/e2/share/fonts
sudo cp -rf etc/* /usr/local/e2/etc
 
echo "**********************<END>**********************"