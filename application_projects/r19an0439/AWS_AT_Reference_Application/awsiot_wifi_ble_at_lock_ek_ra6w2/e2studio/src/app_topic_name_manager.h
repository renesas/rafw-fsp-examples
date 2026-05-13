/***********************************************************************************************************************
 * File Name    : app_topic_name_manager.h
 * Description  : Define topic related APIs used to operate device on AWS platform.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#if !defined(_APP_AWS_TOPIC_NAME_MANAGER_H_)
#define _APP_AWS_TOPIC_NAME_MANAGER_H_

#include "app_aws_user_conf.h"

void makeAWSTopicName(void);
char* getAWSCheckPubName();
char* getAWSCommandSubName();
char* getAWSCommandPubName();

#endif /* _APP_AWS_TOPIC_NAME_MANAGER_H_ */
