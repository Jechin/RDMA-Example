#include "rdma.hpp"


#define SERVER_PORT 13131

struct rdma_cm_id *create_id(struct rdma_event_channel *ec, void *context, enum rdma_port_space ps);
void bind_addr(struct rdma_cm_id *id, uint16_t port);
void listen(struct rdma_cm_id *id);
int on_event_server(struct rdma_cm_event *event);
int on_connect_request(struct rdma_cm_id *id);
int on_connection(struct rdma_cm_id *id);
int on_disconnect(struct rdma_cm_id *id);
void on_completion(struct ibv_wc *wc);


int main(){
    struct rdma_event_channel *ec = create_event_channel();
    struct rdma_cm_id *id = create_id(ec, NULL, RDMA_PS_TCP);
    bind_addr(id, SERVER_PORT);
    listen(id);

    struct rdma_cm_event *event = NULL;

    while (rdma_get_cm_event(ec, &event) == 0) {
        struct rdma_cm_event event_copy;

        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        if (on_event_server(&event_copy))
            break;
    }

    rdma_destroy_id(id);
    rdma_destroy_event_channel(ec);

    return 0;
}


void bind_addr(struct rdma_cm_id *id, uint16_t port){
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    TEST_NZ(rdma_bind_addr(id, (struct sockaddr *)&addr));
    // printf("\033[32m[main]\33[0m Bound address.\n");
}

void listen(struct rdma_cm_id *id){
    TEST_NZ(rdma_listen(id, 10));
    printf("\033[32m[main]\33[0m Listening on port %d.\n", ntohs(rdma_get_src_port(id)));
}

int on_event_server(struct rdma_cm_event *event){
    int r = -1;
    if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
        r = on_connect_request(event->id);
    else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
        r = on_connection(event->id);
    else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
        r = on_disconnect(event->id);
    else
        die("\033[32m[main]\33[0m on_event: unknown event.\n");
    return r;
}

int on_connect_request(struct rdma_cm_id *id){
    // printf("\033[32m[main]\33[0m Received connection request.\n");
    set_completion_fn(on_completion);
    build_connection(id);
    struct rdma_conn_param cm_params;
    build_params(&cm_params);
    TEST_NZ(rdma_accept(id, &cm_params));
    sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid());
    // printf("\033[32m[main]\33[0m Accepted connection.\n");
    return 0;
}

int on_connection(struct rdma_cm_id *id){
    struct connection *conn = (struct connection *)id->context;
    conn->connected = 1;
    conn->crypto = Crypto();
    printf("\033[32m[main]\33[0m Connection established.\n");
    send_mr(id->context);
    // printf("\033[32m[main]\33[0m On_connection: connected.\n");
    // printf("\33[32m[main]\33[0m local message buffer address: %p\n", get_local_message_region(conn));
    // printf("\33[32m[main]\33[0m remote message buffer address: %p\n", get_remote_message_region(conn));
    // printf("\33[32m[main]\33[0m local message buffer content: %s\n", get_local_message_region(conn));
    // printf("\33[32m[main]\33[0m remote message buffer content: %s\n", get_remote_message_region(conn));
    return 0;
}

int on_disconnect(struct rdma_cm_id *id){
    printf("\033[32m[main]\33[0m On_disconnect: disconnected.\n");
    rdma_destroy_qp(id);
    rdma_destroy_id(id);
    return 1;
}

void on_completion(struct ibv_wc *wc)
{
    struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

    if (wc->status != IBV_WC_SUCCESS) {
        switch(wc->opcode){
            case IBV_WC_RECV:
                printf("RECV ERROR\n");
                break;
            case IBV_WC_SEND:
                printf("SEND ERROR\n");
                if (conn->send_msg->type == MSG_RCV)
                    send_message(conn);
                break;
            case IBV_WC_RDMA_WRITE:
                printf("WRITE ERROR\n");
                break;
            default:
                printf("OTHER ERROR\n");
        }
        
        // die("on_completion: status is not IBV_WC_SUCCESS.");
    }
    if (wc->opcode == IBV_WC_RECV) {
        if (conn->recv_msg->type == MSG_MR){
            memcpy(&conn->peer_mr, &conn->recv_msg->data.mr, sizeof(conn->peer_mr));
            printf("\033[33m[poll_cq]\033[0m recv mr:conn->peer_mr.addr: %p, conn->peer_mr.rkey: %u\n", conn->peer_mr.addr, conn->peer_mr.rkey);
        }else if (conn->recv_msg->type == MSG_DONE) {
            printf("\033[33m[poll_cq]\033[0m data trasnfer finish.\n");
        }
            
        printf("\033[33m[poll_cq]\033[0m message type %d\n", conn->recv_msg->type);
        // printf("\033[33m[poll_cq]\033[0m recv message: %s\n", conn->recv_msg->data.mr);
        
        // printf("\033[33m[poll_cq]\033[0m conn->peer_mr.addr contect: %s\n", (char *)conn->peer_mr.addr);
        post_receives(conn);
        
    } else if (wc->opcode == IBV_WC_SEND) {
        // printf("\033[33m[poll_cq]\033[0m get send wc\n");
    }else if (wc->opcode == IBV_WC_RECV_RDMA_WITH_IMM){
        // printf("\033[33m[poll_cq]\033[0m get wirte imm data\n");
        // printf("rdma_remote_mr->addr: %p; contect: %s", conn->rdma_remote_mr->addr, (char *)conn->rdma_remote_mr->addr);
        conn->crypto.decrypto((char *)conn->rdma_remote_mr->addr, ntohl(wc->imm_data), conn->buffer);
        printf("\033[33m[poll_cq]\033[0m get wirte plain data: %s\n", conn->buffer);

        conn->send_msg->type = MSG_RCV;
        send_message(conn);
        
        post_receives(conn);
    } else if (wc->opcode == IBV_WC_RDMA_WRITE) {
        printf("\033[33m[poll_cq]\033[0m get rdma write wc\n");
    } else if (wc->opcode == IBV_WC_RDMA_READ) {
        printf("\033[33m[poll_cq]\033[0m get rdma read wc\n");
    } else {
        die("on_completion: unknown opcode.");
    }
}