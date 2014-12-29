#ifndef _GAGENT_CLOUD_H_
#define _GAGENT_CLOUD_H_


unsigned char mqtt_num_rem_len_bytes(const unsigned char *buf);
unsigned short mqtt_parse_rem_len(const unsigned char* buf);

#endif


