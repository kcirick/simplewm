#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Tag.hh"
#include "Menu.hh"
#include "WMCore.hh"

//------------------------------------------------------------------------
void WMCore::read_config(string config_file) {
   g_config = new Configuration();
   g_config -> loadConfig(config_file);
   g_config -> loadBinding(g_config->getBindingFileName());
   g_config -> loadMenu(g_config->getMenuFileName());
}

void WMCore::setup() {
   say(INFO, "setting up SimpleWM...");
   
   Display *g_display = XOpenDisplay(NULL);
   if(!g_display) say(ERROR, "No display!");

   g_xscreen  = new XScreen(g_display, g_config);

   g_menu = new Menu(g_xscreen, g_config);
   g_menu -> createMenu();

   //TODO
   //rules = new Rule();
   //rules -> load();

   XDefineCursor(g_display, g_xscreen->getRoot(), g_xscreen->default_curs);

   // set up keys and buttons
   g_xscreen->grabKeys(g_xscreen->getRoot());
   g_xscreen->grabButtons(g_xscreen->getRootInput(), CONTEXT_ROOT);

   // set up tags
   g_xscreen->initTags();
   
   // scan for existing windows
   scanWindows();

   g_xscreen -> initEWMHProperties();
   g_xscreen -> setEWMHActiveWindow(None);

   g_xscreen -> updateCurrentTag();
}

void WMCore::scanWindows(){
   say(DEBUG, "WMCore::scanWindows()");
   
   Tag* tag = g_xscreen->getCurrentTag();

   uint nwins;
   Window dwin1, dwin2, *wins;
   XWindowAttributes winattr;
   XQueryTree(g_xscreen->getDisplay(), g_xscreen->getRoot(), &dwin1, &dwin2, &wins, &nwins);
   
   if(nwins==0) return;

   for(uint j=0; j< nwins; j++){
      XGetWindowAttributes(g_xscreen->getDisplay(), wins[j], &winattr);
      if(!winattr.override_redirect && winattr.map_state == IsViewable)
         tag -> addWindow(wins[j]);
   }
   XFree(wins);

   // try to focus the ontop window, if no window, we give focus to root
   Frame* f = tag->getCurrentFrame();
   if(!f) return;

   Client* c = f->getClientVisible();
   if(c) g_xscreen->setInputFocus(c->getWindow());
   else  g_xscreen->setInputFocus(g_xscreen->getRoot());
}

void WMCore::clean_up() {
   say(INFO, "Cleaning up...");
}

//------------------------------------------------------------------------
//--- Event Loop -----
void WMCore::event_loop() {
   say(INFO, "Starting main event loop...");
   
   XEvent ev;
   while(running){
      if(!XNextEvent(g_xscreen->getDisplay(), &ev)) {
         //char buffer[33];
         //sprintf(buffer, "%d", ev.type);
         //say(DEBUG, "code "+string(buffer));
         switch(ev.type) {
            
            case MapRequest:
               handleMapRequestEvent(&ev.xmaprequest);               break;
            case UnmapNotify:
               handleUnmapEvent(&ev.xunmap);                         break;
            case DestroyNotify:
               handleDestroyWindowEvent(&ev.xdestroywindow);         break;
            case ConfigureRequest:
               handleConfigureRequestEvent(&ev.xconfigurerequest);   break;
            case ClientMessage:
               handleClientMessageEvent(&ev.xclient);                break;
            case PropertyNotify:
               handlePropertyEvent(&ev.xproperty);                   break;
            case MappingNotify:
               handleMappingEvent(&ev.xmapping);                     break;
            case Expose:
               handleExposeEvent(&ev.xexpose);                       break;
            case KeyPress:
               handleKeyEvent(&ev.xkey);                             break;
            case ButtonPress:
               handleButtonPressEvent(&ev.xbutton);                  break;
            case ButtonRelease:
               handleButtonReleaseEvent(&ev.xbutton);                break;
            case MotionNotify:
               handleMotionEvent(&ev.xmotion);                       break;
            case EnterNotify:
               handleEnterNotify(&ev.xcrossing);                     break;
            case LeaveNotify:
               handleLeaveNotify(&ev.xcrossing);                     break;
            default:
               //if(XScreen::hasRandr()){
               //   say(DEBUG, "XRANDR event"); 
               //}
               break;

         } // end switch
      }
   }
}

void WMCore::handleKeyEvent(XKeyEvent *ev) {
   say(DEBUG, "handleKeyEvent()");
   KeySym keysym = XkbKeycodeToKeysym(g_xscreen->getDisplay(), ev->keycode, 0, 0);

   if(ev->window == g_menu->getMenuWindow()){
      if(keysym == XK_Up){
         g_menu->selectRow(g_menu->getSelectedRow()-1); 
         g_menu->drawMenu();
      } else if(keysym==XK_Down) {
         g_menu->selectRow(g_menu->getSelectedRow()+1); 
         g_menu->drawMenu();
      } else if(keysym==XK_Return) {
         bool do_exit=false;
         g_menu->executeItem(g_menu->getSelectedRow(), do_exit);
         if(do_exit) running = false; 
      } else if(keysym==XK_Escape) {
         g_menu->hideMenu();
      }
      return;
   }
   vector<KeyMap*> keymap = g_config->getKeymap();

   g_xscreen->stripStateModifiers(&ev->state);
   for(uint i=0; i<keymap.size(); i++)
      if(keysym == keymap.at(i)->keysym && keymap.at(i)->mask == ev->state)
         key_function(keymap.at(i)->keyfn, keymap.at(i)->argument, keysym);

   say(DEBUG, "handleKeyEvent() finish");
}

void WMCore::key_function(int keyfn, string argument, KeySym key){
   //--- QUIT -----
   if(keyfn==QUIT) running=false;

   //--- SPAWN -----
   if(keyfn==SPAWN) 
      if(argument.size()>0) spawn(argument);
   
   //--- TAG -----
   if(keyfn==TAG){
      if(argument=="prev")    g_xscreen->setCurrentTag(g_xscreen->getCurrentTagIndex()-1);
      if(argument=="next")    g_xscreen->setCurrentTag(g_xscreen->getCurrentTagIndex()+1);
      if(argument=="select")  g_xscreen->setCurrentTag(key-XK_1);
      g_xscreen->updateAllTags();
   }
   
   //--- CLIENT -----
   if(keyfn==CLIENT){
      // FIXME These need to be read from config file
      int width_inc = 10;
      int height_inc = 10;

      Tag* tag = g_xscreen->getCurrentTag();
      //Client* client = tag->getCurrentClient();
      Frame* frame = tag->getCurrentFrame();
      if(argument=="mark")          frame->toggleMarked();
      if(argument=="group")         tag -> groupMarkedFrames(frame);
      if(argument=="detach")        tag->detachFrame(frame);
      if(argument=="cycle_frame")   tag -> cycleFrame();
      if(argument=="previous_win")  frame -> selectNextClient(-1);
      if(argument=="next_win")      frame -> selectNextClient(1);
      if(argument=="fix")           g_xscreen->fixFrame(frame);
      if(argument=="iconify")       frame -> setIconified(true);
      if(argument=="kill")          frame -> killVisibleClient(false);
      if(argument=="move"){
         Geometry g = frame->getFrameGeometry();
              if(key==XK_Left)      g.x -= width_inc;
         else if(key==XK_Right)     g.x += width_inc;
         else if(key==XK_Up)        g.y -= height_inc;
         else if(key==XK_Down)      g.y += height_inc;
         frame->setFrameGeometry(g);
      }
      if(argument=="resize") {
         Geometry g = frame->getFrameGeometry();
              if(key==XK_Left)      g.width -= width_inc;
         else if(key==XK_Right)     g.width += width_inc;
         else if(key==XK_Up)        g.height -= height_inc;
         else if(key==XK_Down)      g.height += height_inc;
         frame->setFrameGeometry(g);
      }

      if(argument=="send_to_tag"){
         g_xscreen->sendFrameToTag(frame, key-XK_1);
         // All tags need to be updated
         g_xscreen->updateAllTags();
         return;
      }
      g_xscreen->updateCurrentTag();
   }
}

void WMCore::handleButtonPressEvent(XButtonEvent *ev){
   say(DEBUG, "handleButtonPressEvent()");
   
   Tag* tag = g_xscreen->getCurrentTag();
   Window win;
   Client* client;
   Frame* frame;
   vector<MouseMap*> mousemap = g_config -> getMousemap();

   g_xscreen->stripStateModifiers(&ev->state);
   if((client = (g_xscreen->findClient(ev->window)))){
      win = client->getFrame();
      if((frame = tag->findFrame(win)))
         say(DEBUG, "CONTEXT_CLIENT");
   }
   else if((frame = tag->findFrame(ev->window))){
      // CONTEXT_FRAME
      say(DEBUG, "CONTEXT_FRAME");
      XRaiseWindow(g_xscreen->getDisplay(), frame->getFrameWindow());
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_FRAME && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(frame, mousemap.at(i)->argument, CONTEXT_FRAME);

   }
   else if(ev->window == g_xscreen->getRootInput()){
      // CONTEXT_ROOT
      say(DEBUG, "CONTEXT_ROOT");
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_ROOT && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(NULL, mousemap.at(i)->argument, CONTEXT_ROOT);
   }
   else if(ev->window == g_menu->getMenuWindow()){
      say(DEBUG, "MENU WINDOW");
      int irow = ev->y/g_menu->getRowHeight();
      bool do_exit=false;
      g_menu->executeItem(irow, do_exit);
      if(do_exit) running = false; 
   }
   else {
      say(DEBUG, "CONTEXT_ROOT_ALT");
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_ROOT && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(NULL, mousemap.at(i)->argument, CONTEXT_ROOT);
   }
}

void WMCore::handleButtonReleaseEvent(XButtonEvent *ev){
   say(DEBUG, "handleButtonReleaseEvent()");
}

void WMCore::mouse_function(Frame* frame, string argument, int context){
   if(context==CONTEXT_ROOT){
      if(argument=="show_menu") g_menu->showMenu();
      if(argument=="hide_menu") g_menu->hideMenu();
   }
   if(context==CONTEXT_FRAME){
      if(!frame) return;
      if(argument=="move") frame->dragMoveFrame();
      if(argument=="resize") frame->dragResizeFrame();
   }
}

void WMCore::handleMotionEvent(XMotionEvent *ev){
   //say(DEBUG, "handleMotionEvent()");
   static int srow = 0;
   if(ev->window == g_menu->getMenuWindow()){
      //say(DEBUG, "Menu window");
      int irow = ev->y/g_menu->getRowHeight();
      if(irow == srow) return;

      g_menu->selectRow(irow);
      g_menu->drawMenu();
      srow = irow;
   }
}

void WMCore::handleMapRequestEvent(XMapRequestEvent *ev) {
   say(DEBUG, "handleMapRequestEvent()");

   //Client* client = new Client(ev->window);
   //new Frame(g_xscreen, g_config, ev->window);

   Tag* tag = g_xscreen->getCurrentTag();
   tag->addWindow(ev->window);
   g_xscreen->updateCurrentTag();
   /*
   WindowObject* wo = WindowObject::findWO(ev->window);

   if(wo) wo->handleMapRequest(ev);
   else {
      XWindowAttributes attr;
      XGetWindowAttributes(XScreen::getDisplay(), ev->window, &attr);
      if(!attr.override_redirect){
         Client *client = new Client(ev->window, true);
         if(!client->isAlive()) delete client;
      }
   }*/
}

void WMCore::handleUnmapEvent(XUnmapEvent *ev) {
   say(DEBUG, "handleUnmapEvent()");

/*
	Client *c = Client::findClient(ev->window);
   if(!c) return;
   Frame  *f = Frame::findFrame(c->getFrame());
   if(!f) return;

   say(DEBUG, "Client and Frame found");
   //if (c->getIgnoreUnmap()>0)
   //   c->setIgnoreUnmap(c->getIgnoreUnmap()-1);
   //else
   f->removeClient(c, true);
   */
}

void WMCore::handleDestroyWindowEvent(XDestroyWindowEvent *ev){
   say(DEBUG, "handleDestroyWindowEvent()");
   
   Tag* tag = g_xscreen->getCurrentTag();
   Client* c = g_xscreen->findClient(ev->window);
   if(!c) return;
   Frame* f = tag->findFrame(c->getFrame());
   if(!f) return;

   say(DEBUG, "Client and Frame found");
   uint list_size = f->removeClient(c, true);
   if(list_size==0) tag->removeFrame(f, true);
   g_xscreen->updateCurrentTag();
   
}

void WMCore::handleConfigureRequestEvent(XConfigureRequestEvent *ev){
   say(DEBUG, "handleConfigureRequestEvent()");
   Tag *tag = g_xscreen->getCurrentTag();
	Client *c = g_xscreen->findClient(ev->window);

   /*
	XWindowChanges wc = {
      .x = ev->x,
	   .y = ev->y,
	   .width = ev->width,
	   .height = ev->height,
	   .border_width = 0,
	   .sibling = ev->above,
	   .stack_mode = ev->detail };
   */

	if (c) {
      say(DEBUG, "Found Client");
      
      Frame* f = tag->findFrame(c->getFrame());
      Geometry fgeom = f->getFrameGeometry();
      uint value_mask = ev->value_mask;
      if(value_mask&CWX)      fgeom.x = ev->x;
      if(value_mask&CWY)      fgeom.y = ev->y;
      if(value_mask&CWWidth)  fgeom.width = ev->width + 2*g_config->getBorderWidth();
      if(value_mask&CWHeight) fgeom.height = ev->height + 2*g_config->getBorderWidth();
      f->setFrameGeometry(fgeom);

		/*
      if (ev->value_mask & CWStackMode && e->value_mask & CWSibling) {
			Client *sibling = findClient(e->above);
			if (sibling) 
				wc.sibling = sibling->parent;
		}
		do_window_changes(ev->value_mask, &wc, c);
		//if (c == current) 
		//	discard_enter_events(c);
      */
	} 
   else{
      //FIXME This is temporary. Better solution is needed
      // perhaps via rules
		XMoveResizeWindow(g_xscreen->getDisplay(), ev->window, ev->x, ev->y, ev->width, ev->height);
      XLowerWindow(g_xscreen->getDisplay(), ev->window);
      //XConfigureWindow(g_xscreen->getDisplay(), ev->window, ev->value_mask, &wc);
   }

   g_xscreen->updateCurrentTag();
}

void WMCore::handleClientMessageEvent(XClientMessageEvent *ev){
   say(DEBUG, "handleClientMessageEvent()");
}

void WMCore::handlePropertyEvent(XPropertyEvent *ev){
   say(DEBUG, "handlePropertyEvent()");

   if(ev->atom == g_xscreen->getAtom(NET_WM_DESKTOP)){
      say(DEBUG, "NET_WM_DESKTOP");
   }
   else if(ev->atom == g_xscreen->getAtom(NET_WM_STRUT)){
      say(DEBUG, "NET_WM_STRUT");
   }
   else if(ev->atom == g_xscreen->getAtom(NET_WM_NAME) || ev->atom == XA_WM_NAME){
      say(DEBUG, "NET_WM_NAME");
   }
   else if(ev->atom == XA_WM_NORMAL_HINTS){
      say(DEBUG, "NET_WM_NORMAL_HINTS");
   }
   else if(ev->atom == g_xscreen->getAtom(WM_HINTS)){
      say(DEBUG, "WM_HINTS");
   }

   g_xscreen->updateCurrentTag();
}

void WMCore::handleEnterNotify(XCrossingEvent *ev){
   say(DEBUG, "handleEnterNotify()");

   // Check first to see if it's menu
   if(ev->window==g_menu->getMenuWindow()){
      g_menu->drawMenu();
      return;
   }

   Tag* tag = g_xscreen->getCurrentTag();
   Frame* frame = tag->findFrame(ev->window);
   if(frame){
      tag->setCurrentFrame(frame);
      Client* c = frame->getClientVisible();
      g_xscreen->updateCurrentTag();
      g_xscreen -> setEWMHActiveWindow(c->getWindow());
   }
}

void WMCore::handleLeaveNotify(XCrossingEvent *ev){
   say(DEBUG, "handleLeaveNotify()");
}

void WMCore::handleMappingEvent(XMappingEvent *ev){
   say(DEBUG, "handleMappingEvent()");
}

void WMCore::handleExposeEvent(XExposeEvent *ev){
   say(DEBUG, "handleExposeEvent()");
}
