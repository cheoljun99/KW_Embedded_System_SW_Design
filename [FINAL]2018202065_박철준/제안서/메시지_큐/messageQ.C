#include "includes.h"
#include <time.h>
#define TASK_STK_SIZE 512
#define N_TASKS 2
#define N_MSG 100
OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];
OS_STK LogTaskStk[TASK_STK_SIZE];
OS_EVENT* msg_q; // (1)
void* msg_array[N_MSG]; // (2)
void LogTask(void* data);
void Task(void* data);
void CreateTasks(void);

int main(void)
{
	OSInit();
	CreateTasks(); // (3)
	msg_q = OSQCreate(msg_array, (INT16U)N_MSG); // (4)
	if (msg_q == 0)
	{
		printf("creating msg_q is failed\n");
		return -1;
	}
	OSStart();
	return 0;
}
void CreateTasks(void) // (5)
{
	OSTaskCreate(LogTask, (void*)0, &LogTaskStk[TASK_STK_SIZE - 1], (INT8U)(0));
	OSTaskCreate(Task, (void*)0, &TaskStk[0][TASK_STK_SIZE - 1], (INT8U)(10));
	OSTaskCreate(Task, (void*)0, &TaskStk[1][TASK_STK_SIZE - 1], (INT8U)(20));
}

void LogTask(void* pdata) // (6)
{
	FILE* log;
	void* msg;
	INT8U err;
	log = fopen("log.txt", "w"); // (7)
	for (;;) {
		msg = OSQPend(msg_q, 0, &err); // (8)
		if (msg != 0)
		{
			fprintf(log, "%s", msg); // (9)
			fflush(log);
		}
	}
}

void Task(void* pdata) // (10)
{
	INT8U sleep, err;
	char msg[100];
	srand(time((unsigned int*)0) + (OSTCBCur->OSTCBPrio)); // (11)
	for (;;) {
		sprintf(msg, "%4u: Task %u schedule\n", OSTimeGet(),
			OSTCBCur->OSTCBPrio); // (12)
		err = OSQPost(msg_q, msg); // (13)
		while (err != OS_NO_ERR)
		{
			err = OSQPost(msg_q, msg);
		}
		sleep = (rand() % 5) + 1;
		OSTimeDly(sleep); // (14)
	}
}