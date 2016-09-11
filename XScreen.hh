#ifndef XSCREEN_HH
#define XSCREEN_HH

extern unsigned int xerrors_count;

#define EWMH_WINDOW_TYPE_DESKTOP       (1<<0)
#define EWMH_WINDOW_TYPE_DOCK          (1<<1)
#define EWMH_WINDOW_TYPE_NOTIFICATION  (1<<2)

class Tag;
class Frame;
class Client;

class Head {
   public:
      Head(int myx, int myy, int myw, int myh) :
         x(myx), y(myy), width(myw), height(myh) { };

      int x;
      int y; 
      int width;
      int height;
};

class XScreen {
   public:
      XScreen(Display*, Configuration*);
      virtual ~XScreen();

      //--- static variables
      Cursor   default_curs;
      Cursor   move_curs;
      Cursor   resize_curs;

      GC invert_gc;

      //--- functions
      inline Display* getDisplay() { return g_display; }
      inline Window getRoot() { return g_root; }
      inline int getDepth() { return g_depth; }
      inline Visual* getVisual() { return g_visual; }
      inline Colormap getColormap() { return g_colormap; }
      inline Window getRootInput() { return g_root_input; }

      inline Atom getAtom(AtomName name) { return g_atoms[name]; }


      inline unsigned int getNumLock() { return num_lock; }
      inline unsigned int getScrollLock() { return scroll_lock; }

      inline unsigned int getWidth() { return g_screen_geom.width; }
      inline unsigned int getHeight() { return g_screen_geom.height; }

      inline unsigned int getBorderWidth() { return g_config->getBorderWidth(); }

      inline void setInputFocus(Window w) {
         XSetInputFocus(g_display, w, RevertToPointerRoot, CurrentTime);
      }

      unsigned int getMaskFromKeycode(KeyCode);
      void stripStateModifiers(unsigned int *state) {
         *state &= ~(num_lock | scroll_lock | LockMask);
      }

      void grabKeys(Window);
      void grabButtons(Window, int);

      //static bool getProperty(Window, AtomName, Atom, unsigned long, unsigned char**, unsigned long*);

      // EWMH functions
      void initEWMHProperties();
      void initEWMHClient(Window);
      void setEWMHActiveWindow(Window); 
      void setEWMHClientList();
      void setEWMHDesktop(Window, uint);
      void setEWMHCurrentDesktop();

      unsigned int getWMWindowType(Window);
      void setWmState(Window, ulong);


      inline XColor getBorderColour(int i) { return border_colour[i]; }

      void initTags();
      inline unsigned int getCurrentTagIndex() { return current_tag; }
      inline Tag* getCurrentTag() { return g_tags.at(current_tag); }
      inline void setCurrentTag(unsigned int tag) { current_tag = tag; setEWMHCurrentDesktop();}

      void sendFrameToTag(Frame*, unsigned int);

      //--- Tag functions -----
      void updateAllTags();
      void updateCurrentTag();

      Client* findClient(Window win);

      inline void insertClient(Client* client) { client_list.push_back(client); }
      inline void removeClient(Client* client) {
         for(uint i=0; i<client_list.size(); i++){
            if(client_list.at(i)==client)
               client_list.erase(client_list.begin()+i);
         }
         //client_list.erase(find(client_list.begin(), client_list.end(), client));
      }

      void fixFrame(Frame*);

   private:
      //--- static variables
      Display* g_display;
      Window g_root;
      int g_screen;
      Configuration* g_config;
      Window g_root_input;

      int g_depth;
      Visual* g_visual;
      Colormap g_colormap;
      
      bool has_randr;
      Geometry g_screen_geom;

      unsigned int current_tag;
      vector<Tag*> g_tags;
      vector<Head> g_heads;
      Atom g_atoms[NATOMS];

      XModifierKeymap * modifier_map;
      unsigned int num_lock;
      unsigned int scroll_lock;
      
      XColor border_colour[NBORDERCOL];

      vector<Client*> client_list;

      //--- functions
      void initHeads();

      void grabKey(Window, unsigned int, unsigned int);
      void grabButton(Window, unsigned int, unsigned int);
};

#endif
