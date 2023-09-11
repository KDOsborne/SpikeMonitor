# SpikeMonitor
Live neural data viewer for electrophysiological recordings from NeuroNexus' SmartBox Pro.

![spikemonitor](https://github.com/KDOsborne/SpikeMonitor/assets/34141764/30654be4-7a44-40d2-be57-caf109d0ac24)

Currently supports four probes with a maximum of 64 channels per probe. It will run if recording from >64 channels, but only the first 64 channels will be displayed properly.

To use: start recording from Allego, run the executable, then select the created xdat file from the file dialog.

To build, simply run make. See requirements.txt for a list of dependencies.
