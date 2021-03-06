

#ifndef __SOUND_MIXART_MIXER_H
#define __SOUND_MIXART_MIXER_H

/* exported */
int mixart_update_playback_stream_level(struct snd_mixart* chip, int is_aes, int idx);
int mixart_update_capture_stream_level(struct snd_mixart* chip, int is_aes);
int snd_mixart_create_mixer(struct mixart_mgr* mgr);

#endif /* __SOUND_MIXART_MIXER_H */
