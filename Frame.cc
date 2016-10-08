#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Frame.hh"

//--- Constructor and destructor -----------------------------------------
Frame::Frame(XScreen* xscreen, Client* new_client) : g_xscreen(xscreen) { 
   say(DEBUG, "Frame::Frame() constructor");
   createFrame(new_client);
}

Frame::~Frame() { 
   XDestroyWindow(g_xscreen->getDisplay(), frame);
   for(unsigned int i=0; i<topbar_list.size(); i++)
      XDestroyWindow(g_xscreen->getDisplay(), topbar_list.at(i));
}

//------------------------------------------------------------------------
void Frame::createFrame(Client* client){
   client_list = vector<Client*>();
   topbar_list = vector<Window>();

   iconified      = false;
   marked         = false;
   urgent         = false;
   fixed          = false;

   int border_width = g_xscreen->getBorderWidth();
   unsigned int frame_pixel = (g_xscreen->getBorderColour(UNFOCUSED)).pixel;

   XSetWindowAttributes attr;
   attr.background_pixel  = frame_pixel;
   attr.border_pixel      = frame_pixel;
   attr.event_mask        = SubstructureRedirectMask|SubstructureNotifyMask|
            ButtonPressMask|ButtonMotionMask|EnterWindowMask;
   attr.override_redirect = True;

   geom = client->getGeometry();
   geom.x -= border_width;
   geom.y -= border_width;
   geom.width += 2*border_width;
   geom.height += 2*border_width;

   // Frame
   frame = XCreateWindow(g_xscreen->getDisplay(), g_xscreen->getRoot(), 
      geom.x, geom.y, geom.width, geom.height, 0, 
      g_xscreen->getDepth(), InputOutput, g_xscreen->getVisual(), 
      CWOverrideRedirect|CWBorderPixel|CWBackPixel|CWEventMask, &attr);
   XMapWindow(g_xscreen->getDisplay(), frame);

   // Top bar
   Window topbar = XCreateSimpleWindow(g_xscreen->getDisplay(), frame,
      geom.x, geom.y, geom.width, border_width, 0, frame_pixel, frame_pixel);
   XMapWindow(g_xscreen->getDisplay(), topbar);
   
   topbar_list.push_back(topbar);   // now size = 1

   // Add client to the list
   client_list.push_back(client);   // now size = 1
   iVisibleClient = 0;
}

void Frame::refreshFrame(bool current){
   Display *display = g_xscreen->getDisplay();

   updateFrameGeometry();

   if(current && !iconified) {
      if(!(marked || fixed)){
         XColor frame_colour = g_xscreen->getBorderColour(FOCUSED);
         XSetWindowBackground(display, frame, frame_colour.pixel);
         XSetWindowBackground(display, topbar_list.at(iVisibleClient), frame_colour.pixel);
      }

      XRaiseWindow(display, client_list.at(iVisibleClient)->getWindow());
      g_xscreen->setInputFocus(client_list.at(iVisibleClient)->getWindow());

      g_xscreen -> setEWMHActiveWindow(client_list.at(iVisibleClient)->getWindow());
   } 

   XClearWindow(display, frame);
   for(unsigned int i=0; i<topbar_list.size(); i++)
      XClearWindow(display, topbar_list.at(i));
}

void Frame::dragMoveFrame() {
   say(DEBUG, "Frame::dragMoveFrame()");
   
   Display* display = g_xscreen->getDisplay();
   Window root_win = g_xscreen->getRoot();

   if(XGrabPointer(display, root_win, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, 
            None, g_xscreen->move_curs, CurrentTime)!=GrabSuccess) 
      return;

	XEvent ev;
	int old_cx = geom.x;
	int old_cy = geom.y;

   refreshFrame(true);
   
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
            refreshFrame(true);
				return;
			default: break;
		}
	}
}

void Frame::dragResizeFrame(){
   say(DEBUG, "Frame::dragResizeFrame()");

   Display* display = g_xscreen->getDisplay();
   Window root_win = g_xscreen->getRoot();

   if(XGrabPointer(display, root_win, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, 
            None, g_xscreen->resize_curs, CurrentTime)!=GrabSuccess) 
      return;

	XEvent ev;
	int old_cx = geom.x;
	int old_cy = geom.y;

   refreshFrame(true);

	XWarpPointer(display, None, frame, 0, 0, 0, 0, geom.width, geom.height);
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
            refreshFrame(true);
				return;
			default: break;
		}
	}
}

void Frame::updateFrameGeometry() {
   Display *display = g_xscreen->getDisplay();
   int border_width = g_xscreen->getBorderWidth();

   Geometry cl_geom = client_list.at(iVisibleClient)->getGeometry();
   cl_geom.x = geom.x + border_width;
   cl_geom.y = geom.y + border_width;
   cl_geom.width = geom.width - 2*border_width;
   cl_geom.height = geom.height - 2*border_width;
   client_list.at(iVisibleClient)->setGeometry(cl_geom);

   XColor frame_colour = g_xscreen->getBorderColour(UNFOCUSED);
   if(marked)  frame_colour = g_xscreen->getBorderColour(MARKED);
   if(fixed)   frame_colour = g_xscreen->getBorderColour(FIXED);
   XSetWindowBackground(display, frame, frame_colour.pixel);

   XMoveResizeWindow(display, frame, geom.x, geom.y, geom.width, geom.height);
   XMoveResizeWindow(display, client_list.at(iVisibleClient)->getWindow(), border_width, border_width, cl_geom.width, cl_geom.height);
   unsigned int nclients = client_list.size();
   int topbar_width = cl_geom.width/nclients;
   //say(DEBUG, "# clients = "+to_string(nclients)+" :: topbar width = "+to_string(topbar_width));
   for(unsigned int i=0; i<nclients; i++){
      XSetWindowBackground(display, topbar_list.at(i), frame_colour.pixel);
      XMoveResizeWindow(display, topbar_list.at(i), border_width+i*topbar_width, 0, topbar_width, border_width);
   }

   send_config();
}

void Frame::send_config() {
	XConfigureEvent ce;

   Window win = client_list.at(iVisibleClient)->getWindow();
	Geometry cl_geom = client_list.at(iVisibleClient)->getGeometry();
   ce.type = ConfigureNotify;
	ce.event = win;
	ce.window = win;
	ce.x = cl_geom.x;
	ce.y = cl_geom.y;
	ce.width = cl_geom.width;
	ce.height = cl_geom.height;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = False;

	XSendEvent(g_xscreen->getDisplay(), win, False, StructureNotifyMask, (XEvent *)&ce);
}

void Frame::raiseFrame() {
   XRaiseWindow(g_xscreen->getDisplay(), frame);
   g_xscreen->setInputFocus(client_list.at(iVisibleClient)->getWindow());
   g_xscreen -> setEWMHActiveWindow(client_list.at(iVisibleClient)->getWindow());
}

void Frame::selectNextClient(int direction){
   uint list_size=client_list.size();
   if(direction>0){  // next frame
      if(iVisibleClient<(int)list_size-1) iVisibleClient++;
      else                                iVisibleClient = 0;
   } else {          // previous frame
      if(iVisibleClient==0)   iVisibleClient = list_size-1;
      else                    iVisibleClient--;
   }
}

unsigned int Frame::removeClient(Client *client, bool delete_client){
   say(DEBUG, "Frame::removeClient");

   say(DEBUG, to_string(client_list.size())+" :: "+to_string(topbar_list.size()));
   bool found = false;
   for(uint i=0; i<client_list.size(); i++){
      if(client_list.at(i) == client){
         found = true;
         client_list.erase(client_list.begin()+i);
         XUnmapWindow(g_xscreen->getDisplay(), topbar_list.at(i));
         topbar_list.erase(topbar_list.begin()+i);
         iVisibleClient = (i>0) ? i-1 : 0;
      }
   }
   if(!found) return client_list.size();

   if(delete_client){
      g_xscreen->removeClient(client);
      delete client;
   }
   g_xscreen->setEWMHClientList();
   return client_list.size();
}

void Frame::killClient(bool force_kill, int iclient){
   int i,n,found=0;
   Atom * protocols;
   
   Client* c = client_list.at(iclient);
   if(!force_kill && XGetWMProtocols(g_xscreen->getDisplay(), c->getWindow(), &protocols, &n)){
      for(i=0; i<n; i++)
         if(protocols[i]==g_xscreen->getAtom(WM_DELETE_WINDOW))
            found++;
      XFree(protocols);
   }

   if(found){
      XEvent ev;
      ev.type = ClientMessage;
      ev.xclient.window = c->getWindow();
      ev.xclient.message_type = g_xscreen->getAtom(WM_PROTOCOLS);
      ev.xclient.format = 32;
      ev.xclient.data.l[0] = g_xscreen->getAtom(WM_DELETE_WINDOW);
      ev.xclient.data.l[1] = CurrentTime;

      XSendEvent(g_xscreen->getDisplay(), c->getWindow(), False, NoEventMask, &ev);
   } else
      XKillClient(g_xscreen->getDisplay(), c->getWindow());
}

