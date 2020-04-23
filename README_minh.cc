command
	./http_client -s www.google.com -p / -o version=Q050

	___http_client.c:main():1476

	___http_client.c:create_connections():245

	___http_client.c:http_client_on_new_conn():296

	___http_client.c:create_streams():283

	___http_client.c:http_client_on_hsk_done():397

	___http_client.c:hsk_status_ok():389

	___http_client.c:hsk_status_ok():389

	___http_client.c:hsk_status_ok():389

	___http_client.c:update_sample_stats():96

	___http_client.c:http_client_on_new_stream():498

	___http_client.c:http_client_on_write():642

	___http_client.c:send_headers():546

	___http_client.c:http_client_on_write():642

1. Creat Privacy Enhanced Mail (PEM) files for SSL Certificate Installations
	$ openssl req -newkey rsa:2048 -new -nodes -x509 -days 3650 -keyout key.pem -out cert.pem
	$ chmod +x key.pem cert.pem

2. Run the LSQUIC server
	./http_server -c www.example.com,cert.pem,key.pem -s 0.0.0.0:4433 -L debug 2>server_minh.out

	Original Server process:
		http_server.c:http_server_on_new_conn():76

		http_server.c:http_server_on_new_stream():243

		http_server.c:interop_server_hset_create():1204

		http_server.c:interop_server_hset_prepare_decode():1218

		http_server.c:interop_server_hset_add_header():1241

		http_server.c:interop_server_hset_prepare_decode():1218

		http_server.c:interop_server_hset_add_header():1241

		http_server.c:interop_server_hset_prepare_decode():1218

		http_server.c:interop_server_hset_add_header():1241

		http_server.c:interop_server_hset_prepare_decode():1218

		http_server.c:interop_server_hset_add_header():1241

		http_server.c:interop_server_hset_prepare_decode():1218

		http_server.c:interop_server_hset_add_header():1241

		http_server.c:interop_server_hset_add_header():1241

		http_server.c:http_server_interop_on_read():800

		http_server.c:find_handler():739

		http_server.c:http_server_interop_on_write():1122

		http_server.c:send_headers2():999

		http_server.c:http_server_interop_on_write():1122

		http_server.c:http_server_on_close():641

		http_server.c:interop_server_hset_destroy():1308

		http_server.c:http_server_on_conn_closed():100
------------------------
3. Run client
	./http_client -H www.example.com -s 127.0.0.1:4433 -p / -L debug 2>client_m.out
	-------
	Original Client process
		http_client.c:main():1488

		http_client.c:create_connections():246

		http_client.c:http_client_on_new_conn():297

		http_client.c:create_streams():284

		http_client.c:http_client_on_new_stream():499

		[MINH] info: created new stream, path: /

		[MINH] info: randomly_reprioritize_streams: 0

		http_client.c:http_client_on_hsk_done():398

		http_client.c:hsk_status_ok():390

		http_client.c:hsk_status_ok():390

		http_client.c:hsk_status_ok():390

		http_client.c:update_sample_stats():96

		[MINH] info: update_sample_stats: 0x560a28651a00: 9302
		http_client.c:http_client_on_write():647

		http_client.c:http_client_on_write():684

		http_client.c:send_headers():550

		[MINH] info: path; /

		http_client.c:http_client_on_write():647

		http_client.c:http_client_on_write():652

		http_client.c:http_client_on_write():677

		http_client.c:http_client_on_read():702

		http_client.c:update_sample_stats():96

		[MINH] info: update_sample_stats: 0x560a28651a40: 906HTTP/1.1 200 OK
	server: lsquic/2.13.2
	content-type: text/html
	content-length: 940

	<html>
	   <head>
	       <title>LiteSpeed IETF QUIC Server Index Page</title>
	   </head>
	   <body>
	       <h1>LiteSpeed IETF QUIC Server Index Page</h1>
	       <p>Hello!  Welcome to the interop.  Available services:
	       <ul>
	           <li><b>POST to /cgi-bin/md5sum.cgi</b>.  This will return
	                   MD5 checksum of the request body.
	           <li><b>GET /123K</b> or <b>GET /file-123K</b>.  This will return
		http_client.c:http_client_on_read():702

	                   requested number of payload in the form of repeating text
	                   by Jerome K. Jerome.  The size specification must match
	                   (\d+)[KMG]? and the total size request must not exceed
	                   2 gigabytes.  Then, you will get back that many bytes
	                   of the <a
	                       href=http://www.gutenberg.org/cache/epub/849/pg849.txt
	                                                       >beloved classic</a>.
	       </ul>
	   </body>
	</ht
		http_client.c:http_client_on_read():702
	ml>

		http_client.c:http_client_on_read():702

		http_client.c:update_sample_stats():96

		[MINH] info: update_sample_stats: 0x560a28651a80: 70
		http_client.c:http_client_on_close():808

		http_client.c:http_client_on_conn_closed():344

		http_client.c:create_another_conn_or_stop():324

		http_client.c:create_connections():246
------------------------------------------------------------
4. 	Explanation
4.1 Server
	/http_server -c 127.0.0.1,cert.pem,key.pem -s 127.0.0.1:1234 -L debug 2>server_m.out
	./http_server -c 192.168.168.131,cert.pem,key.pem -s 192.168.168.131:1234 -o support_push=1 -L debug 2>server_m.out

4.2 Client
		./http_client -H 127.0.0.1 -s 127.0.0.1:4433 -p /file-4M -L debug 2>client_m.out
		./http_client -H 192.168.168.131 -s 192.168.168.131:1234 -p /file-200K -p /file-600K -R 2 -r 2 -w 2 -t -L debug 2>client_m.out

"	-H www.example.com
		client_ctx.hostname = www.example.com;
        prog.prog_hostname = www.example.com; 	
    -s 127.0.0.1:4433
    	`host` header.
	-p /
		pe->path = optarg;   
   	-n  	Number of concurrent connections.  Defaults to 1.\n
   	-r  	Total number of requests to send.  Defaults to 1.\n
    -R 		Maximum number of requests per single connection
    	client_ctx.hcc_reqs_per_conn = atoi(optarg);
	-E 		Enable randomly_reprioritize_streams
	-w 		Number of concurrent requests per single connection.\n
		client_ctx.hcc_cc_reqs_per_conn = atoi(optarg);
	-t
        stats_fh = stdout;		"

	unsigned            ch_n_reqs;    /* This number gets decremented as streams are closed and
	                                    * incremented as push promises are accepted.
	                                    */		 	
	unsigned            hcc_cc_reqs_per_conn;  // Number of concurrent requests per single connection.\n"

struct lsquicstream {
    /* Content length specified in incoming `content-length' header field.
     * Used to verify size of DATA frames.
     */
    unsigned long long              sm_cont_len;

request path
	http_client_on_new_stream()
		lsquic_stream_ctx_t st_h->path = st_h->client_ctx->hcc_cur_pe->path; 

-------------------------------------------------------------
5. Server push
	./http_server -c 127.0.0.1,cert.pem,key.pem -s 127.0.0.1:1234 -o version=Q043 -L debug 2>server_m.out
5.1 server Enable
Default: enable server push
	set_engine_option()
		support_push
	    /**
	     * Setting this value to 0 means that
	     *
	     * For client:
	     *  a) we send a SETTINGS frame to indicate that we do not support server
	     *     push; and
	     *  b) All incoming pushed streams get reset immediately.
	     * (For maximum effect, set es_max_streams_in to 0.)
	     *
	     * For server:
	     *  lsquic_conn_push_stream() will return -1.
	     */
	    int             es_support_push;		
	    lsquic_conn_push_stream()
Cac bien va ham lien quan:
	lsquic_frame_reader.c 
		decode_and_pass_payload(struct lsquic_frame_reader *fr)
			hset = fr->fr_hsi_if->hsi_create_header_set(fr->fr_hsi_ctx,
                            READER_PUSH_PROMISE == fr->fr_state.reader_type);
	Frames:
		HTTP_FRAME_HEADERS
		HTTP_FRAME_PUSH_PROMISE
		HTTP_FRAME_CONTINUATION
		HTTP_FRAME_SETTINGS
		HTTP_FRAME_PRIORITY


	lsquic_full_conn.c 
		maybe_send_settings (struct full_conn *conn)
			/* Once handshake has been completed, send settings to peer if appropriate.*/		

Server handles request using Regular expression
	"^/file-([0-9][0-9]*)([KMG]?)\\?push=([^&]*)$"
	'^': setting the position for match. ==> the match must start at the beginning of the string/line
	'*': preceding character is to be used for more than just 1 time
		The regular expression ab*c will give ac, abc, abbc, 
		abbbc….ans so on
	'?': preceding character may/may NOT be present
		We may write the format for document file as – “docx?”
		The ‘?’ tells the computer that x may or may not be 
		present in the name of file format.	
	'\?': match '?' symbol
	'[^&]': match any single character except '&'
	'$': end of string to match

	LSQ_INFO("found handler for %s %s", st_h->req->method_str, st_h->req->path);
		st_h->req->method_str = 'GET'
		st_h->req->path 	 = '/file-50K?push=file-100k'

Client
	change #define CLIENT_PUSH_SUPPORT 0 from 0 to 1 in lsquic_full_conn_ietf.c. 
	This will change the clients transport parameters and allow the server to push	
==> 	./http_client -H 127.0.0.1 -s 127.0.0.1:1234 -p /file-50K?push=/file-100K -o version=Q043 -t -L debug 2>client_m.out
-------------------------------------------------------------
6. Stream priority (just for version Q043)
In prog.c file
prog_connect (struct prog *prog, unsigned char *zero_rtt, size_t zero_rtt_len)
{
    struct service_port *sport;

    sport = TAILQ_FIRST(prog->prog_sports);
    if (NULL == lsquic_engine_connect(prog->prog_engine, LSQVER_043,

6.1 Send 2 requests at the same time
	./http_client -H 192.168.168.129 -s 192.168.168.129:4433 -p /file-200K -p /file-100K -R 2 -r 2 -w 2 -t -L debug 2>client_m.out
	./http_client -H 127.0.0.1 -s 127.0.0.1:1234 -p /file-200K -p /file-100K -R 2 -r 2 -w 2 -t -L debug 2>client_m.out
  Explanation:
  	-p /file-200K -p /file-100K
  		"request 2 files: "file-200k" and "file-100K""
  	-R 2
  		"Maximum number of requests per single connection"
  	-r 2
  		client_ctx.hcc_total_n_reqs = atoi(optarg);
  		"Total # of request ==> ??? Set a big value"
  	-w 2	
  		client_ctx.hcc_cc_reqs_per_conn = atoi(optarg);
  		"Number of concurrent requests per single connection"
6.2 Set priority weights
	http_client.c
		lsquic_stream_set_priority(stream, new_prio);

	test_frame_reader.c
        struct headers {
            uint32_t                stream_id;
            uint32_t                oth_stream_id;
            unsigned short          weight;
            signed char             exclusive;
            unsigned char           flags;
            unsigned                size;
            unsigned                off;
            char                    buf[0x100];
        }	
-------------------------------------------------------------
7. Stream termination
    /* test stream_reset after some number of read bytes */
    if (client_ctx->hcc_reset_after_nbytes &&
        s_stat_downloaded_bytes > client_ctx->hcc_reset_after_nbytes)
    {
        lsquic_stream_reset(stream, 0x1);
        break;
    }
    /* test retire_cid after some number of read bytes */	

     error_code = 0x01; /* QUIC_INTERNAL_ERROR */
    [0x01] = QUIC_FRAME_RST_STREAM,

    http_client.cc 
    	when receiving a DATA frame from server, call http_client_on_read()
    		terminate in this func by 
    			lsquic_stream_reset(stream, 0x1);

8. Run testbed
8.1 Server
	./http_server -c 172.16.23.1,cert.pem,key.pem -s 172.16.23.1:1234
	./http_server -c 172.16.23.1,cert.pem,key.pem -s 172.16.23.1:1234 -L debug 2>server_m.out

8.2 Client	
	// H2BR : -Z
	./http_client -H 172.16.23.1 -s 172.16.23.1:1234 -p /file-20000K -R 1000 -r 800 -A AGG -Z -w 1 -t -L debug 2>client_m.out
	
	// SVC
	./http_client -H 172.16.23.1 -s 172.16.23.1:1234 -p /file-20000K -R 1000 -r 500 -A BACKFILLING -w 1 -t -L debug 2>client_m.out