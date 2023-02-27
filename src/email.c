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
   
    const char *buf = "HELO from.bar.net\n";
    assert(http_send_len(buf, strlen(buf), &http));
    assert(__read(&http));

    const char *buf11 = "AUTH LOGIN\n";
    assert(http_send_len(buf11, strlen(buf11), &http));
    assert(__read(&http));
    
    const char *buf12 = "aGFucw==";
    assert(http_send_len(buf12, strlen(buf12), &http));
    assert(__read(&http));

    const char *buf13 = "c2Nobml0emVsbWl0a2FydG9mZmVsc2FsYXQ=";
    assert(http_send_len(buf13, strlen(buf13), &http));
    assert(__read(&http));
	
    const char *buf2 = "MAIL FROM: from@from-host.org\n";
    assert(http_send_len(buf2, strlen(buf2), &http));
    assert(__read(&http));

    const char *buf3 = "RCPT TO: to@to-host.org\nDATA\n";
    assert(http_send_len(buf3, strlen(buf3), &http));
    assert(__read(&http));

    const char *buf4 = "From: from@from-host.org\n"
	"To: to@to-host.org\n"
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
