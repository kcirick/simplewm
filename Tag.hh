#ifndef TAG_HH
#define TAG_HH

class Tag {
   public:
      Tag(XScreen* screen, Configuration* config, unsigned int id) :
         g_xscreen(screen), g_config(config), tag_id(id), iCurFrame(-1) { };
      ~Tag() { };

      inline Frame* findFrame(Window win) {
         Frame* frame=NULL;
         for(unsigned int i=0; i<frame_list.size(); i++)
            if(frame_list.at(i)->getFrameWindow() == win)
                  frame=frame_list.at(i);
         
         return frame;
      }

      void addWindow(Window);
      void updateTag();
      void showTag();
      void hideTag();
      void removeFrame(Frame*, bool);
      void insertFrame(Frame*);

      inline Frame* getCurrentFrame() { return iCurFrame<0 ? NULL : frame_list.at(iCurFrame); }
      void setCurrentFrame(Frame* frame) {
         for(unsigned int i=0; i<frame_list.size(); i++)
            if(frame_list.at(i)==frame)
               iCurFrame = i;
      }

      void cycleFrame();
      void groupMarkedFrames(Frame*);
      void detachFrame(Frame*);

   private:

      XScreen* g_xscreen;
      Configuration* g_config;

      unsigned int tag_id;
      int iCurFrame;

      vector<Frame*> frame_list;
      //vector<Client*> client_list;
};


#endif
