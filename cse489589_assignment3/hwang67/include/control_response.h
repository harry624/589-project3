#ifndef CONTROL_RESPONSE_H_
#define CONTROL_RESPONSE_H_

void init_response(int sock_index, char* cntrl_payload);
void routing_table_response(int sock_index, char* cntrl_payload);
void update_response(int sock_index, char* cntrl_payload);
void sendfile_response(int sock_index, char* cntrl_payload);
void sendfile_stats_response(int sock_index, char* cntrl_payload);
void last_data_packet_response(int sock_index, char* cntrl_payload);
void penultimate_data_packet_response(int sock_index, char* cntrl_payload);



#endif
