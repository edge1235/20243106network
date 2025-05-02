#include <stdio.h>          // 표준 입출력 함수 (printf, perror 등) 사용
#include <stdlib.h>         // 표준 라이브러리 함수 (exit 등) 사용
#include <string.h>         // 문자열 처리 함수 (strlen, strcmp, strcat 등) 사용
#include <unistd.h>         // 유닉스 시스템 호출 함수 (read, write, close 등) 사용
#include <arpa/inet.h>      // IP 주소 변환 함수 (htonl, htons 등) 사용
#include <sys/socket.h>     // 소켓 프로그래밍 기본 함수 및 구조체 사용
#include <sys/stat.h>       // 파일 상태 정보를 가져오는 함수 (stat 등) 사용
#include <fcntl.h>          // 파일 열기 제어 함수 (open, O_RDONLY 등) 사용

#define BUF_SIZE 2048       // 버퍼 크기를 2048 바이트로 설정
#define PORT 8080           // 서버가 열어둘 포트 번호 8080으로 설정

// 에러가 발생했을 때 메시지를 출력하고 프로그램을 종료하는 함수
void error_handling(char *message)
{
    perror(message);    // 시스템 에러 메시지 출력
    exit(1);            // 프로그램 비정상 종료
}

// 클라이언트에게 HTTP 응답을 전송하는 함수
void send_response(int clnt_sock, char *status, char *content_type, char *body)
{
    char buf[BUF_SIZE]; // 응답 메시지를 저장할 버퍼
    // HTTP 응답 메시지를 버퍼에 작성
    sprintf(buf, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s",
            status, content_type, strlen(body), body);
    // 작성한 응답 메시지를 클라이언트 소켓으로 전송
    send(clnt_sock, buf, strlen(buf), 0);
}

// 클라이언트의 요청을 처리하는 함수
void handle_request(int clnt_sock, char *request)
{
    char method[8], path[128];  // 요청의 메서드(GET, POST 등)와 경로를 저장할 변수
    sscanf(request, "%s %s", method, path);   // 요청에서 메서드와 경로 추출

    char filepath[256] = "./files";  // 파일이 저장된 디렉토리 경로 (상대경로)
    strcat(filepath, path);          // 요청한 경로를 디렉토리 경로에 이어붙임

    // 요청 메서드가 GET인 경우
    if (strcmp(method, "GET") == 0)
    {
        int fd = open(filepath, O_RDONLY);   // 파일을 읽기 전용으로 오픈
        if (fd == -1)
        {
            // 파일이 존재하지 않으면 404 응답
            send_response(clnt_sock, "404 Not Found", "text/plain", "File not found");
        }
        else
        {
            char body[BUF_SIZE];
            read(fd, body, BUF_SIZE); // 파일 내용을 읽음
            close(fd);                // 파일 닫기
            // 읽어온 파일 내용을 200 OK 응답과 함께 전송
            send_response(clnt_sock, "200 OK", "text/plain", body);
        }
    }
    // 요청 메서드가 HEAD인 경우
    else if (strcmp(method, "HEAD") == 0)
    {
        struct stat st;
        if (stat(filepath, &st) == -1)
            // 파일이 없으면 404 응답
            send_response(clnt_sock, "404 Not Found", "text/plain", "");
        else
            // 파일이 있으면 100 Continue 응답
            send_response(clnt_sock, "100 Continue", "text/plain", "");
    }
    // 요청 메서드가 POST인 경우
    else if (strcmp(method, "POST") == 0)
    {
        // 파일이 없을 때만 새로 생성 (O_EXCL 옵션)
        int fd = open(filepath, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd == -1)
            // 이미 파일이 존재하면 409 Conflict 응답
            send_response(clnt_sock, "409 Conflict", "text/plain", "File exists");
        else
        {
            // 파일을 새로 만들고 기본 내용을 작성
            write(fd, "New file created by POST", 24);
            close(fd);  // 파일 닫기
            // 파일 생성 완료 메시지와 함께 201 Created 응답
            send_response(clnt_sock, "201 Created", "text/plain", "File created");
        }
    }
    // 요청 메서드가 PUT인 경우
    else if (strcmp(method, "PUT") == 0)
    {
        // 기존 파일을 열고 내용을 덮어쓰기
        int fd = open(filepath, O_WRONLY | O_TRUNC);
        if (fd == -1)
            // 파일이 없으면 404 응답
            send_response(clnt_sock, "404 Not Found", "text/plain", "File not found");
        else
        {
            // 파일 내용을 수정
            write(fd, "File modified by PUT", 20);
            close(fd);  // 파일 닫기
            // 파일 수정 완료 메시지와 함께 200 OK 응답
            send_response(clnt_sock, "200 OK", "text/plain", "File updated");
        }
    }
    // 지원하지 않는 메서드일 경우
    else
    {
        send_response(clnt_sock, "400 Bad Request", "text/plain", "Invalid Method");
    }
}

// 서버 메인 함수
int main()
{
    int serv_sock, clnt_sock;               // 서버 소켓과 클라이언트 소켓
    struct sockaddr_in serv_addr, clnt_addr; // 서버/클라이언트 주소 정보를 담을 구조체
    socklen_t clnt_adr_sz;                   // 클라이언트 주소 구조체의 크기
    char buf[BUF_SIZE];                      // 요청/응답 데이터를 저장할 버퍼

    // 1. 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);  // IPv4, TCP
    if (serv_sock == -1)
        error_handling("socket() error");

    // 2. 서버 주소 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;          // IPv4 사용
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP에서 접속 허용
    serv_addr.sin_port = htons(PORT);         // 포트 번호 설정

    // 3. 서버 소켓과 주소를 바인딩
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    // 4. 연결 요청 대기
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("Server listening on port %d...\n", PORT);

    // 5. 클라이언트 연결 수락 및 요청 처리 루프
    while (1)
    {
        clnt_adr_sz = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_adr_sz);
        if (clnt_sock == -1)
            continue;   // 연결 실패 시 다음 요청 대기

        // 클라이언트로부터 요청 읽기
        int str_len = read(clnt_sock, buf, BUF_SIZE - 1);
        buf[str_len] = '\0';   // 문자열 끝을 표시

        // 읽은 요청을 처리
        handle_request(clnt_sock, buf);

        // 클라이언트 소켓 닫기
        close(clnt_sock);
    }

    // 서버 소켓 닫기 (실제 무한 루프라 실행되지는 않지만, 구조상 필요)
    close(serv_sock);
    return 0;
}
