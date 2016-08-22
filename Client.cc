#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"

//--- Constructor and destructor -----------------------------------------
Client::Client(XScreen* screen, Window win, Window frame) : 
   g_xscreen(screen), window(win), parent(frame), managed(true) { 
   say(DEBUG, "Client::Client() constructor");
   
   // Dont manage DESKTOP or DOCK type windows
   unsigned int window_type = g_xscreen->getWMWindowType(win);
   if(window_type&EWMH_WINDOW_TYPE_DESKTOP || window_type&EWMH_WINDOW_TYPE_DOCK) {
      say(DEBUG, "DESKTOP or DOCK type");
      XMapWindow(g_xscreen->getDisplay(), win);
      managed = false;
      return;
   }

   initGeometry();
   reparent();

   XSelectInput(g_xscreen->getDisplay(), window, PropertyChangeMask|StructureNotifyMask|FocusChangeMask);

   g_xscreen->initEWMHClient(window);
   g_xscreen->setEWMHDesktop(window, g_xscreen->getCurrentTagIndex());
}

Client::~Client(){ }

//------------------------------------------------------------------------
void Client::initGeometry(){
   XWindowAttributes attr;
   XGetWindowAttributes(g_xscreen->getDisplay(), window, &attr);

   Window dw;
	int x, y, di;
   uint dui;
   XQueryPointer(g_xscreen->getDisplay(), g_xscreen->getRoot(), &dw, &dw, &x, &y, &di, &di, &dui);

   //geom.x=(g_xscreen->getWidth()-attr.width)/2;
   //geom.y=(g_xscreen->getHeight()-attr.height)/2;
   geom.x = x-(attr.width/2);
   geom.y = y-(attr.height/2);
   geom.width=attr.width;
   geom.height=attr.height;
}

void Client::reparent(){
   Display * display = g_xscreen->getDisplay();

   XReparentWindow(display, window, parent, 0, 0);
   XMapWindow(display, window);

   g_xscreen->grabButtons(parent, CONTEXT_FRAME);
}

