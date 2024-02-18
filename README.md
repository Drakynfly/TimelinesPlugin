# Timelines Plugin

This plugin manages save files in a two-dimensional versioning system, tracking the version history of save games.

Timelines Plugin has a dependency on another plugin I wrote: https://github.com/Drakynfly/FaerieSaveData

The plugin does not know how to actually save or load the game state. That must be provided by a SaveSystemInterop class, set in project settings. 
The RestorationSubsystem will make calls into that backend when it needs to.

- Timelines Plugin is provided as is. It is a plugin I am developing for my own projects and that is what drives the features it provides. I made it public because it is fairly simple and generic, and I thought it might be usable by people besides me. It is not guaranteed to match your project's needs though. Feel free to ask on my discord about a feature you are missing, and I'll see what I can do.
- The plugin is fairly well self-documented. No external documentation is provided. Reading source code comments should be sufficient.
- Blueprint support is in-progress, as the system is used via C++ in my projects. The Restoration Subsystem is fully exposed to BP, but the Interop class is not yet.

### Support and Discussion
You can discuss this plugin and get support here: https://discord.gg/AAk9yNwKk8
