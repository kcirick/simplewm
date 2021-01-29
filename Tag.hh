#ifndef TAG_HH
#define TAG_HH

class Tag {
   public:
      Tag(XScreen* screen, unsigned int id) :
         g_xscreen(screen), tag_id(id), iCurClient(-1) { };
      ~Tag() { };

      inline Client* findClient(Window win) {
         Client* client=NULL;
         for(unsigned int i=0; i<client_list.size(); i++)
            if(client_list.at(i)->getWindow() == win)
                   return client_list.at(i);
         
         return client;
      }

      void addWindow(Window);
      void updateTag();
      void showTag();
      void hideTag();
      void insertClient(Client*);
      void removeClient(Client*, bool);

      inline Client* getCurrentClient() { return iCurClient<0 ? NULL : client_list.at(iCurClient); }
      void setCurrentClient(Client* client) {
         for(unsigned int i=0; i<client_list.size(); i++)
            if(client_list.at(i)==client)
               iCurClient = i;
      }
      vector<Client*> getMarkedClients();

      void cycleClient();

   private:
      XScreen* g_xscreen;

      unsigned int tag_id;

      int iCurClient;
      vector<Client*> client_list;
};

#endif
