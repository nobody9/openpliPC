#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/base/eerror.h>
#include <lib/base/ebase.h>
#include <lib/driver/avswitch.h>
#include <lib/gdi/xineLib.h>
eAVSwitch *eAVSwitch::instance = 0;
 
eAVSwitch::eAVSwitch()
{
ASSERT(!instance);
instance = this;
m_video_mode = 0;
}
int eAVSwitch::getVCRSlowBlanking()
{
return 0;
}
eAVSwitch::~eAVSwitch() {}
eAVSwitch *eAVSwitch::getInstance()
{
return instance;
}
bool eAVSwitch::haveScartSwitch()
{
return false;
}
bool eAVSwitch::isActive()
{
return m_active;
}
void eAVSwitch::setInput(int val)
{}
void eAVSwitch::setColorFormat(int format)
{}
void eAVSwitch::setAspectRatio(int ratio)
{       cXineLib *xineLib = cXineLib::getInstance();
          cXineLib::getInstance()->setAspectRatio(ratio);
}
void eAVSwitch::setPolicy43(int mode)
  {cXineLib *xineLib = cXineLib::getInstance();
   cXineLib::getInstance()->setPolicy43(mode);
  }
 
void eAVSwitch::setPolicy169(int mode)
  {cXineLib *xineLib = cXineLib::getInstance();
  cXineLib::getInstance()->setPolicy169(mode);
}
 
void eAVSwitch::setZoom(int zoom43_x, int zoom43_y, int zoom169_x, int zoom169_y)
{cXineLib *xineLib = cXineLib::getInstance();
  cXineLib::getInstance()->setZoom(zoom43_x, zoom43_y, zoom169_x, zoom169_y);
}
 
void eAVSwitch::updateScreen()
{cXineLib *xineLib = cXineLib::getInstance();
  cXineLib::getInstance()->adjust_policy();
}
// 50/60 Hz
void eAVSwitch::setVideomode(int mode)
{
//printf("----------------------- eAVSwitch::setVideomode %d\n", mode);
if (mode == m_video_mode)
  return;
m_video_mode = mode;
}
void eAVSwitch::setWSS(int val) // 0 = auto, 1 = auto(4:3_off)
{}
 
void eAVSwitch::setDeinterlace(int global, int sd, int hd)
{cXineLib *xineLib = cXineLib::getInstance();
  cXineLib::getInstance()->setDeinterlace(global, sd, hd);
}
 
void eAVSwitch::setSDfeatures(int sharpness, int noise)
{cXineLib *xineLib = cXineLib::getInstance();
  cXineLib::getInstance()->setSDfeatures(sharpness, noise);
}
//FIXME: correct "run/startlevel"
eAutoInitP0<eAVSwitch> init_avswitch(eAutoInitNumbers::rc, "AVSwitch Driver");