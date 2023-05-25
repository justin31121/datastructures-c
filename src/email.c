#include <stdio.h>

#define HTTP_IMPLEMENTATION
#define HTTP_NO_SSL
#include "../../datastructures-c/libs/http.h"

#define BASE64_IMPLEMENTATION
#include "../../datastructures-c/libs/base64.h"

bool first = true;

bool __read(Http *http) {
    char buffer[HTTP_BUFFER_CAP];
    ssize_t nbytes_last;

    do {
#ifndef HTTP_NO_SSL
	if( http->conn != NULL) {
	    nbytes_last = SSL_read(http->conn, buffer, HTTP_BUFFER_CAP);
	} else {
	    nbytes_last = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
	}
#else
	nbytes_last = recv(http->socket, buffer, HTTP_BUFFER_CAP, 0);
#endif

	if(nbytes_last > 0) {
	    printf("%.*s\n", (int) (nbytes_last -1), buffer); fflush(stdout);
	}

	if(buffer[nbytes_last - 1] == '\n') {
	    break;
	} else {
	    printf("'%c'\n", buffer[nbytes_last - 1]); fflush(stdout);
	}

    }while(nbytes_last > 0);
    return true;
}

int main() {    
    Http http;
    const char *hostname = "mx01.emig.gmx.net";
    assert(http_init3(&http, hostname, strlen(hostname), false, 25));
    assert(__read(&http));

    const char *buf = "HELO example.com\r\n";
    printf("\t'%s'\n", buf);
    assert(http_send_len(buf, strlen(buf), &http));
    assert(__read(&http));

    const char *buf2 = "MAIL FROM:<sending@example.com>\r\n";
    printf("\t'%s'\n", buf2);
    assert(http_send_len(buf2, strlen(buf2), &http));
    assert(__read(&http));

    const char *buf3 = "RCPT TO:<justinschartner00@gmx.net>\r\n";
    printf("\t'%s'\n", buf3);
    assert(http_send_len(buf3, strlen(buf3), &http));
    assert(__read(&http));

    //DATA\n
    const char *buf6 = "DATA\r\n";
    printf("\t'%s'\n", buf6);
    assert(http_send_len(buf6, strlen(buf6), &http));
    assert(__read(&http));

    const char *buf4 =
	"Message-Id: <%s>\r\n"
	"From: <sending@example.com>\r\n"
	"To: <justinschartner00@gmx.net>\r\n"
	"Subject: Testmail\r\n"
	"\r\n"
	"body\r\n"
	".\r\n";
    char body[1024 * 10];
    snprintf(body, sizeof(body), buf4, "testtest@example");

    printf("\t'%s'\n", body);
    assert(http_send_len(body, strlen(body), &http));
    assert(__read(&http));

    const char *buf5 = "QUIT\r\n";
    printf("\t'%s'\n", buf5);
    assert(http_send_len(buf5, strlen(buf5), &http));

    assert(__read(&http));

       
    printf("ok!\n");
    return 0;
}

