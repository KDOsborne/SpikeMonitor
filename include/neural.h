#ifndef NEURAL_H
#define NEURAL_H

#include <bandpass.h>
#include <bass/bass.h>

#define NUM_PORTS 4
#define DATASIZE 100
#define MAX_SPIKES 100

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
struct Spike {
	float *data;
	float timestamp;
};

struct SpikeLine {
	struct bandpass_struct	bp;
	struct Spike 			spikes[MAX_SPIKES];
	float 					data[DATASIZE],*tracedata;
	unsigned int			currSpike;
	int						enabled;
};

struct probe_info {
	struct SpikeLine* lines;
	float *depth;
	int *sites,*order,nchans,offset;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
int 	init_neural();
int 	read_data(double,float,HSTREAM,char*);
int		ready_file();
void	reopen_file();
void 	close_file();
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals
extern struct probe_info probes[NUM_PORTS];
extern struct SpikeLine* line_;
extern int PROBE, TARGET_CHANNEL, SRATE, SPIKE_WIDTH, TRACESIZE;
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //NEURAL_H