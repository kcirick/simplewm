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
   
   createFrame();

   //XXX
   XSelectInput(g_xscreen->getDisplay(), window, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
   
   g_xscreen->grabButtons(window, CONTEXT_CLIENT);

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

void Client::createFrame() {
   say(DEBUG, "Client::createFrame()");

   int border_width = g_xscreen->getBorderWidth();
   unsigned int frame_pixel = (g_xscreen->getBorderColour(UNFOCUSED)).pixel;

   XSetWindowAttributes attr;
   attr.background_pixel  = frame_pixel;
   attr.border_pixel      = frame_pixel;
   attr.event_mask        = SubstructureRedirectMask|SubstructureNotifyMask|
            ButtonPressMask|ButtonMotionMask|EnterWindowMask;
   attr.override_redirect = True;

   fgeom.x = geom.x - border_width;
   fgeom.y = geom.y - border_width;
   fgeom.width =  geom.width + 2*border_width;
   fgeom.height = geom.height + 2*border_width;

   // Frame
   frame = XCreateWindow(g_xscreen->getDisplay(), g_xscreen->getRoot(), 
      fgeom.x, fgeom.y, fgeom.width, fgeom.height, 0, 
      g_xscreen->getDepth(), InputOutput, g_xscreen->getVisual(), 
      CWOverrideRedirect|CWBorderPixel|CWBackPixel|CWEventMask, &attr);
   XMapWindow(g_xscreen->getDisplay(), frame);

   XReparentWindow(g_xscreen->getDisplay(), window, frame, border_width,  border_width);
   XMapWindow(g_xscreen->getDisplay(), window);
}

void Client::updateFrameGeometry() {
   Display *display = g_xscreen->getDisplay();
   int border_width = g_xscreen->getBorderWidth();

   // updateFrameGeometry;
   fgeom.x = geom.x - border_width;
   fgeom.y = geom.y - border_width;
   fgeom.width = geom.width + 2*border_width;
   fgeom.height = geom.height + 2*border_width;

   XMoveResizeWindow(display, frame, fgeom.x, fgeom.y, fgeom.width, fgeom.height);
   XMoveResizeWindow(display, window, border_width, border_width, geom.width, geom.height);

   // not necessary?
   send_config();
}

void Client::refreshFrame(bool current, bool update_fgeom) {
   Display *display = g_xscreen->getDisplay();

   if(update_fgeom)
      updateFrameGeometry();

   XColor frame_colour = g_xscreen->getBorderColour(UNFOCUSED);
   if(marked)  frame_colour = g_xscreen->getBorderColour(MARKED);
   if(fixed)   frame_colour = g_xscreen->getBorderColour(FIXED);
   if(current && !iconified) {
      if(!(marked || fixed)){
         frame_colour = g_xscreen->getBorderColour(FOCUSED);
      }

      XRaiseWindow(display, window);
      g_xscreen->setInputFocus(window);

      g_xscreen -> setEWMHActiveWindow(window);
   } 
   XSetWindowBackground(display, frame, frame_colour.pixel);

   XClearWindow(display, frame);
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

   refreshFrame(true, false);
   
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

            XMoveWindow(display, frame, geom.x, geom.y);
				break;
			case ButtonRelease:
            XUngrabPointer(display, CurrentTime);
            refreshFrame(true, true);
				return;
			default: break;
		}
	}
}

void Client::dragResize() {
   say(DEBUG, "Client::dragResize()");

   Display* display = g_xscreen->getDisplay();
   Window root_win = g_xscreen->getRoot();
   int border_width = g_xscreen->getBorderWidth();

   if(XGrabPointer(display, root_win, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, 
            None, g_xscreen->resize_curs, CurrentTime)!=GrabSuccess) 
      return;

	XEvent ev;
	int old_cx = fgeom.x;
	int old_cy = fgeom.y;

   refreshFrame(true, false);

	XWarpPointer(display, None, frame, 0, 0, 0, 0, geom.width, geom.height);
	for (;;) {
      g_xscreen->drawOutline(fgeom); /* clear */
		XMaskEvent(display, ButtonReleaseMask|PointerMotionMask, &ev);
      g_xscreen->drawOutline(fgeom); 
		switch (ev.type) {
			case MotionNotify:
				if (ev.xmotion.root != root_win)
					break;
	         fgeom.width  = ev.xmotion.x - old_cx;
	         fgeom.height = ev.xmotion.y - old_cy;
				break;
			case ButtonRelease:
				XUngrabPointer(display, CurrentTime);
            geom.width  = fgeom.width - 2*border_width;
            geom.height = fgeom.height - 2*border_width;
            refreshFrame(true, true);
				return;
			default: break;
		}
	}
}

void Client::destroy_frame() {
   XUnmapWindow(g_xscreen->getDisplay(), frame);
   XDestroyWindow(g_xscreen->getDisplay(), frame);
}
