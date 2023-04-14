#include <stdio.h>

#define HTTP_IMPLEMENTATION
#include "../../datastructures-c/libs/http.h"

#define BASE64_IMPLEMENTATION
#include "../../datastructures-c/libs/base64.h"

bool __read(Http *http) {
    char buffer[HTTP_BUFFER_CAP];
    ssize_t nbytes_last;
    do {
	if( http->conn != NULL) {
	    nbytes_last = SSL_read(http->conn, buffer, HTTP_BUFFER_CAP);
	} else {
	    nbytes_last = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
	}

	if(nbytes_last > 0) {
	    printf("%.*s\n", (int) (nbytes_last -1), buffer);
	}

	if(buffer[nbytes_last - 1] == '\n') {
	    break;
	}
    }while(nbytes_last > 0);
    return true;
}

int main() {
    Http http;
    const char *hostname = "smtp.gmail.com";
    
    assert(http_init3(&http, hostname, strlen(hostname), true, 465));
    assert(__read(&http));
   
    const char *buf = "EHLO sender.example.com\n";
    assert(http_send_len(buf, strlen(buf), &http));
    assert(__read(&http));

    const char *buf11 = "AUTH XOAUTH2 ";
    assert(http_send_len(buf11, strlen(buf11), &http));

    char buffer[10243];
    size_t buffer_size;
    
    char buf12[] = "user=justin.schartn00@gmail.comXauth=Bearer P36j89w1s!XX";
    size_t buf12_len = ((sizeof(buf12) - 1)/sizeof(buf12[0]));
    for(size_t i=0;i<buf12_len;i++) {
	if('X' == buf12[i]) {
	    buf12[i] = 0x01;
	}
    }
    assert(base64_encode(buf12, strlen(buf12), buffer, 10243, &buffer_size));
    buffer[buffer_size] = '\n';
    assert(http_send_len(buffer, buffer_size + 1, &http));
    assert(__read(&http));
	
    const char *buf2 = "MAIL FROM: justin.schartner00@gmail.com\n";
    assert(http_send_len(buf2, strlen(buf2), &http));
    assert(__read(&http));

    const char *buf3 = "RCPT TO: justin.zweite@gmail.com\n";
    assert(http_send_len(buf3, strlen(buf3), &http));
    assert(__read(&http));

    //DATA\n
    const char *buf6 = "DATA\n";
    assert(http_send_len(buf6, strlen(buf6), &http));
    assert(__read(&http));
    
    const char *buf4 = "From: justin.schartner00@gmail.com\n"
	"To: justin.zweite@gmail.com\n"
	"Subject: test\n"
	"\n"
	"body\n"
	".\n";
    assert(http_send_len(buf4, strlen(buf4), &http));
    assert(__read(&http));

    const char *buf5 = "QUIT\n";
    assert(http_send_len(buf5, strlen(buf5), &http));

    assert(__read(&http));

    http_free(&http);
    return 0;
}
