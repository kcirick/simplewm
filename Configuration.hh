#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

class Configuration {
   public:
      Configuration();
      ~Configuration();

      void loadConfig(string);
      
      inline int getNTags() const { return n_tags; }
      inline string getTagName(unsigned int i) { return tag_names.at(i); }
      inline vector<string> getTagNamesAll() { return tag_names; }
      
      inline int getBorderWidth() { return border_width; }
      inline string getBorderColour(int i) { return border_colour[i];}

      inline bool isSloppyFocus() { return sloppy_focus; }

      inline int getMoveResizeStep() { return moveresize_step; }
      inline vector<KeyMap*> getKeymap() { return key_bindings; }
      inline vector<MouseMap*> getMousemap() { return mouse_bindings; }

   private:
      int n_tags;
      vector<string> tag_names;
      int border_width;
      bool sloppy_focus;
      int moveresize_step;

      string border_colour[NBORDERCOL];

      vector <KeyMap*> key_bindings;
      vector <MouseMap*> mouse_bindings;
};

#endif
