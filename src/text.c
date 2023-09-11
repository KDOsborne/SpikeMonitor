#include <text.h>
#include <video.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

text_struct* text_;

const int binary_data[] = { 0, 4329476, 10813440, 11512810,
	0, 0, 0, 4325376,
	17047824, 1116225, 0, 4357252,
	4, 31744, 4, 17043521,
	15521390, 4395140, 16267327, 15249966,
	18415120, 32554511, 31505966, 33038468,
	15252014, 15268367, 131076, 131076,
	17043728, 1016800, 1118273, 15249412,
	0,
	15269425, 16301615, 15238702, 16303663,
	32554047, 32554017, 31520302, 18415153,
	32641183, 25708078, 18128177, 1082431,
	18732593, 18470705, 15255086, 16301089,
	15254838, 16301617, 31504911, 32641156,
	18400814, 18400580, 18405233, 18157905,
	18157700, 32772191, 25436440, 1118480,
	3213379, 4521984, 31, 33554431
};

static const char *textShaderSource = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
	"layout (location = 0) in vec2 aPos;\n"
	"uniform float xOffset;"
	"uniform float yOffset;"
	"uniform float xTravel;"
	"uniform float yTravel;"
	"uniform float scale;"
	"uniform vec2 viewPort;"
	"uniform vec4 aColor;"
	"out vec4 ourColor;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = vec4(((aPos.x+xTravel)*scale)*(viewPort.y/viewPort.x)+xOffset,(aPos.y+yTravel)*scale+yOffset,0.0,1.0);\n"
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

int init_text()
{
	text_ = (text_struct*)malloc(sizeof(text_struct));
	memset(text_, 0, sizeof(text_struct));
	
	int table_size = sizeof(binary_data)/sizeof(int);
	
	text_->shader = create_program(textShaderSource, fragmentShaderSource);
	if(text_->shader == -1)
	{
		free(text_);
		return 0;
	}
	
	glUseProgram(text_->shader);
	
	text_->xu_loc = glGetUniformLocation(text_->shader,"xOffset");
	text_->yu_loc = glGetUniformLocation(text_->shader,"yOffset");
	text_->xtu_loc = glGetUniformLocation(text_->shader,"xTravel");
	text_->ytu_loc = glGetUniformLocation(text_->shader,"yTravel");
	text_->fsu_loc = glGetUniformLocation(text_->shader,"scale");
	text_->colu_loc = glGetUniformLocation(text_->shader,"aColor");
	
	glUniform4f(text_->colu_loc,1.f,1.f,1.f,1.f);
	glUniform2f(glGetUniformLocation(text_->shader,"viewPort"),video_->w,video_->h);
	
	text_->chars = (struct character*)malloc(sizeof(struct character)*table_size);

	float vertices[72];
	for(int i = 0; i < 6; i++)
	{
		for(int j = 0; j < 6; j++)
		{
			vertices[i*12+j*2] = ((float)j-2.5)*(TXT_SIZE/5.f);
			vertices[i*12+j*2+1] =  ((float)i-2.5)*(TXT_SIZE/5.f);
		}
	}
	
	glGenVertexArrays(1, &text_->VAO);
    glGenBuffers(1, &text_->VBO);
	
	glBindVertexArray(text_->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, text_->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	
	for(int i = 0; i < table_size; i++)
	{
		unsigned int *indices = NULL;
		text_->chars[i].num_indices = 0;
		switch (i + ' ')
		{
			case ',':
			case '.':
			case '\'':
			case ':':
			case ';':
			case '!':
				text_->chars[i].width = TXT_SIZE/5.f;
				break;
			default:
				text_->chars[i].width = TXT_SIZE;
		}
		
		glGenBuffers(1, &text_->chars[i].EBO);
		
		for(int j = 0; j < 5; j++)
		{
			for(int k = 0; k < 5; k++)
			{
				if(((binary_data[i] >> (j*5+k)) & 1) == 1)
				{	
					if(indices == NULL)
						indices = (unsigned int*)malloc(sizeof(unsigned int)*6);
					else
						indices =  (unsigned int*)realloc(indices, sizeof(unsigned int)*text_->chars[i].num_indices*6+sizeof(unsigned int)*6);
					
					indices[text_->chars[i].num_indices*6] 	 = 6*(j+1)+k;
					indices[text_->chars[i].num_indices*6+1] = 6*j+k;
					indices[text_->chars[i].num_indices*6+2] = 6*(j+1)+k+1;
					indices[text_->chars[i].num_indices*6+3] = 6*j+k;
					indices[text_->chars[i].num_indices*6+4] = 6*(j+1)+k+1;
					indices[text_->chars[i].num_indices*6+5] = 6*j+k+1;
					
					text_->chars[i].num_indices++;
				}
			}

		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_->chars[i].EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, text_->chars[i].num_indices*sizeof(unsigned int)*6, indices, GL_STATIC_DRAW);
		
		free(indices);
	}
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
	
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return 1;
}

void update_textvp()
{
	glUseProgram(text_->shader);
	glUniform2f(glGetUniformLocation(text_->shader,"viewPort"),video_->w,video_->h);
}

void render_simpletext(char *str, float x, float y, float fs, int flags)
{
	glUniform1f(text_->xu_loc,x);
	glUniform1f(text_->yu_loc,y);
	
	if(flags & TXT_TOPALIGNED)
		glUniform1f(text_->ytu_loc,-TXT_SIZE/2);
	else if(flags & TXT_BOTALIGNED)
		glUniform1f(text_->ytu_loc,TXT_SIZE/2);
	else
		glUniform1f(text_->ytu_loc,0);
	
	glUniform1f(text_->fsu_loc,fs);
	
	glBindVertexArray(text_->VAO);
	
	float total_width = 0.f, x_ = 0, y_ = 0;
	float first = 1;
	if(flags & TXT_TOPALIGNED)
		y_ = -TXT_SIZE/2;
	else if(flags & TXT_BOTALIGNED)
		y_ = TXT_SIZE/2;
	
	for(int i = 0; i < strlen(str); i++)
	{
		if(total_width == 0.f)
		{
			total_width -= (TXT_BEARING);
			for(int j = i; j < strlen(str); j++)
			{
				if(str[j] == '\n' || str[j] == '\r')
					break;
				else if(str[j] == ' ')
				{
					total_width += (TXT_BEARING);
					continue;
				}
				else if(str[j] > '`' || str[j] < ' ')
					str[j] = '`';
				
				total_width += (text_->chars[str[j]-' '].width + TXT_BEARING);
			}
			
			if(flags & TXT_CENTERED)
				x_ -= total_width/2-text_->chars[i].width/2;
			else if(flags & TXT_RGHTALIGNED)
				x_ -= total_width-TXT_BEARING;
			else
				x_ += (text_->chars[0].width/2+TXT_BEARING);
			
			first = 1;
		}
		if(str[i] == '\n')
		{
			y_ -= (TXT_SIZE*1.5);
			x_ = 0;
			total_width = 0.f;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(str[i] == '\r')
		{
			y_ += (TXT_SIZE*1.5);
			x_ = 0;
			total_width = 0.f;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(str[i] == ' ')
		{
			x_ += (TXT_BEARING);
			continue;
		}
		
		if(!first)
			x_ += (text_->chars[str[i]-' '].width/2);
		first = 0;
		
		glUniform1f(text_->xtu_loc, x_);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_->chars[str[i]-' '].EBO);
		glDrawElements(GL_TRIANGLES, text_->chars[str[i]-' '].num_indices*6, GL_UNSIGNED_INT, 0);
	
		x_ += (text_->chars[str[i]-' '].width/2 + TXT_BEARING);
	}
}

void destroy_text()
{
	glDeleteVertexArrays(1, &text_->VAO);
	glDeleteBuffers(1, &text_->VBO);
	
	for(int i = 0; i < sizeof(binary_data)/sizeof(int); i++)
		glDeleteBuffers(1, &text_->chars[i].EBO);
	
	glDeleteProgram(text_->shader);
	
	free(text_->chars);
	free(text_);
	text_ = NULL;
}