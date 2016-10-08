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
         if(id == "tag_names") {
            for(int i=0; i<n_tags; i++)
               tag_names.push_back(getToken(value, ';'));
         }
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
         if(id == "RULE"){
            string attr_name = trimString(getToken(value, '|'));
            string attribute = getToken(attr_name, ':');

            string tag        = trimString(getToken(value, '|'));
            int itag = tag.size() ? stoi(tag) : -1;
            
            string layer      = trimString(getToken(value, '|'));
            int ilayer = -1;
                 if(layer == "BOTTOM") ilayer = BOTTOM;
            else if(layer == "TOP")    ilayer = TOP;

            string fixed      = trimString(getToken(value, '|'));
            bool bfixed = fixed=="true" ? true : false;

            string managed    = trimString(value);
            bool bmanaged = managed=="false" ? false : true;

            say(DEBUG, "RULE :: "+attribute+" :: "+attr_name+" :: "+to_string(itag)+" :: "+to_string(ilayer)+
               " :: "+to_string(bfixed)+" :: "+to_string(bmanaged));

            rules.push_back(new Rule(attribute, attr_name, itag, ilayer, bfixed, bmanaged)); 
         }
      }
      configfile.close();
   }
}

