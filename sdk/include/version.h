#ifndef _HUGEIC_SDK_VER_H_
#define _HUGEIC_SDK_VER_H_
#include "svn_version.h"
#include "app_version.h"

#define SDK_MVER 2  /*sdk main version*/
#define SDK_BVER 7  /*sdk branch version*/
#define SDK_PVER 0  /*sdk patch version*/

#define SDK_VERSION (SDK_MVER<<24|SDK_BVER<<16|SDK_PVER<<8|PROJECT_TYPE)

#define _V_S1_(R) #R
#define _V_S2_(R) _V_S1_(R)

#ifndef SVN_VERSION
#error "SVN_VERSION not defined"
#endif

#ifndef APP_VERSION
#error "APP_VERSION not defined"
#endif

#define VERSION_STR "\r\n** TXSDK-v"_V_S2_(SDK_MVER)"."_V_S2_(SDK_BVER)"."_V_S2_(SDK_PVER)"."_V_S2_(PROJECT_TYPE)"-"_V_S2_(SVN_VERSION)", app-"_V_S2_(APP_VERSION)", build time:"__DATE__" "__TIME__" **\r\n"
#define VERSION_SHOW() _os_printf(VERSION_STR);
#ifndef MODULE_VERSION
#define MODULE_VERSION(name) __at_section(".modver") \
                             const char *modver_##name = #name" v"\
                             _V_S2_(SDK_MVER)"."_V_S2_(SDK_BVER)"."_V_S2_(SDK_PVER)"-"_V_S2_(SVN_VERSION)", build time:"__DATE__" "__TIME__
#endif
#endif

