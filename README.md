# SimpleWM

A simple window manager with Xlib written to fit my specific needs, and also to teach myself the basics of window management. 


## Description

  - Minimal stacking window manager
  - Written in C++
  - Features:
    - No frills (menu, titlebar, icons, pixmap themes, autostart etc...)
    - Text config file (default $HOME/.config/simplewm/configrc)
    - Clients can be fixed (visible on all tags), iconified (i.e. hidden), or marked (marked clients can perform group actions)

Usage:

`> simplewm [--config file][--debug][--version][--help]`


## Client marks and group actions

  1. Multiple clients can be marked with MOD4+m (can be customized in config file)
     - The clients can be in different tags
  2. The marked clients can perform group actions
     - Fix (visible on all tags) with MOD4+f
     - Iconify (minimize) with MOD4+i
     - Send to tag with MOD4+Shift+[1-n_tags] (tag index)
  3. The marked clients can be de-marked with MOD4+m


## Screenshots

v0.2

<a href='https://s6.postimg.cc/8aqgm3lwh/SS_20161008.png' target='_blank'><img src='https://s6.postimg.cc/8aqgm3lwh/SS_20161008.png' width='350' /></a>


## Version log

  - 0.3 (Work in progress)
    - Goal: Simplify code in order to minimize potential bugs. Remove features that complicate code structures
    - Remove window groups
    - Remove Frame class (now part of Client class)
    - Remove rules
    - Add group actions (fix, iconify, and send to tag) on marked clients
  - 0.2 (2019-09-01) ([download][v02])
    - Goal: Code clean up and maximize efficiency
    - Removed root menu to simplify things. There are other external tools that can be used
  - 0.1 (2016-09-08) ([download][v01])
    - Goal: Getting the base WM

   [v01]: https://github.com/kcirick/simplewm/archive/v0.1.tar.gz
   [v02]: https://github.com/kcirick/simplewm/archive/v0.2.tar.gz


## To do / Known issues:

Please use [GitHub Issues Tracker][ghit] to report bugs and issues.

   [ghit]: https://github.com/kcirick/simplewm/issues

  - BUG: Cannot set input on certain WINE applications
  - BUG: Random crashes, sources unknown
  - Code clean up
  - Implement Maximized/Fullscreen EMWH request
  - ~~Implement group action on marked clients~~ (v0.3)
  - ~~Implement rules~~ (removed v0.3)
  - ~~Implement EWMH properties~~ (v0.2)
  - ~~Implement window grouping~~ (v0.1, removed v0.3)
  - ~~Implement fixed state for frames~~ (v0.1)
  - ~~Implement iconic state for frames~~ (v0.1)

