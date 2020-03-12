#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "math.h"
#include <gl/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;
#ifndef M_PI
#define M_PI 3.1415926
#endif


class TrackBallC
{
public:
	TrackBallC()
	{
		m_view3DMatrix = new GLfloat[16];
		memset(m_view3DMatrix,0,sizeof(GLfloat)*16);
		m_view3DMatrix[0]=m_view3DMatrix[5]=m_view3DMatrix[10]=m_view3DMatrix[15]=1.0f;
		m_view3DMatrix[14]=-4;//translate on z a bit back
	}

	~TrackBallC()
	{
		delete[] m_view3DMatrix;
	}

	void SetWH()
	{
		w=glutGet(GLUT_WINDOW_WIDTH);
		h=glutGet(GLUT_WINDOW_HEIGHT);
	}
	void Reset(void)
	{

	}

	void Set(bool mouse,int x,int y)
	{
		SetWH();
		mouseDown=mouse;
		if (mouseDown)//mouse pressed, remember the coordinates
		{
		 //coordinates are relative to the previous location
		  lastPoint=trackBall(x,y);
		  oldX=x;//need to store oldX and oldY for zoom and translate
		  oldY=y;
		} 
	}
	void Rotate(int x,int y)
	{
		if (mouseDown)
		{
		   vec3 curPoint(0,0,0);
		   curPoint=trackBall(x,y);
		   vec3 direction(0,0,0);
		   direction=curPoint-lastPoint;
		   vec3 axis;
		   axis=cross(lastPoint, curPoint);
		   normalize(axis);
		   GLfloat angle=direction.length()*50.f;//50 is the sensitivity
		   glMatrixMode(GL_MODELVIEW);
		   glPushMatrix();
		   glLoadIdentity();
		   glRotatef(angle,axis.x,axis.y,axis.z);
		   //remember the translation vector and set it to zero (rotation will occur in the origin)
		   GLfloat m12=m_view3DMatrix[12],m13=m_view3DMatrix[13],m14=m_view3DMatrix[14];
		   m_view3DMatrix[12]=m_view3DMatrix[13]=m_view3DMatrix[14]=0;
		   glMultMatrixf((GLfloat*)m_view3DMatrix);
		   glGetFloatv(GL_MODELVIEW_MATRIX,(GLfloat*)m_view3DMatrix);
		   //translate back
		   m_view3DMatrix[12]=m12;m_view3DMatrix[13]=m13;m_view3DMatrix[14]=m14;
		   glPopMatrix();
		   lastPoint=curPoint;
		}
	}

	void Set3DViewCamera()
	{
	  glMatrixMode(GL_MODELVIEW);
	  glLoadMatrixf(m_view3DMatrix);
	}

	mat4 GetViewMatrix()
	{
		mat4 v;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				v[j][i] = m_view3DMatrix[i * 4 + j];
			}
		}
		return v;
	}

	void Zoom(int x,int y)
	{
		if (mouseDown)
		{
			GLfloat translateZ=(y-oldY)*0.01f;
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(0,0,translateZ);
			glMultMatrixf(m_view3DMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX,(GLfloat*)m_view3DMatrix);
			glPopMatrix();
			oldX=x;
			oldY=y;
		}
	}

	vec3 trackBall(int x,int y)
	{
		float d;

		vec3 v((2.f*x-w)/w,(h-2.f*y)/h,0.0);
		d = v.length();
		d = (d<1.f)?d:1.f;  // If d is > 1, then clamp it at one.
		v.z = (sqrtf(1.001f-d*d));  // project the line segment up to the surface of the sphere.
		normalize(v);  // We forced d to be less than one, not v, so need to normalize somewhere.
		return v;
	}

	void Translate(int x,int y)
	{
		if (mouseDown)
		{
			GLfloat transX=(x-oldX)*0.01f;
			GLfloat transY=(1-(y-oldY))*0.01f;
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(transX,transY,0);
			glMultMatrixf(m_view3DMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX,(GLfloat*)m_view3DMatrix);
			glPopMatrix();
			oldX=x;
			oldY=y;
		}
	}

private:
		vec3 lastPoint;
		GLint startX,startY,endX,endY,oldX,oldY;
		GLfloat moveX,moveY;//the main window
		bool mouseDown;
		GLfloat* m_view3DMatrix;
		GLint w,h; //windowW and windowH
};

#endif