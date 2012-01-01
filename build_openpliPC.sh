#!/bin/bash

# where install Enigma2 tree
INSTALL_E2DIR="/usr/local/e2"

BACKUP_E2="etc/enigma2 etc/tuxbox/*.xml etc/tuxbox/nim_sockets share/enigma2/xine.conf"

# ----------------------------------------------------------------------

DO_BACKUP=0
DO_RESTORE=0
DO_XINE=1
DO_CONFIGURE=1

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

function usage {
	echo "Usage:"
	echo " -b : backup E2 conf file before re-compile"
	echo " -r : restore E2 conf file after re-compile"
	echo " -x : don't compile xine-lib (compile only enigma2)"
	echo " -nc: don't start configure/autoconf"
	echo " -h : this help"
	echo ""
	echo "common usage:"
	echo "  $0 -b -r : make E2 backup, compile E2, restore E2 conf files"
	echo ""
}

while [ "$1" != "" ]; do
    case $1 in
        -b ) 	DO_BACKUP=1
		shift
        	;;
        -r )  	DO_RESTORE=1
		shift
              	;;
	-x )	DO_XINE=0
		shift
		;;
	-nc )	DO_CONFIGURE=0
		shift
		;;	
	-h )  	usage
	      	exit
	      	;;
	*  )  	echo "Unknown parameter"
	      	usage
	      	exit
	      	;;
    esac
done

if [ "$DO_BACKUP" -eq "1" ]; then
	e2_backup
fi

# ----------------------------------------------------------------------

if [ "$DO_XINE" -eq "1" ]; then

	# Build and install xine-lib:
	PKG="xine-lib"

	cd $PKG
	
  if [ "$DO_CONFIGURE" -eq "1" ]; then	
	echo "-----------------------------------------"
	echo "configuring OpenPliPC $PKG"
	echo "-----------------------------------------"

	./autogen.sh --disable-xinerama --disable-musepack --prefix=/usr
  fi	

	echo "-----------------------------------------"
	echo "build OpenPliPC $PKG, please wait..."
	echo "-----------------------------------------"

	make

	echo "--------------------------------------"
	echo "installing OpenPliPC $PKG"
	echo "--------------------------------------"

	sudo make install
	cd ..

fi

# ----------------------------------------------------------------------

# Build and install enigma2:

PKG="enigma2"

cd $PKG

if [ "$DO_CONFIGURE" -eq "1" ]; then

  echo "--------------------------------------"
  echo "configuring OpenPliPC $PKG"
  echo "--------------------------------------"

  autoreconf -i
  ./configure --prefix=$INSTALL_E2DIR --with-xlib --with-debug
fi  
 
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
