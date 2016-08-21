#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

class Configuration {
   public:
      Configuration();
      ~Configuration();

      void loadConfig(string);
      void loadBinding(string);
      void loadMenu(string);
      
      inline int getNTags() const { return n_tags; }
      inline string getTagName(unsigned int i) { return tag_names.at(i); }
      inline string getTagNamesAll() {return tag_names_all; }

      inline string getBindingFileName() { return binding_file; }
      inline string getMenuFileName() { return menu_file; }

      inline int getBorderWidth() { return border_width; }
      inline string getBorderColour(int i) { return border_colour[i];}
      inline string getMenuColour(int i) { return menu_colour[i];}
      inline string getMenuFontName() { return menu_font;}

      inline vector<KeyMap*> getKeymap() { return key_bindings; }
      inline vector<MouseMap*> getMousemap() { return mouse_bindings; }
      inline vector<MenuItem*> getMenuItems() { return menu_items; }

   private:
      int n_tags;
      vector<string> tag_names;
      string tag_names_all;
      int border_width;

      string border_colour[NBORDERCOL];
      string menu_colour[NMENUCOL];

      string binding_file;
      string menu_file;

      string menu_font;

      vector <KeyMap*> key_bindings;
      vector <MouseMap*> mouse_bindings;
      vector <MenuItem*> menu_items;
};

#endif
