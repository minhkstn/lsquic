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
	Explanation
		./http_client -H www.example.com -s 127.0.0.1:4433 -p / -L debug 2>client_m.out
	-H www.example.com
		client_ctx.hostname = www.example.com;
        prog.prog_hostname = www.example.com; 		