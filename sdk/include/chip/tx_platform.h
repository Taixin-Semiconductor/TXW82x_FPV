#ifndef _HUGE_IC_PLATFORM_H_
#define _HUGE_IC_PLATFORM_H_

#define PRO_TYPE_LMAC        1
#define PRO_TYPE_UMAC        2
#define PRO_TYPE_WNB         3
#define PRO_TYPE_UAV         4
#define PRO_TYPE_FMAC        5
#define PRO_TYPE_IOT         6
#define PRO_TYPE_FPV         7
#define PRO_TYPE_MCU         8
#define PRO_TYPE_VIDEO       9

#define PRO_TYPE_QA          66
#define PRO_TYPE_QC          88

#ifdef TXW81X
#include "txw81x/txw81x.h"
#include "txw81x/pin_names.h"
#include "txw81x/io_function.h"
#include "txw81x/adc_voltage_type.h"
#include "txw81x/sysctrl.h"
#include "txw81x/pmu.h"
#include "txw81x/misc.h"
#include "txw81x/boot_lib.h"
#include "txw81x/test_atcmd.h"
#include "txw81x/byteshift.h"
#endif

#ifdef TXW82X
#include "txw82x/txw82x.h"
#include "txw82x/pin_names.h"
#include "txw82x/io_function.h"
#include "txw82x/adc_voltage_type.h"
#include "txw82x/sysctrl.h"
#include "txw82x/pmu.h"
#include "txw82x/rpc.h"
#include "txw82x/misc.h"
#include "txw82x/boot_lib.h"
#include "txw82x/test_atcmd.h"
#endif

#endif
