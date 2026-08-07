#ifndef _BABYPROCOL_PLAYBACK_H_
#define _BABYPROCOL_PLAYBACK_H_

extern int32_t client_remote_playback_init(const char *filename);
extern int32_t client_remote_playback_deinit(void);

extern int32_t server_remote_playback_init(void);
extern int32_t server_remote_playback_deinit(void);

#endif