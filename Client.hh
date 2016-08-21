#ifndef CLIENT_HH
#define CLIENT_HH

class Client {
   public:
      Client(XScreen*, Window, Window);
      virtual ~Client();

      inline Geometry getGeometry() { return geom; }
      inline Window getWindow()     { return window; }
      inline Window getFrame()      { return parent; }

      inline bool isManaged() { return managed; }

      inline void setGeometry(Geometry this_geom) { geom = this_geom; }
      inline void setFrame(Window frame) { parent = frame; }

      void reparent();

      //bool isTransient() const { return transient_for_window != None; }
      //Client *getTransientForClient() const { return translient_for; }
      //Window getTransientForClientWindow() const { return transient_for_window; }
      //void findAndRaiseIfTransient();
      
   private:
      XScreen* g_xscreen;
      Window window;
      Window parent;
      Geometry geom;
      
      bool managed;

      //bool wm_hints_input;
      //bool updateWindowAttributes();
      
      void initGeometry();

      //void setWMState(unsigned long);
      //long getWMState();
      //unsigned long getWMHints();
};

#endif
