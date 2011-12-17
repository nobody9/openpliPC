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

//#include <gst/gst.h>
//#include <gst/pbutils/missing-plugins.h>
//#include <sys/stat.h>

#define HTTP_TIMEOUT 10

// eServiceFactoryMP3

/*
 * gstreamer suffers from a bug causing sparse streams to loose sync, after pause/resume / skip
 * see: https://bugzilla.gnome.org/show_bug.cgi?id=619434
 * As a workaround, we run the subsink in sync=false mode
 */
#define GSTREAMER_SUBTITLE_SYNC_MODE_BUG
/**/

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
	:m_ref(ref), m_pump(eApp, 1)
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

DEFINE_REF(eServiceMP3::GstMessageContainer);

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
	eDebug("eServiceMP3::setSlowMotion ratio=%f",1.0/(gdouble)ratio);
	return trickSeek(1.0/(gdouble)ratio);
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
 	const gchar *tag = 0;
 
 	switch (w)
 	{
 	case sServiceref: return m_ref;
 	case sProgressive: return m_progressive;
 	case sAspect: return m_aspect;
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
	case sTagCRC:
		tag = "has-crc";
		break;
	default:
		return resNA;
	}

	if (!m_stream_tags || !tag)
		return 0;

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
 	return m_audioStreams.size();
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
 	if (i >= m_audioStreams.size())
		return -2;
		info.m_description = m_audioStreams[i].codec;
/*	if (m_audioStreams[i].type == atMPEG)
		info.m_description = "MPEG";
	else if (m_audioStreams[i].type == atMP3)
		info.m_description = "MP3";
	else if (m_audioStreams[i].type == atAC3)
		info.m_description = "AC3";
	else if (m_audioStreams[i].type == atAAC)
		info.m_description = "AAC";
	else if (m_audioStreams[i].type == atDTS)
		info.m_description = "DTS";
	else if (m_audioStreams[i].type == atPCM)
		info.m_description = "PCM";
	else if (m_audioStreams[i].type == atOGG)
		info.m_description = "OGG";
	else if (m_audioStreams[i].type == atFLAC)
		info.m_description = "FLAC";
	else
		info.m_description = "???";*/
	if (info.m_language.empty())
		info.m_language = m_audioStreams[i].language_code;
	return 0;
}

subtype_t getSubtitleType(GstPad* pad, gchar *g_codec=NULL)
{
}

void eServiceMP3::gstBusCall(GstMessage *msg)
{
}

void eServiceMP3::handleMessage(GstMessage *msg)
{
	if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_STATE_CHANGED && GST_MESSAGE_SRC(msg) != GST_OBJECT(m_gst_playbin))
	{
		/*
		 * ignore verbose state change messages for all active elements;
		 * we only need to handle state-change events for the playbin
		 */
		gst_message_unref(msg);
		return;
	}
	m_pump.send(new GstMessageContainer(1, msg, NULL, NULL));
}

GstBusSyncReply eServiceMP3::gstBusSyncHandler(GstBus *bus, GstMessage *message, gpointer user_data)
{
}

void eServiceMP3::gstHTTPSourceSetAgent(GObject *object, GParamSpec *unused, gpointer user_data)
{
}

audiotype_t eServiceMP3::gstCheckAudioPad(GstStructure* structure)
{
}


void eServiceMP3::gstPoll(ePtr<GstMessageContainer> const &msg)
{
}

eAutoInitPtr<eServiceFactoryMP3> init_eServiceFactoryMP3(eAutoInitNumbers::service+1, "eServiceFactoryMP3");

void eServiceMP3::gstCBsubtitleAvail(GstElement *subsink, GstBuffer *buffer, gpointer user_data)
{
}

void eServiceMP3::gstTextpadHasCAPS(GstPad *pad, GParamSpec * unused, gpointer user_data)
{
}

void eServiceMP3::gstTextpadHasCAPS_synced(GstPad *pad)
{
}

void eServiceMP3::pullSubtitle(GstBuffer *buffer)
{
}

void eServiceMP3::pushSubtitles()
{
}

RESULT eServiceMP3::enableSubtitles(eWidget *parent, ePyObject tuple)
{
}

RESULT eServiceMP3::disableSubtitles(eWidget *parent)
{
	eDebug("eServiceMP3::disableSubtitles");
	m_currentSubtitleStream = -1;
	g_object_set (G_OBJECT (m_gst_playbin), "current-text", m_currentSubtitleStream, NULL);
	m_subtitle_pages.clear();
	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	delete m_subtitle_widget;
	m_subtitle_widget = 0;
	return 0;
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
	
	for (std::vector<subtitleStream>::iterator IterSubtitleStream(m_subtitleStreams.begin()); IterSubtitleStream != m_subtitleStreams.end(); ++IterSubtitleStream)
	{
		subtype_t type = IterSubtitleStream->type;
		switch(type)
		{
		case stUnknown:
		case stVOB:
		case stPGS:
			break;
		default:
		{
			ePyObject tuple = PyTuple_New(5);
//			eDebug("eServiceMP3::getSubtitleList idx=%i type=%i, code=%s", stream_idx, int(type), (IterSubtitleStream->language_code).c_str());
			PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(2));
			PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(stream_idx));
			PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong(int(type)));
			PyTuple_SET_ITEM(tuple, 3, PyInt_FromLong(0));
			PyTuple_SET_ITEM(tuple, 4, PyString_FromString((IterSubtitleStream->language_code).c_str()));
			PyList_Append(l, tuple);
			Py_DECREF(tuple);
		}
		}
		stream_idx++;
	}
	eDebug("eServiceMP3::getSubtitleList finished");
	return l;
}

RESULT eServiceMP3::streamed(ePtr<iStreamedService> &ptr)
{
	ptr = this;
	return 0;
}

PyObject *eServiceMP3::getBufferCharge()
{
	ePyObject tuple = PyTuple_New(5);
	PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(m_bufferInfo.bufferPercent));
	PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(m_bufferInfo.avgInRate));
	PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong(m_bufferInfo.avgOutRate));
	PyTuple_SET_ITEM(tuple, 3, PyInt_FromLong(m_bufferInfo.bufferingLeft));
	PyTuple_SET_ITEM(tuple, 4, PyInt_FromLong(m_buffer_size));
	return tuple;
}

int eServiceMP3::setBufferSize(int size)
{
	m_buffer_size = size;
	g_object_set (G_OBJECT (m_gst_playbin), "buffer-size", m_buffer_size, NULL);
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

