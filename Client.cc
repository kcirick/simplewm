#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"

//--- Constructor and destructor -----------------------------------------
Client::Client(XScreen* screen, Window win) : g_xscreen(screen), window(win) { 
   say(DEBUG, "Client::Client() constructor");
   
   iconified   = false;
   marked      = false;
   urgent      = false;
   fixed       = false;

   initGeometry();
   
   XSelectInput(g_xscreen->getDisplay(), window, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
   
   g_xscreen->grabButtons(window, CONTEXT_CLIENT);

   g_xscreen->initEWMHClient(window);
   g_xscreen->setEWMHDesktop(window, g_xscreen->getCurrentTagIndex());

   // map window
   XMoveResizeWindow(g_xscreen->getDisplay(), window, geom.x, geom.y, geom.width, geom.height);
   XMapWindow(g_xscreen->getDisplay(), window);
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

   // create border
   unsigned int frame_pixel = (g_xscreen->getBorderColour(UNFOCUSED)).pixel;

   XWindowChanges wc;
   wc.border_width = border_width;
   XConfigureWindow(g_xscreen->getDisplay(), window, CWBorderWidth, &wc);
   XSetWindowBorder(g_xscreen->getDisplay(), window, frame_pixel);
}

void Client::updateGeometry(Geometry this_geom) { 
   geom = this_geom; 
         
   XMoveResizeWindow(g_xscreen->getDisplay(), window, geom.x, geom.y, geom.width, geom.height);

   // not necessary?
   send_config();
}

void Client::updateBorderColour(bool current) {
   Display *display = g_xscreen->getDisplay();

   XColor frame_colour = g_xscreen->getBorderColour(UNFOCUSED);
   if(fixed)   frame_colour = g_xscreen->getBorderColour(FIXED);
   if(marked)  frame_colour = g_xscreen->getBorderColour(MARKED);
   if(current && !iconified) {
      if(!(marked || fixed)){
         frame_colour = g_xscreen->getBorderColour(FOCUSED);
      }
   } 
   XSetWindowBorder(display, window, frame_colour.pixel);
}

void Client::send_config() {
	XConfigureEvent ce;

   ce.type = ConfigureNotify;
	ce.event = window;
	ce.window = window;
	ce.x = geom.x;
	ce.y = geom.y;
	ce.width = geom.width;
	ce.height = geom.height;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = False;

	XSendEvent(g_xscreen->getDisplay(), window, False, StructureNotifyMask, (XEvent *)&ce);
}

void Client::kill(bool force_kill) {
   int i,n,found=0;
   Atom * protocols;
   
   if(!force_kill && XGetWMProtocols(g_xscreen->getDisplay(), window, &protocols, &n)){
      for(i=0; i<n; i++)
         if(protocols[i]==g_xscreen->getAtom(WM_DELETE_WINDOW))
            found++;
      XFree(protocols);
   }

   if(found){
      XEvent ev;
      ev.type = ClientMessage;
      ev.xclient.window = window;
      ev.xclient.message_type = g_xscreen->getAtom(WM_PROTOCOLS);
      ev.xclient.format = 32;
      ev.xclient.data.l[0] = g_xscreen->getAtom(WM_DELETE_WINDOW);
      ev.xclient.data.l[1] = CurrentTime;

      XSendEvent(g_xscreen->getDisplay(), window, False, NoEventMask, &ev);
   } else
      XKillClient(g_xscreen->getDisplay(), window);

}

void Client::dragMove() {
   say(DEBUG, "Client::dragMove()");

   Display* display = g_xscreen->getDisplay();
   Window root_win = g_xscreen->getRoot();

   if(XGrabPointer(display, root_win, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, 
            None, g_xscreen->move_curs, CurrentTime)!=GrabSuccess) 
      return;

	XEvent ev;
	int old_cx = geom.x;
	int old_cy = geom.y;

   Window dw;
	int x, y, di;
   uint dui;
   XQueryPointer(display, root_win, &dw, &dw, &x, &y, &di, &di, &dui);

	for (;;) {
		XMaskEvent(display, ButtonReleaseMask|PointerMotionMask, &ev);
		switch (ev.type) {
			case MotionNotify:
				if (ev.xmotion.root != root_win) break;
				geom.x = old_cx + (ev.xmotion.x - x) + 2;
				geom.y = old_cy + (ev.xmotion.y - y) + 2;

            XMoveWindow(display, window, geom.x, geom.y);
				break;
			case ButtonRelease:
            XUngrabPointer(display, CurrentTime);
            updateGeometry(geom);
				return;
			default: break;
		}
	}
}

void Client::dragResize() {
   say(DEBUG, "Client::dragResize()");

   Display* display = g_xscreen->getDisplay();
   Window root_win = g_xscreen->getRoot();

   if(XGrabPointer(display, root_win, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, 
            None, g_xscreen->resize_curs, CurrentTime)!=GrabSuccess) 
      return;

	XEvent ev;
	int old_cx = geom.x;
	int old_cy = geom.y;

	XWarpPointer(display, None, window, 0, 0, 0, 0, geom.width, geom.height);
	for (;;) {
      g_xscreen->drawOutline(geom); /* clear */
		XMaskEvent(display, ButtonReleaseMask|PointerMotionMask, &ev);
      g_xscreen->drawOutline(geom); 
		switch (ev.type) {
			case MotionNotify:
				if (ev.xmotion.root != root_win)
					break;
	         geom.width  = ev.xmotion.x - old_cx;
	         geom.height = ev.xmotion.y - old_cy;
				break;
			case ButtonRelease:
				XUngrabPointer(display, CurrentTime);
            updateGeometry(geom);
				return;
			default: break;
		}
	}
}

