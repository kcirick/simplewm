/*
 * Main.cc
 *   - Main SimpleWM Program
 */

#include <iostream>
#include <unistd.h>

#include "Globals.hh"
#include "WMCore.hh"

static bool debug = false;

//------------------------------------------------------------------------
void say(int level, string message) {
   if(level==DEBUG && !debug) return;

   const string msg_str[NMSG] = { "DEBUG", "INFO", "WARNING", "ERROR" };
   cout << "SimpleWM [" << msg_str[level] << "]: " << message << endl;

   if(level==ERROR) exit(EXIT_FAILURE);
}

void spawn(string cmd) {
   char *sh = NULL;
   if(!(sh=getenv("SHELL"))) sh = (char *)"/bin/sh";
   
   say(DEBUG, "Spawn "+cmd);
   pid_t pid;
   if((pid=fork())==0) {
      setsid();
      execl(sh, sh, "-c", cmd.c_str(), (char *)NULL);
   }
}

string trimString(string str) {
   str.erase(0, str.find_first_not_of(' '));
   str.erase(str.find_last_not_of(' ')+1);
   return str;
}

string getToken(string &str, char delim){
   string token = str.substr(0, str.find_first_of(delim));
   str = str.substr(str.find_first_of(delim)+1);
   return token;
}

//--- Main function ------------------------------------------------------
int main(int argc, char **argv) {
   string config_file;

   // Parse arguments
   for(int i=1; i<argc; i++){
      string iarg = argv[i];
      if(iarg=="--config" && ((i+1)<argc)) {
         config_file = argv[++i];
      }
      else if(iarg=="--debug") {
         debug = true;
      }
      else if(iarg=="--version") {
         say(INFO, "Version-"+string(VERSION));
         exit(EXIT_SUCCESS);
      }
      else if(iarg=="--help") {
         say(INFO, "Usage: simplewm [--config file][--debug][--version][--help]");
         exit(EXIT_SUCCESS);
      }
   }
   
   if(config_file.empty()) 
      config_file = getenv("HOME") + string("/.config/simplewm/configrc");
   
   WMCore *wmcore = new WMCore();
   wmcore -> read_config(config_file);
   wmcore -> setup();
   wmcore -> event_loop();

   // Cleanup
   wmcore -> clean_up();

   return EXIT_SUCCESS;
}
