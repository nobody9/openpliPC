#include <lib/dvb/volume.h>
#include <lib/gdi/xineLib.h>

eDVBVolumecontrol* eDVBVolumecontrol::instance = NULL;

eDVBVolumecontrol* eDVBVolumecontrol::getInstance()
{
	if (instance == NULL)
		instance = new eDVBVolumecontrol;
	return instance;
}

eDVBVolumecontrol::eDVBVolumecontrol()
{
#ifdef HAVE_ALSA
	mainVolume = NULL;
	openMixer();
#endif
	volumeUnMute();
	setVolume(100, 100);
}


void eDVBVolumecontrol::volumeUp(int left, int right)
{
	setVolume(leftVol + left, rightVol + right);
}

void eDVBVolumecontrol::volumeDown(int left, int right)
{
	setVolume(leftVol - left, rightVol - right);
}

int eDVBVolumecontrol::checkVolume(int vol)
{
	if (vol < 0)
		vol = 0;
	else if (vol > 100)
		vol = 100;
	return vol;
}

void eDVBVolumecontrol::setVolume(int left, int right)
{
	cXineLib *xineLib = cXineLib::getInstance();

	/* left, right is 0..100 */
	leftVol = checkVolume(left);
	rightVol = checkVolume(right);

	xineLib->setVolume(leftVol);
	
	eDebug("Setvolume: %d %d", leftVol, rightVol);
}

int eDVBVolumecontrol::getVolume()
{
	return leftVol;
}

bool eDVBVolumecontrol::isMuted()
{
	return muted;
}


void eDVBVolumecontrol::volumeMute()
{
	cXineLib *xineLib = cXineLib::getInstance();

	xineLib->setVolumeMute(1);
	muted = true;
}

void eDVBVolumecontrol::volumeUnMute()
{
	cXineLib *xineLib = cXineLib::getInstance();

	xineLib->setVolumeMute(0);
	muted = false;
}

void eDVBVolumecontrol::volumeToggleMute()
{
	if (isMuted())
		volumeUnMute();
	else
		volumeMute();
}
