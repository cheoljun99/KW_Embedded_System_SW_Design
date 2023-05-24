#include "includes.h"
#include <time.h>

#define TASK_STK_SIZE	512

#define TASK_COUNT	5//태스크 개수
#define TASK_PRIO	10 


#define MACHINE_CNT		12
#define AREA_CNT	5

#define NORMAL	1
#define ERROR	2

#define N_MSG 100 //메시지 큐 사이즈
OS_EVENT* msg_q; //메시지 큐를 위한 OS_EVENT 포인터 
void* msg_array[N_MSG]; // 메시지 큐 배열


typedef struct {
	INT8U color;
	INT8U posX;
	INT8U posY;
	INT8U state;
}BITCOIN_MACHINE_INFO;//채굴기 정보를 가지는 구조체 세마포어 critical path에서 사용


OS_STK TaskStk[TASK_COUNT][TASK_STK_SIZE];
OS_EVENT* sem;// 세마 포어를 위한 OS_EVENT 포인터



BITCOIN_MACHINE_INFO BitcoinMachineInfo[AREA_CNT][MACHINE_CNT];// 구조체 배열 전역변수로 선언


INT8U Bit_MachinePos_X[MACHINE_CNT] = { 3,7,11,15,19,23,27,31,35,39,43,47 }; // 비트코인 채굴기의 X좌표

INT8U Bit_MachinePos_Y[AREA_CNT] = {7,11,15,19,23}; //비트코인 채굴기의 Y좌표

INT16U Message_Q_Count = 0; // 메시지 큐 사용 횟수 카운트 세마포어 critical path에서 사용








static void  TaskStartDispInit();	//초기화면 보여주기
void TaskViewDisp();				//전체 화면 보여주기
void TaskMachineMake();				//기계 생성 및 초기화

void TaskMachineCrate(void *data);	//전체 비트코인 채굴기 생성 관리 및 화면 제어 TASK
void TaskMachineProb(void* data);// 채굴기 고장 TASK
void TaskFixMachine(void* data);// 채굴기 수리 TASK
void TaskInfoPrint(void* data);//채굴기 고장 상태를 출력 하는 TASK
void  TaskViewClear(void* data);//메시지큐의 흔적을 지우는 TASK







int  main (void)
{
	INT16U i;
    OSInit();

	msg_q = OSQCreate(msg_array, (INT16U)N_MSG); // 메시지큐 구조체를 생성하고, 큐 구조체가 포인터 배열을 가르키도록 초기화
	sem = OSSemCreate(1);// 세마포어를 생성하고, cnt로 초기화
	if (msg_q == 0)
	{
		printf("creating msg_q is failed\n");// 세마포어를 생성하고, cnt로 초기화
		return -1;
	}


	OSTaskCreate(TaskViewClear, (void*)NULL, &TaskStk[0][TASK_STK_SIZE - 1], TASK_PRIO / 4);
	OSTaskCreate(TaskInfoPrint, (void*)NULL, &TaskStk[0][TASK_STK_SIZE - 1], TASK_PRIO/2);
	OSTaskCreate(TaskMachineCrate, (void *)NULL, &TaskStk[0][TASK_STK_SIZE - 1], TASK_PRIO);
	OSTaskCreate(TaskMachineProb, (void*)NULL, &TaskStk[1][TASK_STK_SIZE - 1], (INT8U)(TASK_PRIO + 1));
	OSTaskCreate(TaskFixMachine, (void*)NULL, &TaskStk[2][TASK_STK_SIZE - 1], (INT8U)(TASK_PRIO + 2));

    OSStart();
    return 0;
}

//메시지큐 흔적 지우기
void TaskViewClear(void* pdata)
{
	
	while (1)
	{
		//PC_DispStr(22, 3, "중지 전 TaskViewClear    ", DISP_FGND_YELLOW + DISP_BGND_BLUE);

		OSTaskSuspend(TASK_PRIO / 4);//TASK 중지

		//PC_DispStr(22, 3, "중지 후 TaskViewClear    ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		INT8U err;

		OSSemPend(sem, 0, &err);//세마포어를 획득
		while (Message_Q_Count != 0)
		{
			PC_DispStr(54, Message_Q_Count+1, "                         ", DISP_FGND_GRAY + DISP_BGND_BLACK); // 흔적 지우기
			Message_Q_Count--; // 메시지 큐 사용 카운트 감소
		}
		PC_DispStr(54, Message_Q_Count+1, "                         ", DISP_FGND_GRAY + DISP_BGND_BLACK); // 흔적 지우기
		OSSemPost(sem);//세마포어 반환
	}


}

//채굴기 고장상태를 출력 하는 TASK
void TaskInfoPrint(void* pdata)
{
	
	void* msg;
	INT8U err;
	for (;;) {
		//PC_DispStr(22, 3, "TaskInfoPrint         ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		msg = OSQPend(msg_q, 0, &err); //수신한 메시지를 반환
		if (msg != 0)
		{
			PC_DispStr(54, Message_Q_Count+1, msg, DISP_FGND_GRAY + DISP_BGND_BLACK); // 메시지 큐 내용 디스플레이에 출력
			OSSemPend(sem, 0, &err);//세마포어를 획득
			Message_Q_Count++; // 메시지 큐 사용 카운트 증가
			OSSemPost(sem);//세마포어 반환
		}

	}

}


//채굴기 기계 고장 TASK
void TaskMachineProb(void* pdata)
{
	while (1)
	{
		//PC_DispStr(22, 3, "TaskMachineProb         ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		INT8U err;

		OSSemPend(sem, 0, &err);//세마포어를 획득
		// critical path

		srand(time(NULL)); // (11)
		INT8U error_aera = (rand() % 5);
		INT8U error_machine = (rand() % 12);
		
		while (BitcoinMachineInfo[error_aera][error_machine].state == ERROR)
		{
		
			error_aera = (rand() % 5);
			error_machine = (rand() % 12);
		}

		BitcoinMachineInfo[error_aera][error_machine].color = DISP_FGND_RED;
		BitcoinMachineInfo[error_aera][error_machine].state = ERROR;
		char msg[100];//메시지큐에 들어갈 메시지 변수
		sprintf(msg, "%d 구역 %d번째 채굴기 고장\n", error_aera+1, error_machine+1); // 변수에 메시지 저장
		err = OSQPost(msg_q, msg); // 메시지 큐에 저장
		while (err != OS_NO_ERR) 
		{
			err = OSQPost(msg_q, msg);
		}

		TaskViewDisp(); //현재 채굴기 상태 출력

		OSSemPost(sem);//세마포어 반환

		OSTimeDly(1);

	}
	

}

// 채굴기 기계 수리 TASK
void TaskFixMachine(void* pdata)
{
	while (1)
	{
		//PC_DispStr(22, 3, "중지 전 TaskFixMachine      ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		INT8U err;
		OSSemPend(sem, 0, &err);//세마포어 획득
		INT8U i, j;
		for (i = 0; i < AREA_CNT; i++) {
			for (j = 0; j < MACHINE_CNT; j++) {
				if (BitcoinMachineInfo[i][j].state == ERROR) {
					BitcoinMachineInfo[i][j].state = NORMAL;
					BitcoinMachineInfo[i][j].color = DISP_FGND_GREEN;
				}
			}
		}

		OSTaskResume(TASK_PRIO / 4);//메시지 큐 출력 흔적을 지우는 TASK 깨우기
		TaskViewDisp();//현재 채굴기 정보 디스플레이에 출력

		OSSemPost(sem);//세마포어 반환
		//PC_DispStr(22, 3, "중지 후 TaskFixMachine      ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		OSTimeDly(rand() % 22);//채굴기를 수리할 타이밍을 정해주는데 디스플레이에 최대한으로 담을 수 있는 양 이내로 고장 개수를 나타내고 이 후에는 수리
	}

}



//전체 비트코인 채굴기 생성 관리 및 화면 제어
void TaskMachineCrate (void *pdata)
{

	INT8U msg[40];
	INT8U i;
	INT16S key;
	INT8U err;
	TaskStartDispInit();// 채굴장 초기화면 출력

	OSSemPend(sem, 0, &err);//세마포어를 획득
	TaskMachineMake();// 채굴기 생성하고 정상상태로 초기화
	OSSemPost(sem);//세마포어 반환

	TaskViewDisp(); //현재 채굴기 상태 출력

	for (;;) {
		//PC_DispStr(22, 3, "TaskMachineCrate          ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		if (PC_GetKey(&key)) {                              //See if key has been pressed            
            if (key == 0x1B) {                              //Yes, see if it's the ESCAPE key 
                exit(0);                                    //Yes, return to DOS
            }
        }
		 
        PC_GetDateTime(msg);// 현재 시간 정보 저장
        PC_DispStr(60, 24, msg, DISP_FGND_YELLOW + DISP_BGND_BLUE);// 시간 정보 출력   

		TaskViewDisp();//현재 채굴기 상태 출력

		OSTimeDly(1);

    }

}




//비트코인채굴기 상태 초기화
void TaskMachineMake()
{

	INT8U color, posX, posY, position = 0;


	
	for (int i = 0; i < AREA_CNT; i++)
	{
		posY = Bit_MachinePos_Y[i];
		

		for (int j = 0; j < MACHINE_CNT; j++)
		{
			posX = Bit_MachinePos_X[j];

			BitcoinMachineInfo[i][j].posX = posX;
			BitcoinMachineInfo[i][j].posY = posY;
			BitcoinMachineInfo[i][j].color = DISP_FGND_GREEN;
			BitcoinMachineInfo[i][j].state = NORMAL;

		}

	}




}




//현재 채굴기 상태 그리기
static void TaskViewDisp()
{
	INT8U i, j;	

	for(i = 0; i < AREA_CNT; i++){
		for(j = 0; j < MACHINE_CNT; j++){
			if( BitcoinMachineInfo[i][j].state == ERROR || BitcoinMachineInfo[i][j].state == NORMAL){
				PC_DispStr(BitcoinMachineInfo[i][j].posX, BitcoinMachineInfo[i][j].posY, "■", BitcoinMachineInfo[i][j].color);
			}
		}
	}

}





//초기화면 그리기
void  TaskStartDispInit()
{
	PC_DispStr(0, 0,  "  -------------------------------                     _________________________  ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 1,  "  |비트코인 채굴장 관리 프로그램|                    |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 2,  "  -------------------------------                    |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 3,  "                                                     |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 4,  "                                                     |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 5,  "  1구역                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 6,  "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 7,  "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 8,  "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 9,  "  2구역                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 10, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 11, "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 12, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 13, "  3구역                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 14, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 15, "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 16, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 17, "  4구역                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 18, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 19, "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 20, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 21, "  5구역                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 22, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 23, "  |                                              |   |_________________________| ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 24, "  ------------------------------------------------                               ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	//PC_DispStr(0, 3,  "           현재 TASK:                                |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
}																	    
