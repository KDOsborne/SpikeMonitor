#include <video.h>
#include <neural.h>
#include <shape.h>
#include <bass/bass.h>
#include <text.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FPS 30.0
#define SCR_WIDTH 1280
#define SCR_HEIGHT 720

static void scroll_(int);
static void click_(int);
	
HSTREAM 	stream;
char 		TIME_TEXT[32], WINDOW_TEXT[32], LOOK_TEXT[32];
double 		elapsed, xt = 0, yt = 0, xdrag, ydrag;
float 		scale = 0.0125f, zoom = 0.9f, lookback = 1.f, dclick = -1.f;
int 		shift = 0, ctrl = 0, trace = 1, dragging = 0;

static int process_messages() {
	MSG msg;
	int result_ = 0;

	while(PeekMessage(&msg, video_->hWnd, 0, 0, PM_NOREMOVE)) {
		if(GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg); 
			
			switch(msg.message) {
				case WM_KEYDOWN:
					switch (msg.wParam) {
						case VK_F1:
							reopen_file();
							BASS_ChannelPlay(stream,TRUE);
							
						break;
						case VK_UP:
							lookback += 1.0;
							sprintf(LOOK_TEXT,"LOOKBACK: %.0fS",lookback);
						break;
						case VK_DOWN:
							lookback -= 1.0;
							if(lookback < 1.0)
								lookback = 1.0;
							sprintf(LOOK_TEXT,"LOOKBACK: %.0fS",lookback);
						break;
						case VK_ESCAPE:
							if(!dragging && zoom > 1)
							{
								zoom = 0.9;
								xt = 0;
								yt = 0;
								break;
							}
							else if(zoom < 1)
								TARGET_CHANNEL = -1;
						break;
						case VK_F8:
							BASS_ChannelPlay(stream,TRUE);
						break;
						case VK_SPACE:
							PROBE = (PROBE+1) % NUM_PORTS;
							while (probes[PROBE].nchans == 0) {
								PROBE = (PROBE+1) % NUM_PORTS;
							}
							line_ = probes[PROBE].lines;
							
							TARGET_CHANNEL = -1;
						break;
						case VK_TAB:
							trace = !trace;
						break;
						case VK_SHIFT:
							shift = 1;
						break;
						case VK_CONTROL:
							ctrl = 1;
						break;
					}
				break;
				case WM_KEYUP:
					switch (msg.wParam) {
						case VK_SHIFT:
							shift = 0;
						break;
						case VK_CONTROL:
							ctrl = 0;
						break;
					}
				break;
				case WM_LBUTTONDOWN:
					click_(0);
				break;
				case WM_RBUTTONDOWN:
					click_(1);
				break;
				case WM_LBUTTONUP:
					dragging = 0;
				break;
				case WM_MOUSEWHEEL:
					scroll_(GET_WHEEL_DELTA_WPARAM(msg.wParam)>0?1:-1);
				break;
			}
			DispatchMessage(&msg); 
		}
		else
			result_ = -1;
	}
						
	return result_;
}

int main(int argc, char *argv[]) {
	if (!init_neural()) {
		getchar();
		return 1;
	}

	HWND win = NULL;
	if(HIWORD(BASS_GetVersion()) != BASSVERSION) {
		fprintf(stderr,"An incorrect version of BASS.DLL was loaded");
		return 1;
	}
	if(!BASS_Init(-1, 44100, BASS_DEVICE_MONO|BASS_DEVICE_FREQ, win, NULL)) {
		fprintf(stderr,"CANT INITIALIZE AUDIO DEVICE %d\n",BASS_ErrorGetCode());
		return 1;
	}
	
	stream = BASS_StreamCreate(SRATE,1,BASS_SAMPLE_FLOAT,STREAMPROC_PUSH,NULL);
	if(!stream) {
		printf("CANT CREATE AUDIO STREAM %d\n",BASS_ErrorGetCode());
		return 1;
	}
	
	if(!BASS_ChannelStart(stream)) {
		fprintf(stderr,"CAN'T START AUDIO STREAM %d\n",BASS_ErrorGetCode());
		return 1;
	}
	
	sprintf(TIME_TEXT,"TIME: %.3f",0.f);
	sprintf(WINDOW_TEXT,"WINDOW:%.2f",1/scale);
	sprintf(LOOK_TEXT,"LOOKBACK: %.0fS",lookback);
	
	if(!init_video(SCR_WIDTH,SCR_HEIGHT,0,WS_OVERLAPPEDWINDOW,PFD_DOUBLEBUFFER)) {
		return 1;
	}
	if(!init_text()) {
		return 1;
	}
	if(!init_shape()) {
		return 1;
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.f,0.f,0.f,1.f);
	
	LARGE_INTEGER freq,start,end;
	QueryPerformanceFrequency(&freq); 
	QueryPerformanceCounter(&start);
	
	double lastdraw = 0.0;
	
	if (!ready_file())
		return 1;
	
	ShowWindow(video_->hWnd,SW_SHOW);
	
	int return_code = 0;
    while(!return_code) {
		QueryPerformanceCounter(&end);
		
		elapsed = (end.QuadPart-start.QuadPart)*1000/freq.QuadPart;
		
		if(read_data(elapsed, scale, stream, TIME_TEXT) == -1)
			break;
		
		if(elapsed - lastdraw >= 1/FPS*1000) {	
			if(dragging != 0) {
				POINT mousepos;
				double my,nv;
				
				GetCursorPos(&mousepos);
				ScreenToClient(video_->hWnd,&mousepos);
				
				my = (double)mousepos.y;
				
				nv = -(my/video_->h*2.f-1.f)*(1.f/scale)*(16/9.f);
				if(dragging == 1) {
					if(nv > (1.f/scale))
						nv = 1.f/scale;
					else if(nv < 0.f)
						nv = 0.f;
					line_[TARGET_CHANNEL].bp.hthresh = nv;
				} else {
					if(nv < -(1.f/scale))
						nv = -1.f/scale;
					else if(nv > 0.f)
						nv = 0.f;
					line_[TARGET_CHANNEL].bp.lthresh = nv;
				}
			}
			
			glClear(GL_COLOR_BUFFER_BIT);
			
			//Draw Spikes
			glUseProgram(shape_->vertShader);

			glUniform1f(shape_->vscale_loc,scale);
			glUniform1f(shape_->zoom_loc,zoom);
			glUniform2f(shape_->trans_loc,xt,yt);
			glUniform2f(shape_->scale_loc,1/8.f,1/8.f);
			
			glBindVertexArray(shape_->vertVAO);
			glBindBuffer(GL_ARRAY_BUFFER, shape_->vertVBO);
			
			for(int i = 0; i < probes[PROBE].nchans; i++) {
				float r = 1.f, g = 1.f, b = 1.f;
				
				if(!line_[i].enabled)
					continue;
				glUniform2f(shape_->pos_loc,(float)(i%8)/8.f*2.f-1.f+1/8.f,(1.f-((int)(i/8)/8.f))*2.f-1.f-1/8.f);
				if(i == TARGET_CHANNEL) {
					r = 0.0;
					g = 1.0;
				}
				if(trace) {
					glUniform1f(shape_->t_loc,TRACESIZE-1);
					glUniform4f(shape_->col_loc,r,g,b,1.f);
					glBufferData(GL_ARRAY_BUFFER, sizeof(float)*TRACESIZE, line_[i].tracedata, GL_DYNAMIC_DRAW);
					glDrawArrays(GL_LINE_STRIP,0,TRACESIZE);
				} else {
					glUniform1f(shape_->t_loc,SPIKE_WIDTH-1);
					for(int j = 0; j < MAX_SPIKES; j++) {
						if((elapsed - line_[i].spikes[j].timestamp)/ CLOCKS_PER_SEC > lookback)
							continue;
						glUniform4f(shape_->col_loc,r,g,b,1.f-(elapsed-line_[i].spikes[j].timestamp)/CLOCKS_PER_SEC/lookback);
						glBufferData(GL_ARRAY_BUFFER, sizeof(float)*SPIKE_WIDTH, line_[i].spikes[j].data, GL_DYNAMIC_DRAW);
						glDrawArrays(GL_LINE_STRIP,0,SPIKE_WIDTH);
					}
				}
				
				if(i == TARGET_CHANNEL) {
					r = 1.0;
					g = 1.0;
				}
			}
			
			//Draw Lines
			glUseProgram(shape_->lineShader);
			glUniform1f(shape_->lzoom_loc,zoom);
			glUniform2f(shape_->ltrans_loc,xt,yt);
			
			glBindVertexArray(shape_->lineVAO);
			
			for(int i = 0; i < probes[PROBE].nchans; i++) {
				if(!line_[i].enabled)
					continue;
				float x = (float)(i%8)/8.f*2.f-1.f;
				float y = (1.f-((int)(i/8)/8.f))*2.f-1.f-1/8.f;
				
				if(shift || i == TARGET_CHANNEL) {
					glUniform2f(shape_->lpos_loc,x,y+line_[i].bp.hthresh/(1.f/scale)*1/8.f);
					glUniform4f(shape_->lcol_loc,1.f,1.f,0.f,0.55);
					glDrawArrays(GL_LINES,36,2);
				}
				
				if(ctrl || i == TARGET_CHANNEL) {
					glUniform2f(shape_->lpos_loc,x,y+line_[i].bp.lthresh/(1.f/scale)*1/8.f);
					glUniform4f(shape_->lcol_loc,1.f,0.f,1.f,0.55);
					glDrawArrays(GL_LINES,36,2);
				}
				
				glUniform2f(shape_->lpos_loc,0,0);
			}
			
			glUniform4f(shape_->lcol_loc,(PROBE==0||PROBE==3?0.65:0.f),
											(PROBE==2?1.f:0.f), 
												(PROBE==1||PROBE==3?1.f:0.f), 1.f);
			glDrawArrays(GL_LINES,0,18*2);
			
			//Draw Text
			glUseProgram(text_->shader);
			for(int i = 0; i < probes[PROBE].nchans; i++)
			{
				float x = (float)(i%8)/8.f*2.f-1.f;
				float y = (1.f-((int)(i/8)/8.f))*2.f-1.01f;
				char buf[8];
				sprintf(buf,"%c%d",'A'+PROBE,probes[PROBE].sites[i]);
				
				if(!line_[i].enabled) {
					glUniform4f(text_->colu_loc,0.5f,0.5f,0.5f,1.f);
					render_simpletext(buf,x*zoom+xt,y*zoom+yt,2.25*zoom,TXT_TOPALIGNED);
					glUniform4f(text_->colu_loc,1.f,1.f,1.f,1.f);
					continue;
				}
				else if(i == TARGET_CHANNEL) {
					glUniform4f(text_->colu_loc,0.f,1.f,1.f,1.f);
					render_simpletext(buf,x*zoom+xt,y*zoom+yt,2.75*zoom,TXT_TOPALIGNED);
					
					sprintf(buf,"%u",line_[i].currSpike);
					render_simpletext(buf,x*zoom+xt,y*zoom+yt-0.23*zoom,1.75*zoom,TXT_BOTALIGNED);
					
					glUniform4f(text_->colu_loc,1.f,0.f,1.f,1.f);
					sprintf(buf,"%.2f",line_[i].bp.lthresh);
					render_simpletext(buf,(x+0.245)*zoom+xt,y*zoom+yt-0.23*zoom,1.75*zoom,TXT_BOTALIGNED|TXT_RGHTALIGNED);
					
					glUniform4f(text_->colu_loc,1.f,1.f,0.f,1.f);
					sprintf(buf,"%.2f",line_[i].bp.hthresh);
					render_simpletext(buf,(x+0.245)*zoom+xt,y*zoom+yt+1/8*zoom,1.75*zoom,TXT_TOPALIGNED|TXT_RGHTALIGNED);
					
					glUniform4f(text_->colu_loc,1.f,1.f,1.f,1.f);
					continue;
				}
				
				
				render_simpletext(buf,x*zoom+xt,y*zoom+yt,2.25*zoom,TXT_TOPALIGNED);
				
				sprintf(buf,"%d",line_[i].currSpike);
				render_simpletext(buf,x*zoom+xt,y*zoom+yt-0.23*zoom,1.75*zoom,TXT_BOTALIGNED);
				
				if(shift)
					glUniform4f(text_->colu_loc,1.f,1.f,0.f,1.f);
				sprintf(buf,"%.2f",line_[i].bp.hthresh);
				render_simpletext(buf,(x+0.245)*zoom+xt,y*zoom+yt+1/8*zoom,1.75*zoom,TXT_TOPALIGNED|TXT_RGHTALIGNED);
				
				if(ctrl)
					glUniform4f(text_->colu_loc,1.f,0.f,1.f,1.f);
				else
					glUniform4f(text_->colu_loc,1.f,1.f,1.f,1.f);
				sprintf(buf,"%.2f",line_[i].bp.lthresh);
				render_simpletext(buf,(x+0.245)*zoom+xt,y*zoom+yt-0.23*zoom,1.75*zoom,TXT_BOTALIGNED|TXT_RGHTALIGNED);
				
				glUniform4f(text_->colu_loc,1.f,1.f,1.f,1.f);
			}
			
			render_simpletext(TIME_TEXT,-1.f,-0.99f,2.5,TXT_BOTALIGNED);
			render_simpletext(WINDOW_TEXT,1.f,0.99f,2.5,TXT_TOPALIGNED|TXT_RGHTALIGNED);
			render_simpletext(LOOK_TEXT,1.f,-0.99f,2.5,TXT_BOTALIGNED|TXT_RGHTALIGNED);
			
			if(trace)
				render_simpletext("TRACE MODE",-1.f,0.99f,2.5,TXT_TOPALIGNED);
			else
				render_simpletext("SPIKE MODE",-1.f,0.99f,2.5,TXT_TOPALIGNED);
			
			SwapBuffers(video_->hDC);
			
			lastdraw = elapsed;
		}
		
		return_code = process_messages();
    }
	
	close_file();
	
	destroy_shape();
	destroy_text();
	destroy_video();
	
	BASS_Free();
	
    return 0;
}

static void scroll_(int delta) {
	if(zoom < 1) {
		if(shift) {
			for(int i = 0; i < probes[PROBE].nchans; i++) {
				line_[i].bp.hthresh += delta;
				if(line_[i].bp.hthresh > 1/scale)
					line_[i].bp.hthresh = 1/scale;
				else if(line_[i].bp.hthresh < 0)
					line_[i].bp.hthresh = 0;
			}
		}
		if(ctrl) {
			for(int i = 0; i < probes[PROBE].nchans; i++) {
				line_[i].bp.lthresh -= delta;
				if(line_[i].bp.lthresh < -1/scale)
					line_[i].bp.lthresh = -1/scale;
				else if(line_[i].bp.lthresh > 0)
					line_[i].bp.lthresh = 0;
			}
		}
	}
	if((!shift && !ctrl) || (zoom > 1)) {
		scale -= 0.0005*delta;
		if(scale < 0.001)
			scale = 0.001;
		else if(scale > 1)
			scale = 1;
	}
	
	sprintf(WINDOW_TEXT,"WINDOW:%.2f",1/scale);
}

static void click_(int button) {
	POINT mousepos;
	double mx,my;
	int half = 0;

	GetCursorPos(&mousepos);
	ScreenToClient(video_->hWnd,&mousepos);

	mx = (double)mousepos.x;
	my = (double)mousepos.y;
	
	if(my < video_->h/2)
		half = 1;
	else
		half = -1;
	
	mx = (mx-(xt+1.f-zoom)/2*video_->w)/(video_->w*zoom);
	my = (my-(1.f-zoom-yt)/2*video_->h)/(video_->h*zoom);
	
	int row = mx*8.0;
	int column = my*8.0;
	
	switch(button)
	{
		case 0:
			if(zoom > 1) {
				dragging = half;
			} else {
				if(mx >= 0 && my >= 0 && row >= 0 && row <= 7 && column >= 0 && column <= 7) {
					if(line_[row+column*8].enabled) {
						if(zoom < 1) {
							if(elapsed/1000-dclick < 1/3.f && (TARGET_CHANNEL == row+column*8)) {
								zoom = 4.5;
								xt = (4.f-row)/4.f*zoom-1/8.f*zoom;
								yt = ((float)column-4.f)/4.f*zoom+1/8.f*zoom;
							}
							else
								dclick = elapsed/1000;
						}
						TARGET_CHANNEL = row+column*8;
					}
				}
				else {
					TARGET_CHANNEL = -1;
				}
			}
		break;
		case 1:
			if(dragging)
				break;
			if(zoom > 1) {
				zoom = 0.9;
				xt = 0;
				yt = 0;
				break;
			} else {
				if(mx >= 0 && my >= 0 && row >= 0 && row <= 7 && column >= 0 && column <= 7) {
					if(line_[row+column*8].enabled) {
						line_[row+column*8].enabled = 0;
						if(row+column*8 == TARGET_CHANNEL)
							TARGET_CHANNEL = -1;
					}
					else
						line_[row+column*8-1].enabled = 1;
				}
			}
		break;
	}
}
