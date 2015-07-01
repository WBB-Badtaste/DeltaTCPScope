#include "StdAfx.h"
#include "Painter.h"


Painter::Painter(double coordinate[]):
	indexDraw(-1),
	scale(10),
	coordinate_x_min(coordinate[0]),
	coordinate_x_max(coordinate[1]),
	coordinate_y_min(coordinate[2]),
	coordinate_y_max(coordinate[3]),
	coordinate_z_min(coordinate[4]),
	coordinate_z_max(coordinate[5])
{
	for (int i = 0; i < 3000; ++i)
	{
		drawPoint[i][0] = drawPoint[i][1] = drawPoint[i][2] = 0;
	}
 	hMutex = CreateMutex(NULL, FALSE, NULL);
}


Painter::~Painter(void)
{
}

void Painter::DrawCoordinate()
{
	double step_x = (coordinate_x_max - coordinate_x_min) / 10;	
	double step_y = (coordinate_y_max - coordinate_y_min) / 10;
	double step_z = (coordinate_z_max - coordinate_z_min) / 10;
	int pos = 0;

	glColor3f(0,0,0);

	for (pos = coordinate_y_min; pos <= coordinate_y_max; pos += step_y)
	{
		glBegin(GL_LINES);
		glVertex3d(coordinate_x_min, pos, coordinate_z_max);
		glVertex3d(coordinate_x_min, pos, coordinate_z_max + scale);
		glEnd();
	}
	for (pos = coordinate_x_min + step_x; pos <= coordinate_x_max; pos += step_x)
	{
		glBegin(GL_LINES);
		glVertex3d(pos, coordinate_y_min, coordinate_z_max);
		glVertex3d(pos, coordinate_y_min, coordinate_z_max + scale);
		glEnd();
	}
	for (pos = coordinate_z_min; pos <= coordinate_z_max; pos += step_z)
	{
		glBegin(GL_LINES);
		glVertex3d(coordinate_x_max, coordinate_y_min, pos);
		glVertex3d(coordinate_x_max + scale, coordinate_y_min, pos);
		glEnd();
	}

	glBegin(GL_LINES);
	glVertex3d(coordinate_x_min,coordinate_y_min,coordinate_z_max);
	glVertex3d(coordinate_x_min,coordinate_y_max,coordinate_z_max);
	glEnd();
	glBegin(GL_LINES);
	glVertex3d(coordinate_x_min,coordinate_y_min,coordinate_z_max);
	glVertex3d(coordinate_x_max,coordinate_y_min,coordinate_z_max);
	glEnd();
	glBegin(GL_LINES);
	glVertex3d(coordinate_x_max,coordinate_y_min,coordinate_z_min);
	glVertex3d(coordinate_x_max,coordinate_y_min,coordinate_z_max);
	glEnd();

	
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1,0xAAAA);
	for (pos = coordinate_z_min; pos < coordinate_z_max; pos += step_z)
	{
		glBegin(GL_LINES);
		glVertex3d(coordinate_x_min,coordinate_y_min,pos);
		glVertex3d(coordinate_x_min,coordinate_y_max,pos);
		glEnd();
	}
	for (pos = coordinate_y_min; pos <= coordinate_y_max; pos += step_y)
	{
		glBegin(GL_LINES);
		glVertex3d(coordinate_x_min,pos,coordinate_z_min);
		glVertex3d(coordinate_x_min,pos,coordinate_z_max);
		glEnd();
	}
	for (pos = coordinate_z_min; pos < coordinate_z_max; pos += step_z)
	{
		glBegin(GL_LINES);
		glVertex3d(coordinate_x_min,coordinate_y_min,pos);
		glVertex3d(coordinate_x_max,coordinate_y_min,pos);
		glEnd();
	}
	for (pos = coordinate_x_min + step_x; pos < coordinate_x_max; pos += step_x)
	{
		glBegin(GL_LINES);
		glVertex3d(pos,coordinate_y_min,coordinate_z_min);
		glVertex3d(pos,coordinate_y_min,coordinate_z_max);
		glEnd();
	}
	for (pos = coordinate_y_min + step_y; pos <= coordinate_y_max; pos += step_y)
	{
		glBegin(GL_LINES);
		glVertex3d(coordinate_x_min,pos,coordinate_z_min);
		glVertex3d(coordinate_x_max,pos,coordinate_z_min);
		glEnd();
	}
	for (pos = coordinate_x_min + step_x; pos <= coordinate_x_max; pos += step_x)
	{
		glBegin(GL_LINES);
		glVertex3d(pos,coordinate_y_min,coordinate_z_min);
		glVertex3d(pos,coordinate_y_max,coordinate_z_min);
		glEnd();
	}
	glDisable(GL_LINE_STIPPLE);
};

void Painter::DrawPath()
{
	WaitForSingleObject(hMutex, INFINITE);
	int index = indexDraw;
	ReleaseMutex(hMutex);
	glColor3f(0,1,0);
	glBegin(GL_LINE_STRIP); 
	for (int i = 0; i <= index; i++)
	{
		glVertex3d(drawPoint[i][0],drawPoint[i][1],drawPoint[i][2]);
	}
	glEnd();
}

void Painter::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	DrawCoordinate();
	DrawPath();
	glFlush();
}

void Painter::AddTcpPoint(double point[])
{
	WaitForSingleObject(hMutex, INFINITE);
	if (++indexDraw == 3000)
		indexDraw = 0;                         
	drawPoint[indexDraw][0] = point[0];
	drawPoint[indexDraw][1] = point[2];
	drawPoint[indexDraw][2] = -point[1];
	ReleaseMutex(hMutex);
}