#include "Globals.hh"
#include "Configuration.hh"
#include "XScreen.hh"
#include "Menu.hh"

const unsigned long MENU_MASK =  ButtonPressMask|ButtonReleaseMask|
	EnterWindowMask|LeaveWindowMask|PointerMotionMask|ButtonMotionMask|
	ExposureMask|StructureNotifyMask|KeyPressMask;

int max(int value1, int value2){
   return value1>value2 ? value1 : value2;
}

//--- Constructor and destructor -----------------------------------------
Menu::Menu(XScreen* screen, Configuration* config) : g_xscreen(screen), g_config(config) { 
   menu_items = g_config->getMenuItems();
   visible = false;
}

Menu::~Menu() { }

//------------------------------------------------------------------------
//FIXME get inspiration from 9menu: https://github.com/arnoldrobbins/9menu/blob/master/9menu.c
//
void Menu::createMenu() {
   say(DEBUG, "Menu::createMenu()");

   font = XLoadQueryFont(g_xscreen->getDisplay(), g_config->getMenuFontName().c_str());

   unsigned long fg_colour = g_xscreen->getMenuColour(FG).pixel;
   unsigned long bg_colour = g_xscreen->getMenuColour(BG).pixel;
   unsigned long selfg_colour = g_xscreen->getMenuColour(SEL_FG).pixel;
   unsigned long selbg_colour = g_xscreen->getMenuColour(SEL_BG).pixel;

   XGCValues gv1, gv2, gv3;
   gv1.foreground = fg_colour;
	gv1.background = bg_colour;
	gv1.font = font->fid;
	gv1.line_width = 2;

   gv2.foreground = selbg_colour;
   gv2.background = bg_colour;
   gv2.font = font->fid;
   gv1.line_width = 2;

   gv3.foreground = selfg_colour;
   gv3.background = bg_colour;
   gv3.font = font->fid;
   gv3.line_width = 2;

	unsigned long mask = GCForeground | GCBackground | GCFont | GCLineWidth;
	menu_gc = XCreateGC(g_xscreen->getDisplay(), g_xscreen->getRoot(), mask, &gv1);
   selbg_gc  = XCreateGC(g_xscreen->getDisplay(), g_xscreen->getRoot(), mask, &gv2);
   selfg_gc  = XCreateGC(g_xscreen->getDisplay(), g_xscreen->getRoot(), mask, &gv3);
   
   // Main menu window
   int max_width = 0;
   for(unsigned int i=0; i<menu_items.size(); i++){
      string ilabel = menu_items.at(i)->label;
      max_width = max(max_width, XTextWidth(font, ilabel.c_str(), ilabel.length()));
   }

   row_height = font->ascent + font->descent + 2;
   geom.x = 0;
   geom.y = 0;
   geom.width = max_width + 4;
   geom.height = menu_items.size()*row_height;
   
   main_window = XCreateSimpleWindow(g_xscreen->getDisplay(), g_xscreen->getRoot(),
         geom.x, geom.y, geom.width, geom.height, 2, fg_colour, bg_colour);

   XSelectInput(g_xscreen->getDisplay(), main_window, MENU_MASK);

}

void Menu::showMenu() {
   say(DEBUG, "Menu::showMenu()");
   
   if(!visible)
	   XMapWindow(g_xscreen->getDisplay(), main_window);

   selected_row = 0;

   moveMenu();
   drawMenu();

   visible = true;
}

void Menu::hideMenu() {
   if(visible)
      XUnmapWindow(g_xscreen->getDisplay(), main_window);
   visible = false;
}

void Menu::drawMenu() {
   XClearWindow(g_xscreen->getDisplay(), main_window);

   int tx, ty;
   for(unsigned int i=0; i<menu_items.size(); i++){
      if(menu_items.at(i)->menu_type == SEPARATOR){
         ty = i*row_height + row_height/2;
         XDrawLine(g_xscreen->getDisplay(), main_window, menu_gc, 0, ty, geom.width, ty);
      } else {
         string ilabel = menu_items.at(i)->label;
         tx = (geom.width - XTextWidth(font, ilabel.c_str(), ilabel.length())) / 2;
         ty = i*row_height;

         if(int(i)==selected_row){
            XFillRectangle(g_xscreen->getDisplay(), main_window, selbg_gc, 0, ty, geom.width, row_height);
            XDrawString(g_xscreen->getDisplay(), main_window, selfg_gc, tx, ty+font->ascent+1, ilabel.c_str(), ilabel.length());
         } else {
            XDrawString(g_xscreen->getDisplay(), main_window, menu_gc, tx, ty+font->ascent+1, ilabel.c_str(), ilabel.length());
         }
      }
   }

   XRaiseWindow(g_xscreen->getDisplay(), main_window);
   g_xscreen->setInputFocus(main_window);
}

void Menu::moveMenu() {
   Window dw;
   int di;
	int x, y;
   unsigned int dui;

   XQueryPointer(g_xscreen->getDisplay(), main_window, &dw, &dw, &x, &y, &di, &di, &dui);
   XMoveWindow(g_xscreen->getDisplay(), main_window, x, y);
}

void Menu::executeItem(int row, bool &do_exit){

   string this_item = (menu_items.at(row))->command;

   string command_type = getToken(this_item, ' ');
   string command_arg  = this_item;

   if(command_type == "Exec"){
      spawn(command_arg);
   } if(command_type == "Exit") {
     do_exit = true; 
   }
   hideMenu();
}

