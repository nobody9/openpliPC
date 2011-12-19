#!/bin/bash
#To build vdpauinfo and qvdpautest

echo "-----------------------------------------"
echo "*** INSTALL REQUIRED PACKAGES ***"
echo "-----------------------------------------"
REQPKG="autoconf automake build-essential \
	qt4-dev-tools \
	"

for p in $REQPKG; do
	echo -n ">>> Checking \"$p\" : "
	dpkg -s $p >/dev/null
	if [ "$?" -eq "0" ]; then
		echo "package is installed, skip it"
	else
		echo "package NOT present, installing it"
		sudo apt-get -y install $p
	fi
done

cd libs

PKG="vdpauinfo-0.0.6"
echo "-----------------------------------------"
echo "Build and install $PKG"
echo "-----------------------------------------"
if [ -d $PKG ]; then
	echo "Erasing older build dir"
	rm -Rf $PKG
fi

tar xzf $PKG.tar.gz
cd $PKG
./autogen.sh
./configure
make
cd ..


PKG="qvdpautest-0.5.1"
echo "-----------------------------------------"
echo "Build and install $PKG"
echo "-----------------------------------------"
if [ -d $PKG ]; then
	echo "Erasing older build dir"
	rm -Rf $PKG
fi

tar xzf qvdpautest-0.5.1.tar.gz
cd $PKG
qmake
make
cd ..


cd ..
echo "*********************<END>*********************"


