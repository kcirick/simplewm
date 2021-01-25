#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
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
      int moveresize_step = g_config -> getMoveResizeStep();

      Tag* tag = g_xscreen->getCurrentTag();
      Client* client = tag->getCurrentClient();

      if(argument=="mark")          client->toggleMarked();
      if(argument=="cycle")         tag -> cycleClient();
      if(argument=="fix")           g_xscreen->fixClient(client);
      if(argument=="iconify")       client -> setIconified(true);
      if(client && argument=="kill") client -> kill(false);
      if(argument=="move"){
         Geometry g = client->getGeometry();
              if(key==XK_Left)      g.x -= moveresize_step;
         else if(key==XK_Right)     g.x += moveresize_step;
         else if(key==XK_Up)        g.y -= moveresize_step;
         else if(key==XK_Down)      g.y += moveresize_step;
         client->setGeometry(g);
         client->updateFrameGeometry();
      }
      if(argument=="resize") {
         Geometry g = client->getGeometry();
              if(key==XK_Left)      g.width -= moveresize_step;
         else if(key==XK_Right)     g.width += moveresize_step;
         else if(key==XK_Up)        g.height -= moveresize_step;
         else if(key==XK_Down)      g.height += moveresize_step;
         client->setGeometry(g);
         client->updateFrameGeometry();
      }
      if(argument=="send_to_tag"){
         g_xscreen->sendClientToTag(client, key-XK_1);
         XUnmapWindow(g_xscreen->getDisplay(), client->getFrame());

         g_xscreen->setEWMHClientList();
      }
      g_xscreen->updateCurrentTag();
   }
}

void WMCore::handleButtonPressEvent(XButtonEvent *ev){
   say(DEBUG, "handleButtonPressEvent()");
   
   Tag* tag = g_xscreen->getCurrentTag();
   Client* client;
   vector<MouseMap*> mousemap = g_config -> getMousemap();

   g_xscreen->stripStateModifiers(&ev->state);
   if((client = (g_xscreen->findClient(ev->window)))){
      say(DEBUG, "CONTEXT_CLIENT");
      XRaiseWindow(g_xscreen->getDisplay(), client->getFrame());
      tag->setCurrentClient(client);
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_CLIENT && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(client, mousemap.at(i)->argument, CONTEXT_CLIENT);
   }
   else if((client = tag->findClientWithFrame(ev->window))){
      say(DEBUG, "CONTEXT_FRAME");
      XRaiseWindow(g_xscreen->getDisplay(), client->getFrame());
      tag->setCurrentClient(client);
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_FRAME && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(client, mousemap.at(i)->argument, CONTEXT_FRAME);
   }
   else {
      say(DEBUG, "CONTEXT_ROOT");
      for(uint i=0; i<mousemap.size(); i++)
         if(mousemap.at(i)->context==CONTEXT_ROOT && 
            mousemap.at(i)->mask == ev->state && mousemap.at(i)->button == ev->button)
            mouse_function(NULL, mousemap.at(i)->argument, CONTEXT_ROOT);
   }
   g_xscreen->updateCurrentTag();
}

void WMCore::mouse_function(Client* client, string argument, int context){
   if(context==CONTEXT_ROOT){
      if(argument=="test") test();
   }
   if(context==CONTEXT_CLIENT || context==CONTEXT_FRAME){
      if(!client) return;
      if(argument=="move") client->dragMove();
      if(argument=="resize") client->dragResize();
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
   //say(DEBUG, "-->Here<--");
   //g_xscreen->removeWindow(ev->window);
}

void WMCore::handleDestroyWindowEvent(XDestroyWindowEvent *ev){
   say(DEBUG, "handleDestroyWindowEvent()");
    
   g_xscreen->removeWindow(ev->window);
   g_xscreen->updateCurrentTag();
}

void WMCore::handleEnterNotify(XCrossingEvent *ev){
   say(DEBUG, "handleEnterNotify()");
   
   Tag* tag = g_xscreen->getCurrentTag();
   Client* client = tag->findClientWithFrame(ev->window);
   if(client && g_xscreen->isSloppyFocus()){
      // no action if frame is already focused
      if(client == tag->getCurrentClient()) return;
      
      tag->setCurrentClient(client);
      g_xscreen->updateCurrentTag();
   }
}

//---
void WMCore::handleConfigureRequestEvent(XConfigureRequestEvent *ev){
   say(DEBUG, "handleConfigureRequestEvent()");

	Client *c = g_xscreen->findClient(ev->window);

	if (c) {
      say(DEBUG, "---> Found Client");
      
      Geometry geom = c->getGeometry();
      uint value_mask = ev->value_mask;
      if(value_mask&CWX)      geom.x = ev->x;
      if(value_mask&CWY)      geom.y = ev->y;
      if(value_mask&CWWidth)  geom.width = ev->width;
      if(value_mask&CWHeight) geom.height = ev->height;
      c->setGeometry(geom);
      c->updateFrameGeometry();
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
   if(ev->message_type == g_xscreen->getAtom(NET_CURRENT_DESKTOP)) {
      say(DEBUG, "NET_CURRENT_DESKTOP");   
      g_xscreen->setCurrentTag(ev->data.l[0]);
   } else if (ev->message_type == g_xscreen->getAtom(NET_ACTIVE_WINDOW)) {
      // TODO
      say(DEBUG, "NET_ACTIVE_WINDOW");
   } else if (ev->message_type == g_xscreen->getAtom(STATE)) {
      // TODO
      say(DEBUG, "NET_WM_STATE");
      
      if ((Atom)ev->data.l[1] == g_xscreen->getAtom(STATE_FULLSCREEN) ||
          (Atom)ev->data.l[2] == g_xscreen->getAtom(STATE_FULLSCREEN) ) {
         say(DEBUG, "FULLSCREEN Request");
      }
   }
}

void WMCore::handlePropertyEvent(XPropertyEvent *ev){
   //say(DEBUG, "handlePropertyEvent()");

   if(ev->atom == g_xscreen->getAtom(NET_WM_DESKTOP)){
      say(DEBUG, "Property Event: NET_WM_DESKTOP");
   }
   else if(ev->atom == g_xscreen->getAtom(NET_WM_STRUT)){
      say(DEBUG, "Property Event: NET_WM_STRUT");
   }
   else if(ev->atom == g_xscreen->getAtom(NET_WM_NAME) || ev->atom == XA_WM_NAME){
      say(DEBUG, "Property Event: NET_WM_NAME");
   }
   else if(ev->atom == XA_WM_NORMAL_HINTS){
      say(DEBUG, "Property Event: NET_WM_NORMAL_HINTS");
   }
   else if(ev->atom == g_xscreen->getAtom(WM_HINTS)){
      say(DEBUG, "Property Event: WM_HINTS");
   }
}

