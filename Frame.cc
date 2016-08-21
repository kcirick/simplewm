#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Frame.hh"

//--- Constructor and destructor -----------------------------------------
Frame::Frame(XScreen* xscreen, Configuration* config, Window win) : g_xscreen(xscreen), g_config(config) { 
   say(DEBUG, "Frame::Frame() constructor");

   createFrame();

   Client* new_client = new Client(g_xscreen, win, frame);
   client_list.push_back(new_client); // now size = 1
   iVisibleClient = 0;

   updateFrameGeometry(true);
}

Frame::~Frame() { 
   XDestroyWindow(g_xscreen->getDisplay(), frame);
   for(unsigned int i=0; i<topbar_list.size(); i++)
      XDestroyWindow(g_xscreen->getDisplay(), topbar_list.at(i));
}

//------------------------------------------------------------------------
void Frame::createFrame(){
   client_list = vector<Client*>();
   topbar_list = vector<Window>();

   iconified      = false;
   marked         = false;
   urgent         = false;
   fixed          = false;

   XSetWindowAttributes attr;
   attr.background_pixmap = ParentRelative;
   attr.background_pixel  = (g_xscreen->getBorderColour(UNFOCUSED)).pixel;
   attr.border_pixel      = (g_xscreen->getBorderColour(UNFOCUSED)).pixel;
   attr.event_mask        = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|ButtonReleaseMask|EnterWindowMask;
   attr.override_redirect = True;

   // Frame
   frame = XCreateWindow(g_xscreen->getDisplay(), g_xscreen->getRoot(), 
      0, 0, 100, 100, 0,   // we don't know the size yet
      g_xscreen->getDepth(), InputOutput, g_xscreen->getVisual(), 
      CWOverrideRedirect|CWBorderPixel|CWBackPixmap|CWBackPixel|CWEventMask, &attr);
   XMapWindow(g_xscreen->getDisplay(), frame);

   // Top bar
   Window topbar = XCreateWindow(g_xscreen->getDisplay(), frame,
      0, 0, 10, 10, 0,  // we don't know the size yet
      g_xscreen->getDepth(), CopyFromParent, g_xscreen->getVisual(),
      CWOverrideRedirect|CWBorderPixel|CWBackPixmap|CWBackPixel|CWEventMask, &attr);
   XMapWindow(g_xscreen->getDisplay(), topbar);
   
   topbar_list.push_back(topbar);   // now size = 1
}

void Frame::refreshFrame(bool current){
   Display *display = g_xscreen->getDisplay();

   updateFrameGeometry(false);

   if(current && !iconified) {
      if(!(marked || fixed)){
         XColor frame_colour = g_xscreen->getBorderColour(FOCUSED);
         XSetWindowBackground(display, frame, frame_colour.pixel);
         XSetWindowBackground(display, topbar_list.at(iVisibleClient), frame_colour.pixel);
      }

      XRaiseWindow(display, client_list.at(iVisibleClient)->getWindow());
      g_xscreen->setInputFocus(client_list.at(iVisibleClient)->getWindow());
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

            XMoveWindow(display, frame, geom.x-2, geom.y-2);
				break;
			case ButtonRelease:
            //send_config();
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
      drawOutline(g_xscreen, geom); /* clear */
		XMaskEvent(display, ButtonReleaseMask|PointerMotionMask, &ev);
      drawOutline(g_xscreen, geom);
		switch (ev.type) {
			case MotionNotify:
				if (ev.xmotion.root != root_win)
					break;
	         geom.width  = ev.xmotion.x - old_cx;
	         geom.height = ev.xmotion.y - old_cy;
				break;
			case ButtonRelease:
            //send_config();
				XUngrabPointer(display, CurrentTime);
            refreshFrame(true);
				/* In case maximise state has changed: */
				//ewmh_set_net_wm_state(c);
				return;
			default: break;
		}
	}
}

void Frame::updateFrameGeometry(bool init) {

   Display *display = g_xscreen->getDisplay();
   int border_width = g_config->getBorderWidth();

   Geometry cl_geom = client_list.at(iVisibleClient)->getGeometry();
   if(init) {
      geom = cl_geom;
      geom.x -= border_width;
      geom.y -= border_width;
      geom.width += 2*border_width;
      geom.height += 2*border_width;
   } else {
      cl_geom.x = geom.x + border_width;
      cl_geom.y = geom.y + border_width;
      cl_geom.width = geom.width - 2*border_width;
      cl_geom.height = geom.height - 2*border_width;
      client_list.at(iVisibleClient)->setGeometry(cl_geom);
   }

   XColor frame_colour = g_xscreen->getBorderColour(UNFOCUSED);
   if(marked)  frame_colour = g_xscreen->getBorderColour(MARKED);
   if(fixed)   frame_colour = g_xscreen->getBorderColour(FIXED);
   XSetWindowBackground(display, frame, frame_colour.pixel);

   XMoveResizeWindow(display, frame, geom.x, geom.y, geom.width, geom.height);
   XMoveResizeWindow(display, client_list.at(iVisibleClient)->getWindow(), border_width, border_width, cl_geom.width, cl_geom.height);
   unsigned int nclients = client_list.size();
   int topbar_width = cl_geom.width/nclients;
   say(DEBUG, "# clients = "+to_string(nclients)+" :: topbar width = "+to_string(topbar_width));
   for(unsigned int i=0; i<nclients; i++){
      XSetWindowBackground(display, topbar_list.at(i), frame_colour.pixel);
      XMoveResizeWindow(display, topbar_list.at(i), border_width+i*topbar_width, 0, topbar_width, border_width);
   }
}

void Frame::send_config() {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.event = frame;
	ce.window = frame;
	ce.x = geom.x;
	ce.y = geom.y;
	ce.width = geom.width;
	ce.height = geom.height;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = False;

	XSendEvent(g_xscreen->getDisplay(), frame, False, StructureNotifyMask, (XEvent *)&ce);
}

void Frame::raiseFrame() {
   XRaiseWindow(g_xscreen->getDisplay(), frame);
   g_xscreen->setInputFocus(client_list.at(iVisibleClient)->getWindow());
   XClearWindow(g_xscreen->getDisplay(), client_list.at(iVisibleClient)->getWindow());
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

   return client_list.size();
}

void Frame::killVisibleClient(bool force_kill){
   int i,n,found=0;
   Atom * protocols;
   
   Client* c = client_list.at(iVisibleClient);
   if(!force_kill && XGetWMProtocols(g_xscreen->getDisplay(), c->getWindow(), &protocols, &n)){
      for(i=0; i<n; i++)
         if(protocols[i]==g_xscreen->getAtom(WM_DELETE_WINDOW))
            found++;
      XFree(protocols);
   }

   if(found){
      //send_xmessage(c->getWindow(), g_atoms[WM_PROTOCOLS], g_atoms[WM_DELETE_WINDOW]);
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

