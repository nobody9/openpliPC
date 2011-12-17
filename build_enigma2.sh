#!/bin/sh

# where install Enigma2 tree
INSTALL_DIR = "/usr/local/e2"

#define enigma2 ref and enigma2 patch
###REF="40eea73ce3269c031d231e8cf4ef307cfbc1466c"
###PATCH="openpliPC_20111216.patch"

#Build and install enigma2:
#echo "--------------------------------------"
#echo "downloading OpenPli sources, please wait..."
#echo "--------------------------------------"
#PKG="enigma2"
#if [ -d $PKG ]; then
#	echo "Erasing older build dir"
#	rm -Rf $PKG
#	rm -f $PKG*
#fi
#git clone git://openpli.git.sourceforge.net/gitroot/openpli/enigma2

#echo "--------------------------------------"
#echo "downloading OpenPli sources, please wait..."
#echo "--------------------------------------"

cd enigma2
#git checkout $REF
 
echo "--------------------------------------"
echo "configuring OpenPli sources"
echo "--------------------------------------"
#patch -p1 < ../patches/$PATCH

autoreconf -i
./configure --prefix=$INSTALL_DIR --with-xlib --with-debug
 
echo "--------------------------------------"
echo "build OpenPli, please wait..."
echo "--------------------------------------"
make
 
echo "--------------------------------------"
echo "installing OpenPli in $INSTALL_DIR"
echo "--------------------------------------"
sudo make install
cd ..
 
echo "--------------------------------------"
echo "final changes"
echo "--------------------------------------"

# strip binary
sudo strip $INSTALL_DIR/bin/enigma2

# removing pre-compiled py files
sudo find $INSTALL_DIR/lib/enigma2/python/ -name "*.py[oc]" -exec rm {} \;

# copying needed files
sudo mkdir -p $INSTALL_DIR/etc/enigma2
sudo mkdir -p $INSTALL_DIR/etc/tuxbox
sudo cp share/fonts/* $INSTALL_DIR/share/fonts
sudo cp -rf etc/* $INSTALL_DIR/etc

echo ""
echo "**********************<END>**********************"