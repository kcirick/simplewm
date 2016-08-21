#ifndef WMCORE_HH
#define WMCORE_HH

class WMCore {
   public:
      WMCore() : running(true) { };
      ~WMCore() { };

      void read_config(string);
      void setup();
      void event_loop();
      void clean_up();

   private:
      Configuration  * g_config;
      XScreen        * g_xscreen;
      Menu           * g_menu;

      bool running;
      
      //--- functions -----
      void scanWindows();

      void key_function(int, string, KeySym);
      void mouse_function(Frame *, string, int);

      //--- Event handlers -----
      void handleKeyEvent(XKeyEvent *);
      void handleMapRequestEvent(XMapRequestEvent *);
      void handleUnmapEvent(XUnmapEvent *);
      void handleDestroyWindowEvent(XDestroyWindowEvent *);
      void handleConfigureRequestEvent(XConfigureRequestEvent *);
      void handleClientMessageEvent(XClientMessageEvent *);
      void handlePropertyEvent(XPropertyEvent *);
      void handleButtonPressEvent(XButtonEvent *);
      void handleButtonReleaseEvent(XButtonEvent *);
      void handleEnterNotify(XCrossingEvent *);
      void handleLeaveNotify(XCrossingEvent *);
      void handleMotionEvent(XMotionEvent *);
      void handleMappingEvent(XMappingEvent *);
      void handleExposeEvent(XExposeEvent *);
};

#endif
