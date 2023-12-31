/********************************************/
/*  author: Mohamed Walid               	*/
/*  version: V02                            */
/*  Date: 18 NOV 2023	   					*/
/*  SWC : SCHEDULER                			*/
/********************************************/


#include "STD_Types.h"
#include "BIT_MATH.h"

#include "SCHED_int.h"
#include "SCHED_priv.h"
#include "SCHED_config.h"
#include "TIM2_int.h"


static Task_t private_arrTask_tSysTasks[SCHED_PRIOITY_LEVELS_NUMBER][SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL];

TaskAdditionState_t SCHED_enumAddTask(void (*copy_pvTaskFunc)(void*), uint16 copy_uint16Periodicity, uint16 copy_uint16FirstDelay, uint8 copy_uint8Priority, uint8 copy_uint8TaskID){
	
	TIM2_voidDisableOCMInt();
	TaskAdditionState_t local_sint32TaskAdditionState;
	
	if(copy_uint8Priority<SCHED_PRIOITY_LEVELS_NUMBER){						// checking whether the priority is valid or not
		
		for(uint8 i=0;i<SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL;i++){
			
			if(private_arrTask_tSysTasks[copy_uint8Priority][i].taskFunc == NULL ){

				private_arrTask_tSysTasks[copy_uint8Priority][i].taskFunc = copy_pvTaskFunc;
				private_arrTask_tSysTasks[copy_uint8Priority][i].periodicity = copy_uint16Periodicity;
				private_arrTask_tSysTasks[copy_uint8Priority][i].firstDelay = copy_uint16FirstDelay;
				private_arrTask_tSysTasks[copy_uint8Priority][i].taskID = copy_uint8TaskID;

				local_sint32TaskAdditionState = taskAdded;
				if(private_arrTask_tSysTasks[copy_uint8Priority][i].firstDelay==0){
					private_arrTask_tSysTasks[copy_uint8Priority][i].taskState = ready;
				}
				else{
					private_arrTask_tSysTasks[copy_uint8Priority][i].taskState = waiting;
				}
				break;
			}
			else {

				local_sint32TaskAdditionState = taskNotAdded;
			}
		}
	}
	TIM2_voidEnableOCMInt();
	return local_sint32TaskAdditionState;
}

void SCHED_voidDeleteTask(uint8 copy_uint8TaskID){

	TIM2_voidDisableOCMInt();

	for(uint8 i=0;i<SCHED_PRIOITY_LEVELS_NUMBER;i++){

		for(uint8 j=0;j<SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL;j++){

			if(private_arrTask_tSysTasks[i][j].taskID==copy_uint8TaskID){
				private_arrTask_tSysTasks[i][j].taskFunc = NULL;
				break;
			}
		}

	}

	TIM2_voidEnableOCMInt();
}


void SCHED_voidSuspendTask(uint8 copy_uint8TaskID){

	for(uint8 i=0;i<SCHED_PRIOITY_LEVELS_NUMBER;i++){

		for(uint8 j=0;j<SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL;j++){

			if(private_arrTask_tSysTasks[i][j].taskID==copy_uint8TaskID){
				private_arrTask_tSysTasks[i][j].taskState = suspended;
				break;
			}
		}

	}

}


void SCHED_voidResumeTask(uint8 copy_uint8TaskID){

	for(uint8 i=0;i<SCHED_PRIOITY_LEVELS_NUMBER;i++){

		for(uint8 j=0;j<SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL;j++){

			if(private_arrTask_tSysTasks[i][j].taskID==copy_uint8TaskID){
				private_arrTask_tSysTasks[i][j].taskState = ready;
				break;
			}
		}
	}
}

static void SCHED_voidOrganize(void){

	for(uint8 i=0;i<SCHED_PRIOITY_LEVELS_NUMBER;i++){

		for(uint8 j=0;j<SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL;j++){

			if(private_arrTask_tSysTasks[i][j].taskFunc != NULL){

				if(private_arrTask_tSysTasks[i][j].taskState == ready){

					private_arrTask_tSysTasks[i][j].taskState = running;
					private_arrTask_tSysTasks[i][j].taskFunc(NULL);
					private_arrTask_tSysTasks[i][j].firstDelay = private_arrTask_tSysTasks[i][j].periodicity-SCHED_TICK_TIME_IN_MILLI_SEC;
					private_arrTask_tSysTasks[i][j].taskState = waiting;

				}else if(private_arrTask_tSysTasks[i][j].taskState == waiting){

					private_arrTask_tSysTasks[i][j].firstDelay-=SCHED_TICK_TIME_IN_MILLI_SEC;

					if(private_arrTask_tSysTasks[i][j].firstDelay==0){

						private_arrTask_tSysTasks[i][j].taskState = ready;

					}
				}
			}
		}
	}
}

void SCHED_voidInit(void){

#if	SCHED_TICK_TIME_IN_MILLI_SEC > 32

#error "TICK TIME is not valid"

#elif	(SCHED_TICK_TIME_IN_MILLI_SEC > 8) && (SCHED_SELECTED_PRESCALER_VALUE < 1024)

#error "TICK TIME and prescaler value aren't appropriate"

#elif  	(SCHED_TICK_TIME_IN_MILLI_SEC > 4) && (SCHED_SELECTED_PRESCALER_VALUE < 256)

#error "TICK TIME and prescaler value aren't appropriate"

#elif  	(SCHED_TICK_TIME_IN_MILLI_SEC > 2) && (SCHED_SELECTED_PRESCALER_VALUE < 128)

#error "TICK TIME and prescaler value aren't appropriate"

#elif  	(SCHED_TICK_TIME_IN_MILLI_SEC > 1) && (SCHED_SELECTED_PRESCALER_VALUE < 64)

#error "TICK TIME and prescaler value aren't appropriate"

#elif  	SCHED_SELECTED_PRESCALER_VALUE < 32

#error "Prescaler value isn't valid"

#endif

	for(uint8 i=0;i<SCHED_PRIOITY_LEVELS_NUMBER;i++){

		for(uint8 j=0;j<SCHED_MAX_TASKS_NUMBER_AT_THE_SAME_LEVEL;j++){

				private_arrTask_tSysTasks[i][j].taskFunc = NULL;
		}

	}

	TIM2_voidInit();
	TIM2_voidSetCompareMatchValue((((uint32)SCHED_TICK_TIME_IN_MILLI_SEC * SCHED_INPUT_FREQ_IN_KHZ)/SCHED_SELECTED_PRESCALER_VALUE));
	TIM2_voidSetCallBackFunc(&SCHED_voidOrganize);
}

void SCHED_voidStart(void){

	TIM2_voidEnableOCMInt();
	TIM2_voidStart();
}
