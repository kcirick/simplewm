#ifndef MENU_HH
#define MENU_HH

class Menu {
   public:
      Menu(XScreen*, Configuration*);
      ~Menu();

      inline Window getMenuWindow() { return main_window; }

      void createMenu();
      void showMenu();
      void hideMenu();
      void drawMenu();

      void executeItem(int, bool&);

      inline int getSelectedRow() { return selected_row; }
      inline void selectRow(int row) {
         selected_row = row;
         if(row<0) selected_row = 0;
         if(row>(int)menu_items.size()-1) selected_row = menu_items.size()-1;
      }
      inline int getRowHeight() { return row_height; }

   private:
      XScreen* g_xscreen;
      Configuration* g_config;
      Window main_window;
      GC menu_gc, selbg_gc, selfg_gc;
      XFontStruct *font;

      vector<MenuItem*> menu_items;
      int selected_row;

      bool visible;
      int row_height;
      Geometry geom;

      void moveMenu();
};

#endif
