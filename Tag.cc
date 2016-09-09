#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Tag.hh"

//------------------------------------------------------------------------
void Tag::addWindow(Window win){
   say(DEBUG, "Tag::addWindow()");

   Client* client = new Client(g_xscreen, win);
   if(!client->isManaged()){
      delete client;
      return;
   }

   g_xscreen->insertClient(client);
   
   Frame* frame = new Frame(g_xscreen, client);
   frame_list.push_back(frame);
   iCurFrame = frame_list.size()-1;

   client->reparent(frame->getFrameWindow());

   g_xscreen->setEWMHClientList();
}

void Tag::updateTag(){
   if(frame_list.size()==0) return;

   //say(DEBUG, "===> updateTag: nframes = "+to_string(frame_list.size())+" - current = "+to_string(iCurFrame));
   for(unsigned int i=0; i<frame_list.size(); i++)
      if(frame_list.at(i)->isIconified())
         XUnmapWindow(g_xscreen->getDisplay(), frame_list.at(i)->getFrameWindow());
      else
         frame_list.at(i) -> refreshFrame((int)i==iCurFrame);
}

void Tag::showTag(){
   for(unsigned int i=0; i<frame_list.size(); i++){
      if(frame_list.at(i)->isIconified())
         XUnmapWindow(g_xscreen->getDisplay(), frame_list.at(i)->getFrameWindow());
      else 
         XMapWindow(g_xscreen->getDisplay(), frame_list.at(i)->getFrameWindow());
   }
}

void Tag::hideTag(){
   for(unsigned int i=0; i<frame_list.size(); i++)
      if(!frame_list.at(i)->isFixed())
         XUnmapWindow(g_xscreen->getDisplay(), frame_list.at(i)->getFrameWindow());
}

void Tag::cycleFrame(){
   unsigned int list_size=frame_list.size();
   Frame* current_frame = frame_list.at(iCurFrame);

   XGrabKeyboard(g_xscreen->getDisplay(), g_xscreen->getRoot(), True, GrabModeAsync, GrabModeAsync, CurrentTime);

	XEvent ev;
	for (;;) {
      drawOutline(g_xscreen, current_frame->getFrameGeometry());
		XMaskEvent(g_xscreen->getDisplay(), KeyPressMask|KeyReleaseMask, &ev);
      KeySym keysym = XkbKeycodeToKeysym(g_xscreen->getDisplay(), ev.xkey.keycode, 0, 0);
      drawOutline(g_xscreen, current_frame->getFrameGeometry());

		switch (ev.type) {
			case KeyPress:
            if(iCurFrame<(int)list_size-1)   iCurFrame++;
            else                             iCurFrame = 0;
            current_frame = frame_list.at(iCurFrame);
				break;
			case KeyRelease:
            //refreshFrame(true);
            if(keysym!=XK_Tab){
               Client* vc = frame_list.at(iCurFrame)->getClientVisible();
               if(current_frame->isIconified()){
                  current_frame->setIconified(false);
                  XMapWindow(g_xscreen->getDisplay(), current_frame->getFrameWindow());
               }
               XRaiseWindow(g_xscreen->getDisplay(), vc->getFrame());
               XUngrabKeyboard(g_xscreen->getDisplay(), CurrentTime);               
               return;
            }
            break;
			default: break;
		}
	}
}

void Tag::groupMarkedFrames(Frame* root_frame) {
   say(DEBUG, "Tag::groupMarkedFrames()");

   // Do nothing if visible client in the frame is not marked
   if(!root_frame->isMarked()) return;

   for(unsigned int i=0; i<frame_list.size(); i++){
      Frame* f = frame_list.at(i);
      say(DEBUG, "FRAME WINDOW = "+to_string(f->getFrameWindow()));

      if(!f->isMarked()) continue;
      
      if(f == root_frame){
         f->toggleMarked();
         iCurFrame = i;
         continue; // don't add duplicate 
      }

      vector<Client*> clist = f->getClientList();

      for(unsigned int ic=0; ic<clist.size(); ic++){
         Client* c = clist.at(ic);
         c->reparent(root_frame->getFrameWindow());
         root_frame->addToClientList(c);

         f->removeClient(c, false);

         // add top bar for each client added
         Window topbar = XCreateSimpleWindow(g_xscreen->getDisplay(), root_frame->getFrameWindow(),
            0, 0, 10, 10, 0,  // we don't know the size yet
            (g_xscreen->getBorderColour(UNFOCUSED)).pixel, (g_xscreen->getBorderColour(UNFOCUSED)).pixel);

         XMapWindow(g_xscreen->getDisplay(), topbar);
         root_frame->addToTopbarList(topbar);
      }
      removeFrame(f, false, true);
      i--;
   }

   say(DEBUG, "groupMarkedClient() finish");
}

void Tag::detachFrame(Frame* root_frame){
   say(DEBUG, "Tag::detachFrame()");

   // do nothing if there is only one client in the frame
   if(root_frame->getNClients()==1 ) return;

   Client *this_client = root_frame -> getClientVisible();
   g_xscreen->removeClient(this_client);
   Window this_window = this_client->getWindow();
   root_frame->removeClient(this_client, true);

   // This will create a new frame and client
   addWindow(this_window);
}

void Tag::removeFrame(Frame* frame, bool unmap_only, bool delete_frame){
   say(DEBUG, "Tag::removeFrame()");

   vector<Client*> clist = frame -> getClientList();
   //if(delete_frame && clist.size()>0) return;   // can't delete frame if there is client

   // remove clients from client_list
   //for(unsigned int i=0; i<clist.size(); i++)
   //   g_xscreen->removeClient(clist.at(i));

   if(unmap_only){
      XUnmapWindow(g_xscreen->getDisplay(), frame->getFrameWindow());
      return;
   }

   bool found = false;
   for(unsigned int i=0; i<frame_list.size(); i++){
      if(frame_list.at(i) == frame){ 
         frame_list.erase(frame_list.begin()+i);
         iCurFrame = (i>0) ? (i-1) : 0;
         found = true;
      }
   }
   if(!found) return;

   if(delete_frame) delete frame;
}

void Tag::insertFrame(Frame* frame) {
   say(DEBUG, "Tag::insertFrame()");

   // add clients to client_list
   vector<Client*> clist = frame->getClientList();
   for(unsigned int i=0; i<clist.size(); i++)
      g_xscreen->setEWMHDesktop(clist.at(i)->getWindow(), tag_id);
      //g_xscreen->insertClient(clist.at(i));

   // add frame
   frame_list.push_back(frame);
   //iCurFrame = (int)frame_list.size()-1;
}
