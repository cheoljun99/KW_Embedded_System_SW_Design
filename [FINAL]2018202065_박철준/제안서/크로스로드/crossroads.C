#include "includes.h"
#include <time.h>

#define TASK_STK_SIZE	512
#define TASK_COUNT	5
#define TASK_PRIO	10
#define SEM_MAX		15
#define ROAD_CNT	4

#define NORTH	0
#define WEST	1
#define SOUTH	2
#define EAST	3

#define COLORS	7

#define MOVE	1
#define STOP	2
#define READY	3
#define REMOVAL 4
#define WARNING 5

#define LEFTPOS(pos) (pos == 0 ? SEM_MAX - 1 : pos - 1)

typedef struct {
	INT8U color;
	INT8U posX;
	INT8U posY;
	INT8U state;
}CAR_INFO;



OS_STK TaskStk[TASK_COUNT][TASK_STK_SIZE];
OS_EVENT *SemRoad[ROAD_CNT];
CAR_INFO CarInfo[ROAD_CNT][SEM_MAX];

INT8U SigState[2] = {STOP, STOP};
INT8U StartPos[ROAD_CNT][2] = { 36, 2,  2, 13, 41, 22, 76, 11};
INT8S MOVEXY[ROAD_CNT]		= { 1,  2,  -1, -2};
INT8U RoadMax[ROAD_CNT]		= { 22, 76,  2,  2};
INT8U CrossRoad[ROAD_CNT]	= { 9,  32, 15, 44};
INT8U ColorArray[COLORS]	= { DISP_FGND_RED + DISP_BGND_LIGHT_GRAY,
								DISP_FGND_BLUE + DISP_BGND_LIGHT_GRAY,
								DISP_FGND_GREEN + DISP_BGND_LIGHT_GRAY,
								DISP_FGND_YELLOW + DISP_BGND_LIGHT_GRAY,
								DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY,
								DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY,								
								DISP_FGND_GRAY + DISP_BGND_LIGHT_GRAY };
INT8U Collision[ROAD_CNT] = { FALSE, FALSE, FALSE, FALSE};


static void  TaskViewClear();		//초기화면 보여주기
static void  TaskStartDispInit();	//초기화면 보여주기
void TaskViewDisp();				//차량 및 신호등 전체 화면 보여주기


void TaskRandProb(void *data);	//전체 차량의 이동 관리 TASK
void TaskCarMake(void *data);		//세마포어를 이용한 차량 생성 TASK
void TaskSignalLamp(void *data);	//신호등 TASK


int  main (void)
{
	INT16U i;
    OSInit();    
	OSTaskCreate(TaskSignalLamp, (void *)NULL, &TaskStk[0][TASK_STK_SIZE - 1], TASK_PRIO/2);
	OSTaskCreate(TaskRandProb, (void *)NULL, &TaskStk[0][TASK_STK_SIZE - 1], TASK_PRIO);
	
	for( i = 1; i < 5; i++){
		OSTaskCreate(TaskCarMake, (void *)NULL, &TaskStk[i][TASK_STK_SIZE - 1], (INT8U)(TASK_PRIO + i));
	}

    OSStart();
    return 0;
}




//전체 차량 이동 관리 및 화면 제어
void TaskRandProb (void *pdata)
{
	INT8U msg[40];
	INT8U i;
	INT16S key;

	TaskStartDispInit();

	for (;;) {
		
		if (PC_GetKey(&key)) {                             /* See if key has been pressed              */
            if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                exit(0);                                   /* Yes, return to DOS                       */
            }
        }

        PC_GetDateTime(msg);
        PC_DispStr(60, 22, msg, DISP_FGND_YELLOW + DISP_BGND_BLUE);        

		TaskViewDisp();



		Collision[NORTH] = FALSE;
		Collision[SOUTH] = FALSE;
		Collision[EAST] = FALSE;
		Collision[WEST] = FALSE;

		for(i = 0; i < SEM_MAX; i++){		

			if(CarInfo[NORTH][i].state != REMOVAL){
				if(CarInfo[NORTH][i].posY < RoadMax[NORTH]){					
					if(CarInfo[NORTH][i].posY > CrossRoad[NORTH] || SigState[0] == MOVE){
						CarInfo[NORTH][i].posY += MOVEXY[NORTH];
					}else if(CarInfo[NORTH][i].posY + MOVEXY[NORTH] <= CrossRoad[NORTH] && CarInfo[NORTH][LEFTPOS(i)].posY != CarInfo[NORTH][i].posY + MOVEXY[NORTH]){
						CarInfo[NORTH][i].posY += MOVEXY[NORTH];
					}

					if(CarInfo[NORTH][i].posY == StartPos[NORTH][1]){
						Collision[NORTH] = TRUE;
					}
				}else{
					CarInfo[NORTH][i].state = REMOVAL;
					OSSemPost(SemRoad[NORTH]);
				}
			}
			
			if(CarInfo[SOUTH][i].state != REMOVAL){
				if(CarInfo[SOUTH][i].posY > RoadMax[SOUTH]){
					if(CarInfo[SOUTH][i].posY < CrossRoad[SOUTH] || SigState[0] == MOVE){
						CarInfo[SOUTH][i].posY += MOVEXY[SOUTH];
					}else if(CarInfo[SOUTH][i].posY + MOVEXY[SOUTH] >= CrossRoad[SOUTH] && CarInfo[SOUTH][LEFTPOS(i)].posY != CarInfo[SOUTH][i].posY + MOVEXY[SOUTH]){
						CarInfo[SOUTH][i].posY += MOVEXY[SOUTH];
					}

					if(CarInfo[SOUTH][i].posY == StartPos[SOUTH][1]){
						Collision[SOUTH] = TRUE;
					}
				}else{
					CarInfo[SOUTH][i].state = REMOVAL;
					OSSemPost(SemRoad[SOUTH]);
				}
			}

			if(CarInfo[WEST][i].state != REMOVAL){
				if(CarInfo[WEST][i].posX < RoadMax[WEST]){
					if(CarInfo[WEST][i].posX > CrossRoad[WEST] || SigState[1] == MOVE){
						CarInfo[WEST][i].posX += MOVEXY[WEST];
					}else if(CarInfo[WEST][i].posX + MOVEXY[WEST] <= CrossRoad[WEST] && CarInfo[WEST][LEFTPOS(i)].posX != CarInfo[WEST][i].posX + MOVEXY[WEST]){
						CarInfo[WEST][i].posX += MOVEXY[WEST];
					}
					
					if(CarInfo[WEST][i].posX == StartPos[WEST][0]){
						Collision[WEST] = TRUE;
					}
				}else{
					CarInfo[WEST][i].state = REMOVAL;
					OSSemPost(SemRoad[WEST]);
				}
			}

			if(CarInfo[EAST][i].state != REMOVAL){
				if(CarInfo[EAST][i].posX > RoadMax[EAST]){
					if(CarInfo[EAST][i].posX < CrossRoad[EAST] || SigState[1] == MOVE){
						CarInfo[EAST][i].posX += MOVEXY[EAST];
					}else if(CarInfo[EAST][i].posX + MOVEXY[EAST] >= CrossRoad[EAST] && CarInfo[EAST][LEFTPOS(i)].posX != CarInfo[EAST][i].posX + MOVEXY[EAST]){
						CarInfo[EAST][i].posX += MOVEXY[EAST];
					}

					if(CarInfo[EAST][i].posX == StartPos[EAST][0]){
						Collision[EAST] = TRUE;
					}
				}else{
					CarInfo[EAST][i].state = REMOVAL;
					OSSemPost(SemRoad[EAST]);
				}				
			}
		}		

		OSTimeDly(1);
    }
}




//신호등 TASK
void TaskSignalLamp(void *data)
{
	while(TRUE){
		SigState[0] = MOVE;	//col
		SigState[1] = STOP;	//row
		OSTimeDly(10);
		SigState[0] = WARNING;
		OSTimeDly(5);
		SigState[0] = STOP;
		SigState[1] = MOVE;
		OSTimeDly(10);
		SigState[1] = WARNING;
		OSTimeDly(5);
	}
}




//세마포어를 이용 차량 생성
void TaskCarMake (void *pdata)
{

	INT8U delay, color, ERR, dir, posX, posY, position = 0;	

	srand(time((unsigned int *)0) + (OSTCBCur->OSTCBPrio * 237 >> 4));

	dir = (OSTCBCur->OSTCBPrio % 4);
	
	SemRoad[dir] = OSSemCreate(SEM_MAX);

	posX = StartPos[dir][0];
	posY = StartPos[dir][1];
	
	while(TRUE){
		OSSemPend(SemRoad[dir], 0, &ERR);		

		while(Collision[dir]){			
			OSTimeDly(1);
		}

		color = ColorArray[(INT8U)(rand() % (COLORS - 1))];
		
		if(position >= SEM_MAX){
			position = 0;
		}
		
		CarInfo[dir][position].posX = posX;
		CarInfo[dir][position].posY = posY;
		CarInfo[dir][position].color = color;
		CarInfo[dir][position].state = READY;
		Collision[dir] = TRUE;
		
		delay = (INT8U)(rand() % 7 + 1);

		OSTimeDly(delay);
		
		position++;
	}
}




//각차량 + 신호등 그리기
static void TaskViewDisp()
{
	INT8U i, j;	

	TaskViewClear();

	for(i = 0; i < ROAD_CNT; i++){
		for(j = 0; j < SEM_MAX; j++){
			if(CarInfo[i][j].state == READY || CarInfo[i][j].state == STOP || CarInfo[i][j].state == MOVE){
				PC_DispStr(CarInfo[i][j].posX, CarInfo[i][j].posY, "■", CarInfo[i][j].color);
			}
		}
	}

	
	PC_DispStr(52, 8,  "● ● ●", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(20, 19, "● ● ●", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	
	switch(SigState[0]){
		case MOVE:
			PC_DispStr(52, 8,  "●", ColorArray[2]);
		break;
		case WARNING:	
			PC_DispStr(55, 8,  "●", ColorArray[3]);
		break;
		case STOP:	
			PC_DispStr(58, 8,  "●", ColorArray[0]);
		break;
	}

	switch(SigState[1]){
		case MOVE:
			PC_DispStr(20, 19,  "●", ColorArray[2]);
		break;
		case WARNING:	
			PC_DispStr(23, 19,  "●", ColorArray[3]);
		break;
		case STOP:	
			PC_DispStr(26, 19,  "●", ColorArray[0]);
		break;
	}
}




//초기화면 그리기
void  TaskStartDispInit()
{
	PC_DispStr(0, 0,  "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 1,  "                              CROSSROADS  PROGRAM                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 2,  "                                  |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 3,  "                                  |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 4,  "    *************************     |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 5,  "    *<-PRESS 'ESC' TO QUIT->*     |    !    |        N → S                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 6,  "    *************************     |    !    |        S → N                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 7,  "                                  |    !    |      ----------                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 8,  "                                  |    !    |      |        |                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 9,  "                                  |    !    |      ----------                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 10, "  ================================           =================================  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 11, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 12, "  --------------------------------           ---------------------------------  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 13, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 14, "  ================================           =================================  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 15, "                                  |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 16, "                     W → E       |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 17, "                     E → W       |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 18, "                   ----------     |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 19, "                   |        |     |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 20, "                   ----------     |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(0, 21, "                                  |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 22, "                                  |    !    |                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);	
}





//차량이동 흔적 지우기
static void TaskViewClear()
{
	PC_DispStr(36, 2,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 3,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 4,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 5,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 6,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 7,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 8,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 9,  "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 10, "       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(2,  11, "                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 12, "       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(2,  13, "                                                                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);	
	PC_DispStr(36, 14, "       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 15, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 16, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 17, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 18, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 19, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 20, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 21, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr(36, 22, "   !   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);    
}