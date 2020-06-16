#define MAX_SEGMENT_ID  200
#define MAX_LAYER_ID    4   // 1 BL + 3 ELs
#define TRUE            true
#define FALSE           false

const int   MINH_BITRATE_SET 		[MAX_LAYER_ID] = {1500, 2000, 3500, 5000};   //Kbps
const int   MINH_SUM_BITRATE_SET	[MAX_LAYER_ID] = {1500, 3500, 7500, 12500}; //Kbps
const char* MINH_PATH_SET			[MAX_LAYER_ID] = {"/file-187K", "/file-250K", "/file-437K", "/file-625K"};

const int 	MINH_BUFFER_SIZE = 20000;
const int   MINH_REBUF_THRESHOLD_EXIT = 5000;
const int   MINH_SD = 1000;

const int 	RETRANS_BUFF_TRIGGER_ON = MINH_BUFFER_SIZE/2;
const int 	RETRANS_BUFF_THRES = MINH_BUFFER_SIZE/4;

// data record
// lsquic_time_t   start_stream_recorder   [MAX_SEGMENT_ID];           
// long double     download_time_recorder  [MAX_SEGMENT_ID];
// double          throughput_recorder     [MAX_SEGMENT_ID];    // in Kbps
// double          buffer_recorder         [MAX_SEGMENT_ID]; // in s
// int             bitrate_recorder        [MAX_SEGMENT_ID][2];
int             retrans_seg_id_recorder [MAX_SEGMENT_ID];

lsquic_time_t   streaming_start_time;

static int      minh_client_seg = -1;
static int 		minh_retrans_seg = -1;
static double   minh_cur_buf = 0;   // in milisecon
static double   minh_throughput = 0; // in Kbps
static double 	estimated_throughput = 0;

static bool     minh_rebuf = TRUE;
static bool     retrans_check = FALSE;
static bool     set_prior_check = FALSE;
static bool 	minh_retrans_trigger = FALSE;
static bool 	minh_retrans_extension = TRUE;
static bool 	retransmitting = FALSE;

static char*    next_path = "/file-123K";
static char*    retrans_path = "/file-432K";

static unsigned termn_seg_id = 0;
// int             seg_id_stream_id_map[MAX_SEGMENT_ID*2][2]; //[seg_id, stream_id]

static int      priority_next = 2;
static int      priority_retrans = 8;
static int 		retrans_num_segs = 0;
static int 		retrans_seg_id = 0;	
static int 		cur_layer_id = 1;

static unsigned long	downloaded_bytes_2on_close = 0;

struct seg_layers   // a stream = 1 layer
{
   int              bitrate;
   unsigned long    stream_id;
   double           throughput;
   double      		buffer;

   long double	    start_download_time;
   long double      download_time;
} seg_layer;

struct segments
{
    int         num_layers;
    seg_layers  layer[];
    // int         id;
    

} segment;