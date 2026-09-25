# Everything App

My Attempt at a life organiser app

## Architecture

- everything_*platform*.x files
    Platform specific code, intialisation of window and event loop

- everything.c
    Actual business logic of the application

- drawing.c
    Immediate mode drawing functions

- views.c
    Retained mode UI functions

## Building

The project builds with GNU make.

```shell
$ make          # build the app library and executable
$ make run      # build and start the app
$ make lib      # rebuild only the library, then press F5 in the running app to hot reload
$ make clean    # remove build outputs
```

Linux needs the Wayland client development package (`libwayland-dev`).

On Windows, run make from a Visual Studio developer prompt with a POSIX shell
such as Git Bash or MSYS2, so `cl.exe` and `date` are both available.