#include "rdma.hpp"

const int TIMEOUT_IN_MS = 500; /* ms */
clock_t startTime, endTime, check_time_1, check_time_2;
int write_round;
int all_round;
int check_point;
FILE * file;
Logger<TextDecorator> mylog("logfile.txt","RDMA plaintext Throughput",true,true);
char * logline = (char *)malloc(100);

int on_event_client(struct rdma_cm_event *event);
int on_addr_resolved(struct rdma_cm_id *id);
int on_route_resolved(struct rdma_cm_id *id);
int on_connection(struct rdma_cm_id *id);
int on_disconnect(struct rdma_cm_id *id);
void on_completion(struct ibv_wc *wc);
static void write_remote(struct rdma_cm_id *id, uint32_t len);

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "usage: %s <write_round>\n", argv[0]);
        return 1;
    }
    all_round = atoi(argv[1]);
    write_round = 0;
    check_point = all_round / 10;
    
    file = fopen("sendtext.txt", "r");
    // file_handle = open("sendtext.txt", O_RDONLY);
    struct rdma_event_channel *ec = create_event_channel();
    struct rdma_cm_id *id = create_id(ec, NULL, RDMA_PS_TCP);
    struct addrinfo *addr;
    TEST_NZ(getaddrinfo("192.168.224.129", "13131", NULL, &addr));
    TEST_NZ(rdma_resolve_addr(id, NULL, addr->ai_addr, TIMEOUT_IN_MS));
    freeaddrinfo(addr);

    struct rdma_cm_event *event = NULL;

    while (rdma_get_cm_event(ec, &event) == 0) {
        struct rdma_cm_event event_copy;

        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        if (on_event_client(&event_copy))
            break;
    }

    rdma_destroy_event_channel(ec);
    return 0;
}

int on_event_client(struct rdma_cm_event *event){
    int r = 0;

    if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED)
        r = on_addr_resolved(event->id);
    else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED)
        r = on_route_resolved(event->id);
    else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
        r = on_connection(event->id);
    else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
        r = on_disconnect(event->id);
    else if (event->event == RDMA_CM_EVENT_ADDR_ERROR)
        die("address error");
    else if (event->event == RDMA_CM_EVENT_ROUTE_ERROR)
        die("route error");
    else if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
        die("unexpected connect request");
    else if (event->event == RDMA_CM_EVENT_CONNECT_RESPONSE)
        die("unexpected connect response");
    else if (event->event == RDMA_CM_EVENT_CONNECT_ERROR)
        die("connect error");
    else if (event->event == RDMA_CM_EVENT_UNREACHABLE)
        die("remote unreachable");
    else if (event->event == RDMA_CM_EVENT_REJECTED)
        die("connection rejected");
    else if (event->event == RDMA_CM_EVENT_DEVICE_REMOVAL)
        die("device removal");
    else
        die("unknown event");

    return r;
}

int on_addr_resolved(struct rdma_cm_id *id){
    // printf("\33[32m[main]\33[0m address resolved.\n");
    set_completion_fn(on_completion);
    build_connection(id);
    sprintf(get_local_message_region(id->context), "message from active/client side with pid %d", getpid());
    TEST_NZ(rdma_resolve_route(id, TIMEOUT_IN_MS));
    return 0;
}

int on_route_resolved(struct rdma_cm_id *id){
    // printf("\33[32m[main]\33[0m route resolved.\n");

    struct rdma_conn_param cm_params;
    build_params(&cm_params);
    TEST_NZ(rdma_connect(id, &cm_params));
    return 0;
}

int on_connection(struct rdma_cm_id *id){
    struct connection *conn = (struct connection *)id->context;
    conn->connected = 1;
    send_mr(id->context);
    printf("after send mr, conn->send_msg: %p\n", conn->send_msg);
    printf("\33[32m[main]\33[0m connection established.\n");
    // printf("\33[32m[main]\33[0m local message buffer address: %p\n", get_local_message_region(conn));
    // printf("\33[32m[main]\33[0m remote message buffer address: %p\n", get_remote_message_region(conn));
    // printf("\33[32m[main]\33[0m local message buffer content: %s\n", get_local_message_region(conn));
    // printf("\33[32m[main]\33[0m remote message buffer content: %s\n", get_remote_message_region(conn));
    return 0;
}

int on_disconnect(struct rdma_cm_id *id)
{
    printf("\33[32m[main]\33[0m disconnected.\n");

    rdma_destroy_qp(id);
    rdma_destroy_id(id);
    return 1; /* exit event loop */
}

void on_completion(struct ibv_wc *wc)
{
    struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

    if (wc->status != IBV_WC_SUCCESS) {
        switch(wc->opcode){
            case IBV_WC_RECV:
                printf("\033[31m[error]\033[0m RECV ERROR\n");
                post_receives(conn);
                break;
            case IBV_WC_SEND:
                printf("\033[31m[error]\033[0m SEND ERROR\n");
                break;
            case IBV_WC_RDMA_WRITE:
                printf("\033[31m[error]\033[0m WRITE ERROR\n");
                fread(conn->rdma_local_region, 1, RDMA_BUFFER_SIZE, file);
                fseek(file, 0, SEEK_SET);
                write_remote(conn->id, RDMA_BUFFER_SIZE);
                break;
            default:
                printf("\033[31m[error]\033[0m OTHER ERROR\n");
        }
        
        // die("on_completion: status is not IBV_WC_SUCCESS.");
    }
    if (wc->opcode == IBV_WC_RECV) {
        if (conn->recv_msg->type == MSG_MR){
            memcpy(&conn->peer_mr, &conn->recv_msg->data.mr, sizeof(conn->peer_mr));
            printf("\033[33m[poll_cq]\033[0m got MR message\n");
            printf("\033[33m[poll_cq]\033[0m conn->peer_mr.addr: %p, conn->peer_mr.rkey: %u\n", conn->peer_mr.addr, conn->peer_mr.rkey);
            fread(conn->rdma_local_region, 1, RDMA_BUFFER_SIZE, file);
            startTime = clock();
            check_time_1 = startTime;
            printf("\033[31m[clock]\033[0m startTime: %ld\n", startTime);
            write_remote(conn->id, RDMA_BUFFER_SIZE);
            fseek(file, 0, SEEK_SET);
            post_receives(conn);
        }
        else if (conn->recv_msg->type == MSG_RCV) {
            // printf("\033[33m[poll_cq]\033[0m Server recv data, send again.\n");
            if (!(++write_round % check_point)){
                check_time_2 = clock();
                sprintf(logline, "Time for %d's %d transfers: %fs:", write_round / check_point, check_point, (double)(check_time_2 - check_time_1) / CLOCKS_PER_SEC);
                printf("%s\n", logline);
                mylog.Log(logline);
                check_time_1 = check_time_2;
            }
            if (write_round == all_round){
                endTime = clock();
                printf("\033[33m[poll_cq]\033[0m get 10 rdma write wcs\n");
                printf("\033[31m[clock]\033[0m endTime: %ld\n", endTime);
                printf("time: %f s\n", (double)(endTime - startTime) / CLOCKS_PER_SEC);
                sprintf(logline, "Total time for %d transfers: %fs", write_round, (double)(endTime - startTime) / CLOCKS_PER_SEC);
                mylog.Log(logline);

                conn->send_msg->type = MSG_DONE;
                send_message(conn);
            }else{
                fread(conn->rdma_local_region, 1, RDMA_BUFFER_SIZE, file);
                fseek(file, 0, SEEK_SET);
                write_remote(conn->id, RDMA_BUFFER_SIZE);
                post_receives(conn);
            }
        }
    } else if (wc->opcode == IBV_WC_SEND) {
        printf("\033[33m[poll_cq]\033[0m get send wc\n");
        if (conn->send_msg->type == MSG_DONE)
            rdma_disconnect(conn->id);
    } else if (wc->opcode == IBV_WC_RDMA_WRITE) {
        
    } else if (wc->opcode == IBV_WC_RDMA_READ) {
        printf("\033[33m[poll_cq]\033[0m get rdma read wc\n");
    } else {
        die("on_completion: unknown opcode.");
    }
}

static void write_remote(struct rdma_cm_id *id, uint32_t len){
    struct connection *conn = (struct connection *)id->context;

    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = (uintptr_t)id;
    wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    wr.send_flags = IBV_SEND_SIGNALED;
    wr.imm_data = htonl(len);
    wr.wr.rdma.remote_addr = (uintptr_t)conn->peer_mr.addr;
    wr.wr.rdma.rkey = conn->peer_mr.rkey;
    if (len) {
        
        wr.sg_list = &sge;
        wr.num_sge = 1;

        sge.addr = (uintptr_t)conn->rdma_local_region;
        sge.length = len;
        sge.lkey = conn->rdma_local_mr->lkey;
    }
    TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
}