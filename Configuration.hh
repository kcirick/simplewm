#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

class Configuration {
   public:
      Configuration();
      ~Configuration();

      void loadConfig(string);
      
      inline int getNTags() const { return n_tags; }
      inline string getTagName(unsigned int i) { return tag_names.at(i); }
      inline string getTagNamesAll() {
         string tag_names_all = accumulate(tag_names.begin(), tag_names.end(), string{},
                                    [](const string& a, string b) {
                                        return a.empty() ? b : a + ' ' + b;
                                    });
         return tag_names_all;
      }

      inline int getBorderWidth() { return border_width; }
      inline string getBorderColour(int i) { return border_colour[i];}

      inline vector<KeyMap*> getKeymap() { return key_bindings; }
      inline vector<MouseMap*> getMousemap() { return mouse_bindings; }
      inline vector<Rule*> getRules() { return rules; }

   private:
      int n_tags;
      vector<string> tag_names;
      int border_width;

      string border_colour[NBORDERCOL];

      vector <KeyMap*> key_bindings;
      vector <MouseMap*> mouse_bindings;
      vector <Rule*> rules;
};

#endif
