#include <fstream>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include "Globals.hh"
#include "Configuration.hh"

//--- Constructor and destructor -----------------------------------------
Configuration::Configuration(){
   // Set default values
   n_tags = 4;
   border_width = 2;
   border_colour[FOCUSED]     == "#0000FF";
   border_colour[UNFOCUSED]   == "#333333";
   border_colour[URGENT]      == "#FF0000";
   border_colour[MARKED]      == "#00FF00";
   border_colour[FIXED]       == "#0000FF";

   menu_colour[FG]      == "#000000";
   menu_colour[BG]      == "#FFFFFF";
   menu_colour[SEL_FG]  == "#FF0000";
   menu_colour[SEL_BG]  == "#1793D0";

   binding_file = getenv("HOME") + string("/.config/simplewm/binding");
   menu_file = getenv("HOME") + string("/.config/simplewm/menu");

   menu_font == "fixed"; //default font
}

Configuration::~Configuration(){ }

//------------------------------------------------------------------------
void Configuration::loadConfig(string filename){
   say(DEBUG, string("Loading config file "+filename)); 

   ifstream configfile(filename.data());
   string line;
   if(configfile.is_open()){
      while(getline(configfile, line)) {
         if(line.empty() || line[0]=='#') continue;

         string id = trimString(getToken(line, '='));
         string value = trimString(line);

         if(id == "n_tags")  n_tags = stoi(value);
         if(id == "border_width")  border_width = stoi(value);
         if(id == "border_colour") {
            for(unsigned int i=0; i<NBORDERCOL; i++)
               border_colour[i] = getToken(value, ' ');
         }
         if(id == "menu_colour") {
            for(unsigned int i=0; i<NMENUCOL; i++)
               menu_colour[i] = getToken(value, ' ');
         }
         if(id == "tag_names") {
            for(int i=0; i<n_tags; i++)
               tag_names.push_back(getToken(value, ';'));
         }
         if(id == "binding_file")
            binding_file = getenv("HOME") + string("/.config/simplewm/") + value;
         if(id == "menu_file")
            menu_file = getenv("HOME") + string("/.config/simplewm/") + value;
         if(id == "menu_font")
            menu_font = value;
      }
      configfile.close();
   }
}

void Configuration::loadBinding(string filename){
   say(DEBUG, string("Loading binding file "+filename));

   ifstream bindingfile(filename.data());
   string line;
   if(bindingfile.is_open()){
      while(getline(bindingfile, line)) {
         if(line.empty() || line[0]=='#') continue;

         string id = trimString(getToken(line, '='));
         string value = trimString(line);

         if(id == "KEY") {
            string binding = getToken(value, ' ');
            string function = getToken(value, ' ');
            string args = value;

            unsigned int mod = 0;
            KeySym keysym;
            string key_char = getToken(binding, '+');
            bool do_continue = true;
            while(do_continue){
               if(key_char == binding) do_continue = false;

                    if(key_char == "Shift")     mod |= ShiftMask;
               else if(key_char == "Control")   mod |= ControlMask;
               else if(key_char == "Mod1")      mod |= Mod1Mask;
               else if(key_char == "Mod2")      mod |= Mod2Mask;
               else if(key_char == "Mod3")      mod |= Mod3Mask;
               else if(key_char == "Mod4")      mod |= Mod4Mask;
               else if(key_char == "Mod5")      mod |= Mod5Mask;
               else                             keysym = XStringToKeysym(key_char.c_str());

               key_char = getToken(binding, '+');
            }
            
            int this_fn = -1;
                 if(function == "QUIT")   this_fn = QUIT;
            else if(function == "TAG")    this_fn = TAG;
            else if(function == "SPAWN")  this_fn = SPAWN;
            else if(function == "CLIENT") this_fn = CLIENT;
            else if(function == "MENU")   this_fn = MENU;
            
            key_bindings.push_back(new KeyMap(mod, keysym, this_fn, string(args)));
         } // end KEY
         if(id == "MOUSE"){
            string binding = getToken(value, ' ');
            string context = getToken(value, ' ');
            string args = value;

            unsigned int mod = 0;
            unsigned int button;
            string button_char = getToken(binding, '+');
            bool do_continue = true;
            while(do_continue){
               if(button_char == binding) do_continue = false;

                    if(button_char == "Shift")   mod |= ShiftMask;
               else if(button_char == "Control") mod |= ControlMask;
               else if(button_char == "Mod1")    mod |= Mod1Mask;
               else if(button_char == "Mod2")    mod |= Mod2Mask;
               else if(button_char == "Mod3")    mod |= Mod3Mask;
               else if(button_char == "Mod4")    mod |= Mod4Mask;
               else if(button_char == "Mod5")    mod |= Mod5Mask;
               else if(button_char == "Button1") button = Button1;
               else if(button_char == "Button2") button = Button2;
               else if(button_char == "Button3") button = Button3;
               else if(button_char == "Button4") button = Button4;
               else if(button_char == "Button5") button = Button5;

               button_char = getToken(binding, '+');
            }

            int this_context = -1;
                 if(context == "ROOT")    this_context = CONTEXT_ROOT;
            else if(context == "FRAME")   this_context = CONTEXT_FRAME;
            
            mouse_bindings.push_back(new MouseMap(mod, button, this_context, string(args)));
         } // end MOUSE
      }
      bindingfile.close();
   }
}

void Configuration::loadMenu(string filename){
   say(DEBUG, string("Loading menu file "+filename));
   
   ifstream menufile(filename.data());
   string line;
   if(menufile.is_open()){
      while(getline(menufile, line)) {
         if(line.empty() || line[0]=='#') continue;

         string id = trimString(getToken(line, '='));
         string value = trimString(line);

         if(id == "ENTRY") {
            string label = trimString(getToken(value, '|'));
            string command = trimString(value);

            menu_items.push_back(new MenuItem(ENTRY, label, command));
         } // end ENTRY
         if(id == "SEPARATOR")
            menu_items.push_back(new MenuItem(SEPARATOR, "", ""));
      }
      menufile.close();
   }
}

