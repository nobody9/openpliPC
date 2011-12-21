	/* note: this requires gstreamer 0.10.x and a big list of plugins. */
	/* it's currently hardcoded to use a big-endian alsasink as sink. */
#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/base/nconfig.h>
#include <lib/base/object.h>
#include <lib/dvb/decoder.h>
#include <lib/components/file_eraser.h>
#include <lib/gui/esubtitle.h>
#include <lib/service/servicemp3.h>
#include <lib/service/service.h>
#include <lib/gdi/gpixmap.h>

#include <string>
#include <sys/stat.h>

#define HTTP_TIMEOUT 10

// eServiceFactoryMP3

eServiceFactoryMP3::eServiceFactoryMP3()
{
	ePtr<eServiceCenter> sc;
	
	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		extensions.push_back("dts");
		extensions.push_back("mp2");
		extensions.push_back("mp3");
		extensions.push_back("ogg");
		extensions.push_back("mpg");
		extensions.push_back("vob");
		extensions.push_back("wav");
		extensions.push_back("wave");
		extensions.push_back("m4v");
		extensions.push_back("mkv");
		extensions.push_back("avi");
		extensions.push_back("divx");
		extensions.push_back("dat");
		extensions.push_back("flac");
		extensions.push_back("flv");
		extensions.push_back("mp4");
		extensions.push_back("mov");
		extensions.push_back("m4a");
		extensions.push_back("mts");
		extensions.push_back("m2ts");
		sc->addServiceFactory(eServiceFactoryMP3::id, this, extensions);
	}

	m_service_info = new eStaticServiceMP3Info();
}

eServiceFactoryMP3::~eServiceFactoryMP3()
{
	ePtr<eServiceCenter> sc;
	
	eServiceCenter::getPrivInstance(sc);
	if (sc)
		sc->removeServiceFactory(eServiceFactoryMP3::id);
}

DEFINE_REF(eServiceFactoryMP3)

	// iServiceHandler
RESULT eServiceFactoryMP3::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
		// check resources...
	ptr = new eServiceMP3(ref);
	return 0;
}

RESULT eServiceFactoryMP3::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryMP3::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryMP3::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = m_service_info;
	return 0;
}

class eMP3ServiceOfflineOperations: public iServiceOfflineOperations
{
	DECLARE_REF(eMP3ServiceOfflineOperations);
	eServiceReference m_ref;
public:
	eMP3ServiceOfflineOperations(const eServiceReference &ref);
	
	RESULT deleteFromDisk(int simulate);
	RESULT getListOfFilenames(std::list<std::string> &);
	RESULT reindex();
};

DEFINE_REF(eMP3ServiceOfflineOperations);

eMP3ServiceOfflineOperations::eMP3ServiceOfflineOperations(const eServiceReference &ref): m_ref((const eServiceReference&)ref)
{
}

RESULT eMP3ServiceOfflineOperations::deleteFromDisk(int simulate)
{
	if (!simulate)
	{
		std::list<std::string> res;
		if (getListOfFilenames(res))
			return -1;
		
		eBackgroundFileEraser *eraser = eBackgroundFileEraser::getInstance();
		if (!eraser)
			eDebug("FATAL !! can't get background file eraser");

		for (std::list<std::string>::iterator i(res.begin()); i != res.end(); ++i)
		{
			eDebug("Removing %s...", i->c_str());
			if (eraser)
				eraser->erase(i->c_str());
			else
				::unlink(i->c_str());
		}
	}
	return 0;
}

RESULT eMP3ServiceOfflineOperations::getListOfFilenames(std::list<std::string> &res)
{
	res.clear();
	res.push_back(m_ref.path);
	return 0;
}

RESULT eMP3ServiceOfflineOperations::reindex()
{
	return -1;
}


RESULT eServiceFactoryMP3::offlineOperations(const eServiceReference &ref, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = new eMP3ServiceOfflineOperations(ref);
	return 0;
}

// eStaticServiceMP3Info


// eStaticServiceMP3Info is seperated from eServiceMP3 to give information
// about unopened files.

// probably eServiceMP3 should use this class as well, and eStaticServiceMP3Info
// should have a database backend where ID3-files etc. are cached.
// this would allow listing the mp3 database based on certain filters.

DEFINE_REF(eStaticServiceMP3Info)

eStaticServiceMP3Info::eStaticServiceMP3Info()
{
}

RESULT eStaticServiceMP3Info::getName(const eServiceReference &ref, std::string &name)
{
	if ( ref.name.length() )
		name = ref.name;
	else
	{
		size_t last = ref.path.rfind('/');
		if (last != std::string::npos)
			name = ref.path.substr(last+1);
		else
			name = ref.path;
	}
	return 0;
}

int eStaticServiceMP3Info::getLength(const eServiceReference &ref)
{
	return -1;
}

int eStaticServiceMP3Info::getInfo(const eServiceReference &ref, int w)
{
	switch (w)
	{
	case iServiceInformation::sTimeCreate:
		{
			struct stat s;
			if(stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_mtime;
			}
		}
		break;
	}
	return iServiceInformation::resNA;
}

PyObject* eStaticServiceMP3Info::getInfoObject(const eServiceReference &ref, int w)
{
	switch(w)
	{
	case iServiceInformation::sFileSize:
		{
			struct stat s;
			if(stat(ref.path.c_str(), &s) == 0)
			{
				return PyLong_FromLongLong(s.st_size);
			}
		}
		break;
	}
	Py_RETURN_NONE;
}

// eServiceMP3
int eServiceMP3::ac3_delay,
    eServiceMP3::pcm_delay;

eServiceMP3::eServiceMP3(eServiceReference ref)
	:m_ref(ref)
{
	cXineLib *xineLib = cXineLib::getInstance();
//xineLib->stopVideo();
	m_state = stIdle;
	eDebug("eServiceMP3::construct!");

	const char *filename = m_ref.path.c_str();
//const char *ext = strrchr(filename, '.');
	int uzunluk;
	uzunluk=strlen(filename);
	char myfilesrt[1000];
	sprintf(myfilesrt,"%s",filename);
	myfilesrt[uzunluk-4]='\0';
	char myfile[1000];
//xine videoFileName.avi#subtitle:subtitleFileName.srt
	sprintf(myfile,"%s#subtitle:%s.srt",filename,myfilesrt);
//sprintf(myfile,"%s",filename);
	ASSERT(m_state == stIdle);
	xineLib->FilmVideo(myfile);
	m_state = stRunning;
	m_event(this, evStart);
	return ;
 
}

eServiceMP3::~eServiceMP3()
{
	if (m_state == stRunning)
		stop();
}

DEFINE_REF(eServiceMP3);


RESULT eServiceMP3::connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
	return 0;
}

RESULT eServiceMP3::start()
{
//	ASSERT(m_state == stIdle);
	m_state = stRunning;
	m_event(this, evStart);

	return 0;
}

void eServiceMP3::sourceTimeout()
{
	eDebug("eServiceMP3::http source timeout! issuing eof...");
	m_event((iPlayableService*)this, evEOF);
}

RESULT eServiceMP3::stop()
{
	ASSERT(m_state != stIdle);

	if (m_state == stStopped)
		return -1;

	m_state = stStopped;
	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->stopVideo();

	return 0;
}

RESULT eServiceMP3::setTarget(int target)
{
	return -1;
}

RESULT eServiceMP3::pause(ePtr<iPauseableService> &ptr)
{
	ptr=this;
	return 0;
}

RESULT eServiceMP3::setSlowMotion(int ratio)
{
	if (!ratio)
		return 0;
return 0;
}

RESULT eServiceMP3::setFastForward(int ratio)
{
	eDebug("eServiceMP3::setFastForward ratio=%i",ratio);
	return trickSeek(ratio);
}

		// iPausableService
RESULT eServiceMP3::pause()
{
	if (m_state != stRunning)
		return -1;

	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->VideoPause();

	return 0;
}

RESULT eServiceMP3::unpause()
{
	if (m_state != stRunning)
		return -1;

	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->VideoResume();

	return 0;
}

	/* iSeekableService */
RESULT eServiceMP3::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::getLength(pts_t &pts)
{
	if (m_state != stRunning)
		return -1;

	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->VideoPosisyon();
	pts=xineLib->Vlength*90;
	return 0;
}

RESULT eServiceMP3::seekToImpl(pts_t to)
{
	return 0;
}

RESULT eServiceMP3::seekTo(pts_t to)
{
	RESULT ret = 0;
	return ret;
}


RESULT eServiceMP3::trickSeek(int ratio)
{
	printf("----Ratio=%d\n",ratio);
	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->VideoIleriF();
	//if (!ratio) return seekRelative(0, 0);
	return 0;
}


RESULT eServiceMP3::seekRelative(int direction, pts_t to)
{
	eDebug("eDVBServicePlay::seekRelative: jump %d, %lld", direction, to);
	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->VideoGeriT(to/90*direction);
	return 0;
}

RESULT eServiceMP3::getPlayPosition(pts_t &pts)
{
	if (m_state != stRunning)
		return -1;

	cXineLib *xineLib = cXineLib::getInstance();
	xineLib->VideoPosisyon();
	pts=xineLib->Vpos*90;
	return 0;
}

RESULT eServiceMP3::setTrickmode(int trick)
{
		/* trickmode is not yet supported by our dvbmediasinks. */
	return -1;
}

RESULT eServiceMP3::isCurrentlySeekable()
{
	if (m_state != stRunning)
		return 0;

	int ret = 3; // seeking and fast/slow winding possible
	return ret;
}

RESULT eServiceMP3::info(ePtr<iServiceInformation>&i)
{
	i = this;
	return 0;
}

RESULT eServiceMP3::getName(std::string &name)
{
	std::string title = m_ref.getName();
	if (title.empty())
	{
		name = m_ref.path;
		size_t n = name.rfind('/');
		if (n != std::string::npos)
			name = name.substr(n + 1);
	}
	else
		name = title;
	return 0;
}

int eServiceMP3::getInfo(int w)
{
	cXineLib *xineLib = cXineLib::getInstance();
 
 	switch (w)
 	{
 	case sServiceref: return m_ref;
	case sVideoHeight:
		return xineLib->getVideoHeight();
		break;
	case sVideoWidth:
		return xineLib->getVideoWidth();
		break;
	case sFrameRate:
		return xineLib->getVideoFrameRate();
		break;
 	case sTagTitle:
 	case sTagArtist:
 	case sTagAlbum:
	case sTagTitleSortname:
	case sTagArtistSortname:
	case sTagAlbumSortname:
	case sTagDate:
	case sTagComposer:
	case sTagGenre:
	case sTagComment:
	case sTagExtendedComment:
	case sTagLocation:
	case sTagHomepage:
	case sTagDescription:
	case sTagVersion:
	case sTagISRC:
	case sTagOrganization:
	case sTagCopyright:
	case sTagCopyrightURI:
	case sTagContact:
	case sTagLicense:
	case sTagLicenseURI:
	case sTagCodec:
	case sTagAudioCodec:
	case sTagVideoCodec:
	case sTagEncoder:
	case sTagLanguageCode:
	case sTagKeywords:
	case sTagChannelMode:
	case sUser+12:
		return resIsString;
	case sTagTrackGain:
	case sTagTrackPeak:
	case sTagAlbumGain:
	case sTagAlbumPeak:
	case sTagReferenceLevel:
	case sTagBeatsPerMinute:
	case sTagImage:
 	case sTagPreviewImage:
 	case sTagAttachment:
		return resIsPyObject;
		break;
	default:
		return resNA;
	}

	return 0;
}

std::string eServiceMP3::getInfoString(int w)
{
	return "";
}

PyObject *eServiceMP3::getInfoObject(int w)
{
	return 0;
//	Py_RETURN_NONE;
}

RESULT eServiceMP3::audioChannel(ePtr<iAudioChannelSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::audioTracks(ePtr<iAudioTrackSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::subtitle(ePtr<iSubtitleOutput> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::audioDelay(ePtr<iAudioDelay> &ptr)
{
	ptr = this;
	return 0;
}

int eServiceMP3::getNumberOfTracks()
{
}

int eServiceMP3::getCurrentTrack()
{
}

RESULT eServiceMP3::selectTrack(unsigned int i)
{
	pts_t ppos;
	getPlayPosition(ppos);
	ppos -= 90000;
	if (ppos < 0)
		ppos = 0;

	int ret = selectAudioStream(i);
	if (!ret) {
		/* flush */
		seekTo(ppos);
	}

	return ret;
}

int eServiceMP3::selectAudioStream(int i)
{
}

int eServiceMP3::getCurrentChannel()
{
	return STEREO;
}

RESULT eServiceMP3::selectChannel(int i)
{
	eDebug("eServiceMP3::selectChannel(%i)",i);
	return 0;
}

RESULT eServiceMP3::getTrackInfo(struct iAudioTrackInfo &info, unsigned int i)
{
	return 0;
}

eAutoInitPtr<eServiceFactoryMP3> init_eServiceFactoryMP3(eAutoInitNumbers::service+1, "eServiceFactoryMP3");

void eServiceMP3::pushSubtitles()
{
}

RESULT eServiceMP3::enableSubtitles(eWidget *parent, ePyObject tuple)
{
}

PyObject *eServiceMP3::getCachedSubtitle()
{
// 	eDebug("eServiceMP3::getCachedSubtitle");
	Py_RETURN_NONE;
}

PyObject *eServiceMP3::getSubtitleList()
{
// 	eDebug("eServiceMP3::getSubtitleList");
	ePyObject l = PyList_New(0);
	int stream_idx = 0;
	eDebug("eServiceMP3::getSubtitleList finished");
	return l;
}

RESULT eServiceMP3::streamed(ePtr<iStreamedService> &ptr)
{
	ptr = this;
	return 0;
}

int eServiceMP3::getAC3Delay()
{
	return ac3_delay;
}

int eServiceMP3::getPCMDelay()
{
	return pcm_delay;
}

void eServiceMP3::setAC3Delay(int delay)
{
}

void eServiceMP3::setPCMDelay(int delay)
{
}

