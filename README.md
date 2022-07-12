# Timelines Plugin

This plugin manages save files in a two-dimensional versioning system, tracking the version history of save games.

The plugin does not know how to actually save or load the game state. That must be provided by a SaveSystemInterop class, set in project settings. 
The TimelineSubsystem will make calls into that backend when it needs to.

### Compatibility
TimelinesPlugin is compatible with EasyMultiSave. It was in fact designed originally to be a wrapper around EMS, and later made generic to any save system.
An example implementation to bind Timelines to EMS can be found here: https://drive.google.com/drive/folders/1c4Y1IG0fQLkYUQdKolgX1elESVxqfSOP?usp=sharing
