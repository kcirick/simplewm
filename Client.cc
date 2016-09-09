#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"

//--- Constructor and destructor -----------------------------------------
Client::Client(XScreen* screen, Window win) : 
   g_xscreen(screen), window(win), managed(true) { 
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
   
   XSelectInput(g_xscreen->getDisplay(), window, FocusChangeMask);

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

   geom.x = x-(attr.width/2);
   geom.y = y-(attr.height/2);
   geom.width=attr.width;
   geom.height=attr.height;

   // Fix spilling of the window outside the screen
   int border_width = g_xscreen->getBorderWidth();
   int spill_x = geom.x+geom.width - g_xscreen->getWidth();
   int spill_y = geom.y+geom.height - g_xscreen->getHeight();
   if(spill_x > 0) geom.x -= spill_x+border_width;
   if(spill_y > 0) geom.y -= spill_y+border_width;
   if(geom.x < 0) geom.x = border_width;
   if(geom.y < 0) geom.y = border_width;
}

void Client::reparent(Window frame){

   parent = frame;

   Display * display = g_xscreen->getDisplay();

   XReparentWindow(display, window, parent, 0, 0);

   XMapWindow(display, window);

   g_xscreen->grabButtons(parent, CONTEXT_FRAME);
}

