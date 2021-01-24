# SimpleWM

A simple window manager with Xlib written to fit my specific needs, and also to teach myself the basics of window management. 

## Description

  - Minimal stacking window manager
  - Written in C++
  - Features:
    - No frills (menu, titlebar, icons, pixmap themes, autostart etc...)
    - Text config file (default $HOME/.config/simplewm/configrc)
    - Clients can be fixed (visible on all tags), iconified (i.e. hidden), or marked (no action yet)

Usage:

`> simplewm [--config file][--debug][--version][--help]`


## Screenshot

v0.2

<a href='https://s6.postimg.cc/8aqgm3lwh/SS_20161008.png' target='_blank'><img src='https://s6.postimg.cc/8aqgm3lwh/SS_20161008.png' width='350' /></a>

## Version Log

  - 0.3 (Work in progress)
    - Goal: Simplify code in order to minimize potential bugs. Remove features that complicates code structures
    - Remove window groups
    - Remove Frame class (now part of Client class)
    - Remove rules
  - 0.2 (2019-09-01) (<a href="https://github.com/kcirick/simplewm/archive/v0.2.zip">download .zip</a>)
    - Goal: Code clean up and maximize efficiency
    - Removed root menu to simplify things. There are other external tools that can be used
  - 0.1 (2016-09-08) (<a href="https://github.com/kcirick/simplewm/archive/v0.1.zip">download .zip</a>)
    - Goal: Getting the base WM

## To do / Known issues:

  - BUG: Cannot set input on certain WINE applications
  - BUG: Random crashes, sources unknown
  - Code clean up
  - Implement Maximized/Fullscreen EMWH request
  - Implement group action on marked clients
  - ~~Implement EWMH properties~~ (v0.2)
  - ~~Implement rules~~ (removed v0.3)
  - ~~Implement window grouping~~ (v0.1)
  - ~~Implement fixed state for frames~~ (v0.1)
  - ~~Implement iconic state for frames~~ (v0.1)
