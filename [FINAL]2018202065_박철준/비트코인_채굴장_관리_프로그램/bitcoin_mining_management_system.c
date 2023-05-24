#include "includes.h"
#include <time.h>

#define TASK_STK_SIZE	512

#define TASK_COUNT	5//�½�ũ ����
#define TASK_PRIO	10 


#define MACHINE_CNT		12
#define AREA_CNT	5

#define NORMAL	1
#define ERROR	2

#define N_MSG 100 //�޽��� ť ������
OS_EVENT* msg_q; //�޽��� ť�� ���� OS_EVENT ������ 
void* msg_array[N_MSG]; // �޽��� ť �迭


typedef struct {
	INT8U color;
	INT8U posX;
	INT8U posY;
	INT8U state;
}BITCOIN_MACHINE_INFO;//ä���� ������ ������ ����ü �������� critical path���� ���


OS_STK TaskStk[TASK_COUNT][TASK_STK_SIZE];
OS_EVENT* sem;// ���� ��� ���� OS_EVENT ������



BITCOIN_MACHINE_INFO BitcoinMachineInfo[AREA_CNT][MACHINE_CNT];// ����ü �迭 ���������� ����


INT8U Bit_MachinePos_X[MACHINE_CNT] = { 3,7,11,15,19,23,27,31,35,39,43,47 }; // ��Ʈ���� ä������ X��ǥ

INT8U Bit_MachinePos_Y[AREA_CNT] = {7,11,15,19,23}; //��Ʈ���� ä������ Y��ǥ

INT16U Message_Q_Count = 0; // �޽��� ť ��� Ƚ�� ī��Ʈ �������� critical path���� ���








static void  TaskStartDispInit();	//�ʱ�ȭ�� �����ֱ�
void TaskViewDisp();				//��ü ȭ�� �����ֱ�
void TaskMachineMake();				//��� ���� �� �ʱ�ȭ

void TaskMachineCrate(void *data);	//��ü ��Ʈ���� ä���� ���� ���� �� ȭ�� ���� TASK
void TaskMachineProb(void* data);// ä���� ���� TASK
void TaskFixMachine(void* data);// ä���� ���� TASK
void TaskInfoPrint(void* data);//ä���� ���� ���¸� ��� �ϴ� TASK
void  TaskViewClear(void* data);//�޽���ť�� ������ ����� TASK







int  main (void)
{
	INT16U i;
    OSInit();

	msg_q = OSQCreate(msg_array, (INT16U)N_MSG); // �޽���ť ����ü�� �����ϰ�, ť ����ü�� ������ �迭�� ����Ű���� �ʱ�ȭ
	sem = OSSemCreate(1);// ������� �����ϰ�, cnt�� �ʱ�ȭ
	if (msg_q == 0)
	{
		printf("creating msg_q is failed\n");// ������� �����ϰ�, cnt�� �ʱ�ȭ
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

//�޽���ť ���� �����
void TaskViewClear(void* pdata)
{
	
	while (1)
	{
		//PC_DispStr(22, 3, "���� �� TaskViewClear    ", DISP_FGND_YELLOW + DISP_BGND_BLUE);

		OSTaskSuspend(TASK_PRIO / 4);//TASK ����

		//PC_DispStr(22, 3, "���� �� TaskViewClear    ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		INT8U err;

		OSSemPend(sem, 0, &err);//������� ȹ��
		while (Message_Q_Count != 0)
		{
			PC_DispStr(54, Message_Q_Count+1, "                         ", DISP_FGND_GRAY + DISP_BGND_BLACK); // ���� �����
			Message_Q_Count--; // �޽��� ť ��� ī��Ʈ ����
		}
		PC_DispStr(54, Message_Q_Count+1, "                         ", DISP_FGND_GRAY + DISP_BGND_BLACK); // ���� �����
		OSSemPost(sem);//�������� ��ȯ
	}


}

//ä���� ������¸� ��� �ϴ� TASK
void TaskInfoPrint(void* pdata)
{
	
	void* msg;
	INT8U err;
	for (;;) {
		//PC_DispStr(22, 3, "TaskInfoPrint         ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		msg = OSQPend(msg_q, 0, &err); //������ �޽����� ��ȯ
		if (msg != 0)
		{
			PC_DispStr(54, Message_Q_Count+1, msg, DISP_FGND_GRAY + DISP_BGND_BLACK); // �޽��� ť ���� ���÷��̿� ���
			OSSemPend(sem, 0, &err);//������� ȹ��
			Message_Q_Count++; // �޽��� ť ��� ī��Ʈ ����
			OSSemPost(sem);//�������� ��ȯ
		}

	}

}


//ä���� ��� ���� TASK
void TaskMachineProb(void* pdata)
{
	while (1)
	{
		//PC_DispStr(22, 3, "TaskMachineProb         ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		INT8U err;

		OSSemPend(sem, 0, &err);//������� ȹ��
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
		char msg[100];//�޽���ť�� �� �޽��� ����
		sprintf(msg, "%d ���� %d��° ä���� ����\n", error_aera+1, error_machine+1); // ������ �޽��� ����
		err = OSQPost(msg_q, msg); // �޽��� ť�� ����
		while (err != OS_NO_ERR) 
		{
			err = OSQPost(msg_q, msg);
		}

		TaskViewDisp(); //���� ä���� ���� ���

		OSSemPost(sem);//�������� ��ȯ

		OSTimeDly(1);

	}
	

}

// ä���� ��� ���� TASK
void TaskFixMachine(void* pdata)
{
	while (1)
	{
		//PC_DispStr(22, 3, "���� �� TaskFixMachine      ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		INT8U err;
		OSSemPend(sem, 0, &err);//�������� ȹ��
		INT8U i, j;
		for (i = 0; i < AREA_CNT; i++) {
			for (j = 0; j < MACHINE_CNT; j++) {
				if (BitcoinMachineInfo[i][j].state == ERROR) {
					BitcoinMachineInfo[i][j].state = NORMAL;
					BitcoinMachineInfo[i][j].color = DISP_FGND_GREEN;
				}
			}
		}

		OSTaskResume(TASK_PRIO / 4);//�޽��� ť ��� ������ ����� TASK �����
		TaskViewDisp();//���� ä���� ���� ���÷��̿� ���

		OSSemPost(sem);//�������� ��ȯ
		//PC_DispStr(22, 3, "���� �� TaskFixMachine      ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		OSTimeDly(rand() % 22);//ä���⸦ ������ Ÿ�̹��� �����ִµ� ���÷��̿� �ִ������� ���� �� �ִ� �� �̳��� ���� ������ ��Ÿ���� �� �Ŀ��� ����
	}

}



//��ü ��Ʈ���� ä���� ���� ���� �� ȭ�� ����
void TaskMachineCrate (void *pdata)
{

	INT8U msg[40];
	INT8U i;
	INT16S key;
	INT8U err;
	TaskStartDispInit();// ä���� �ʱ�ȭ�� ���

	OSSemPend(sem, 0, &err);//������� ȹ��
	TaskMachineMake();// ä���� �����ϰ� ������·� �ʱ�ȭ
	OSSemPost(sem);//�������� ��ȯ

	TaskViewDisp(); //���� ä���� ���� ���

	for (;;) {
		//PC_DispStr(22, 3, "TaskMachineCrate          ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
		if (PC_GetKey(&key)) {                              //See if key has been pressed            
            if (key == 0x1B) {                              //Yes, see if it's the ESCAPE key 
                exit(0);                                    //Yes, return to DOS
            }
        }
		 
        PC_GetDateTime(msg);// ���� �ð� ���� ����
        PC_DispStr(60, 24, msg, DISP_FGND_YELLOW + DISP_BGND_BLUE);// �ð� ���� ���   

		TaskViewDisp();//���� ä���� ���� ���

		OSTimeDly(1);

    }

}




//��Ʈ����ä���� ���� �ʱ�ȭ
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




//���� ä���� ���� �׸���
static void TaskViewDisp()
{
	INT8U i, j;	

	for(i = 0; i < AREA_CNT; i++){
		for(j = 0; j < MACHINE_CNT; j++){
			if( BitcoinMachineInfo[i][j].state == ERROR || BitcoinMachineInfo[i][j].state == NORMAL){
				PC_DispStr(BitcoinMachineInfo[i][j].posX, BitcoinMachineInfo[i][j].posY, "��", BitcoinMachineInfo[i][j].color);
			}
		}
	}

}





//�ʱ�ȭ�� �׸���
void  TaskStartDispInit()
{
	PC_DispStr(0, 0,  "  -------------------------------                     _________________________  ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 1,  "  |��Ʈ���� ä���� ���� ���α׷�|                    |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 2,  "  -------------------------------                    |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 3,  "                                                     |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 4,  "                                                     |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 5,  "  1����                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 6,  "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 7,  "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 8,  "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 9,  "  2����                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 10, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 11, "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 12, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 13, "  3����                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 14, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 15, "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 16, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 17, "  4����                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 18, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 19, "  |                                              |   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 20, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
    PC_DispStr(0, 21, "  5����                                              |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 22, "  ------------------------------------------------   |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 23, "  |                                              |   |_________________________| ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	PC_DispStr(0, 24, "  ------------------------------------------------                               ", DISP_FGND_GRAY + DISP_BGND_BLACK);
	//PC_DispStr(0, 3,  "           ���� TASK:                                |                         | ", DISP_FGND_GRAY + DISP_BGND_BLACK);
}																	    
