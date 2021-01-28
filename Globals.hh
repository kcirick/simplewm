#ifndef GLOBALS_HH
#define GLOBALS_HH

#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <X11/X.h>
#include <X11/Xlib.h>

using namespace std;

//--- classes -----
class Geometry {
   public:
      Geometry() : x(0), y(0), width(1), height(1) { }
      Geometry(int _x, int _y, unsigned int _w, unsigned int _h) :
         x(_x), y(_y), width(_w), height(_h) { }
      ~Geometry() { }

      int x, y;
      unsigned int width, height;

      inline Geometry &operator = (const Geometry &geom) {
         x = geom.x;          y = geom.y;
         width = geom.width;  height = geom.height;
         return *this;
      }
      inline bool operator == (const Geometry &geom) {
         return ((x==geom.x) && (y==geom.y) && (width==geom.width) && (height==geom.height));
      }
      inline bool operator != (const Geometry &geom) {
         return ((x!=geom.x) || (y!=geom.y) || (width!=geom.width) || (height!=geom.height));
      }
};
   
class KeyMap {
   public:
      KeyMap(unsigned int _mask, KeySym _keysym, int _keyfn, string _argument) :
         mask(_mask), keysym(_keysym), keyfn(_keyfn), argument(_argument) { }
      ~KeyMap() { }

      unsigned int mask;
      KeySym keysym;
      int keyfn;
      string argument;
};

class MouseMap {
   public:
      MouseMap(unsigned int _mask, unsigned int _button, int _context, string _argument) :
         mask(_mask), button(_button), context(_context), argument(_argument) { }
      ~MouseMap() { }

      unsigned int mask;
      unsigned int button;
      int context;
      string argument;
};

//--- enums -----
enum MessageType { DEBUG, INFO, WARNING, ERROR, NMSG };

enum AtomName {
   // EWMH atoms
   NET_SUPPORTED,
   NET_CLIENT_LIST, 
   NET_NUMBER_OF_DESKTOPS,
   NET_DESKTOP_GEOMETRY, NET_DESKTOP_VIEWPORT, NET_WORKAREA,
   NET_CURRENT_DESKTOP, NET_DESKTOP_NAMES,
   NET_ACTIVE_WINDOW, NET_SUPPORTING_WM_CHECK,
   NET_WM_NAME, NET_WM_VISIBLE_NAME,
   NET_WM_DESKTOP, NET_WM_STRUT, NET_WM_PID,

   WINDOW_TYPE,
   WINDOW_TYPE_DESKTOP, WINDOW_TYPE_DOCK,
   WINDOW_TYPE_NOTIFICATION,
   WINDOW_TYPE_TOOLBAR, WINDOW_TYPE_MENU,
   WINDOW_TYPE_UTILITY, WINDOW_TYPE_SPLASH,
   WINDOW_TYPE_DIALOG, WINDOW_TYPE_NORMAL,
   
   STATE,
   STATE_STICKY,
   STATE_MAXIMIZED_VERT, STATE_MAXIMIZED_HORZ,
   STATE_SHADED,
   STATE_SKIP_TASKBAR, STATE_SKIP_PAGER,
   STATE_HIDDEN, STATE_FULLSCREEN,
   STATE_ABOVE, STATE_BELOW,
   STATE_URGENT,

   EWMH_ALLOWED_ACTIONS,
   EWMH_ACTION_MOVE, EWMH_ACTION_RESIZE,
   EWMH_ACTION_MINIMIZE, EWMH_ACTION_SHADE,
   EWMH_ACTION_STICK,
   EWMH_ACTION_MAXIMIZE_VERT, EWMH_ACTION_MAXIMIZE_HORZ,
   EWMH_ACTION_FULLSCREEN, EWMH_ACTION_CHANGE_DESKTOP,
   EWMH_ACTION_CLOSE,

   // ICCCM Atom Names
   WM_NAME,
   WM_HINTS,
   WM_CLASS,
   WM_STATE,
   WM_CHANGE_STATE,
   WM_PROTOCOLS,
   WM_DELETE_WINDOW,
   WM_TAKE_FOCUS,

   NATOMS
};

enum KeyFunctions { SPAWN, QUIT, TAG, CLIENT, NFUNC };

enum MouseContext { CONTEXT_ROOT, CONTEXT_CLIENT, CONTEXT_FRAME, NCONTEXT };

enum BorderColours { FOCUSED, UNFOCUSED, URGENT, MARKED, FIXED, NBORDERCOL };

enum Layer { BOTTOM, TOP };

//--- functions in Main.cc -----
void say(int, string);
void spawn(string);

#endif
