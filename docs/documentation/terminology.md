---
icon: octicons/book-24
title: Terminology
---
This page discusses and defines important _terms_, _names_ and _conventions_ that are used in **GITS** and this documentation. An understanding of these is important to understand and master **GITS**.

# Stream

Streams are the central files gits uses, they contain the information that was captured by the **Recorder** and the **Player** can play it back.

Streams recorded on one system are not tied only to this particular configuration when playing it back. **GITS** player gives user a huge degree of portability not only between platform driver or OS but also on the API level.

## Binary Gits-Stream

Binary **GITS** streams are collections of files produced by **GITS** recorder. Some of those resources may not be created by **GITS** recorder, depending on feature set of application being recorded.

# Recorder

Recorder operates, by creating a `token` for each recorder function in the thread that invoked that API function. `Token` represents all the information necessary to play back function it represents. Recorder then passes each newly created `token` to a separate thread which is used to persist data to hdd.
Normally all IO is done in separate threads of execution to minimize impact on the recorded application.
# Player

The player's main task is to read in the recorded binary stream and play it back on the current machine. It sets a rendering surface, initializes the API and plays back the commands that have been tokenized. 

# Configuration

Both **Recoder** and **Player** have various options and settings. The best way to set them up is by editing and adjusting the configuration file of **GITS**: `gits_config.yml`. The file is structured into different sections that are further discussed in other parts of the documentation.

As an alternative to the configuration file you can also pass arguments to the executables. As the settings, switches and configuration options are getting more and more over time we _strongly advice you_ to use the configuration file for the setup.

# Subcapture

**GITS** also allows recording streams containing only a subset of API calls made by the application (e.g., only select frames). We call them substreams or subcaptures.

A subcapture can also be understood as a trimmed binary stream. You can e.g. use **GITS** to extract individual frames, drawcalls, ... from a longer binary stream.

# Generator

A lot of component specific logic is generated automatically by Python scripts. Generator files have their respective APIs indicated in their names, with the exception of few OpenGL files (for legacy reasons).

