#include <windows.h>

#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
using namespace std;
#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"
#include "imgui_impl_glut.h"
#include "VideoMux.h"
#include "include/trackball.h"
#include "NoiseGen.h"

//names of the shader files to load
static const std::string vertex_shader("CloudGeneration_vs.glsl");
static const std::string fragment_shader("CloudGeneration_fs.glsl");

GLuint shader_program = -1;
GLuint texture_id = -1; //Texture map for fish

GLuint quad_vao = -1;
GLuint quad_vbo = -1;

GLuint quad_texture = -1;  // Texture rendered into
GLuint cloud_shape = -1;  // Texture rendered into
GLuint cloud_detail = -1;  // Texture rendered into

////names of the mesh and texture files to load
//static const std::string mesh_name = "Amago0.obj";
//static const std::string texture_name = "AmagoT.bmp";

//some trackball variables -> used in mouse feedback
TrackBallC trackball;
bool mouseLeft, mouseMid, mouseRight;

const float  PI = 3.141592f;

//MeshData mesh_data;
float time_sec = 0.0f;
float viewAngle = -PI;
float height = -2.0f;
bool recording = false;

// slider
float _shape[4] = { 0.154, 0.641,0.205,0.171 };
float _detail[3] = { 0.103, 0.128,0.462 };

// ray marching box info
float _boxScale[3] = {2.0,0.5,2.0};
float _boxPos[3] = {0.0,0.6,0.0};

float _offset[3] = {0.0,0.0,0.0};

// funcs
void reload_shader();
void reset_scene();
void ReloadNoiseTexture();

//Draw the user interface using ImGui
void draw_gui()
{
   ImGui_ImplGlut_NewFrame();
   ImGui::Begin("Cloud Parameters");
   ImGui::SliderFloat("Cam height", &height, -2.0f, 2.0f);
   //ImGui::SliderFloat("View angle", &viewAngle, -PI, +PI);
   ImGui::SliderFloat3("Box Scale", _boxScale, 0.1f, 2.0f);
   ImGui::SliderFloat3("Box Pos", _boxPos, -1.0f, 1.0f);
   ImGui::Text("Offset of the 4D sampling.");
   ImGui::SliderFloat3("Offset", _offset, -1.0f, 1.0f);
   ImGui::Text("Give the basic shape of the cloud.");
   ImGui::SliderFloat4("Cloud Shape", _shape, 0.0f, 1.0f);
   ImGui::Text("Please do NOT set all detail parameter to max value.");
   ImGui::SliderFloat3("Cloud Detail", _detail, 0.0f, 1.0f);

   ImGui::Text("Click the button below to get prefer parameters.");
   if (ImGui::Button("Reset Scene"))
   {
	   reset_scene();
   }
   ImGui::End();
   /*if (ImGui::Button("Reload Shaders"))
   {
	   reload_shader();
   }*/
   ImGui::Render();
 }

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
	//clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shader_program);

	const int w = glutGet(GLUT_WINDOW_WIDTH);
	const int h = glutGet(GLUT_WINDOW_HEIGHT);
	const float aspect_ratio = float(w) / float(h);

	//Make the viewport match the screeb texture size.
	int tex_w, tex_h;
	glBindTexture(GL_TEXTURE_2D, quad_texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_h);

	//Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	trackball.Set3DViewCamera();

	const float radius = 1.0f;
	float camX = sin(viewAngle) * radius;
	float camZ = cos(viewAngle) * radius;
	// lookAt eye(right), center, up
	glm::mat4 V = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0, -height, 0), glm::vec3(0.0, 1.0, 0.0));;
	

	glm::mat4 P = glm::perspective(45.0f, aspect_ratio, 0.1f, 100.0f);

	const int size_loc = 1;	// "windowSize"
	glUniform2f(size_loc, w, h);

	const int V_loc = 2;	//"_CameraToWorld"
	glUniformMatrix4fv(V_loc, 1, false, glm::value_ptr(V));

	const int IP_loc = 3;	//"_CameraInverseProjection"
	glm::mat4 IP = glm::inverse(P);
	glUniformMatrix4fv(IP_loc, 1, false, glm::value_ptr(IP));

	const int box_scale_loc = 4;	// "_boxScale"
	glUniform3f(box_scale_loc, _boxScale[0], _boxScale[1], _boxScale[2]);

	const int box_pos_loc = 5;	// "_boxScale"
	glUniform3f(box_pos_loc, _boxPos[0], _boxPos[1], _boxPos[2]);

	const int shape_loc = 6;	// "_shape"
	glUniform4f(shape_loc, _shape[0], _shape[1], _shape[2], _shape[3]);

	const int detail_loc = 9;	// "_detail"
	glUniform3f(detail_loc, _detail[0], _detail[1], _detail[2]);

	const int offset_loc = 10;	// "_offset"
	glUniform3f(offset_loc, _offset[0], _offset[1], _offset[2]);

	glBindVertexArray(quad_vao);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
         
	draw_gui();

	if (recording == true)
	{
		glFinish();

		glReadBuffer(GL_BACK);
		read_frame_to_encode(&rgb, &pixels, w, h);
		encode_frame(rgb);
	}

	glutSwapBuffers();
}

// glut idle callback.
//This function gets called between frames
void idle()
{
	glutPostRedisplay();

	const int time_ms = glutGet(GLUT_ELAPSED_TIME);
	time_sec = 0.001f*time_ms;

	glUseProgram(shader_program);
	const int time_loc = 0;
	glUniform1f(time_loc, time_sec); // we bound our texture to texture unit 0
}
void reset_scene()
{
	viewAngle = -PI;
	height = -2.0f;
	_boxScale[0] = 2.0f;
	_boxScale[1] = 0.5f;
	_boxScale[2] = 2.0f;
	_boxPos[0] = 0.0f;
	_boxPos[1] = 0.6f;
	_boxPos[2] = 0.0f;
	_offset[0] = 0.0f;
	_offset[1] = 0.0f;
	_offset[2] = 0.0f;
	_shape[0] = 0.154f;
	_shape[1] = 0.641f;
	_shape[2] = 0.205f;
	_shape[3] = 0.171f;
	_detail[0] = 0.103f;
	_detail[1] = 0.128f;
	_detail[2] = 0.462f;

}

void reload_shader()
{
	GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

	if(new_shader == -1) // loading failed
	{
		 glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	 }
	else
	{
		glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

		if(shader_program != -1)
		{
			glDeleteProgram(shader_program);
		}
		shader_program = new_shader;

	}

	// send noise texture again
	ReloadNoiseTexture();
}

// Display info about the OpenGL implementation provided by the graphics driver.
void printGlInfo()
{
	std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
	std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
	std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}

void initOpenGl()
{
	//Initialize glew so that new OpenGL function names can be used
	glewInit();

	glEnable(GL_DEPTH_TEST);

	reload_shader();

	//Load a mesh and a texture
	//mesh_data = LoadMesh(mesh_name); //Helper function: Uses Open Asset Import library.
	//texture_id = LoadTexture(texture_name.c_str()); //Helper function: Uses FreeImage library

	// Generate noise texture in compute shader
	// do something...£¿

	// Generate quad
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);
	float vertices[] = { 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f };
	//create vertex buffers for vertex coords
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	int pos_loc = glGetAttribLocation(shader_program, "pos_attrib");
	if (pos_loc >= 0)
	{
		glEnableVertexAttribArray(pos_loc);
		glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
	}

	const int w = glutGet(GLUT_SCREEN_WIDTH);
	const int h = glutGet(GLUT_SCREEN_HEIGHT);

	// Create screen texture
	glGenTextures(1, &quad_texture);
	glBindTexture(GL_TEXTURE_2D, quad_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// glut callbacks need to send keyboard and mouse events to imgui
void keyboard(unsigned char key, int x, int y)
{
	ImGui_ImplGlut_KeyCallback(key);
	//std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

	switch(key)
	{
		case 'r':
		case 'R':
			reload_shader();     
		break;
	}
}

void keyboard_up(unsigned char key, int x, int y)
{
	ImGui_ImplGlut_KeyUpCallback(key);
}

void special_up(int key, int x, int y)
{
	ImGui_ImplGlut_SpecialUpCallback(key);
}

void passive(int x, int y)
{
	ImGui_ImplGlut_PassiveMouseMotionCallback(x,y);
}

void special(int key, int x, int y)
{
	ImGui_ImplGlut_SpecialCallback(key);
}

void motion(int x, int y)
{
	ImGui_ImplGlut_MouseMotionCallback(x, y);
	if (mouseLeft)  trackball.Rotate(x, y);
	if (mouseMid)   trackball.Translate(x, y);
	if (mouseRight) trackball.Zoom(x, y);
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
	ImGui_ImplGlut_MouseButtonCallback(button, state);
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		trackball.Set(true, x, y);
		mouseLeft = true;
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		trackball.Set(false, x, y);
		mouseLeft = false;
	}
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
	{
		trackball.Set(true, x, y);
		mouseMid = true;
	}
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
	{
		trackball.Set(true, x, y);
		mouseMid = false;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		trackball.Set(true, x, y);
		mouseRight = true;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	{
		trackball.Set(true, x, y);
		mouseRight = false;
	}
}

void ReloadNoiseTexture()
{
	glUseProgram(shader_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, cloud_shape);
	const int tex_shape_loc = 7;		// "_CloudShape"
	glUniform1i(tex_shape_loc, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, cloud_detail);
	const int tex_detail_loc = 8;		// "_CloudDetail"
	glUniform1i(tex_detail_loc, 1);
	glUseProgram(0);
}

void GenNoiseTexture()
{
	NoiseGen noiseMaster;

	glUseProgram(shader_program);
	
	glActiveTexture(GL_TEXTURE0);
	noiseMaster.GetGloudShape(cloud_shape);
	glBindTexture(GL_TEXTURE_3D, cloud_shape);
	const int tex_shape_loc = 7;		// "_CloudShape"
	glUniform1i(tex_shape_loc, 0);

	glActiveTexture(GL_TEXTURE1);
	noiseMaster.GetGloudDetail(cloud_detail);
	glBindTexture(GL_TEXTURE_3D, cloud_detail);
	const int tex_detail_loc = 8;		// "_CloudDetail"
	glUniform1i(tex_detail_loc, 1);
	glUseProgram(0);
}

int main (int argc, char **argv)
{
	//Configure initial window state using freeglut
	glutInit(&argc, argv); 
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition (5, 5);
	glutInitWindowSize (1280, 720);
	int win = glutCreateWindow ("Cloud Generation");

	printGlInfo();

	//Register callback functions with glut. 
	glutDisplayFunc(display); 
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutKeyboardUpFunc(keyboard_up);
	glutSpecialUpFunc(special_up);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);

	glutIdleFunc(idle);

	initOpenGl();
	ImGui_ImplGlut_Init(); // initialize the imgui system
	GenNoiseTexture();

	//Enter the glut event loop.
	glutMainLoop();
	glutDestroyWindow(win);
	return 0;		
}


