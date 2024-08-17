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