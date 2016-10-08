#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Tag.hh"
#include "WMCore.hh"

#define MAX(a,b) ((a)>(b) ? (a) : (b))

//------------------------------------------------------------------------
void WMCore::read_config(string config_file) {
   g_config = new Configuration();
   g_config -> loadConfig(config_file);
}

void WMCore::test() {
   // Reserved for testing/debugging purposes
   say(DEBUG, "TEST!");
}

void WMCore::setup() {
   say(INFO, "setting up SimpleWM...");
   
   Display *g_display = XOpenDisplay(NULL);
   if(!g_display) say(ERROR, "No display!");

   g_xscreen  = new XScreen(g_display, g_config);

   XDefineCursor(g_display, g_xscreen->getRoot(), g_xscreen->default_curs);

   //--- set up keys and buttons
   g_xscreen->grabKeys(g_xscreen->getRoot());
   g_xscreen->grabButtons(g_xscreen->getRootInput(), CONTEXT_ROOT);

   //--- scan for existing windows
   scanWindows();

   g_xscreen -> initEWMHProperties();
   g_xscreen -> setEWMHActiveWindow(None);

   g_xscreen -> updateCurrentTag();
}

void WMCore::scanWindows(){
   say(DEBUG, "WMCore::scanWindows()");
   
   uint nwins;
   Window dwin1, dwin2, *wins;
   XWindowAttributes winattr;
   XQueryTree(g_xscreen->getDisplay(), g_xscreen->getRoot(), &dwin1, &dwin2, &wins, &nwins);
   
   if(nwins==0) return;

   for(uint j=0; j< nwins; j++){
      XGetWindowAttributes(g_xscreen->getDisplay(), wins[j], &winattr);
      if(!winattr.override_redirect && winattr.map_state == IsViewable)
         g_xscreen -> addWindow(wins[j]);
   }
   XFree(wins);
}

void WMCore::clean_up() {
   say(INFO, "Cleaning up...");

   XCloseDisplay(g_xscreen->getDisplay());
}

//------------------------------------------------------------------------
//--- Event Loop -----
void WMCore::event_loop() {
   say(INFO, "Starting main event loop...");
   
   // First get randr information
   bool has_randr =false;
   int randr_event_base = 0;
   has_randr = g_xscreen->hasRandr(&randr_event_base);
   say(DEBUG, "randr_event_base = "+to_string(randr_event_base));

   XEvent ev;
   while(running){
      if(!XNextEvent(g_xscreen->getDisplay(), &ev)) {
         //say(DEBUG, "code "+to_string(ev.type));
         switch(ev.type) {
            
            case KeyPress:
               handleKeyEvent(&ev.xkey);                             break;
            case ButtonPress:
               handleButtonPressEvent(&ev.xbutton);                  break;

            case MapRequest:
               handleMapRequestEvent(&ev.xmaprequest);               break;
            case UnmapNotify:
               handleUnmapEvent(&ev.xunmap);                         break;
            case DestroyNotify:
               handleDestroyWindowEvent(&ev.xdestroywindow);         break;
            case EnterNotify:
               handleEnterNotify(&ev.xcrossing);                     break;

            case ConfigureRequest:
               handleConfigureRequestEvent(&ev.xconfigurerequest);   break;
            case ClientMessage:
               handleClientMessageEvent(&ev.xclient);                break;
            case PropertyNotify:
               handlePropertyEvent(&ev.xproperty);                   break;

            default:
               if(has_randr && ev.type == randr_event_base + RRScreenChangeNotify){
                  say(DEBUG, "RandR event"); 
                  XRRUpdateConfiguration(&ev);
               }
               break;

         } // end switch
      }
   }
}

//---
void WMCore::handleKeyEvent(XKeyEvent *ev) {
   say(DEBUG, "handleKeyEvent()");
   KeySym keysym = XkbKeycodeToKeysym(g_xscreen->getDisplay(), ev->keycode, 0, 0);

   vector<KeyMap*> keymap = g_config->getKeymap();

   g_xscreen->stripStateModifiers(&ev->state);
   for(uint i=0; i<keymap.size(); i++)
      if(keysym == keymap.at(i)->keysym && keymap.at(i)->mask == ev->state)
         key_function(keymap.at(i)->keyfn, keymap.at(i)->argument, keysym);
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
   }
   
   //--- CLIENT -----
   if(keyfn==CLIENT){
      // FIXME These need to be read from config file
      int width_inc = 10;
      int height_inc = 10;

      Tag* tag = g_xscreen->getCurrentTag();
      Frame* frame = tag->getCurrentFrame();
      if(argument=="mark")          frame->toggleMarked();
      if(argument=="group")         tag -> groupMarkedFrames(frame);
      if(argument=="detach")        tag->detachFrame(frame);
      if(argument=="cycle_frame")   tag -> cycleFrame();
      if(argument=="previous_win")  frame -> selectNextClient(-1);
      if(argument=="next_win")      frame -> selectNextClient(1);
      if(argument=="fix")           g_xscreen->fixFrame(frame);
      if(argument=="iconify")       frame -> setIconified(true);
      if(frame && argument=="kill") frame -> killClient(false, frame->getVisibleClientIndex());
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
         XUnmapWindow(g_xscreen->getDisplay(), frame->getFrameWindow());

         g_xscreen->setEWMHClientList();
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
      XRaiseWindow(g_xscreen->getDisplay(), frame->getFrameWindow());
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_FRAME && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(frame, mousemap.at(i)->argument, CONTEXT_FRAME);

   }
   else {
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_ROOT && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(NULL, mousemap.at(i)->argument, CONTEXT_ROOT);
   }
}

void WMCore::mouse_function(Frame* frame, string argument, int context){
   if(context==CONTEXT_ROOT){
      if(argument=="test") test();
   }
   if(context==CONTEXT_FRAME){
      if(!frame) return;
      if(argument=="move") frame->dragMoveFrame();
      if(argument=="resize") frame->dragResizeFrame();
   }
}

//---
void WMCore::handleMapRequestEvent(XMapRequestEvent *ev) {
   say(DEBUG, "handleMapRequestEvent()");

   // Don't do anything if there is already a client
   Client *c = g_xscreen->findClient(ev->window);
   if(c) return;

   g_xscreen->addWindow(ev->window);
   g_xscreen->updateCurrentTag();
}

void WMCore::handleUnmapEvent(XUnmapEvent *ev) {
   say(DEBUG, "handleUnmapEvent()");

   if(!ev->send_event) return;

   ulong data[2];
   data[0] = WithdrawnState;
   data[1] = None; // No Icon
   XChangeProperty(g_xscreen->getDisplay(), ev->window, g_xscreen->getAtom(WM_STATE), g_xscreen->getAtom(WM_STATE),
      32, PropModeReplace, (unsigned char*)data, 2);

   g_xscreen->unsetProperty(ev->window, STATE);
   g_xscreen->unsetProperty(ev->window, NET_WM_DESKTOP);

   g_xscreen->removeWindow(ev->window, false);
}

void WMCore::handleDestroyWindowEvent(XDestroyWindowEvent *ev){
   say(DEBUG, "handleDestroyWindowEvent()");
   
   g_xscreen->removeWindow(ev->window, true);
   g_xscreen->updateCurrentTag();
}

void WMCore::handleEnterNotify(XCrossingEvent *ev){
   say(DEBUG, "handleEnterNotify()");

   Tag* tag = g_xscreen->getCurrentTag();
   Frame* frame = tag->findFrame(ev->window);
   if(frame){
      // no action if frame is already focused
      if(frame == tag->getCurrentFrame()) return;
      
      tag->setCurrentFrame(frame);
      g_xscreen->updateCurrentTag();
   }
}

//---
void WMCore::handleConfigureRequestEvent(XConfigureRequestEvent *ev){
   say(DEBUG, "handleConfigureRequestEvent()");

   Tag *tag = g_xscreen->getCurrentTag();
	Client *c = g_xscreen->findClient(ev->window);

	if (c) {
      say(DEBUG, "---> Found Client");
      
      Frame* f = tag->findFrame(c->getFrame());
      Geometry fgeom = f->getFrameGeometry();
      uint value_mask = ev->value_mask;
      if(value_mask&CWX)      fgeom.x = ev->x;
      if(value_mask&CWY)      fgeom.y = ev->y;
      if(value_mask&CWWidth)  fgeom.width = ev->width + 2*g_config->getBorderWidth();
      if(value_mask&CWHeight) fgeom.height = ev->height + 2*g_config->getBorderWidth();
      f->setFrameGeometry(fgeom);

      tag->setCurrentFrame(f);
      f->raiseFrame();
      g_xscreen->updateCurrentTag();
	} 
   else{
      XWindowChanges wc;
      wc.x              = ev->x;
      wc.y              = ev->y;
      wc.width          = ev->width;
      wc.height         = ev->height;
      wc.border_width   = ev->border_width;
      wc.sibling        = ev->above;
      wc.stack_mode     = ev->detail;

      XConfigureWindow(g_xscreen->getDisplay(), ev->window, ev->value_mask, &wc);
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
}

