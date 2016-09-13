#ifndef WMCORE_HH
#define WMCORE_HH

class Configuration;
class Frame;
class XScreen;

class WMCore {
   public:
      WMCore() : running(true) { };
      ~WMCore() { };

      void test();

      void read_config(string);
      void setup();
      void event_loop();
      void clean_up();

   private:
      Configuration  * g_config;
      XScreen        * g_xscreen;

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
      void handleEnterNotify(XCrossingEvent *);
      void handleMappingEvent(XMappingEvent *);
};

#endif
