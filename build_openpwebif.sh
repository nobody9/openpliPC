#!/bin/bash
echo "-----------------------------------------"
echo "*** INSTALL REQUIRED PACKAGES ***"
echo "-----------------------------------------"
REQPKG="python-cheetah python-twisted-web \
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

# where is Enigma2 plugins diretory
INSTALL_E2PDIR="/usr/local/e2/lib/enigma2/python/Plugins"

# ----------------------------------------------------------------------

# Build and install e2openplugin-OpenWebif:
PKG="e2openplugin-OpenWebif"
echo "-----------------------------------------"
echo "getting $PKG"
echo "-----------------------------------------"
if [ -d $PKG ]; then
	echo "Erasing older build dir"
	rm -Rf $PKG
	rm -f $PKG*
fi
git clone git://github.com/E2OpenPlugins/$PKG.git
cd $PKG/plugin

echo "--------------------------------------"
echo "installing $PKG"
echo "--------------------------------------"

sudo mkdir -p $INSTALL_E2PDIR/Extensions/OpenWebif

echo "--------------------------------------"
echo "build $PKG, please wait..."
echo "--------------------------------------"
sudo cp -R . $INSTALL_E2PDIR/Extensions/OpenWebif
sudo cheetah-compile -R --nobackup $INSTALL_E2PDIR/Extensions/OpenWebif
sudo python -O -m compileall $INSTALL_E2PDIR/Extensions/OpenWebif

cd ..

# ----------------------------------------------------------------------

echo ""
echo "**********************<END>**********************"
