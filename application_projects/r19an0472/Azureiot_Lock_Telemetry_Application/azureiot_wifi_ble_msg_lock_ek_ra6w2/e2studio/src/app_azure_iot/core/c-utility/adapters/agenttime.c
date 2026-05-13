// Copyright (c) Microsoft. All rights reserved.
// Modifications Copyright (c) 2026 Renesas Electronics Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <time.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h"
//AZURE_CHANGES[[::
#include "r_rtc_w_helper.h"
//AZURE_CHANGES]]

time_t get_time(time_t* p)
{
//AZURE_CHANGES azurefrtoswork[[::
	__time64_t now;
	ra6w1_time64(p, &now);
	return (time_t)now;
   // return time(p);
//AZURE_CHANGES azurefrtoswork]]
}

struct tm* get_gmtime(time_t* currentTime)
{
    return gmtime(currentTime);
}

time_t get_mktime(struct tm* cal_time)
{
    return mktime(cal_time);
}

char* get_ctime(time_t* timeToGet)
{
    return ctime(timeToGet);
}

double get_difftime(time_t stopTime, time_t startTime)
{
    return difftime(stopTime, startTime);
}
