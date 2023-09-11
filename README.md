# SpikeMonitor
Live neural data viewer for electrophysiological recordings from NeuroNexus' SmartBox Pro.

Currently supports four probes with a maximum of 64 channels per probe. It will run if recording from >64 channels, but only the first 64 channels will be displayed properly.

To use, start recording from Allego, run the executable, then select the growing xdat file from the file dialog.

To build, simply run make. See requirements.txt for a list of dependencies.
