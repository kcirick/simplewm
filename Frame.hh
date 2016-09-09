#ifndef FRAME_HH
#define FRAME_HH

class Frame {
   public:
      Frame(XScreen*, Client*);
      virtual ~Frame();

      inline bool isIconified()  { return iconified; }
      inline bool isUrgent()     { return urgent; }
      inline bool isMarked()     { return marked; }
      inline bool isFixed()      { return fixed; }

      inline void setIconified(bool flag) { iconified = flag; }
      inline void setUrgent(bool flag)    { urgent = flag; }
      inline void toggleMarked()          { marked = !marked; }
      inline void toggleFixed()           { fixed = !fixed; }

      inline vector<Client*> getClientList() { return client_list; }
      inline Client* getClientVisible() { return client_list.at(iVisibleClient); }
      inline void setVisibleClientIndex(unsigned int i) { iVisibleClient = i; }
      inline unsigned int getNClients() { return client_list.size(); }

      inline vector<Window> getTopbarList() { return topbar_list; }

      inline void addToClientList(Client* client) { client_list.push_back(client); iVisibleClient++;}
      inline void addToTopbarList(Window window)  { topbar_list.push_back(window); }

      inline Window getFrameWindow() { return frame; }

      inline Geometry getFrameGeometry() { return geom; }
      inline void setFrameGeometry(Geometry new_geom) { geom = new_geom; }

      void refreshFrame(bool);
      void raiseFrame();

      void dragMoveFrame();
      void dragResizeFrame();

      void selectNextClient(int);

      unsigned int removeClient(Client *, bool);

      void killVisibleClient(bool);
   
   private:
      XScreen* g_xscreen;

      Window frame;
      Geometry geom;
      vector<Window> topbar_list;
      vector<Client*> client_list;
      int iVisibleClient;
      bool marked;
      bool urgent;
      bool iconified;
      bool fixed;

      //--- functions -----
      void createFrame(Client*);
      void updateFrameGeometry();

      void send_config();
};

#endif
