/* Copyright (c) 2017 - 2020 LiteSpeed Technologies Inc.  See LICENSE. */
/*
 * http_client.c -- A simple HTTP/QUIC client
 */

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#else
#include <Windows.h>
#include <WinSock2.h>
#include <io.h>
#include <stdlib.h>
#include <getopt.h>
#define STDOUT_FILENO 1
#define random rand
#pragma warning(disable:4996) //POSIX name deprecated
#endif
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <event2/event.h>
#include <math.h>
#include <stdbool.h>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "lsquic.h"
#include "test_common.h"
#include "prog.h"

#include "../src/liblsquic/lsquic_logger.h"
#include "../src/liblsquic/lsquic_int_types.h"
#include "../src/liblsquic/lsquic_util.h"
/* include directly for reset_stream testing */
#include "../src/liblsquic/lsquic_varint.h"
#include "../src/liblsquic/lsquic_hq.h"
#include "../src/liblsquic/lsquic_sfcw.h"
#include "../src/liblsquic/lsquic_hash.h"
#include "../src/liblsquic/lsquic_stream.h"
/* include directly for retire_cid testing */
#include "../src/liblsquic/lsquic_conn.h"
#include "lsxpack_header.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* This is used to exercise generating and sending of priority frames */
static int randomly_reprioritize_streams;

static int s_display_cert_chain;

/* If this file descriptor is open, the client will accept server push and
 * dump the contents here.  See -u flag.
 */
static int promise_fd = -1;

/* Set to true value to use header bypass.  This means that the use code
 * creates header set via callbacks and then fetches it by calling
 * lsquic_stream_get_hset() when the first "on_read" event is called.
 */
static int g_header_bypass;

static int s_discard_response;

// Minh ADD-S
#define MAX_SEGMENT_ID  200
#define MAX_LAYER_ID    4   // 1 BL + 3 ELs
#define TRUE            true
#define FALSE           false

const int   MINH_BITRATE_SET[] = {1500, 2000, 3500, 5000};   //Kbps
const int   MINH_SUM_BITRATE_SET[] = {1500, 3500, 7500, 12500}; //Kbps
char*       MINH_PATH_SET[] = {"/file-187K", "/file-250K", "/file-437K", "/file-625K"};

#define   MINH_BUFFER_SIZE          20000
#define   MINH_REBUF_THRESHOLD_EXIT 5000
#define   MINH_SD                   1000

const int   RETRANS_BUFF_TRIGGER_ON = MINH_BUFFER_SIZE/2;
const int   RETRANS_BUFF_THRES = MINH_BUFFER_SIZE/4;

// data record
// lsquic_time_t   start_stream_recorder   [MAX_SEGMENT_ID];           
// long double     download_time_recorder  [MAX_SEGMENT_ID];
// double          throughput_recorder     [MAX_SEGMENT_ID];    // in Kbps
// double          buffer_recorder         [MAX_SEGMENT_ID]; // in s
// int             bitrate_recorder        [MAX_SEGMENT_ID][2];
int             retrans_seg_id_recorder [MAX_SEGMENT_ID];

lsquic_time_t   streaming_start_time;

static int      minh_client_seg = 0;
static int      minh_retrans_seg = -1;
static double   minh_cur_buf = 0;   // in milisecon
static double   minh_throughput = 0; // in Kbps
static double   estimated_throughput = 0;

static bool     minh_rebuf = TRUE;
static bool     retrans_check = FALSE;
static bool     set_prior_check = FALSE;
static bool     minh_retrans_trigger = FALSE;
static bool     minh_retrans_extension = TRUE;
static bool     retransmitting = FALSE;
static bool     termination_check = FALSE;

static char*    next_path = "/file-187K";
static char*    retrans_path;

static unsigned termn_seg_id = 0;
// int             seg_id_stream_id_map[MAX_SEGMENT_ID*2][2]; //[seg_id, stream_id]

static int      priority_next = 2;
static int      priority_retrans = 8;
// static int      retrans_num_segs = 0;
static int      retrans_seg_id = 0; 
static int      cur_layer_id = 1;

static unsigned long    downloaded_bytes_2on_close = 0;
static unsigned long    retrans_stream_id = 0;
static unsigned long    next_stream_id = 0;

struct Seg_layers   // a stream = 1 layer
{
   int              bitrate;
   unsigned long    stream_id;
   double           throughput;
   double           buffer;

   long double      start_download_time;
   long double      download_time;
};

struct Segments
{
    int                 num_layers;
    struct Seg_layers   layer[MAX_LAYER_ID];
    // int         id;
    

};

struct lsquic_stream_ctx {
    lsquic_stream_t     *stream;
    struct http_client_ctx   *client_ctx;
    const char          *path;
    enum {
        HEADERS_SENT    = (1 << 0),
        PROCESSED_HEADERS = 1 << 1,
    }                    sh_flags;
    lsquic_time_t        sh_created;
    lsquic_time_t        sh_ttfb;
    unsigned             count;
    struct lsquic_reader reader;
};

struct Segments segment[MAX_SEGMENT_ID];

static void
minh_AGG_ABR(){
    printf("AGG ABR Starts\n");
    printf("cur_layer_id = %d\n", cur_layer_id);
    int i;

    if (minh_rebuf && minh_cur_buf >= MINH_REBUF_THRESHOLD_EXIT)    // stop rebuffering
    {
        printf("AGG ABR 1\n");
        minh_rebuf = false;
    }

    if (minh_cur_buf < MINH_SD || minh_rebuf) // still or start rebuffering ==> go to the next segment and choose BL
    {  
        printf("AGG ABR 2\n");
        if (!minh_rebuf)
        {
            minh_rebuf = true;
            minh_cur_buf = 0;
        }

        cur_layer_id = 1;
        minh_client_seg ++;
        segment[minh_client_seg].num_layers = 1;

    }
    else // normal
    {
        printf("minh_client_seg: %d. num_layers: %d\n", minh_client_seg, segment[minh_client_seg].num_layers);
        if (cur_layer_id == segment[minh_client_seg].num_layers){ // go to next request.
            printf("AGG ABR 3\n");
            minh_client_seg ++;
            cur_layer_id = 1;

            for (i = MAX_LAYER_ID; i >=1; i--){
                if (MINH_SUM_BITRATE_SET[i] < estimated_throughput){
                    segment[minh_client_seg].num_layers = i;
                    break;
                }
            }
        }
        else{
            printf("AGG ABR 4\n");
            cur_layer_id ++;
        }        
    }

    segment[minh_client_seg].layer[cur_layer_id-1].bitrate = MINH_BITRATE_SET[cur_layer_id-1];
    // segment[minh_client_seg].layer[cur_layer_id-1].start_download_time = lsquic_time_now();

    next_path = MINH_PATH_SET[cur_layer_id-1];
}

static void 
minh_retransmission_technique(){
    // if (minh_client_seg == 4){
    //     printf("RETRANS\n");
    //     retrans_check = true;
    //     set_prior_check = true;
    // }

    // trigger retrans
    if (estimated_throughput > MINH_SUM_BITRATE_SET[segment[minh_client_seg].num_layers] &&
        minh_retrans_trigger == FALSE &&
        minh_client_seg > MINH_REBUF_THRESHOLD_EXIT &&
        minh_cur_buf >= RETRANS_BUFF_TRIGGER_ON)
    {
        minh_retrans_trigger = TRUE;
        printf("--- TRIGGER ON ---\n");
    }
    else if (minh_cur_buf < RETRANS_BUFF_THRES)
    {
        minh_retrans_trigger = FALSE;
    }

    // without retransmission No retrans
    if (minh_retrans_extension == FALSE)
    {
        minh_retrans_trigger = FALSE;
        printf("-------------------- RETRANS. IS DISABLE ----------------------\n");
    }

    // if retrans is ON
    if (minh_retrans_trigger){
        if (cur_layer_id == 1)  // just jump to the next segment. ??? Chi check retrans trong truong hop nay?
        { 
            // find the highest gap amplitude
            int i;
            int m_gap_amplitude = 0;
            int m_retrans_seg_id = 0;

            bool found_a_gap = FALSE;
            for (i = minh_client_seg - (int) minh_cur_buf + 2; i < minh_client_seg; i++)
            {
                // if num_layers khac vs i-1, i+1 thi la gap

                if (i == minh_client_seg-1)
                {
                    if (segment[i-1].num_layers - segment[i].num_layers > m_gap_amplitude)
                    {
                        m_gap_amplitude = segment[i-1].num_layers - segment[i].num_layers;
                        m_retrans_seg_id = i;
                        found_a_gap = TRUE;

                        printf("# layers of current segment:%d\n", segment[i].num_layers);
                        printf("# layers of previous segment:%d\n", segment[i-1].num_layers);                        
                    }                 
                }
                else
                {
                    if (segment[i-1].num_layers - segment[i].num_layers > m_gap_amplitude || 
                        segment[i+1].num_layers - segment[i].num_layers > m_gap_amplitude)
                    {
                        m_gap_amplitude = MAX(segment[i-1].num_layers - segment[i].num_layers,
                                              segment[i+1].num_layers - segment[i].num_layers);
                        m_retrans_seg_id = i;
                        found_a_gap = TRUE;

                        printf("# layers of latter segment:%d\n", segment[i+1].num_layers);
                        printf("# layers of current segment:%d\n", segment[i].num_layers);
                        printf("# layers of previous segment:%d\n", segment[i-1].num_layers);                        
                    } 
                }

                if(found_a_gap)
                {
                    // tinh t^a, T^R, B^e
                    int retrans_rate = MINH_BITRATE_SET[segment[m_retrans_seg_id].num_layers];
                    double t_avai = (m_retrans_seg_id -(minh_client_seg-1))*MINH_SD + minh_cur_buf;
                    double retrans_throughput = MINH_SD*retrans_rate / t_avai;
                    double estimated_buffer = minh_cur_buf + MINH_SD*(1 - (MINH_BITRATE_SET[0] + retrans_rate)/estimated_throughput);
                    const int THETA = MINH_BUFFER_SIZE/2;
                    if (retrans_throughput < estimated_throughput &&
                        estimated_buffer > THETA)
                    {
                        retrans_path = MINH_PATH_SET[segment[m_retrans_seg_id].num_layers];
                        double division_factor = MAX(retrans_throughput/256, 
                                                        (estimated_buffer-retrans_throughput)/256);
                        assert(division_factor != 0);

                        priority_retrans = (int) retrans_throughput / division_factor;
                        priority_next    = (int) (estimated_buffer-retrans_throughput) / division_factor;

                        assert(priority_retrans >= 1 && priority_retrans <= 256 &&
                               priority_next >= 1 && priority_next <= 256);

                        retrans_check = TRUE;
                        retransmitting = TRUE;
                        minh_retrans_seg ++;
                        retrans_seg_id = m_retrans_seg_id;
                        retrans_seg_id_recorder[minh_retrans_seg] = m_retrans_seg_id;
                    }
                }
            }
        }
    }
}

static void
minh_update_stats(lsquic_stream_t *stream, lsquic_stream_ctx_t *st_h, int m_segment_id, bool retrans_layer){
    long double m_dowload_time = (long double) (lsquic_time_now()- st_h->sh_created)/1000; //ms
    int layer_id = 0;

    if (m_segment_id == 0){segment[m_segment_id].num_layers = 1;}

    if (retrans_layer)  // this layer is retransmitted
    {
        if (lsquic_stream_id(stream) == termn_seg_id)
        {
            printf("TERMINATION: this segment was requested for termination\n");
            return;
        }
        else
        {
            segment[m_segment_id].num_layers ++;
            layer_id = segment[m_segment_id].num_layers;
        }

        printf("***Note: Update stats: retransmitted layer\n");
    }
    else            // this layer is current/next segment
    {
        layer_id = cur_layer_id;
        if (minh_rebuf)
        {
            minh_cur_buf += MINH_SD;
        }
        else if (layer_id == 1) // this is the first layer of next segment
        {
            minh_cur_buf = (minh_cur_buf + MINH_SD - m_dowload_time < 0) ? 0 :
                            minh_cur_buf + MINH_SD - m_dowload_time;
        }
        else
        {
            minh_cur_buf = (minh_cur_buf - m_dowload_time < 0) ? 0 :
                            minh_cur_buf - m_dowload_time;
        }

        segment[m_segment_id].layer[layer_id-1].buffer = minh_cur_buf;
    }

    minh_throughput = (downloaded_bytes_2on_close > 91) ?
                                (downloaded_bytes_2on_close-91)*8.0/m_dowload_time :
                                minh_throughput ;// Kbps
    estimated_throughput = minh_throughput;

    // recorder
    segment[m_segment_id].layer[layer_id-1].bitrate = MINH_BITRATE_SET[layer_id-1];
    segment[m_segment_id].layer[layer_id-1].throughput = minh_throughput;
    segment[m_segment_id].layer[layer_id-1].download_time = m_dowload_time;  
    segment[m_segment_id].layer[layer_id-1].start_download_time = (long double) (st_h->sh_created - streaming_start_time)/1000;
    

    // recorder
    // download_time_recorder[minh_client_seg] = m_dowload_time;
    // buffer_recorder[minh_client_seg] = minh_cur_buf;
    // throughput_recorder[minh_client_seg] = minh_throughput;

    downloaded_bytes_2on_close = 0;

    printf("\tUpdate stats: Segment %d current buff %.0f ms \tthroughput %.3f kbps in %.0Lf ms. Estimate throughput: %.3f\n", 
                                m_segment_id, minh_cur_buf, minh_throughput, m_dowload_time, estimated_throughput);

}

// static void 
// minh_terminate_stream(lsquic_stream_t *stream, lsquic_stream_ctx_t *st_h){
//     long double inst_download_time = (long double) (lsquic_time_now() - st_h->sh_created)/1000000;
//     double inst_buf = (minh_cur_buf - inst_buf) > 0 ?
//                        minh_cur_buf - inst_buf :
//                        0;
//     printf("instant buffer = %.3f of stream id: %"PRIu64"\n", inst_buf, lsquic_stream_id(stream));
// }
// Minh ADD-E

struct sample_stats
{
    unsigned        n;
    unsigned long   min, max;
    unsigned long   sum;        /* To calculate mean */
    unsigned long   sum_X2;     /* To calculate stddev */
};

static struct sample_stats  s_stat_to_conn,     /* Time to connect */
                            s_stat_ttfb,
                            s_stat_req;     /* From TTFB to EOS */
static unsigned s_stat_conns_ok, s_stat_conns_failed;
static unsigned long s_stat_downloaded_bytes;

static void
update_sample_stats (struct sample_stats *stats, unsigned long val)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    LSQ_DEBUG("%s: %p: %lu", __func__, stats, val);

    if (stats->n)
    {
        if (val < stats->min)
            stats->min = val;
        else if (val > stats->max)
            stats->max = val;
    }
    else
    {
        stats->min = val;
        stats->max = val;
    }
    stats->sum += val;
    stats->sum_X2 += val * val;
    ++stats->n;
}


// static void
// calc_sample_stats (const struct sample_stats *stats,
//         long double *mean_p, long double *stddev_p)
// {
//     // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
//     unsigned long mean, tmp;

//     if (stats->n)
//     {
//         mean = stats->sum / stats->n;
//         *mean_p = (long double) mean;
//         if (stats->n > 1)
//         {
//             tmp = stats->sum_X2 - stats->n * mean * mean;
//             tmp /= stats->n - 1;
//             *stddev_p = sqrtl((long double) tmp);
//         }
//         else
//             *stddev_p = 0;
//     }
//     else
//     {
//         *mean_p = 0;
//         *stddev_p = 0;
//     }
// }


#ifdef WIN32
static char *
strndup(const char *s, size_t n)
{
    char *copy;

    copy = malloc(n + 1);
    if (copy)
    {
        memcpy(copy, s, n);
        copy[n] = '\0';
    }

    return copy;
}
#endif

struct lsquic_conn_ctx;

struct path_elem {
    TAILQ_ENTRY(path_elem)      next_pe;
    const char                 *path;
};

struct http_client_ctx {
    TAILQ_HEAD(, lsquic_conn_ctx)
                                 conn_ctxs;
    const char                  *hostname;
    const char                  *method;
    const char                  *payload;
    char                         payload_size[20];

    /* hcc_path_elems holds a list of paths which are to be requested from
     * the server.  Each new request gets the next path from the list (the
     * iterator is stored in hcc_cur_pe); when the end is reached, the
     * iterator wraps around.
     */
    TAILQ_HEAD(, path_elem)      hcc_path_elems;
    struct path_elem            *hcc_cur_pe;

    unsigned                     hcc_total_n_reqs;
    unsigned                     hcc_reqs_per_conn;
    unsigned                     hcc_concurrency;
    unsigned                     hcc_cc_reqs_per_conn;
    unsigned                     hcc_n_open_conns;
    unsigned                     hcc_reset_after_nbytes;
    unsigned                     hcc_retire_cid_after_nbytes;
    
    char                        *hcc_zero_rtt_file_name;

    enum {
        HCC_SKIP_0RTT           = (1 << 0),
        HCC_SEEN_FIN            = (1 << 1),
        HCC_ABORT_ON_INCOMPLETE = (1 << 2),
    }                            hcc_flags;
    struct prog                 *prog;
    const char                  *qif_file;
    FILE                        *qif_fh;
};

struct lsquic_conn_ctx {
    TAILQ_ENTRY(lsquic_conn_ctx) next_ch;
    lsquic_conn_t       *conn;
    struct http_client_ctx   *client_ctx;
    lsquic_time_t        ch_created;
    unsigned             ch_n_reqs;    /* This number gets decremented as streams are closed and
                                        * incremented as push promises are accepted.
                                        */
    unsigned             ch_n_cc_streams;   /* This number is incremented as streams are opened
                                             * and decremented as streams are closed. It should
                                             * never exceed hcc_cc_reqs_per_conn in client_ctx.
                                             */
    enum {
        CH_ZERO_RTT_SAVED   = 1 << 0,
    }                    ch_flags;
};


struct hset_elem
{
    STAILQ_ENTRY(hset_elem)     next;
    struct lsxpack_header       xhdr;
};


STAILQ_HEAD(hset, hset_elem);

static void
hset_dump (const struct hset *, FILE *);
static void
hset_destroy (void *hset);
static void
display_cert_chain (lsquic_conn_t *);


static void
create_connections (struct http_client_ctx *client_ctx)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    size_t len;
    FILE *file;
    unsigned char zero_rtt[0x2000];

    if (0 == (client_ctx->hcc_flags & HCC_SKIP_0RTT)
                                    && client_ctx->hcc_zero_rtt_file_name)
    {
        file = fopen(client_ctx->hcc_zero_rtt_file_name, "rb");
        if (!file)
        {
            LSQ_DEBUG("cannot open %s for reading: %s",
                        client_ctx->hcc_zero_rtt_file_name, strerror(errno));
            goto no_file;
        }
        len = fread(zero_rtt, 1, sizeof(zero_rtt), file);
        if (0 == len && !feof(file))
            LSQ_WARN("error reading %s: %s",
                        client_ctx->hcc_zero_rtt_file_name, strerror(errno));
        fclose(file);
        LSQ_INFO("create connection zero_rtt %zu bytes", len);
    }
    else no_file:
        len = 0;

    while (client_ctx->hcc_n_open_conns < client_ctx->hcc_concurrency &&
           client_ctx->hcc_total_n_reqs > 0)
        if (0 != prog_connect(client_ctx->prog, len ? zero_rtt : NULL, len))
        {
            LSQ_ERROR("connection failed");
            exit(EXIT_FAILURE);
        }
}


static void
create_streams (struct http_client_ctx *client_ctx, lsquic_conn_ctx_t *conn_h)
{
    printf("\n====================== CREATE STREAM ==================================================================\n");
#if 0    
    while (conn_h->ch_n_reqs - conn_h->ch_n_cc_streams &&
            conn_h->ch_n_cc_streams < client_ctx->hcc_cc_reqs_per_conn)
    {
        lsquic_conn_make_stream(conn_h->conn);
        conn_h->ch_n_cc_streams++;
    }
#else
    if (retrans_check){
        client_ctx->hcc_cc_reqs_per_conn = 2;
    }
    while (conn_h->ch_n_reqs - conn_h->ch_n_cc_streams &&
            conn_h->ch_n_cc_streams < client_ctx->hcc_cc_reqs_per_conn)
    {
        lsquic_conn_make_stream(conn_h->conn);
        conn_h->ch_n_cc_streams++;
    }   
#endif    
}


static lsquic_conn_ctx_t *
http_client_on_new_conn (void *stream_if_ctx, lsquic_conn_t *conn)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct http_client_ctx *client_ctx = stream_if_ctx;
    lsquic_conn_ctx_t *conn_h = calloc(1, sizeof(*conn_h));
    conn_h->conn = conn;
    conn_h->client_ctx = client_ctx;
    conn_h->ch_n_reqs = MIN(client_ctx->hcc_total_n_reqs,
                                                client_ctx->hcc_reqs_per_conn);
    client_ctx->hcc_total_n_reqs -= conn_h->ch_n_reqs;
    TAILQ_INSERT_TAIL(&client_ctx->conn_ctxs, conn_h, next_ch);
    ++conn_h->client_ctx->hcc_n_open_conns;
    if (!TAILQ_EMPTY(&client_ctx->hcc_path_elems))
        create_streams(client_ctx, conn_h);
    conn_h->ch_created = lsquic_time_now();
    return conn_h;
}


struct create_another_conn_or_stop_ctx
{
    struct event            *event;
    struct http_client_ctx  *client_ctx;
};


static void
create_another_conn_or_stop (evutil_socket_t sock, short events, void *ctx)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct create_another_conn_or_stop_ctx *const cacos = ctx;
    struct http_client_ctx *const client_ctx = cacos->client_ctx;

    event_del(cacos->event);
    event_free(cacos->event);
    free(cacos);

    create_connections(client_ctx);
    if (0 == client_ctx->hcc_n_open_conns)
    {
        LSQ_INFO("All connections are closed: stop engine");
        prog_stop(client_ctx->prog);
    }
}


static void
http_client_on_conn_closed (lsquic_conn_t *conn)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    lsquic_conn_ctx_t *conn_h = lsquic_conn_get_ctx(conn);
    struct create_another_conn_or_stop_ctx *cacos;
    enum LSQUIC_CONN_STATUS status;
    struct event_base *eb;
    char errmsg[80];

    status = lsquic_conn_status(conn, errmsg, sizeof(errmsg));
    LSQ_INFO("Connection closed.  Status: %d.  Message: %s", status,
        errmsg[0] ? errmsg : "<not set>");
    if (conn_h->client_ctx->hcc_flags & HCC_ABORT_ON_INCOMPLETE)
    {
        if (!(conn_h->client_ctx->hcc_flags & HCC_SEEN_FIN))
            abort();
    }
    TAILQ_REMOVE(&conn_h->client_ctx->conn_ctxs, conn_h, next_ch);
    --conn_h->client_ctx->hcc_n_open_conns;

    cacos = calloc(1, sizeof(*cacos));
    if (!cacos)
    {
        LSQ_ERROR("cannot allocate cacos");
        exit(1);
    }
    eb = prog_eb(conn_h->client_ctx->prog);
    cacos->client_ctx = conn_h->client_ctx;
    cacos->event = event_new(eb, -1, 0, create_another_conn_or_stop, cacos);
    if (!cacos->event)
    {
        LSQ_ERROR("cannot allocate event");
        exit(1);
    }
    if (0 != event_add(cacos->event, NULL))
    {
        LSQ_ERROR("cannot add cacos event");
        exit(1);
    }
    event_active(cacos->event, 0, 0);

    free(conn_h);
}


static int
hsk_status_ok (enum lsquic_hsk_status status)
{
    return status == LSQ_HSK_OK || status == LSQ_HSK_0RTT_OK;
}


static void
http_client_on_hsk_done (lsquic_conn_t *conn, enum lsquic_hsk_status status)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    lsquic_conn_ctx_t *conn_h = lsquic_conn_get_ctx(conn);
    struct http_client_ctx *client_ctx = conn_h->client_ctx;

    if (hsk_status_ok(status))
        LSQ_INFO("handshake success %s",
                                status == LSQ_HSK_0RTT_OK ? "with 0-RTT" : "");
    else if (status == LSQ_HSK_FAIL)
        LSQ_INFO("handshake failed");
    else if (status == LSQ_HSK_0RTT_FAIL)
    {
        LSQ_INFO("handshake failed because of 0-RTT, will retry without it");
        client_ctx->hcc_flags |= HCC_SKIP_0RTT;
        ++client_ctx->hcc_concurrency;
        ++client_ctx->hcc_total_n_reqs;
    }
    else
        assert(0);

    if (hsk_status_ok(status) && s_display_cert_chain)
        display_cert_chain(conn);

    if (hsk_status_ok(status))
    {
        conn_h = lsquic_conn_get_ctx(conn);
        ++s_stat_conns_ok;
        update_sample_stats(&s_stat_to_conn,
                                    lsquic_time_now() - conn_h->ch_created);
        if (TAILQ_EMPTY(&client_ctx->hcc_path_elems))
        {
            LSQ_INFO("no paths mode: close connection");
            lsquic_conn_close(conn_h->conn);
        }
    }
    else
        ++s_stat_conns_failed;
}


static void
http_client_on_zero_rtt_info (lsquic_conn_t *conn, const unsigned char *buf,
                                                                size_t bufsz)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    lsquic_conn_ctx_t *const conn_h = lsquic_conn_get_ctx(conn);
    struct http_client_ctx *const client_ctx = conn_h->client_ctx;
    FILE *file;
    size_t nw;

    assert(client_ctx->hcc_zero_rtt_file_name);

    /* Our client is rather limited: only one file and only one ticket per
     * connection can be saved.
     */
    if (conn_h->ch_flags & CH_ZERO_RTT_SAVED)
    {
        LSQ_DEBUG("zero-rtt already saved for this connection");
        return;
    }

    file = fopen(client_ctx->hcc_zero_rtt_file_name, "wb");
    if (!file)
    {
        LSQ_WARN("cannot open %s for writing: %s",
            client_ctx->hcc_zero_rtt_file_name, strerror(errno));
        return;
    }

    nw = fwrite(buf, 1, bufsz, file);
    if (nw == bufsz)
    {
        LSQ_DEBUG("wrote %zd bytes of zero-rtt information to %s",
            nw, client_ctx->hcc_zero_rtt_file_name);
        conn_h->ch_flags |= CH_ZERO_RTT_SAVED;
    }
    else
        LSQ_WARN("error: fwrite(%s) returns %zd instead of %zd: %s",
            client_ctx->hcc_zero_rtt_file_name, nw, bufsz, strerror(errno));

    fclose(file);
}


// struct lsquic_stream_ctx {
//     lsquic_stream_t     *stream;
//     struct http_client_ctx   *client_ctx;
//     const char          *path;
//     enum {
//         HEADERS_SENT    = (1 << 0),
//         PROCESSED_HEADERS = 1 << 1,
//     }                    sh_flags;
//     lsquic_time_t        sh_created;
//     lsquic_time_t        sh_ttfb;
//     unsigned             count;
//     struct lsquic_reader reader;
// };


static lsquic_stream_ctx_t *
http_client_on_new_stream (void *stream_if_ctx, lsquic_stream_t *stream)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    // minh_client_seg ++;
    
    const int pushed = lsquic_stream_is_pushed(stream);

    if (pushed)
    {
        LSQ_INFO("not accepting server push");
        lsquic_stream_refuse_push(stream);
        return NULL;
    }

    

    lsquic_stream_ctx_t *st_h = calloc(1, sizeof(*st_h));
    st_h->stream = stream;
    st_h->client_ctx = stream_if_ctx;
    st_h->sh_created = lsquic_time_now();

// Minh [retransmission] MOD-S
#if 0
    if (st_h->client_ctx->hcc_cur_pe)
    {
        // printf("\t[MINH] info: 1 st_h->client_ctx->hcc_cur_pe->next_pe: %s\n", st_h->client_ctx->hcc_cur_pe->path);
        st_h->client_ctx->hcc_cur_pe = TAILQ_NEXT(
                                        st_h->client_ctx->hcc_cur_pe, next_pe);
        if (!st_h->client_ctx->hcc_cur_pe){  /* Wrap around */
            // printf("\t[MINH] info: 2 st_h->client_ctx->hcc_cur_pe: %s\n", st_h->client_ctx->hcc_cur_pe);
            st_h->client_ctx->hcc_cur_pe =
                                TAILQ_FIRST(&st_h->client_ctx->hcc_path_elems);
        }
    }
    else{
        st_h->client_ctx->hcc_cur_pe = TAILQ_FIRST(
                                            &st_h->client_ctx->hcc_path_elems);
    }
#else
    if (!retrans_check){
        st_h->path = next_path;
        segment[minh_client_seg].layer[cur_layer_id-1].stream_id = lsquic_stream_id(stream);
        next_stream_id = lsquic_stream_id(stream);
        printf("path = NEXT path: %s in stream_id:  %"PRIu64"\n", st_h->path, lsquic_stream_id(stream));
        struct http_client_ctx *const client_ctx = st_h->client_ctx;
        client_ctx->hcc_cc_reqs_per_conn = 1;
    }
    else {
        st_h->path = retrans_path;
        segment[retrans_seg_id].layer[cur_layer_id-1].stream_id = lsquic_stream_id(stream);
        retrans_stream_id = lsquic_stream_id(stream);
        printf("path = RETRANS path: %s in stream_id: %"PRIu64"\n", st_h->path, lsquic_stream_id(stream));

        retrans_check = false;
    }
#endif    
// Minh [retransmission] MOD-E

    if (st_h->client_ctx->payload)
    {
        st_h->reader.lsqr_read = test_reader_read;
        st_h->reader.lsqr_size = test_reader_size;
        st_h->reader.lsqr_ctx = create_lsquic_reader_ctx(st_h->client_ctx->payload);
        if (!st_h->reader.lsqr_ctx)
            exit(1);
    }
    else
        st_h->reader.lsqr_ctx = NULL;

    // start_stream_recorder[minh_client_seg] = lsquic_time_now();

    // bitrate_recorder[minh_client_seg][1] = lsquic_stream_id(stream);
    printf("\t[MINH] info: created stream id: %"PRIu64". path: %s. segment: %d At time: %Lf ms\n",
                lsquic_stream_id(stream),st_h->path, minh_client_seg, (long double)(lsquic_time_now() - streaming_start_time) / 1000);  
    
    lsquic_stream_wantwrite(stream, 1);

    if (set_prior_check){
        if (st_h->path == retrans_path){
            printf("PRIORITY_retrans\n");
            lsquic_stream_set_priority(stream, priority_retrans);
        }
        else{
            printf("PRIORITY_next\n");
            lsquic_stream_set_priority(stream, priority_next);
            set_prior_check = false;
        }
    }

    return st_h;
}


static void
send_headers (lsquic_stream_ctx_t *st_h)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    const char *hostname = st_h->client_ctx->hostname;
    if (!hostname)
        hostname = st_h->client_ctx->prog->prog_hostname;
    struct lsxpack_header headers_arr[7];
#define V(v) (v), strlen(v)
    lsxpack_header_set_ptr(&headers_arr[0], V(":method"), V(st_h->client_ctx->method));
    lsxpack_header_set_ptr(&headers_arr[1], V(":scheme"), V("https"));
    lsxpack_header_set_ptr(&headers_arr[2], V(":path"), V(st_h->path));
    lsxpack_header_set_ptr(&headers_arr[3], V(":authority"), V(hostname));
    lsxpack_header_set_ptr(&headers_arr[4], V("user-agent"), V(st_h->client_ctx->prog->prog_settings.es_ua));
    /* The following headers only gets sent if there is request payload: */
    lsxpack_header_set_ptr(&headers_arr[5], V("content-type"), V("application/octet-stream"));
    lsxpack_header_set_ptr(&headers_arr[6], V("content-length"), V( st_h->client_ctx->payload_size));
    lsquic_http_headers_t headers = {
        .count = sizeof(headers_arr) / sizeof(headers_arr[0]),
        .headers = headers_arr,
    };
    if (!st_h->client_ctx->payload)
        headers.count -= 2;
    if (0 != lsquic_stream_send_headers(st_h->stream, &headers,
                                    st_h->client_ctx->payload == NULL))
    {
        LSQ_ERROR("cannot send headers: %s", strerror(errno));
        exit(1);
    }
}


/* This is here to exercise lsquic_conn_get_server_cert_chain() API */
static void
display_cert_chain (lsquic_conn_t *conn)
{
    STACK_OF(X509) *chain;
    X509_NAME *name;
    X509 *cert;
    unsigned i;
    char buf[100];

    chain = lsquic_conn_get_server_cert_chain(conn);
    if (!chain)
    {
        LSQ_WARN("could not get server certificate chain");
        return;
    }

    for (i = 0; i < sk_X509_num(chain); ++i)
    {
        cert = sk_X509_value(chain, i);
        name = X509_get_subject_name(cert);
        LSQ_INFO("cert #%u: name: %s", i,
                            X509_NAME_oneline(name, buf, sizeof(buf)));
        X509_free(cert);
    }

    sk_X509_free(chain);
}


static void
http_client_on_write (lsquic_stream_t *stream, lsquic_stream_ctx_t *st_h)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    ssize_t nw;

    if (st_h->sh_flags & HEADERS_SENT)
    {
        if (st_h->client_ctx->payload && test_reader_size(st_h->reader.lsqr_ctx) > 0)
        {
            nw = lsquic_stream_writef(stream, &st_h->reader);
            if (nw < 0)
            {
                LSQ_ERROR("write error: %s", strerror(errno));
                exit(1);
            }
            if (test_reader_size(st_h->reader.lsqr_ctx) > 0)
            {
                lsquic_stream_wantwrite(stream, 1);
            }
            else
            {
                lsquic_stream_shutdown(stream, 1);
                lsquic_stream_wantread(stream, 1);
            }
        }
        else
        {
            lsquic_stream_shutdown(stream, 1);
            lsquic_stream_wantread(stream, 1);
        }
    }
    else
    {
        st_h->sh_flags |= HEADERS_SENT;
        send_headers(st_h);
    }
}


static size_t
discard (void *ctx, const unsigned char *buf, size_t sz, int fin)
{
    return sz;
}


static void
http_client_on_read (lsquic_stream_t *stream, lsquic_stream_ctx_t *st_h)
{
    // printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct http_client_ctx *const client_ctx = st_h->client_ctx;
    struct hset *hset;
    ssize_t nread;
    unsigned old_prio, new_prio;
    unsigned char buf[0x200];
    unsigned nreads = 0;
#ifdef WIN32
    srand(GetTickCount());
#endif

    do
    {
        if (g_header_bypass && !(st_h->sh_flags & PROCESSED_HEADERS))
        {
            hset = lsquic_stream_get_hset(stream);
            if (!hset)
            {
                LSQ_ERROR("could not get header set from stream");
                exit(2);
            }
            st_h->sh_ttfb = lsquic_time_now();
            update_sample_stats(&s_stat_ttfb, st_h->sh_ttfb - st_h->sh_created);
            if (s_discard_response)
                LSQ_DEBUG("discard response: do not dump headers");
            else
                hset_dump(hset, stdout);
            hset_destroy(hset);
            st_h->sh_flags |= PROCESSED_HEADERS;
        }
        else if (nread = (s_discard_response
                            ? lsquic_stream_readf(stream, discard, NULL)
                            : lsquic_stream_read(stream, buf, sizeof(buf))),
                    nread > 0)
        {
            // TODO: terimination
            if (0){
                termination_check = FALSE;
                retrans_stream_id = lsquic_stream_id(stream);
            }
            s_stat_downloaded_bytes += nread;
            stream->sm_cont_len += nread;
            downloaded_bytes_2on_close += nread;
            /* test stream_reset after some number of read bytes */
            // if (client_ctx->hcc_reset_after_nbytes &&
            //     s_stat_downloaded_bytes > client_ctx->hcc_reset_after_nbytes)
            // {
            //     lsquic_stream_reset(stream, 0x1);
            //     break;
            // }
            /* test retire_cid after some number of read bytes */
            // if (client_ctx->hcc_retire_cid_after_nbytes &&
            //     s_stat_downloaded_bytes > client_ctx->hcc_retire_cid_after_nbytes)
            // {
            //     lsquic_conn_retire_cid(lsquic_stream_conn(stream));
            //     client_ctx->hcc_retire_cid_after_nbytes = 0;
            //     break;
            // }
            if (!g_header_bypass && !(st_h->sh_flags & PROCESSED_HEADERS))
            {
                /* First read is assumed to be the first byte */
                st_h->sh_ttfb = lsquic_time_now();
                update_sample_stats(&s_stat_ttfb,
                                    st_h->sh_ttfb - st_h->sh_created);
                st_h->sh_flags |= PROCESSED_HEADERS;
            }
            // if (!s_discard_response)
            //     fwrite(buf, 1, nread, stdout);
            if (randomly_reprioritize_streams && (st_h->count++ & 0x3F) == 0)
            {
                old_prio = lsquic_stream_priority(stream);
                new_prio = 1 + (random() & 0xFF);
#ifndef NDEBUG
                const int s =
#endif
                lsquic_stream_set_priority(stream, new_prio);
                assert(s == 0);
                LSQ_DEBUG("changed stream %"PRIu64" priority from %u to %u",
                                lsquic_stream_id(stream), old_prio, new_prio);
            }
        }
        else if (0 == nread)
        {
            update_sample_stats(&s_stat_req, lsquic_time_now() - st_h->sh_ttfb);
            client_ctx->hcc_flags |= HCC_SEEN_FIN;
            lsquic_stream_shutdown(stream, 0);
            break;
        }
        else if (client_ctx->prog->prog_settings.es_rw_once && EWOULDBLOCK == errno)
        {
            LSQ_NOTICE("emptied the buffer in 'once' mode");
            break;
        }
        else if (lsquic_stream_is_rejected(stream))
        {
            LSQ_NOTICE("stream was rejected");
            lsquic_stream_close(stream);
            break;
        }
        else
        {
            LSQ_ERROR("could not read: %s", strerror(errno));
            exit(2);
        }
    }
    while (client_ctx->prog->prog_settings.es_rw_once
            && nreads++ < 3 /* Emulate just a few reads */);
}

// static void
// minh_submit_request(){
//     struct path_elem *pe;

//     pe = calloc(1, sizeof(*pe));
//     pe->path = "/file-50K";
//     TAILQ_INSERT_TAIL(&client_ctx.hcc_path_elems, pe, next_pe);   
// }


static void
http_client_on_close (lsquic_stream_t *stream, lsquic_stream_ctx_t *st_h)
{
    unsigned long stream_id = lsquic_stream_id(stream);
    printf("\n\t******************* CLOSE STREAM ***************\n");
    printf("\t[MINH] closed stream id: %"PRIu64" \tpath: %s \tcontent length: %llu bytes \n", 
                                    stream_id, st_h->path, stream->sm_cont_len - 91);
    const int pushed = lsquic_stream_is_pushed(stream);
    if (pushed)
    {
        assert(NULL == st_h);
        return;
    }

    LSQ_INFO("%s called", __func__);
    struct http_client_ctx *const client_ctx = st_h->client_ctx;
    lsquic_conn_t *conn = lsquic_stream_conn(stream);
    lsquic_conn_ctx_t *conn_h;
    TAILQ_FOREACH(conn_h, &client_ctx->conn_ctxs, next_ch)
        if (conn_h->conn == conn)
            break;
    assert(conn_h);
    --conn_h->ch_n_reqs;
    --conn_h->ch_n_cc_streams;

// Minh [retransmission] ADD-S
    if (retransmitting && !termination_check && (retrans_stream_id == stream_id))   // retransmit successfully (NO termination)
    {   
        printf("RETRANSMIT SUCCESSFULLY\n");
        retransmitting = FALSE;
        minh_update_stats(stream, st_h, retrans_seg_id, TRUE);
        // segment[retrans_seg_id].num_layers ++;
        // segment[retrans_seg_id].layer[num_layers-1].bitrate = MINH_BITRATE_SET[num_layers-1];
        // segment[retrans_seg_id].layer[num_layers-1].throughput = (stream->sm_cont_len - 91); ;
        // segment[retrans_seg_id].layer[num_layers-1].download_time = ;

       // tuong duong vs minh_update_stats(stream, st_h, seg_id = retrans_seg_id , layer_id = num_layers-1);
    }
    else if (next_stream_id == stream_id)
    {
        printf("next_stream_id == stream_id\n");
        // cur_layer_id ++;    // nen cho vao minh_AGG
        minh_update_stats(stream, st_h, minh_client_seg, FALSE);
    }

    minh_AGG_ABR();
    minh_retransmission_technique();
// Minh [retransmission] ADD-E

    if (0 == conn_h->ch_n_reqs)
    {
        LSQ_INFO("all requests completed, closing connection");
        lsquic_conn_close(conn_h->conn);
    }
    else
    {
        LSQ_INFO("%u active stream, %u request remain, creating %u new stream",
            conn_h->ch_n_cc_streams,
            conn_h->ch_n_reqs - conn_h->ch_n_cc_streams,
            MIN((conn_h->ch_n_reqs - conn_h->ch_n_cc_streams),
                (client_ctx->hcc_cc_reqs_per_conn - conn_h->ch_n_cc_streams)));
        create_streams(client_ctx, conn_h);
    }
    if (st_h->reader.lsqr_ctx)
        destroy_lsquic_reader_ctx(st_h->reader.lsqr_ctx);
    free(st_h);
}


static struct lsquic_stream_if http_client_if = {
    .on_new_conn            = http_client_on_new_conn,
    .on_conn_closed         = http_client_on_conn_closed,
    .on_new_stream          = http_client_on_new_stream,
    .on_read                = http_client_on_read,
    .on_write               = http_client_on_write,
    .on_close               = http_client_on_close,
    .on_hsk_done            = http_client_on_hsk_done,
};


static void
usage (const char *prog)
{
    const char *const slash = strrchr(prog, '/');
    if (slash)
        prog = slash + 1;
    printf(
"Usage: %s [opts]\n"
"\n"
"Options:\n"
"   -p PATH     Path to request.  May be specified more than once.  If no\n"
"                 path is specified, the connection is closed as soon as\n"
"                 handshake succeeds.\n"
"   -n CONNS    Number of concurrent connections.  Defaults to 1.\n"
"   -r NREQS    Total number of requests to send.  Defaults to 1.\n"
"   -R MAXREQS  Maximum number of requests per single connection.  Some\n"
"                 connections will have fewer requests than this.\n"
"   -w CONCUR   Number of concurrent requests per single connection.\n"
"                 Defaults to 1.\n"
"   -M METHOD   Method.  Defaults to GET.\n"
"   -P PAYLOAD  Name of the file that contains payload to be used in the\n"
"                 request.  This adds two more headers to the request:\n"
"                 content-type: application/octet-stream and\n"
"                 content-length\n"
"   -K          Discard server response\n"
"   -I          Abort on incomplete reponse from server\n"
"   -4          Prefer IPv4 when resolving hostname\n"
"   -6          Prefer IPv6 when resolving hostname\n"
"   -0 FILE     Provide RTT info file (reading or writing)\n"
#ifndef WIN32
"   -C DIR      Certificate store.  If specified, server certificate will\n"
"                 be verified.\n"
#endif
"   -a          Display server certificate chain after successful handshake.\n"
"   -b N_BYTES  Send RESET_STREAM frame after the client has read n bytes.\n"
"   -t          Print stats to stdout.\n"
"   -T FILE     Print stats to FILE.  If FILE is -, print stats to stdout.\n"
"   -q FILE     QIF mode: issue requests from the QIF file and validate\n"
"                 server responses.\n"
"   -e TOKEN    Hexadecimal string representing resume token.\n"
            , prog);
}


#ifndef WIN32
static X509_STORE *store;

/* Windows does not have regex... */
static int
ends_in_pem (const char *s)
{
    int len;

    len = strlen(s);

    return len >= 4
        && 0 == strcasecmp(s + len - 4, ".pem");
}


static X509 *
file2cert (const char *path)
{
    X509 *cert = NULL;
    BIO *in;

    in = BIO_new(BIO_s_file());
    if (!in)
        goto end;

    if (BIO_read_filename(in, path) <= 0)
        goto end;

    cert = PEM_read_bio_X509_AUX(in, NULL, NULL, NULL);

  end:
    BIO_free(in);
    return cert;
}


static int
init_x509_cert_store (const char *path)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct dirent *ent;
    X509 *cert;
    DIR *dir;
    char file_path[NAME_MAX];
    int ret;

    dir = opendir(path);
    if (!dir)
    {
        LSQ_WARN("Cannot open directory `%s': %s", path, strerror(errno));
        return -1;
    }

    store = X509_STORE_new();

    while ((ent = readdir(dir)))
    {
        if (ends_in_pem(ent->d_name))
        {
            ret = snprintf(file_path, sizeof(file_path), "%s/%s",
                                                            path, ent->d_name);
            if (ret < 0)
            {
                LSQ_WARN("file_path formatting error %s", strerror(errno));
                continue;
            }
            else if ((unsigned)ret >= sizeof(file_path))
            {
                LSQ_WARN("file_path was truncated %s", strerror(errno));
                continue;
            }
            cert = file2cert(file_path);
            if (cert)
            {
                if (1 != X509_STORE_add_cert(store, cert))
                    LSQ_WARN("could not add cert from %s", file_path);
            }
            else
                LSQ_WARN("could not read cert from %s", file_path);
        }
    }
    (void) closedir(dir);
    return 0;
}


static int
verify_server_cert (void *ctx, STACK_OF(X509) *chain)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    X509_STORE_CTX store_ctx;
    X509 *cert;
    int ver;

    if (!store)
    {
        if (0 != init_x509_cert_store(ctx))
            return -1;
    }

    cert = sk_X509_shift(chain);
    X509_STORE_CTX_init(&store_ctx, store, cert, chain);

    ver = X509_verify_cert(&store_ctx);

    X509_STORE_CTX_cleanup(&store_ctx);

    if (ver != 1)
        LSQ_WARN("could not verify server certificate");

    return ver == 1 ? 0 : -1;
}
#endif


static void *
hset_create (void *hsi_ctx, lsquic_stream_t *stream, int is_push_promise)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct hset *hset;

    if (s_discard_response)
        return (void *) 1;
    else if ((hset = malloc(sizeof(*hset))))
    {
        STAILQ_INIT(hset);
        return hset;
    }
    else
        return NULL;
}


static struct lsxpack_header *
hset_prepare_decode (void *hset_p, struct lsxpack_header *xhdr,
                                                        size_t req_space)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct hset *const hset = hset_p;
    struct hset_elem *el;
    char *buf;

    if (0 == req_space)
        req_space = 0x100;

    if (req_space > LSXPACK_MAX_STRLEN)
    {
        LSQ_WARN("requested space for header is too large: %zd bytes",
                                                                    req_space);
        return NULL;
    }

    if (!xhdr)
    {
        buf = malloc(req_space);
        if (!buf)
        {
            LSQ_WARN("cannot allocate buf of %zd bytes", req_space);
            return NULL;
        }
        el = malloc(sizeof(*el));
        if (!el)
        {
            LSQ_WARN("cannot allocate hset_elem");
            free(buf);
            return NULL;
        }
        STAILQ_INSERT_TAIL(hset, el, next);
        lsxpack_header_prepare_decode(&el->xhdr, buf, 0, req_space);
    }
    else
    {
        el = (struct hset_elem *) ((char *) xhdr
                                        - offsetof(struct hset_elem, xhdr));
        if (req_space <= xhdr->val_len)
        {
            LSQ_ERROR("requested space is smaller than already allocated");
            return NULL;
        }
        buf = realloc(el->xhdr.buf, req_space);
        if (!buf)
        {
            LSQ_WARN("cannot reallocate hset buf");
            return NULL;
        }
        el->xhdr.buf = buf;
        el->xhdr.val_len = req_space;
    }

    return &el->xhdr;
}


static int
hset_add_header (void *hset_p, struct lsxpack_header *xhdr)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    unsigned name_len, value_len;
    /* Not much to do: the header value are in xhdr */

    if (xhdr)
    {
        name_len = xhdr->name_len;
        value_len = xhdr->val_len;
        s_stat_downloaded_bytes += name_len + value_len + 4;    /* ": \r\n" */
    }
    else
        s_stat_downloaded_bytes += 2;   /* \r\n "*/

    return 0;
}


static void
hset_destroy (void *hset_p)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    struct hset *hset = hset_p;
    struct hset_elem *el, *next;

    if (!s_discard_response)
    {
        for (el = STAILQ_FIRST(hset); el; el = next)
        {
            next = STAILQ_NEXT(el, next);
            free(el->xhdr.buf);
            free(el);
        }
        free(hset);
    }
}


static void
hset_dump (const struct hset *hset, FILE *out)
{
    printf("\t[MINH] info: %s:%s():%d\n", __FILE__, __func__, __LINE__);
    const struct hset_elem *el;

    STAILQ_FOREACH(el, hset, next)
        if (el->xhdr.flags & (LSXPACK_HPACK_IDX|LSXPACK_QPACK_IDX))
            fprintf(out, "%.*s (%s static table idx %u): %.*s\n",
                (int) el->xhdr.name_len, lsxpack_header_get_name(&el->xhdr),
                el->xhdr.flags & LSXPACK_HPACK_IDX ? "hpack" : "qpack",
                el->xhdr.flags & LSXPACK_HPACK_IDX ? el->xhdr.hpack_index
                                                    : el->xhdr.qpack_index,
                (int) el->xhdr.val_len, lsxpack_header_get_value(&el->xhdr));
        else
            fprintf(out, "%.*s: %.*s\n",
                (int) el->xhdr.name_len, lsxpack_header_get_name(&el->xhdr),
                (int) el->xhdr.val_len, lsxpack_header_get_value(&el->xhdr));

    fprintf(out, "\n");
    fflush(out);
}


/* These are basic and for illustration purposes only.  You will want to
 * do your own verification by doing something similar to what is done
 * in src/liblsquic/lsquic_http1x_if.c
 */
static const struct lsquic_hset_if header_bypass_api =
{
    .hsi_create_header_set  = hset_create,
    .hsi_prepare_decode     = hset_prepare_decode,
    .hsi_process_header     = hset_add_header,
    .hsi_discard_header_set = hset_destroy,
};


// static void
// display_stat (FILE *out, const struct sample_stats *stats, const char *name)
// {
//     long double mean, stddev;

//     calc_sample_stats(stats, &mean, &stddev);
//     fprintf(out, "%s: n: %u; min: %.2Lf ms; max: %.2Lf ms; mean: %.2Lf ms; "
//         "sd: %.2Lf ms\n", name, stats->n, (long double) stats->min / 1000,
//         (long double) stats->max / 1000, mean / 1000, stddev / 1000);
// }


static lsquic_conn_ctx_t *
qif_client_on_new_conn (void *stream_if_ctx, lsquic_conn_t *conn)
{
    lsquic_conn_make_stream(conn);
    return stream_if_ctx;
}


static void
qif_client_on_conn_closed (lsquic_conn_t *conn)
{
    struct http_client_ctx *client_ctx = (void *) lsquic_conn_get_ctx(conn);
    LSQ_INFO("connection is closed: stop engine");
    prog_stop(client_ctx->prog);
}


struct qif_stream_ctx
{
    int                         reqno;
    struct lsquic_http_headers  headers;
    char                       *qif_str;
    size_t                      qif_sz;
    size_t                      qif_off;
    char                       *resp_str;   /* qif_sz allocated */
    size_t                      resp_off;   /* Read so far */
    enum {
        QSC_HEADERS_SENT = 1 << 0,
        QSC_GOT_HEADERS  = 1 << 1,
    }                           flags;
};

// #define MAX(a, b) ((a) > (b) ? (a) : (b))

lsquic_stream_ctx_t *
qif_client_on_new_stream (void *stream_if_ctx, lsquic_stream_t *stream)
{
    struct http_client_ctx *const client_ctx = stream_if_ctx;
    FILE *const fh = client_ctx->qif_fh;
    struct qif_stream_ctx *ctx;
    struct lsxpack_header *header;
    static int reqno;
    size_t nalloc;
    int i;
    char *end, *tab, *line;
    char line_buf[0x1000];

    ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
    {
        perror("calloc");
        exit(1);
    }
    ctx->reqno = reqno++;

    nalloc = 0;
    while ((line = fgets(line_buf, sizeof(line_buf), fh)))
    {
        end = strchr(line, '\n');
        if (!end)
        {
            fprintf(stderr, "no newline\n");
            exit(1);
        }

        if (end == line)
            break;

        if (*line == '#')
            continue;

        tab = strchr(line, '\t');
        if (!tab)
        {
            fprintf(stderr, "no TAB\n");
            exit(1);
        }

        if (nalloc + (end + 1 - line) > ctx->qif_sz)
        {
            if (nalloc)
                nalloc = MAX(nalloc * 2, nalloc + (end + 1 - line));
            else
                nalloc = end + 1 - line;
            ctx->qif_str = realloc(ctx->qif_str, nalloc);
            if (!ctx->qif_str)
            {
                perror("realloc");
                exit(1);
            }
        }
        memcpy(ctx->qif_str + ctx->qif_sz, line, end + 1 - line);

        ctx->headers.headers = realloc(ctx->headers.headers,
                sizeof(ctx->headers.headers[0]) * (ctx->headers.count + 1));
        if (!ctx->headers.headers)
        {
            perror("realloc");
            exit(1);
        }
        header = &ctx->headers.headers[ctx->headers.count++];
        lsxpack_header_set_ptr(header, (void *) ctx->qif_sz, tab - line,
                    (void *) (ctx->qif_sz + (tab - line + 1)), end - tab - 1);

        ctx->qif_sz += end + 1 - line;
    }

    for (i = 0; i < ctx->headers.count; ++i)
    {
        ctx->headers.headers[i].buf = ctx->qif_str
                + (uintptr_t) ctx->headers.headers[i].buf;
        ctx->headers.headers[i].name_ptr = ctx->qif_str
                + (uintptr_t) ctx->headers.headers[i].name_ptr;
    }

    lsquic_stream_wantwrite(stream, 1);

    if (!line)
    {
        LSQ_DEBUG("Input QIF file ends; close file handle");
        fclose(client_ctx->qif_fh);
        client_ctx->qif_fh = NULL;
    }

    return (void *) ctx;
}


static void
qif_client_on_write (struct lsquic_stream *stream, lsquic_stream_ctx_t *h)
{
    struct qif_stream_ctx *const ctx = (void *) h;
    size_t towrite;
    ssize_t nw;

    if (ctx->flags & QSC_HEADERS_SENT)
    {
        towrite = ctx->qif_sz - ctx->qif_off;
        nw = lsquic_stream_write(stream, ctx->qif_str + ctx->qif_off, towrite);
        if (nw >= 0)
        {
            LSQ_DEBUG("wrote %zd bytes to stream", nw);
            ctx->qif_off += nw;
            if (ctx->qif_off == (size_t) nw)
            {
                lsquic_stream_shutdown(stream, 1);
                lsquic_stream_wantread(stream, 1);
                LSQ_DEBUG("finished writing request %d", ctx->reqno);
            }
        }
        else
        {
            LSQ_ERROR("cannot write to stream: %s", strerror(errno));
            lsquic_stream_wantwrite(stream, 0);
            lsquic_conn_abort(lsquic_stream_conn(stream));
        }
    }
    else
    {
        if (0 == lsquic_stream_send_headers(stream, &ctx->headers, 0))
        {
            ctx->flags |= QSC_HEADERS_SENT;
            LSQ_DEBUG("sent headers");
        }
        else
        {
            LSQ_ERROR("cannot send headers: %s", strerror(errno));
            lsquic_stream_wantwrite(stream, 0);
            lsquic_conn_abort(lsquic_stream_conn(stream));
        }
    }
}


static void
qif_client_on_read (struct lsquic_stream *stream, lsquic_stream_ctx_t *h)
{
    struct qif_stream_ctx *const ctx = (void *) h;
    struct hset *hset;
    ssize_t nr;
    unsigned char buf[1];

    LSQ_DEBUG("reading response to request %d", ctx->reqno);

    if (!(ctx->flags & QSC_GOT_HEADERS))
    {
        hset = lsquic_stream_get_hset(stream);
        if (!hset)
        {
            LSQ_ERROR("could not get header set from stream");
            exit(2);
        }
        LSQ_DEBUG("got header set for response %d", ctx->reqno);
        hset_dump(hset, stdout);
        hset_destroy(hset);
        ctx->flags |= QSC_GOT_HEADERS;
    }
    else
    {
        if (!ctx->resp_str)
        {
            ctx->resp_str = malloc(ctx->qif_sz);
            if (!ctx->resp_str)
            {
                perror("malloc");
                exit(1);
            }
        }
        if (ctx->resp_off < ctx->qif_sz)
        {
            nr = lsquic_stream_read(stream, ctx->resp_str + ctx->resp_off,
                                        ctx->qif_sz - ctx->resp_off);
            if (nr > 0)
            {
                ctx->resp_off += nr;
                LSQ_DEBUG("read %zd bytes of reponse %d", nr, ctx->reqno);
            }
            else if (nr == 0)
            {
                LSQ_INFO("response %d too short", ctx->reqno);
                LSQ_WARN("response %d FAIL", ctx->reqno);
                lsquic_stream_shutdown(stream, 0);
            }
            else
            {
                LSQ_ERROR("error reading from stream");
                lsquic_stream_wantread(stream, 0);
                lsquic_conn_abort(lsquic_stream_conn(stream));
            }
        }
        else
        {
            /* Collect EOF */
            nr = lsquic_stream_read(stream, buf, sizeof(buf));
            if (nr == 0)
            {
                if (0 == memcmp(ctx->qif_str, ctx->resp_str, ctx->qif_sz))
                    LSQ_INFO("response %d OK", ctx->reqno);
                else
                    LSQ_WARN("response %d FAIL", ctx->reqno);
                lsquic_stream_shutdown(stream, 0);
            }
            else if (nr > 0)
            {
                LSQ_INFO("response %d too long", ctx->reqno);
                LSQ_WARN("response %d FAIL", ctx->reqno);
                lsquic_stream_shutdown(stream, 0);
            }
            else
            {
                LSQ_ERROR("error reading from stream");
                lsquic_stream_shutdown(stream, 0);
                lsquic_conn_abort(lsquic_stream_conn(stream));
            }
        }
    }
}


static void
qif_client_on_close (struct lsquic_stream *stream, lsquic_stream_ctx_t *h)
{
    struct lsquic_conn *conn = lsquic_stream_conn(stream);
    struct http_client_ctx *client_ctx = (void *) lsquic_conn_get_ctx(conn);
    struct qif_stream_ctx *const ctx = (void *) h;
    free(ctx->qif_str);
    free(ctx->resp_str);
    free(ctx->headers.headers);
    free(ctx);
    if (client_ctx->qif_fh)
        lsquic_conn_make_stream(conn);
    else
        lsquic_conn_close(conn);
}


const struct lsquic_stream_if qif_client_if = {
    .on_new_conn            = qif_client_on_new_conn,
    .on_conn_closed         = qif_client_on_conn_closed,
    .on_new_stream          = qif_client_on_new_stream,
    .on_read                = qif_client_on_read,
    .on_write               = qif_client_on_write,
    .on_close               = qif_client_on_close,
};


int
main (int argc, char **argv)
{
    int opt, s, was_empty;
    
    FILE *stats_fh = NULL;
    // long double elapsed;
    struct http_client_ctx client_ctx;
    struct stat st;
    struct path_elem *pe;
    struct sport_head sports;
    struct prog prog;
    const char *token = NULL;

    TAILQ_INIT(&sports);
    memset(&client_ctx, 0, sizeof(client_ctx));
    TAILQ_INIT(&client_ctx.hcc_path_elems);
    TAILQ_INIT(&client_ctx.conn_ctxs);
    client_ctx.method = "GET";
    client_ctx.hcc_concurrency = 1;
    client_ctx.hcc_cc_reqs_per_conn = 1;
    client_ctx.hcc_reqs_per_conn = 1;
    client_ctx.hcc_total_n_reqs = 1;
    client_ctx.hcc_reset_after_nbytes = 0;
    client_ctx.hcc_retire_cid_after_nbytes = 0;
    client_ctx.prog = &prog;
#ifdef WIN32
    WSADATA wsd;
    WSAStartup(MAKEWORD(2, 2), &wsd);
#endif

    prog_init(&prog, LSENG_HTTP, &sports, &http_client_if, &client_ctx);

    while (-1 != (opt = getopt(argc, argv, PROG_OPTS
                                    "46Br:R:IKu:EP:M:n:w:H:p:0:q:e:hatT:b:d:"
#ifndef WIN32
                                                                      "C:"
#endif
                                                                            )))
    {
        switch (opt) {
        case 'a':
            ++s_display_cert_chain;
            break;
        case '4':
        case '6':
            prog.prog_ipver = opt - '0';
            break;
        case 'B':
            g_header_bypass = 1;
            prog.prog_api.ea_hsi_if = &header_bypass_api;
            prog.prog_api.ea_hsi_ctx = NULL;
            break;
        case 'I':
            client_ctx.hcc_flags |= HCC_ABORT_ON_INCOMPLETE;
            break;
        case 'K':
            ++s_discard_response;
            break;
        case 'u':   /* Accept p<U>sh promise */
            promise_fd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if (promise_fd < 0)
            {
                perror("open");
                exit(1);
            }
            prog.prog_settings.es_support_push = 1;     /* Pokes into prog */
            break;
        case 'E':   /* E: randomly reprioritize str<E>ams.  Now, that's
                     * pretty random. :)
                     */
            randomly_reprioritize_streams = 1;
            break;
        case 'n':
            client_ctx.hcc_concurrency = atoi(optarg);
            break;
        case 'w':
            client_ctx.hcc_cc_reqs_per_conn = atoi(optarg);
            break;
        case 'P':
            client_ctx.payload = optarg;
            if (0 != stat(optarg, &st))
            {
                perror("stat");
                exit(2);
            }
            sprintf(client_ctx.payload_size, "%jd", (intmax_t) st.st_size);
            break;
        case 'M':
            client_ctx.method = optarg;
            break;
        case 'r':
            client_ctx.hcc_total_n_reqs = atoi(optarg);
            break;
        case 'R':
            client_ctx.hcc_reqs_per_conn = atoi(optarg);
            break;
        case 'H':
            client_ctx.hostname = optarg;
            prog.prog_hostname = optarg;            /* Pokes into prog */
            break;
        case 'p':
            pe = calloc(1, sizeof(*pe));
            pe->path = optarg;
            TAILQ_INSERT_TAIL(&client_ctx.hcc_path_elems, pe, next_pe);
            break;
        case 'h':
            usage(argv[0]);
            prog_print_common_options(&prog, stdout);
            exit(0);
        case 'q':
            client_ctx.qif_file = optarg;
            break;
        case 'e':
            if (TAILQ_EMPTY(&sports))
                token = optarg;
            else
                sport_set_token(TAILQ_LAST(&sports, sport_head), optarg);
            break;
#ifndef WIN32
        case 'C':
            prog.prog_api.ea_verify_cert = verify_server_cert;
            prog.prog_api.ea_verify_ctx = optarg;
            break;
#endif
        case 't':
            stats_fh = stdout;
            break;
        case 'T':
            if (0 == strcmp(optarg, "-"))
                stats_fh = stdout;
            else
            {
                stats_fh = fopen(optarg, "w");
                if (!stats_fh)
                {
                    perror("fopen");
                    exit(1);
                }
            }
            break;
        case 'b':
            client_ctx.hcc_reset_after_nbytes = atoi(optarg);
            break;
        case 'd':
            client_ctx.hcc_retire_cid_after_nbytes = atoi(optarg);
            break;
        case '0':
            http_client_if.on_zero_rtt_info = http_client_on_zero_rtt_info;
            client_ctx.hcc_zero_rtt_file_name = optarg;
            break;
        default:
            if (0 != prog_set_opt(&prog, opt, optarg))
                exit(1);
        }
    }
    client_ctx.hcc_reqs_per_conn = 1000;
    printf("DUMMYNET-S\n");
    // if (system("sudo ./complex_3g.sh &")) {printf("ERROR Cannot start DummyNet\n"); exit(1);};
    printf("DUMMYNET-E\n");

#if LSQUIC_CONN_STATS
    prog.prog_api.ea_stats_fh = stats_fh;
#endif
    prog.prog_settings.es_ua = LITESPEED_ID;

    if (client_ctx.qif_file)
    {
        client_ctx.qif_fh = fopen(client_ctx.qif_file, "r");
        if (!client_ctx.qif_fh)
        {
            fprintf(stderr, "Cannot open %s for reading: %s\n",
                                    client_ctx.qif_file, strerror(errno));
            exit(1);
        }
        LSQ_NOTICE("opened QIF file %s for reading\n", client_ctx.qif_file);
        prog.prog_api.ea_stream_if = &qif_client_if;
        g_header_bypass = 1;
        prog.prog_api.ea_hsi_if = &header_bypass_api;
        prog.prog_api.ea_hsi_ctx = NULL;
    }
    else if (TAILQ_EMPTY(&client_ctx.hcc_path_elems))
    {
        fprintf(stderr, "Specify at least one path using -p option\n");
        exit(1);
    }

    streaming_start_time = lsquic_time_now();
    was_empty = TAILQ_EMPTY(&sports);
    if (0 != prog_prep(&prog))
    {
        LSQ_ERROR("could not prep");
        exit(EXIT_FAILURE);
    }
    if (!(client_ctx.hostname || prog.prog_hostname))
    {
        fprintf(stderr, "Specify hostname (used for SNI and :authority) via "
            "-H option\n");
        exit(EXIT_FAILURE);
    }
    if (was_empty && token)
        sport_set_token(TAILQ_LAST(&sports, sport_head), token);

    if (client_ctx.qif_file)
    {
        if (0 != prog_connect(&prog, NULL, 0))
        {
            LSQ_ERROR("connection failed");
            exit(EXIT_FAILURE);
        }
    }
    else
        create_connections(&client_ctx);

    LSQ_DEBUG("entering event loop");

    s = prog_run(&prog);

    // if (stats_fh)
    // {
        // elapsed = (long double) (lsquic_time_now() - streaming_start_time) / 1000000;
        // fprintf(stats_fh, "\noverall statistics as calculated by %s:\n", argv[0]);
        // display_stat(stats_fh, &s_stat_to_conn, "time for connect");
        // display_stat(stats_fh, &s_stat_req, "time for request");
        // display_stat(stats_fh, &s_stat_ttfb, "time to 1st byte");
        // fprintf(stats_fh, "downloaded %lu application bytes in %.3Lf seconds\n",
        //     s_stat_downloaded_bytes, elapsed);
        // fprintf(stats_fh, "%.2Lf reqs/sec; %.0Lf bytes/sec\n",
        //     (long double) s_stat_req.n / elapsed,
        //     (long double) s_stat_downloaded_bytes / elapsed);
        // fprintf(stats_fh, "read handler count %lu\n", prog.prog_read_count);

    int seg_idx, layer_idx;
    printf("=======================Streaming session status=======================\n");
    printf("StartTime\t SegID\t #Layers\t LayerID\t Bitrate\t Throughput\t Buffer\t StreamID\t DownloadTime\n");
    // for ( seg_idx = 0; seg_idx < MAX_SEGMENT_ID && segment[seg_idx].num_layers != 0; seg_idx ++)
    for ( seg_idx = 0; seg_idx < MAX_SEGMENT_ID; seg_idx ++)
    {
        for (layer_idx = 0; layer_idx < segment[seg_idx].num_layers; layer_idx ++)
        {
            if (!segment[seg_idx].layer[layer_idx].throughput){ continue;}   // in case of terminated layers

            printf("%.3Lf\t %d\t %d\t %d\t %d\t %.1f\t %.1f\t %"PRIu64"\t %.0Lf\n", 
                segment[seg_idx].layer[layer_idx].start_download_time/1000, 
                seg_idx, 
                segment[seg_idx].num_layers, 
                layer_idx,
                segment[seg_idx].layer[layer_idx].bitrate,
                segment[seg_idx].layer[layer_idx].throughput,
                segment[seg_idx].layer[layer_idx].buffer,
                segment[seg_idx].layer[layer_idx].stream_id,
                segment[seg_idx].layer[layer_idx].download_time);           
        }
    }

    // MINH killall bash ./complex
    // if (system("sudo killall bash ./complex_3g.sh")) {printf("Killed Dummynet\n");};        
    // }

    // write on file
    FILE *stats_file;
    stats_file = fopen("./statistics.txt", "w+");

    fprintf(stats_file, "StartTime\t SegID\t #Layers\t LayerID\t Bitrate\t Throughput\t Buffer\t StreamID\t DownloadTime\n");
    // for ( seg_idx = 0; seg_idx < MAX_SEGMENT_ID && segment[seg_idx].num_layers != 0; seg_idx ++)
    for ( seg_idx = 0; seg_idx < MAX_SEGMENT_ID; seg_idx ++)
    {
        for (layer_idx = 0; layer_idx < segment[seg_idx].num_layers; layer_idx ++)
        {
            if (!segment[seg_idx].layer[layer_idx].throughput){ continue;}   // in case of terminated layers

            fprintf(stats_file, "%.3Lf\t %d\t %d\t %d\t %d\t %.1f\t %.1f\t %"PRIu64"\t %.0Lf\n", 
                segment[seg_idx].layer[layer_idx].start_download_time/1000, 
                seg_idx, 
                segment[seg_idx].num_layers, 
                layer_idx,
                segment[seg_idx].layer[layer_idx].bitrate,
                segment[seg_idx].layer[layer_idx].throughput,
                segment[seg_idx].layer[layer_idx].buffer,
                segment[seg_idx].layer[layer_idx].stream_id,
                segment[seg_idx].layer[layer_idx].download_time);           
        }
    }
    printf("======================= Writing on files -E =======================\n");

    prog_cleanup(&prog);
    if (promise_fd >= 0)
        (void) close(promise_fd);

    while ((pe = TAILQ_FIRST(&client_ctx.hcc_path_elems)))
    {
        TAILQ_REMOVE(&client_ctx.hcc_path_elems, pe, next_pe);
        free(pe);
    }

    if (client_ctx.qif_fh)
        (void) fclose(client_ctx.qif_fh);

    exit(0 == s ? EXIT_SUCCESS : EXIT_FAILURE);
}
