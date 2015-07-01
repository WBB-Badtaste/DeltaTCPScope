// DeltaTCPScope.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "glut.h"
#include <Windows.h>
#include <Winuser.h>
#include "DeltaRobot.h"
#include <process.h>
#include <iostream>

#include <nyceapi.h>
#include <sacapi.h>
#include <rocksapi.h>

#include "Painter.h"
#include "Camera.h"

using namespace std;

HANDLE hReadTCP = 0;
BOOL bRead = FALSE;

Camera *pCamera;
Painter *pPainter;
double coordinate[6] = {0, 300, -600, -300, -150, 150};
double target[3] = {(coordinate[1] + coordinate[0]) / 2, (coordinate[3] + coordinate[2]) / 2, (coordinate[5] + coordinate[4]) / 2};

double screenWidth=GetSystemMetrics(SM_CXSCREEN);  
double screenHeight=GetSystemMetrics(SM_CYSCREEN); 
const double rate_angle2pu = 0.5 / M_PI * 10000;

#define NUM_AXES 3
const char *axName[ NUM_AXES ] = { "DEF_AXIS_1", "DEF_AXIS_2", "DEF_AXIS_3"};
BOOL axCon[ NUM_AXES ];
SAC_AXIS axId[ NUM_AXES ];
NYCE_STATUS nyceStatus = NYCE_OK;

unsigned __stdcall ReadTcpFunc(void *)
{
	while(bRead)
	{
		double jointPos[3];
		double tcp[3];
		for (uint32_t ax = 0; ax < NUM_AXES; ax++)
		{
			nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacReadVariable(axId[ax], SAC_VAR_AXIS_POS, &jointPos[ax]);
		}
		delta_calcForward(jointPos[0] / rate_angle2pu, jointPos[1] / rate_angle2pu, jointPos[2] / rate_angle2pu, tcp[0], tcp[1], tcp[2]);
		pPainter->AddTcpPoint(tcp);
		Sleep(10);
	}
	return 0;
}

void HandleError(const char *name)
{
	cout<<"\n\nError occur at:"<<name<<"\nError Code:"<<NyceGetStatusString(nyceStatus)<<"\n"<<endl;
}

void HandleError(NYCE_STATUS Status, const char *name)
{
	cout<<"\n\nError occur at:"<<name<<"\nError Code:"<<NyceGetStatusString(Status)<<"\n"<<endl;
}

void TermAxis(void)
{
	int ax;
	SAC_STATE sacState = SAC_NO_STATE;
	SAC_SPG_STATE sacSpgState;
	nyceStatus = NYCE_OK;
	for (ax = 0; ax < NUM_AXES; ax++ )
	{
		if (axCon[ax] != TRUE) continue;
		nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacReadState(axId[ ax ], &sacState, &sacSpgState);
		if(NyceSuccess(nyceStatus) && sacState == SAC_MOVING)
		{
			nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacQuickStop(axId[ ax ]);
			if (NyceSuccess(nyceStatus))
			{
				nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_MOTION_STOPPED, 10 );
				if (NyceError(nyceStatus))
				{
					HandleError(axName[ ax ]);
					nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacReset(axId[ ax ]);
					nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_RESET, 10 );
				}
			}
		}
		nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacShutdown(axId[ ax ]);
		nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_SHUTDOWN, 10 );
		nyceStatus = NyceError(nyceStatus) ? nyceStatus : SacDisconnect(axId[ ax ]);
		if(NyceError(nyceStatus)) HandleError(axName[ ax ]);
		else axCon[ax] = FALSE;
	}
}

BOOL InitAxis(void)
{
	int ax;
	SAC_SPG_STATE sacSpgState;
	SAC_STATE sacState;
	SAC_CONFIGURE_AXIS_PARS axisPars;
	nyceStatus = NYCE_OK;
	for (ax = 0; ax < NUM_AXES; ax++ )
	{
		nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacConnect( axName[ ax ], &axId[ ax ] );
		if ( NyceSuccess(nyceStatus)) axCon[ax] = TRUE;

		nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacReadState( axId[ ax ], &sacState, &sacSpgState);
		if(NyceSuccess(nyceStatus))
		{
			switch (sacState)
			{
			default:
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacShutdown( axId[ ax ]);
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_SHUTDOWN, 10 );
			case SAC_IDLE:
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacInitialize( axId[ ax ], SAC_USE_FLASH );
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_INITIALIZE, 10 );
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacGetAxisConfiguration( axId[ ax ], &axisPars );
				if ( NyceSuccess(nyceStatus) && axisPars.motorType == SAC_BRUSHLESS_AC_MOTOR )
				{
					nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacAlignMotor( axId[ ax ] );
					nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_ALIGN_MOTOR, 10 );
				}
			case SAC_FREE:
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacLock( axId[ ax ] );
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_LOCK, 10 );
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacHome( axId[ ax ] );
				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_HOMING_COMPLETED, 10 );
				break;
// 			case SAC_MOVING:
// 				printf("Waiting the motion stop...");
// 				nyceStatus =  NyceError(nyceStatus) ? nyceStatus : SacSynchronize( axId[ ax ], SAC_REQ_MOTION_STOPPED, 30 );
// 				break;
			}                           
		}

		if(NyceError(nyceStatus))
		{
			HandleError(axName[ ax ]);
			TermAxis();
			return FALSE;
		}
	}

	return TRUE;
}

void KeyboardFunc(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'b':
	case 'B':
		InitAxis();
		bRead = TRUE;
		hReadTCP = (HANDLE)_beginthreadex(NULL,NULL,ReadTcpFunc,NULL,NULL,NULL);
		break;
	case 't':
	case 'T':
		bRead = FALSE;
		WaitForSingleObject(hReadTCP, INFINITE);
		TermAxis();
		break;
	case 'a':
	case 'A':
		break;
	case 'd':
	case 'D':
		break;
	case 'w':
	case 'W':
		pCamera->MoveFront();
		break;
	case 's':
	case 'S':
		pCamera->MoveBack();		
		break;
	case 'q':
	case 'Q':
		break;
	case 'e':
	case 'E':
		break;
	default:
		break;
	}
}

enum mouseMotionType
{
	noMotion = 0,
	xRotation = 1,
	yRotation = 2,
	xTranslation = 3,
	yTranslation = 4,
	zTranslation = 5	
};

enum mousebuttonType
{
	noButton = 0,
	leftButton = 1,
	middleButton = 2,
	rightButton = 3,
	twoButton = 4
};

mouseMotionType mmt = noMotion;
mousebuttonType mbt = noButton;

double mmThreshold = 2;
int mouseLocation[2];

void MousePassFunc(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		mouseLocation[0] = x;
		mouseLocation[1] = y;
		switch(button)
		{
		case GLUT_LEFT_BUTTON:
			if (mbt == rightButton)
			{
				if (x - mouseLocation[0] && y - mouseLocation[1])
				{
					pCamera->Store();
				}
				mmt = noMotion;
				mbt = twoButton;
			}
			else
				mbt = leftButton;
			break;
		case GLUT_MIDDLE_BUTTON:
			mbt = middleButton;
			break;
		case GLUT_RIGHT_BUTTON:
			if (mbt == leftButton)
			{
				if (x - mouseLocation[0] && y - mouseLocation[1])
				{
					pCamera->Store();
				}
				mmt = noMotion;
				mbt = twoButton;
			}
			else
				mbt = rightButton;
			break;
		default:
			break;
		}
	}
	if (state == GLUT_UP)
	{
		if (x - mouseLocation[0] && y - mouseLocation[1])
		{
			pCamera->Store();
		}
		mmt = noMotion;
		if (mbt == twoButton)
		{
			if (button == GLUT_LEFT_BUTTON)
			{
				mbt = rightButton;
			}
			if (button == GLUT_RIGHT_BUTTON)
			{
				mbt = leftButton;
			}
		}
		else
		{
			mbt = noButton;
		}
	}
}

void MouseMotionFunc(int x, int y)
{
	switch(mmt)
	{
	case noMotion:
		if (mbt == leftButton)
		{
			if (x - mouseLocation[0] > mmThreshold || mouseLocation[0] - x > mmThreshold)
				mmt = xRotation;
			if (y - mouseLocation[1] > mmThreshold || mouseLocation[1] - y > mmThreshold)
				mmt = yRotation;
		}
		if (mbt == rightButton)
		{
			if (x - mouseLocation[0] > mmThreshold || mouseLocation[0] - x > mmThreshold)
				mmt = xTranslation;
			if (y - mouseLocation[1] > mmThreshold || mouseLocation[1] - y > mmThreshold)
				mmt = yTranslation;
		}
		if (mbt == twoButton)
		{
			if (y - mouseLocation[1] > mmThreshold || mouseLocation[1] - y > mmThreshold)
				mmt = zTranslation;
		}
	case xRotation:
		pCamera->MoveAngle(x - mouseLocation[0], 0);
		break;
	case yRotation:
		pCamera->MoveAngle(0, y - mouseLocation[1]);
		break;
	case xTranslation:
		pCamera->MoveDistance(x - mouseLocation[0], 0, 0);
		break;
	case yTranslation:
		pCamera->MoveDistance(0, y - mouseLocation[1], 0);
		break;
	case zTranslation:
		pCamera->MoveDistance(0, 0, y - mouseLocation[1]);
		break;
	default:
		break;
	}
}

void OpenglDraw()
{
	pPainter->Draw();
}

int main(int argc, char* argv[])
{
	NyceInit(NYCE_NET);

	glutInit(&argc, argv);  // 初始化GLUT
	glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE); // 修改了参数为GLUT_SINGLE (单缓冲)和GLUT_RGB(非索引)
	glutInitWindowPosition(100, 100);  // 显示窗口在屏幕的相对位置
	glutInitWindowSize(screenWidth / 2, screenHeight / 2); // 设置显示窗口大小

	glutCreateWindow("DeltaRobotTCPScope"); // 创建窗口，附带标题
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30, screenWidth / screenHeight, 0, 1000);

	pCamera = new Camera(target);
	pPainter = new Painter(coordinate);

	glutDisplayFunc(OpenglDraw);
	glutIdleFunc(OpenglDraw);
	glutKeyboardFunc(KeyboardFunc);
	glutMouseFunc(MousePassFunc);
	glutMotionFunc(MouseMotionFunc);
	glutMainLoop(); // GLUT 状态机
	return 0;
}

