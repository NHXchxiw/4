#ifndef _APP_H_
#define _APP_H_
#include "includes.h"


/*
*********************************************************************************************************
*                                        FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void TaskCreate(void);
void TaskStart(void *pdata);
void TaskPPM(void *pdata);
void TaskMotor(void *pdata);
void TaskGY86(void *pdata);

#endif /* _APP_H_ */
