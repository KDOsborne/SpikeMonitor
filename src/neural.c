#include <neural.h>

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int selectfile();
static int openfile();
static int seekfile();
static void parse_json();
static char* copy_string(char*,int);

LARGE_INTEGER 		FILE_SIZE;
HANDLE 				hFile;
struct 				probe_info probes[NUM_PORTS];
struct 				SpikeLine* line_;
float 				*databuffer, *audiobuffer;
char 				DATA_FILE[MAX_PATH], JSON_FILE[MAX_PATH];
int 				PROBE, SRATE, TARGET_CHANNEL, SPIKE_WIDTH, TRACESIZE, STRIDE, DCOUNT, DTRACE;

int init_neural() {
	PROBE = 0;
	DCOUNT = 0;
	DTRACE = 0;
	STRIDE = 0;
	SRATE = 0;
	TARGET_CHANNEL = -1;
	SPIKE_WIDTH = 0;
	TRACESIZE = 0;
	
	if(selectfile() == -1)
		return 0;
	
	memset(probes,0,sizeof(struct probe_info)*NUM_PORTS);
	
	parse_json();
	
	if (SRATE == 0)
		return 0;
	
	for(int i = 0; i < NUM_PORTS; i++) {
		for(int j = 0; j < probes[i].nchans; j++) {
			init_bandpass(SRATE,SRATE/10.f,SRATE/100.f,&probes[i].lines[j].bp);
			probes[i].lines[j].enabled = 1;
		}
	}
	
	while (probes[PROBE].nchans == 0) {
		PROBE = (PROBE+1) % NUM_PORTS;
	}
	
	line_ = probes[PROBE].lines;
	
	databuffer = (float*)malloc(sizeof(float)*STRIDE);
	audiobuffer = (float*)malloc(sizeof(float)*DATASIZE);
	
	return 1;
}

int read_data(double time, float scale, HSTREAM stream, char *text) {
	int64_t t = (int64_t)round(time)*(SRATE/1000.f)*STRIDE;
	
	if(!GetFileSizeEx(hFile, &FILE_SIZE)) {
		fprintf(stderr,"CAN'T GET FILE SIZE: %ld",GetLastError());
		return -1;
	}
	
	while(FILE_SIZE.QuadPart % (STRIDE*sizeof(float)) != 0)
		FILE_SIZE.QuadPart--;
	
	LARGE_INTEGER filepos, li;
	li.QuadPart = 0;

	if(!SetFilePointerEx(hFile,li,&filepos,FILE_CURRENT)) {
		fprintf(stderr,"CAN'T GET FILE POSITION: %ld",GetLastError());
		return -1;
	}
	
	sprintf(text,"TIME: %.3f",(double)filepos.QuadPart/SRATE/STRIDE);
	
	DWORD bytesRead;
	while(FILE_SIZE.QuadPart-filepos.QuadPart > 0) {
		while(ReadFile(hFile,databuffer,sizeof(float)*STRIDE,&bytesRead,NULL) && DCOUNT < DATASIZE) {
			if(bytesRead != STRIDE*sizeof(float))
				return 0;
			for(int i = 0; i < probes[PROBE].nchans; i++)
				line_[i].data[DCOUNT] = databuffer[i+probes[PROBE].offset];
			filepos.QuadPart += bytesRead;
			DCOUNT++;
		}
		
		if(bytesRead != STRIDE*sizeof(float))
			return 0;
		
		DCOUNT = 0;
		int s = -1;
		for(int i = 0; i < probes[PROBE].nchans; i++) {
			if(!line_[i].enabled)
				continue;
			
			if(i == TARGET_CHANNEL) {
				s = bandpass(line_[i].data,audiobuffer,1/scale,DATASIZE,&line_[i].bp);
				if (BASS_StreamPutData(stream,audiobuffer,DATASIZE*sizeof(float)) == -1) {
					fprintf(stderr,"CAN'T PUT AUDIO %d\r",BASS_ErrorGetCode());
				}
			} else {
				s = bandpass(line_[i].data,NULL,1/scale,DATASIZE,&line_[i].bp);
			}

			memcpy(line_[i].tracedata+DTRACE,line_[i].data,sizeof(float)*DATASIZE);
			if(s != 0) {
				if(s < 20) {
					if(DTRACE == 0)
						continue;
					else
						memcpy(line_[i].spikes[line_[i].currSpike%MAX_SPIKES].data,line_[i].tracedata+(DTRACE-(20-s)),sizeof(float)*SPIKE_WIDTH);
				}
				else
					memcpy(line_[i].spikes[line_[i].currSpike%MAX_SPIKES].data,line_[i].data+(s-20),sizeof(float)*SPIKE_WIDTH);
				
				line_[i].spikes[line_[i].currSpike%MAX_SPIKES].timestamp = time;
				line_[i].currSpike++;
			}
		}
		
		DTRACE += DATASIZE;
		if(DTRACE >= TRACESIZE)
			DTRACE = 0;
	}
	
	return 0;
}

int	ready_file() {
	if(openfile() == -1)
		return 0;
	if(seekfile() == -1)
		return 0;
	
	return 1;
}

void reopen_file() {
	if (selectfile() != -1) {
		CloseHandle(hFile);
		openfile();
	}
	seekfile();
}

void close_file() {
	CloseHandle(hFile);
}

static int selectfile() {
	OPENFILENAME ofn;
	
	memset(DATA_FILE,0,sizeof(DATA_FILE));

	memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = 0;
    ofn.lpstrFile = DATA_FILE;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(DATA_FILE);
    ofn.lpstrFilter = "XDAT\0*.xdat\0";
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
 
    if(!GetOpenFileName(&ofn))
		return -1;
	
	memcpy(JSON_FILE,DATA_FILE,sizeof(DATA_FILE));
	strrchr(JSON_FILE,'_')[0] = '\0';
	strcat(JSON_FILE,".xdat.json");
	
	return 0;
}

static int openfile() {
	hFile = CreateFile(DATA_FILE, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
	  fprintf(stderr,"CAN'T OPEN DATA FILE: %ld",GetLastError());
	  return -1;
	}
	
	return 0;
}

static int seekfile() {
	if(!GetFileSizeEx(hFile, &FILE_SIZE)) {
	  fprintf(stderr,"CAN'T GET FILE SIZE: %ld",GetLastError());
	  CloseHandle(hFile);
	  return -1;
	}
	
	while(FILE_SIZE.QuadPart % (STRIDE*sizeof(float)) != 0)
		FILE_SIZE.QuadPart--;

	if(!SetFilePointerEx(hFile,FILE_SIZE,NULL,FILE_BEGIN)) {
		fprintf(stderr,"CAN'T SET FILE POSITION: %ld",GetLastError());
		CloseHandle(hFile);
		return -1;
	}
	
	return 0;
}

static void parse_json() {
	FILE* fp;
	char *filetext, *index, *temp, *i1, *i2, *tok;
	int len;
	
	fp = fopen(JSON_FILE,"r");
	if(!fp) {
		fprintf(stderr,"UNABLE TO OPEN JSON FILE\n");
		return;
	}
	
	//Get file length
	fseek(fp,0,SEEK_END);
	len = ftell(fp);
	rewind(fp);
	
	//Allocate memory and read file
	filetext = (char*)malloc(len);
	fread(filetext,1,len,fp);
	fclose(fp);
	
	//Locate sample rate information
	index = strstr(filetext,"\"samp_freq\":");
	if (index == NULL)
		goto cleanup;
	
	i1 = strstr(index,":");
	i2 = strstr(index,",");
	
	if (i1 == NULL || i2 == NULL)
		goto cleanup;
	
	temp = copy_string(i1+1,i2-i1);
	if(!temp)
		goto cleanup;
	
	SRATE = atoi(temp);
	SPIKE_WIDTH = SRATE/500;
	TRACESIZE = SRATE/2;
	
	free(temp);
	
	//Locate channel information
	index = strstr(filetext,"\"signals\":");
	if (index == NULL)
		goto cleanup;
	
	i1 = strstr(index,"{");
	i2 = strstr(index,"}");
	
	if (i1 == NULL || i2 == NULL)
		goto cleanup;
	
	temp = copy_string(i1,i2-i1+1);
	if(!temp)
		goto cleanup;
	
	int element = -1;
	
	tok = strtok(temp,":,");
	while(tok != NULL) {
		if (element != -1) {
			switch (element) {
				case 0:
					STRIDE = atoi(tok);
				break;
			}
			element = -1;
		} else {
			if (strstr(tok,"total"))
				element = 0;
			else 
				element = -1;
		}
		
		tok = strtok(NULL,":,");
	}
	
	free(temp);
	
	//Locate port information
	index = strstr(filetext,"\"port\":");
	if (index == NULL)
		goto cleanup;
	
	i1 = strstr(index,"[");
	i2 = strstr(index,"]");
	
	if (i1 == NULL || i2 == NULL)
		goto cleanup;
	
	temp = copy_string(i1+1,i2-i1);
	if(!temp)
		goto cleanup;
	
	tok = strtok(temp,",");
	while(tok != NULL) {
		if (strstr(tok,"GPIO")) {
			//Skip
		} else { 
			if (tok[1]-'A' >= 0 && tok[1]-'A' <= NUM_PORTS) {
				probes[tok[1]-'A'].nchans++;
			}
		}
		tok = strtok(NULL,",");
	}
	
	free(temp);
	
	//Locate site information
	index = strstr(filetext,"\"site_num\":");
	if (index == NULL)
		goto cleanup;
	
	i1 = strstr(index,"[");
	i2 = strstr(index,"]");
	
	if (i1 == NULL || i2 == NULL)
		goto cleanup;
	
	temp = copy_string(i1+1,i2-i1);
	if(!temp)
		goto cleanup;
	
	for(int i = 0; i < NUM_PORTS; i++) {
		if(probes[i].nchans > 0) {
			probes[i].lines = (struct SpikeLine*)malloc(sizeof(struct SpikeLine)*probes[i].nchans);
			probes[i].depth = (float*)malloc(sizeof(float)*probes[i].nchans);
			probes[i].sites = (int*)malloc(sizeof(int)*probes[i].nchans);
			probes[i].order = (int*)malloc(sizeof(int)*probes[i].nchans);
			
			memset(probes[i].lines,0,sizeof(struct SpikeLine)*probes[i].nchans);
			memset(probes[i].depth,0,sizeof(float)*probes[i].nchans);
			memset(probes[i].sites,0,sizeof(int)*probes[i].nchans);
			memset(probes[i].order,0,sizeof(int)*probes[i].nchans);
			
			for (int j = 0; j < probes[i].nchans; j++) {
				probes[i].lines[j].tracedata = (float*)malloc(sizeof(float)*TRACESIZE);
				memset(probes[i].lines[j].tracedata,0,sizeof(float)*TRACESIZE);
				
				for (int k = 0; k < MAX_SPIKES; k++) {
					probes[i].lines[j].spikes[k].data = (float*)malloc(sizeof(float)*SPIKE_WIDTH);
					memset(probes[i].lines[j].spikes[k].data,0,sizeof(float)*SPIKE_WIDTH);
				}
			}
		}
		if (i > 0)
			probes[i].offset = probes[i-1].offset+probes[i-1].nchans;
	}
	
	tok = strtok(temp,",");
	for (int i = 0; i < NUM_PORTS; i++) {
		for (int j = 0; j < probes[i].nchans; j++) {
			if (tok != NULL) {
				probes[i].sites[j] = atoi(tok);
				tok = strtok(NULL,",");
			}
		}
	}
	
	free(temp);
	
	//Locate depth information
	index = strstr(filetext,"\"site_ctr_tcs_y\":");
	if (index == NULL)
		goto cleanup;
	
	i1 = strstr(index,"[");
	i2 = strstr(index,"]");
	
	if (i1 == NULL || i2 == NULL)
		goto cleanup;
	
	temp = copy_string(i1+1,i2-i1);
	if(!temp)
		goto cleanup;
	
	tok = strtok(temp,",");
	for (int i = 0; i < NUM_PORTS; i++) {
		for (int j = 0; j < probes[i].nchans; j++) {
			if (tok != NULL) {
				probes[i].depth[j] = atof(tok);
				tok = strtok(NULL,",");
			}
		}
	}
	
	free(temp);
	
cleanup:
	free(filetext);
}

static char* copy_string(char* str, int len) {
	char* cpy = (char*)malloc(len);
	if (!cpy) {
		fprintf(stderr,"FAILED TO MALLOC STRING IN copy_string()\n");
		return NULL;
	}
	
	memset(cpy,0,len);
	strncpy(cpy,str,len-1);
	
	return cpy;
}
