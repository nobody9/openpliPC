#!/bin/sh

# where install Enigma2 tree
INSTALL_E2DIR="/usr/local/e2"

BACKUP_E2="etc/enigma2 etc/tuxbox/*.xml etc/tuxbox/nim_sockets"

# ----------------------------------------------------------------------

DO_BACKUP=0
DO_RESTORE=0

function e2_backup {
        echo "-----------------------------"
        echo "BACKUP E2 CONFIG"
        echo "-----------------------------"

	tar -C $INSTALL_E2DIR -v -c -z -f e2backup.tgz $BACKUP_E2
}

function e2_restore {
        echo "-----------------------------"
        echo "RESTORE OLD E2 CONFIG"
        echo "-----------------------------"
        
	if [ -f e2backup.tgz ]; then
		sudo tar -C $INSTALL_E2DIR -v -x -z -f e2backup.tgz
	fi
}


while [ "$1" != "" ]; do
    case $1 in
        -b )  DO_BACKUP=1
              ;;
        -r )  DO_RESTORE=1
              ;;
    esac
    shift
done

if [ "$DO_BACKUP" -eq "1" ]; then
	e2_backup
fi

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

ln -sf $INSTALL_E2DIR/bin/enigma2 ./e2bin

if [ "$DO_RESTORE" -eq "1" ]; then
	e2_restore
fi

echo ""
echo "**********************<END>**********************"
