#include <shape.h>
#include <video.h>

static void init_line();

static const char *vertexShaderSource = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
    "layout (location = 0) in float value;\n"
	"uniform float vscale;"
	"uniform float zoom;"
	"uniform float t;"
	"uniform vec2 pos;"
	"uniform vec2 trans;"
	"uniform vec2 scale;"
	"uniform vec4 aColor;"
	"out vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(((gl_VertexID/t*2.f-1.f)*scale.x+pos.x)*zoom+trans.x, (value*vscale*scale.y+pos.y)*zoom+trans.y, 0.0, 1.0);\n"
	"	ourColor = aColor;\n"
    "}\0";
	
static const char *lineShaderSource = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
    "layout (location = 0) in vec2 aPos;\n"
	"uniform float zoom;"
	"uniform vec2 pos;"
	"uniform vec2 trans;"
	"uniform vec4 aColor;"
	"out vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4((aPos.x+pos.x)*zoom+trans.x, (aPos.y+pos.y)*zoom+trans.y, 0.0, 1.0);\n"
	"	ourColor = aColor;\n"
    "}\0";

static const char *fragmentShaderSource = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
    "out vec4 FragColor;\n"
	"in vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = ourColor;\n"
    "}\n\0";
	
struct shape_struct* shape_;

int init_shape() {
	shape_ = (struct shape_struct*)malloc(sizeof(struct shape_struct));
	memset(shape_,0,sizeof(struct shape_struct));
	
	shape_->vertShader = create_program(vertexShaderSource,fragmentShaderSource);
	shape_->lineShader = create_program(lineShaderSource,fragmentShaderSource);
	
	if(shape_->vertShader == -1 || shape_->lineShader == -1) {
		return 0;
	}
	
    glGenVertexArrays(1, &shape_->vertVAO);
    glGenBuffers(1, &shape_->vertVBO);
	
	glBindVertexArray(shape_->vertVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, shape_->vertVBO);
	
	glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); 
	
	glUseProgram(shape_->vertShader);
	
	shape_->col_loc = glGetUniformLocation(shape_->vertShader, "aColor");
	shape_->vscale_loc = glGetUniformLocation(shape_->vertShader, "vscale");
	shape_->pos_loc = glGetUniformLocation(shape_->vertShader, "pos");
	shape_->scale_loc = glGetUniformLocation(shape_->vertShader, "scale");
	shape_->zoom_loc = glGetUniformLocation(shape_->vertShader, "zoom");
	shape_->trans_loc = glGetUniformLocation(shape_->vertShader, "trans");
	shape_->t_loc = glGetUniformLocation(shape_->vertShader, "t");
	
	glUseProgram(shape_->lineShader);
	
	shape_->lzoom_loc = glGetUniformLocation(shape_->lineShader, "zoom");
	shape_->ltrans_loc = glGetUniformLocation(shape_->lineShader, "trans");
	shape_->lpos_loc = glGetUniformLocation(shape_->lineShader, "pos");
	shape_->lcol_loc = glGetUniformLocation(shape_->lineShader, "aColor");

	init_line();
	
	return 1;
}

void destroy_shape() {
	glDeleteVertexArrays(1, &shape_->vertVAO);
	glDeleteVertexArrays(1, &shape_->lineVAO);
    glDeleteBuffers(1, &shape_->vertVBO);
	glDeleteBuffers(1, &shape_->lineVBO);
	
	glDeleteProgram(shape_->vertShader);
	glDeleteProgram(shape_->lineShader);
	
	free(shape_);
}

static void init_line() {
	int nvertices = 18*4;
	float vertices[nvertices+4];
	for(int i = 0; i < nvertices/4; i++) {
		if(i < 9) {
			vertices[i*4]=-1.f;
			vertices[i*4+1]=(float)i/8.f*2.f-1.f;
			vertices[i*4+2]=1.f;
			vertices[i*4+3]=(float)i/8.f*2.f-1.f;
		} else {
			vertices[i*4]=(float)(i-9)/8.f*2.f-1.f;
			vertices[i*4+1]=1.f;
			vertices[i*4+2]=(float)(i-9)/8.f*2.f-1.f;
			vertices[i*4+3]=-1.f;
		}
	}
	
	vertices[nvertices] = 0.f;
	vertices[nvertices+1] = 0.f;
	vertices[nvertices+2] = 1/4.f;
	vertices[nvertices+3] = 0.f;
	
	glGenVertexArrays(1, &shape_->lineVAO);
    glGenBuffers(1, &shape_->lineVBO);
	
	glBindVertexArray(shape_->lineVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, shape_->lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
	
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
}


