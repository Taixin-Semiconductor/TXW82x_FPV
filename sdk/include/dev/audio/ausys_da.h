#ifndef __AUSYS_DA_H
#define __AUSYS_DA_H

#ifdef __cplusplus
extern "C" {
#endif



/*!
 * DA platform test mode cmd
 */
enum ausys_da_test {
    AUSYS_DA_TEST_OPEN,
    AUSYS_DA_TEST_CLOSE,
    AUSYS_DA_TEST_PLAY_SINE,
};

int32 ausys_da_test_mode(uint32 cmd, uint32 param1, uint32 param2, uint32 param3);

#ifdef __cplusplus
}
#endif


#endif