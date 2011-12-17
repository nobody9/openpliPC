#!/bin/sh

# where install Enigma2 tree
INSTALL_E2DIR = "/usr/local/e2"

#define enigma2 ref and enigma2 patch
###REF="40eea73ce3269c031d231e8cf4ef307cfbc1466c"
###PATCH="openpliPC_20111216.patch"

# ----------------------------------------------------------------------

# Build and install xine-lib:
PKG="xine-lib"
echo "-----------------------------------------"
echo "configuring OpenPliPC $PKG"
echo "-----------------------------------------"

#if [ -d $PKG ]; then
#	echo "Erasing older build dir"
#	rm -Rf $PKG
#	rm -f $PKG*
#fi
#git clone git://projects.vdr-developer.org/$PKG.git

cd $PKG

#git checkout df-osd-handling+alter-vdpau-h264-decoder
#git checkout $XINELIB_REF
#patch -p1 < ../patches/$XINELIB_PATCH

./autogen.sh --disable-xinerama --disable-musepack --prefix=/usr

echo "-----------------------------------------"
echo "build OpenPliPC $PKG, please wait..."
echo "-----------------------------------------"

make

echo "--------------------------------------"
echo "installing OpenPliPC $PKG"
echo "--------------------------------------"

sudo make install
cd ..

# ----------------------------------------------------------------------

# Build and install enigma2:

#echo "--------------------------------------"
#echo "downloading OpenPli sources, please wait..."
#echo "--------------------------------------"

PKG="enigma2"

#if [ -d $PKG ]; then
# echo "Erasing older build dir"
#	rm -Rf $PKG
#	rm -f $PKG*
#fi
#git clone git://openpli.git.sourceforge.net/gitroot/openpli/enigma2

#echo "--------------------------------------"
#echo "downloading OpenPli sources, please wait..."
#echo "--------------------------------------"

echo "--------------------------------------"
echo "configuring OpenPliPC $PKG"
echo "--------------------------------------"

cd $PKG

#git checkout $REF
#patch -p1 < ../patches/$PATCH

autoreconf -i
./configure --prefix=$INSTALL_E2DIR --with-xlib --with-debug
 
echo "--------------------------------------"
echo "build OpenPliPC $PKG, please wait..."
echo "--------------------------------------"

make
 
echo "--------------------------------------"
echo "installing OpenPliPC $PKG in $INSTALL_E2DIR"
echo "--------------------------------------"

sudo make install
cd ..
 
echo "--------------------------------------"
echo "final step: installing E2 conf files"
echo "--------------------------------------"

# strip binary
sudo strip $INSTALL_E2DIR/bin/enigma2

# removing pre-compiled py files
sudo find $INSTALL_E2DIR/lib/enigma2/python/ -name "*.py[oc]" -exec rm {} \;

# copying needed files
sudo mkdir -p $INSTALL_E2DIR/etc/enigma2
sudo mkdir -p $INSTALL_E2DIR/etc/tuxbox
sudo cp share/fonts/* $INSTALL_E2DIR/share/fonts
sudo cp -rf etc/* $INSTALL_E2DIR/etc

echo ""
echo "**********************<END>**********************"