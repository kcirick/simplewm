#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Client.hh"
#include "Tag.hh"

//------------------------------------------------------------------------
void Tag::addWindow(Window win){
   say(DEBUG, "Tag::addWindow()");

   Client* client = new Client(g_xscreen, win);

   g_xscreen->insertClient(client);

   client_list.push_back(client);
   iCurClient = client_list.size()-1;   

   g_xscreen->setEWMHClientList();
   g_xscreen->setEWMHActiveWindow(win);
}

void Tag::updateTag(){
   if(client_list.size()==0) return;

   say(DEBUG, "===> updateTag: nframes = "+to_string(client_list.size())+" - current = "+to_string(iCurClient));
   for(unsigned int i=0; i<client_list.size(); i++)
      if(client_list.at(i)->isIconified())
         XUnmapWindow(g_xscreen->getDisplay(), client_list.at(i)->getFrame());
      else {
         XMapWindow(g_xscreen->getDisplay(), client_list.at(i)->getFrame());
         client_list.at(i) -> refreshFrame((int)i==iCurClient);
      }
}

void Tag::showTag(){
   for(unsigned int i=0; i<client_list.size(); i++){
      if(client_list.at(i)->isIconified())
         XUnmapWindow(g_xscreen->getDisplay(), client_list.at(i)->getFrame());
      else 
         XMapWindow(g_xscreen->getDisplay(), client_list.at(i)->getFrame());
   }
}

void Tag::hideTag(){
   for(unsigned int i=0; i<client_list.size(); i++)
      if(!client_list.at(i)->isFixed())
         XUnmapWindow(g_xscreen->getDisplay(), client_list.at(i)->getFrame());
}

void Tag::cycleClient(){

   say(DEBUG, "Tag::cycleClient()");

   // do nothing if there are no frames to cycle
   if(iCurClient<0) return;

   unsigned int list_size=client_list.size();
   Client* current = client_list.at(iCurClient);

   XGrabKeyboard(g_xscreen->getDisplay(), g_xscreen->getRoot(), True, GrabModeAsync, GrabModeAsync, CurrentTime);

	XEvent ev;
	for (;;) {
      g_xscreen->drawOutline(current->getFrameGeometry());
		XMaskEvent(g_xscreen->getDisplay(), KeyPressMask|KeyReleaseMask, &ev);
      KeySym keysym = XkbKeycodeToKeysym(g_xscreen->getDisplay(), ev.xkey.keycode, 0, 0);
      g_xscreen->drawOutline(current->getFrameGeometry());

		switch (ev.type) {
			case KeyPress:
            if(iCurClient<(int)list_size-1)  iCurClient++;
            else                             iCurClient = 0;
            current = client_list.at(iCurClient);
				break;
			case KeyRelease:
            if(keysym!=XK_Tab){
               Client* vc = client_list.at(iCurClient);
               if(current->isIconified()){
                  current->setIconified(false);
                  XMapWindow(g_xscreen->getDisplay(), current->getFrame());
               }
               XRaiseWindow(g_xscreen->getDisplay(), vc->getFrame());
               XUngrabKeyboard(g_xscreen->getDisplay(), CurrentTime);               

               g_xscreen -> setEWMHActiveWindow(vc->getWindow());
               return;
            }
            break;
			default: break;
		}
	}
}

void Tag::removeClient(Client* client, bool delete_client){
   say(DEBUG, "Tag::removeClient");

   say(DEBUG, to_string(client_list.size()));

   bool found = false;
   for(uint i=0; i<client_list.size(); i++){
      if(client_list.at(i) == client){
         found = true;
         client_list.erase(client_list.begin()+i);
         iCurClient--;
      }
   }
   if(!found) return;

   if(delete_client){
      g_xscreen->removeClient(client);
      delete client;
   }

   g_xscreen->setEWMHClientList();
   return;
}

void Tag::insertClient(Client* client){
   say(DEBUG, "Tag::insertClient()");

   // add clients to client_list
   g_xscreen->setEWMHDesktop(client->getWindow(), tag_id);

   // add frame
   client_list.push_back(client);
   if(iCurClient<0) iCurClient=0;
}

