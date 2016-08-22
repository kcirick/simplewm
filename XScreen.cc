#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>

#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Tag.hh"

bool ignore_xerror = false;
unsigned int xerrors_count = 0;
int randr_event_base;

const char* atomnames[NATOMS] = {
   // EWMH atoms
   "_NET_SUPPORTED",
   "_NET_CLIENT_LIST",
   "_NET_NUMBER_OF_DESKTOPS",
   "_NET_DESKTOP_GEOMETRY", "_NET_DESKTOP_VIEWPORT", "_NET_WORKAREA",
   "_NET_CURRENT_DESKTOP", "_NET_DESKTOP_NAMES",
   "_NET_ACTIVE_WINDOW", "_NET_SUPPORTING_WM_CHECK",
   "_NET_WM_NAME", "_NET_WM_VISIBLE_NAME",
   "_NET_WM_DESKTOP", "_NET_WM_STRUT", "_NET_WM_PID",

   "_NET_WM_WINDOW_TYPE",
   "_NET_WM_WINDOW_TYPE_DESKTOP", "_NET_WM_WINDOW_TYPE_DOCK",
   "_NET_WM_WINDOW_TYPE_NOTIFICATION", 
   "_NET_WM_WINDOW_TYPE_TOOLBAR", "_NET_WM_WINDOW_TYPE_MENU",
   "_NET_WM_WINDOW_TYPE_UTILITY", "_NET_WM_WINDOW_TYPE_SPLASH",
   "_NET_WM_WINDOW_TYPE_DIALOG", "_NET_WM_WINDOW_TYPE_NORMAL",
   
   "_NET_WM_STATE",
   "_NET_WM_STATE_STICKY",
   "_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ",
   "_NET_WM_STATE_SHADED",
   "_NET_WM_STATE_SKIP_TASKBAR", "_NET_WM_STATE_SKIP_PAGER",
   "_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_FULLSCREEN",
   "_NET_WM_STATE_ABOVE", "_NET_WM_STATE_BELOW",
   "_NET_WM_STATE_URGENT",

   "_NET_WM_ALLOWED_ACTIONS",
   "_NET_WM_ACTION_MOVE", "_NET_WM_ACTION_RESIZE",
   "_NET_WM_ACTION_MINIMIZE", "_NET_WM_ACTION_SHADE",
   "_NET_WM_ACTION_STICK",
   "_NET_WM_ACTION_MAXIMIZE_VERT", "_NET_WM_ACTION_MAXIMIZE_HORZ",
   "_NET_WM_ACTION_FULLSCREEN", "_NET_WM_ACTION_CHANGE_DESKTOP",
   "_NET_WM_ACTION_CLOSE",

   // ICCCM Atom Names
   "WM_NAME",
   "WM_HINTS",
   "WM_CLASS",
   "WM_STATE",
   "WM_CHANGE_STATE",
   "WM_PROTOCOLS",
   "WM_DELETE_WINDOW",
   "WM_TAKE_FOCUS"
};

const unsigned long ROOT_EVENT_MASK = 
   StructureNotifyMask|PropertyChangeMask|
   SubstructureNotifyMask|SubstructureRedirectMask|
   ColormapChangeMask|FocusChangeMask|EnterWindowMask|
   ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;

int handle_xerror(Display *display, XErrorEvent *e) {
   xerrors_count++;
   if (ignore_xerror) return 0;

   if(e->error_code == BadAccess && e->request_code == X_ChangeWindowAttributes)
      exit(EXIT_FAILURE);

   char error_buffer[256];
   char resource_buffer[33];
   XGetErrorText(display, e->error_code, error_buffer, 256);
   sprintf(resource_buffer, "%d", (int)e->resourceid);
   say(WARNING, string("XError: "+string(error_buffer)+" / ID: "+string(resource_buffer)));

   return 0;
}

#define WINCOUNT_MAX 128

//--- Constructor and destructor -----------------------------------------
XScreen::XScreen(Display *display, Configuration *config) : g_display(display), g_config(config) {
   say(DEBUG, "XScreen constructor");

   XSetErrorHandler(handle_xerror);

   g_root   = DefaultRootWindow(g_display);
   g_screen = DefaultScreen(g_display);

   g_depth = DefaultDepth(g_display, g_screen);
   g_visual = DefaultVisual(g_display, g_screen);
   g_colormap = DefaultColormap(g_display, g_screen);

   modifier_map = XGetModifierMapping(g_display);

   g_screen_geom.width = WidthOfScreen(ScreenOfDisplay(g_display, g_screen));
   g_screen_geom.height = HeightOfScreen(ScreenOfDisplay(g_display, g_screen));

   /// Set up Root input layer
   XSetWindowAttributes attr;
   attr.event_mask        = ButtonPressMask|ButtonReleaseMask;
   attr.override_redirect = True;

   g_root_input = XCreateWindow(g_display, g_root, 
      g_screen_geom.x, g_screen_geom.y, g_screen_geom.width, g_screen_geom.height, 0,
      g_depth, InputOnly, g_visual, CWOverrideRedirect|CWEventMask, &attr);

   XMapWindow(g_display, g_root_input);
   ///

   default_curs   = XCreateFontCursor(g_display, XC_left_ptr);
   move_curs      = XCreateFontCursor(g_display, XC_fleur);
   resize_curs    = XCreateFontCursor(g_display, XC_plus);

   XColor cdummy;
   for(int i=0; i<NBORDERCOL; i++){
      string colour_name = g_config->getBorderColour(i);
      XAllocNamedColor(g_display, g_colormap, colour_name.c_str(), &border_colour[i], &cdummy);
   }
   for(int i=0; i<NMENUCOL; i++){
      string colour_name = g_config->getMenuColour(i);
      XAllocNamedColor(g_display, g_colormap, colour_name.c_str(), &menu_colour[i], &cdummy);
   }

   //--- Init EWMH atoms -----
   if(! XInternAtoms(g_display, const_cast<char**>(atomnames), NATOMS, 0, g_atoms))
      say(WARNING, "XInternAtoms didn't return all requested atoms");

   // Randr extension
   int dummy;
   has_randr = XRRQueryExtension(display, &randr_event_base, &dummy);

   initHeads();
   if(!g_heads.size())
      g_heads.push_back(Head(0,0,g_screen_geom.width,g_screen_geom.height));

   // Set up num_lock and scroll_lock keys
   num_lock = getMaskFromKeycode(XKeysymToKeycode(g_display, XK_Num_Lock));
   scroll_lock = getMaskFromKeycode(XKeysymToKeycode(g_display, XK_Scroll_Lock));

	XGCValues gv; 
   gv.function = GXinvert;
	gv.subwindow_mode = IncludeInferiors;
	gv.line_width = 2;
   invert_gc = XCreateGC(g_display, g_root, GCFunction|GCSubwindowMode|GCLineWidth, &gv);

   XSelectInput(g_display, g_root, ROOT_EVENT_MASK);
   XRRSelectInput(g_display, g_root, RRScreenChangeNotifyMask);

   //reset client_list
   client_list.clear();
}

XScreen::~XScreen() { }

//------------------------------------------------------------------------

void XScreen::initHeads() {
   g_heads.clear();

   if(!has_randr) return;

   XRRScreenResources *resources = XRRGetScreenResources(g_display, g_root);
   if(!resources) return;

   for(int i=0; i<resources->noutput; i++) {
      XRROutputInfo *output = XRRGetOutputInfo(g_display, resources, resources->outputs[i]);
      if(output -> crtc) {
         XRRCrtcInfo *crtc = XRRGetCrtcInfo(g_display, resources, output->crtc);

         g_heads.push_back(Head(crtc->x, crtc->y, crtc->width, crtc->height));
         say(DEBUG, "Randr: "+to_string(crtc->x)+"x"+to_string(crtc->y)+"+"+
                              to_string(crtc->width)+"+"+to_string(crtc->height));

         XRRFreeCrtcInfo(crtc);
      }
      XRRFreeOutputInfo(output);
   }
   XRRFreeScreenResources(resources);
}

void XScreen::initTags() {
   g_tags.clear();

   current_tag = 0; 
   for(int i=0; i<g_config -> getNTags(); i++)
      g_tags.push_back(new Tag(this, g_config, i));
}

unsigned int XScreen::getMaskFromKeycode(KeyCode keycode) {
   if(!modifier_map || modifier_map->max_keypermod < 1)
      return 0;

   int max_info = modifier_map->max_keypermod*8;
   for(int i=0; i<max_info; i++)
      if(modifier_map->modifiermap[i] == keycode)
         return (1<<(i/modifier_map->max_keypermod));

   return 0;
}

void XScreen::grabKeys(Window win){
   vector<KeyMap*> keymap = g_config -> getKeymap();

   for(unsigned int i=0; i<keymap.size(); i++)
      grabKey(win, keymap.at(i)->mask, keymap.at(i)->keysym);
}

void XScreen::grabKey(Window win, unsigned int mod, unsigned int keysym){
   KeyCode key = XKeysymToKeycode(g_display, keysym);

   XGrabKey(g_display, key, mod, win, True, GrabModeAsync, GrabModeAsync);
   XGrabKey(g_display, key, mod|LockMask, win, True, GrabModeAsync, GrabModeAsync);
   if(num_lock){
      XGrabKey(g_display, key, mod|num_lock, win, True, GrabModeAsync, GrabModeAsync);
      XGrabKey(g_display, key, mod|num_lock|LockMask, win, True, GrabModeAsync, GrabModeAsync);
   }
   if(scroll_lock){
      XGrabKey(g_display, key, mod|scroll_lock, win, True, GrabModeAsync, GrabModeAsync);
      XGrabKey(g_display, key, mod|scroll_lock|LockMask, win, True, GrabModeAsync, GrabModeAsync);
   }
   if(num_lock && scroll_lock){
      XGrabKey(g_display, key, mod|num_lock|scroll_lock, win, True, GrabModeAsync, GrabModeAsync);
      XGrabKey(g_display, key, mod|num_lock|scroll_lock|LockMask, win, True, GrabModeAsync, GrabModeAsync);
   }
}

void XScreen::grabButtons(Window win, int this_context) {
   vector<MouseMap*> mousemap = g_config -> getMousemap();

   for(unsigned int i=0; i<mousemap.size(); i++){
      if(mousemap.at(i)->context == this_context)
         grabButton(win, mousemap.at(i)->mask, mousemap.at(i)->button);
   }
}

void XScreen::grabButton(Window win, unsigned int mod, unsigned int button) {
   long mask = ButtonPressMask|ButtonReleaseMask; 

   XGrabButton(g_display, button, mod, win, True, mask, GrabModeAsync, GrabModeAsync, None, None);
   XGrabButton(g_display, button, mod|LockMask, win, True, mask, GrabModeAsync, GrabModeAsync, None, None);
   if(num_lock){
      XGrabButton(g_display, button, mod|num_lock, win, True, mask, GrabModeAsync, GrabModeAsync, None, None);
      XGrabButton(g_display, button, mod|num_lock|LockMask, win, True, mask, GrabModeAsync, GrabModeAsync, None, None);
   }
}

//------------------------------------------------------------------------
void XScreen::initEWMHProperties(){
   say(DEBUG, "XScreen::initEWMHProperties()");

	unsigned long n_tags = g_config->getNTags();
	unsigned long tag = current_tag;
	unsigned long workarea[4] = {
		(unsigned long)g_screen_geom.x, (unsigned long)g_screen_geom.y,
		g_screen_geom.width, g_screen_geom.height};
   string tag_names = g_config -> getTagNamesAll();

	XChangeProperty(g_display, g_root, g_atoms[NET_SUPPORTED], XA_ATOM, 32, 
         PropModeReplace, (unsigned char *)&g_atoms, EWMH_ACTION_CLOSE+1);

	XChangeProperty(g_display, g_root, g_atoms[NET_NUMBER_OF_DESKTOPS], XA_CARDINAL, 32, 
         PropModeReplace, (unsigned char *)&n_tags, 1);
	XChangeProperty(g_display, g_root, g_atoms[NET_DESKTOP_GEOMETRY], XA_CARDINAL, 32, 
         PropModeReplace, (unsigned char *)&workarea[2], 2);
	XChangeProperty(g_display, g_root, g_atoms[NET_DESKTOP_VIEWPORT], XA_CARDINAL, 32, 
         PropModeReplace, (unsigned char *)&workarea[0], 2);
	XChangeProperty(g_display, g_root, g_atoms[NET_WORKAREA], XA_CARDINAL, 32, 
         PropModeReplace, (unsigned char *)&workarea, 4);
	XChangeProperty(g_display, g_root, g_atoms[NET_CURRENT_DESKTOP], XA_CARDINAL, 32, 
         PropModeReplace, (unsigned char *)&tag, 1);
   XChangeProperty(g_display, g_root, g_atoms[NET_DESKTOP_NAMES], XA_STRING, 8,
         PropModeReplace, (unsigned char *)tag_names.c_str(), tag_names.size()); 
	XChangeProperty(g_display, g_root, g_atoms[NET_SUPPORTING_WM_CHECK], XA_WINDOW, 32, 
         PropModeReplace, (unsigned char *)&g_root, 1);
}

void XScreen::initEWMHClient(Window window){
   const int nelements = 11;
   Atom allowed_actions[nelements];
   for(int i=0; i<nelements; i++)
      allowed_actions[i] = g_atoms[EWMH_ACTION_MOVE+i];
   
   XChangeProperty(g_display, window, g_atoms[EWMH_ALLOWED_ACTIONS], XA_ATOM, 32,
         PropModeReplace, (unsigned char*)&allowed_actions, nelements);
}

void XScreen::setEWMHCurrentDesktop(){
	XChangeProperty(g_display, g_root, g_atoms[NET_CURRENT_DESKTOP], XA_CARDINAL, 32, 
         PropModeReplace, (unsigned char *)&current_tag, 1);
}

void XScreen::setEWMHActiveWindow(Window win) { 
   XChangeProperty(g_display, g_root, g_atoms[NET_ACTIVE_WINDOW], XA_WINDOW, 32,
         PropModeReplace, (unsigned char*)&win, 1);
}

void XScreen::setEWMHClientList() {
   Window windows[WINCOUNT_MAX];
   int i=0;
   say(DEBUG, "nclients = "+to_string(client_list.size()));
   for(i=0; i<(int)client_list.size(); i++)
      windows[i] = client_list.at(i)->getWindow();

   XChangeProperty(g_display, g_root, g_atoms[NET_CLIENT_LIST], XA_WINDOW, 32,
         PropModeReplace, (unsigned char*)windows, i);
}

void XScreen::setEWMHDesktop(Window window, uint tag) {
   XChangeProperty(g_display, window, g_atoms[NET_WM_DESKTOP], XA_CARDINAL, 32,
         PropModeReplace, (unsigned char*)&tag, 1);
}

unsigned int XScreen::getWMWindowType(Window win){
   Atom *aprop;
   unsigned long nitems;
   unsigned long expected=1024;
   unsigned int type = 0;

   Atom r_type;
   int r_format;
   unsigned long bytes_after;
   unsigned char *prop;

   if( XGetWindowProperty(g_display, win, g_atoms[WINDOW_TYPE], 0L, expected, False,
                           XA_ATOM, &r_type, &r_format, &nitems, &bytes_after, &prop) == Success) {
      if(r_type == XA_ATOM)
         aprop = (Atom*)prop;
   } 

   if( !aprop ) return type;

   for(unsigned int i=0; i<nitems; i++){
      if(aprop[i] == g_atoms[WINDOW_TYPE_DESKTOP])
         type |= EWMH_WINDOW_TYPE_DESKTOP;
      if(aprop[i] == g_atoms[WINDOW_TYPE_DOCK])
         type |= EWMH_WINDOW_TYPE_DOCK;
      if(aprop[i] == g_atoms[WINDOW_TYPE_NOTIFICATION])
         type |= EWMH_WINDOW_TYPE_NOTIFICATION;
   }
   XFree(aprop);

   return type;
}

Client* XScreen::findClient(Window win) {
   Client* client=NULL;
   for(unsigned int i=0; i<client_list.size(); i++)
      if(client_list.at(i)->getWindow() == win)
         client=client_list.at(i);

   return client;
}

void XScreen::updateAllTags(){
   for(unsigned int i=0; i<g_tags.size(); i++){
      if(i==current_tag)   g_tags.at(i) -> showTag();
      else                 g_tags.at(i) -> hideTag();
   }
   updateCurrentTag();
   
   // To update the client list
   setEWMHClientList();
}

void XScreen::updateCurrentTag(){
   g_tags.at(current_tag) -> updateTag();
}

void XScreen::sendFrameToTag(Frame* frame, unsigned int target_tag){
   Tag* old_tag = g_tags.at(current_tag);
   Tag* new_tag = g_tags.at(target_tag);

   old_tag -> removeFrame(frame, false, false);
   new_tag -> insertFrame(frame);

   vector<Client*> clist = frame->getClientList();
   for(uint i=0; i<clist.size(); i++)
      setEWMHDesktop(clist.at(i)->getWindow(), target_tag);
}

void XScreen::fixFrame(Frame* frame){
   bool isFixed = frame->isFixed();
   
   if(isFixed){
      for(unsigned int i=0; i<g_tags.size(); i++)
         if(i!=current_tag) g_tags.at(i)->removeFrame(frame, false, false);
   } else {
      for(unsigned int i=0; i<g_tags.size(); i++)
         if(i!=current_tag) g_tags.at(i)->insertFrame(frame);
   }

   frame->toggleFixed();
}

void XScreen::setWmState(Window win, ulong state){
   ulong data[2];
   data[0] = state;
   data[1] = None; // No icon

   XChangeProperty(g_display, win, g_atoms[WM_STATE], g_atoms[WM_STATE], 32,
         PropModeReplace, (unsigned char*) data, 2);
}
