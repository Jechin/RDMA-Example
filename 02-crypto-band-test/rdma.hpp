#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <errno.h>
#include <malloc.h>
#include <ctime>
#include <fstream> 
#include <iostream>
#include "minilogger.h"
#include "crypto.hpp"

#define TEST_NZ(x) do { if ( (x)) die("\033[33merror\33[0m: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("\033[33merror\33[0m: " #x " failed (returned zero/null)."); } while (0)

typedef void (*completion_fn)(struct ibv_wc *wc);
static completion_fn on_completion_fn = NULL;
static const int RDMA_BUFFER_SIZE = 1024;
static struct context *s_ctx = NULL;
enum message_type {
    MSG_MR,
    MSG_RCV,
    MSG_DONE
};

struct message {
    enum message_type type;

    union {
        struct ibv_mr mr;
    } data;
};

struct context {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *comp_channel;

    pthread_t cq_poller_thread;
};

struct connection {
    struct rdma_cm_id *id;
    struct ibv_qp *qp;
    Crypto crypto;

    int connected;

    struct ibv_mr *recv_mr;
    struct ibv_mr *send_mr;
    struct ibv_mr *rdma_local_mr;
    struct ibv_mr *rdma_remote_mr;
    struct ibv_mr *buffer_mr;

    struct ibv_mr peer_mr;

    struct message *recv_msg;
    struct message *send_msg;

    char *rdma_local_region;
    char *rdma_remote_region;

    char *buffer;

    // enum {
    //     SS_INIT,
    //     SS_MR_SENT,
    //     SS_RDMA_SENT,
    //     SS_DONE_SENT
    // } send_state;

    // enum {
    //     RS_INIT,
    //     RS_MR_RECV,
    //     RS_DONE_RECV
    // } recv_state;
};

void die(const char *reason);
struct rdma_event_channel *create_event_channel();
struct rdma_cm_id *create_id(struct rdma_event_channel *ec, void *context, enum rdma_port_space ps);
void build_connection(struct rdma_cm_id *id);
void build_context(struct ibv_context *verbs);
void build_params(struct rdma_conn_param *params);
void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
void * poll_cq(void *);
void register_memory(struct connection *conn);
void post_receives(struct connection *conn);
void send_mr(void *context);
void send_message(struct connection *conn);
char * get_local_message_region(void *context);
void on_completion(struct ibv_wc *wc);
void set_connected(void *context);
void set_completion_fn(completion_fn fn);


void die(const char *reason)
{
    fprintf(stderr, "%s\n", reason);
    printf("\033[31merrON[%d]errMsg[%s] \033[0m\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
}

struct rdma_event_channel *create_event_channel(){
    struct rdma_event_channel *ec = NULL;
    TEST_Z(ec = rdma_create_event_channel());
    // printf("\033[32m[main]\33[0m Created event channel.\n");
    return ec;
}

struct rdma_cm_id *create_id(struct rdma_event_channel *ec, void *context, enum rdma_port_space ps){
    struct rdma_cm_id *id = NULL;
    TEST_NZ(rdma_create_id(ec, &id, context, ps));
    // printf("\033[32m[main]\33[0m Created id.\n");
    return id;
}

void build_connection(struct rdma_cm_id *id)
{
    struct connection *conn;
    struct ibv_qp_init_attr qp_attr;

    build_context(id->verbs);
    build_qp_attr(&qp_attr);

    TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

    
    conn = (struct connection *)malloc(sizeof(struct connection));
    id->context = conn;

    conn->id = id;
    conn->qp = id->qp;

    //   conn->send_state = SS_INIT;
    //   conn->recv_state = RS_INIT;

    conn->connected = 0;

    register_memory(conn);
    post_receives(conn);
}


void build_context(struct ibv_context *verbs)
{
    if (s_ctx) {
        if (s_ctx->ctx != verbs)
        die("cannot handle events in more than one context.");
        return;
    }

    s_ctx = (struct context *)malloc(sizeof(struct context));

    s_ctx->ctx = verbs;

    TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
    TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
    TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
    TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

    TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
}


void build_params(struct rdma_conn_param *params)
{
    memset(params, 0, sizeof(*params));

    params->initiator_depth = params->responder_resources = 1;
    params->rnr_retry_count = 7; /* infinite retry */
}


void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
    memset(qp_attr, 0, sizeof(*qp_attr));

    qp_attr->send_cq = s_ctx->cq;
    qp_attr->recv_cq = s_ctx->cq;
    qp_attr->qp_type = IBV_QPT_RC;

    qp_attr->cap.max_send_wr = 10;
    qp_attr->cap.max_recv_wr = 10;
    qp_attr->cap.max_send_sge = 1;
    qp_attr->cap.max_recv_sge = 1;
}

void * poll_cq(void *ctx)
{
    printf("\033[33m[poll_cq]\033[0m poll_cq start at pid %d\n", getpid());
    struct ibv_cq *cq;
    struct ibv_wc wc;

    while (1) {
        TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
        ibv_ack_cq_events(cq, 1);
        TEST_NZ(ibv_req_notify_cq(cq, 0));
        // printf("\033[33m[poll_cq]\033[0m getting event.............\n");
        while (ibv_poll_cq(cq, 1, &wc)){
            on_completion_fn(&wc);
        }
            
    }

    return NULL;
}

void register_memory(struct connection *conn)
{
    conn->send_msg = (struct message *)malloc(sizeof(struct message));
    conn->recv_msg = (struct message *)malloc(sizeof(struct message));

    conn->rdma_local_region = (char *)malloc(RDMA_BUFFER_SIZE);
    // posix_memalign((void **)(&(conn->rdma_local_region)), sysconf(_SC_PAGESIZE), RDMA_BUFFER_SIZE);
    conn->rdma_remote_region = (char *)malloc(RDMA_BUFFER_SIZE);
    conn->buffer = (char *)malloc(RDMA_BUFFER_SIZE);

    TEST_Z(conn->send_mr = ibv_reg_mr(
        s_ctx->pd, 
        conn->send_msg, 
        sizeof(struct message), 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));

    TEST_Z(conn->recv_mr = ibv_reg_mr(
        s_ctx->pd, 
        conn->recv_msg, 
        sizeof(struct message), 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));

    TEST_Z(conn->rdma_local_mr = ibv_reg_mr(
        s_ctx->pd, 
        conn->rdma_local_region, 
        RDMA_BUFFER_SIZE, 
        IBV_ACCESS_LOCAL_WRITE));

    TEST_Z(conn->rdma_remote_mr = ibv_reg_mr(
        s_ctx->pd, 
        conn->rdma_remote_region, 
        RDMA_BUFFER_SIZE, 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));

    TEST_Z(conn->buffer_mr = ibv_reg_mr(
        s_ctx->pd, 
        conn->buffer, 
        RDMA_BUFFER_SIZE, 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));
}

void post_receives(struct connection *conn)
{
    struct ibv_recv_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    wr.wr_id = (uintptr_t)conn;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    sge.addr = (uintptr_t)conn->recv_msg;
    sge.length = sizeof(struct message);
    sge.lkey = conn->recv_mr->lkey;

    TEST_NZ(ibv_post_recv(conn->qp, &wr, &bad_wr));
}

void send_mr(void *context)
{
    struct connection *conn = (struct connection *)context;
    memcpy(&conn->send_msg->data.mr, conn->rdma_remote_mr, sizeof(struct ibv_mr));
    send_message(conn);
    // printf("\033[32m[main]\033[0m send mr success\n");
}

void send_message(struct connection *conn)
{
    struct ibv_send_wr wr, *bad_wr = NULL;
    struct ibv_sge sge;

    memset(&wr, 0, sizeof(wr));

    wr.wr_id = (uintptr_t)conn;
    wr.opcode = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = IBV_SEND_SIGNALED;

    sge.addr = (uintptr_t)conn->send_msg;
    sge.length = sizeof(struct message);
    sge.lkey = conn->send_mr->lkey;

    while (!conn->connected);
    TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));
    // printf("\033[32m[main]\033[0m send_message start at pid %d\n", getpid());
}

char * get_local_message_region(void *context)
{
    return ((struct connection *)context)->rdma_local_region;
}

char * get_remote_message_region(struct connection *conn)
{
    return conn->rdma_remote_region;
}


void set_connected(void *context){
    ((struct connection *)context)->connected = 1;
    // printf("\033[32m[main]\33[0m Set connected.\n");
}

void set_completion_fn(completion_fn fn){
    on_completion_fn = fn;
}