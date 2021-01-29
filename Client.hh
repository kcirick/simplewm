#ifndef CLIENT_HH
#define CLIENT_HH

class Client {
   public:
      Client(XScreen*, Window);
      virtual ~Client();

      inline bool isIconified()  { return iconified; }
      inline bool isUrgent()     { return urgent; }
      inline bool isMarked()     { return marked; }
      inline bool isFixed()      { return fixed; }

      inline void setIconified(bool flag) { iconified = flag; }
      inline void setUrgent(bool flag)    { urgent = flag; }
      inline void toggleMarked()          { marked = !marked; }
      inline void toggleFixed()           { fixed = !fixed; }

      inline Window getWindow()     { return window; }

      inline Geometry getGeometry() { return geom; }
      void updateGeometry(Geometry this_geom);

      void updateBorderColour(bool);

      void kill(bool);

      void dragMove();
      void dragResize();

      //bool isTransient() const { return transient_for_window != None; }
      //Client *getTransientForClient() const { return translient_for; }
      //Window getTransientForClientWindow() const { return transient_for_window; }
      //void findAndRaiseIfTransient();
      
   private:
      XScreen* g_xscreen;
      Window window;
      Geometry geom;
      
      bool marked;
      bool urgent;
      bool iconified;
      bool fixed;
      //bool wm_hints_input;
      //bool updateWindowAttributes();
      
      void initGeometry();

      void send_config();

      //void setWMState(unsigned long);
      //long getWMState();
      //unsigned long getWMHints();
};

#endif
