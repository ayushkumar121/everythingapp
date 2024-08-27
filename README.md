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

This project includes a custom build system written in C. 
To bootstrap the build system, use a standard C compiler. 
Once bootstrapped, you can invoke the build system directly.

After the initial build, the build system can rebuild itself if necessary.

For Linux and Macos
```shell
$ cc -o make src/make.c # Only required for bootstraping
$ ./make
```

For Windows
```shell
$ cl.exe src/make.c # Only required for bootstraping
$ .\make.exe
```