#!/bin/bash

# where is Enigma2 plugins diretory
INSTALL_E2PDIR="/usr/local/e2/usr/lib/enigma2/python/Plugins"

# ----------------------------------------------------------------------

	# Build and install e2openplugin-OpenWebif:
	PKG="e2openplugin-OpenWebif"
	echo "-----------------------------------------"
	echo "getting $PKG"
	echo "-----------------------------------------"

	git clone git://github.com/E2OpenPlugins/PKG.git
	cd $PKG/plugin

	echo "--------------------------------------"
	echo "installing $PKG"
	echo "--------------------------------------"

	sudo mkdir -p $INSTALL_E2PDIR/Extensions/OpenWebif

	echo "--------------------------------------"
	echo "build $PKG, please wait..."
	echo "--------------------------------------"

	sudo cheetah-compile -R --nobackup $INSTALL_E2PDIR/Extensions/OpenWebif
	sudo python -O -m compileall $INSTALL_E2PDIR/Extensions/OpenWebif

	cd ..

# ----------------------------------------------------------------------

echo ""
echo "**********************<END>**********************"
