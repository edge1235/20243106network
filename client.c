// 클라이언트 프로그램을 작성한다 (서버에 HTTP 요청을 보내고 응답을 받는 기능 수행)

#include <stdio.h>          // 표준 입출력 함수 (printf, perror 등) 사용
#include <stdlib.h>         // 표준 라이브러리 함수 (exit 등) 사용
#include <string.h>         // 문자열 처리 함수 (strlen, sprintf 등) 사용
#include <unistd.h>         // 유닉스 시스템 호출 함수 (read, write, close 등) 사용
#include <arpa/inet.h>      // IP 주소 변환 함수 (inet_addr 등) 사용
#include <sys/socket.h>     // 소켓 프로그래밍 관련 함수 (socket, connect 등) 사용

#define BUF_SIZE 2048       // 송수신 데이터 버퍼 크기 설정 (2KB)
#define PORT 8080           // 서버가 열려 있는 포트 번호를 8080으로 설정

// 에러 발생 시 에러 메시지를 출력하고 프로그램을 종료하는 함수
void error_handling(char *message)
{
    perror(message);        // 전달받은 메시지와 함께 시스템 에러 원인을 출력
    exit(1);                // 프로그램을 비정상 종료 (리턴코드 1)
}

// 메인 함수 - 프로그램의 시작점
int main(int argc, char *argv[])
{
    // 명령행 인자가 3개(프로그램명 포함 1개 + METHOD + PATH)가 아닐 경우
    if (argc != 3)
    {
        printf("Usage: %s <METHOD> <PATH>\n", argv[0]);  // 사용법 출력
        exit(1);  // 프로그램 종료
    }

    int sock;                        // 클라이언트 소켓 파일 디스크립터
    struct sockaddr_in serv_addr;     // 서버 주소 정보를 저장할 구조체
    char buf[BUF_SIZE];               // 서버로 전송할 요청 및 서버 응답을 저장할 버퍼

    // 1. 클라이언트 소켓 생성 (IPv4, TCP 스트림 소켓)
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");  // 소켓 생성 실패 시 에러 처리

    // 2. 서버 주소 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));  // serv_addr 메모리를 0으로 초기화
    serv_addr.sin_family = AF_INET;            // IPv4 주소 체계 사용
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // 서버 IP를 127.0.0.1(로컬호스트)로 설정
    serv_addr.sin_port = htons(PORT);           // 포트 번호를 네트워크 바이트 오더로 변환하여 저장

    // 3. 서버에 연결 요청
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");  // 연결 실패 시 에러 처리

    // 4. 서버로 전송할 HTTP 요청 메시지를 작성
    sprintf(buf, "%s %s HTTP/1.1\r\n\r\n", argv[1], argv[2]); 
    // 명령행 인자로 받은 METHOD와 PATH를 사용하여 HTTP/1.1 요청 포맷으로 작성
    // 예: "GET /index.html HTTP/1.1" 와 같은 형태로 전송 준비

    // 5. 작성한 요청 메시지를 서버에 전송
    write(sock, buf, strlen(buf));

    // 6. 서버로부터 응답을 읽어들임
    int str_len = read(sock, buf, BUF_SIZE - 1);  // 최대 BUF_SIZE-1 만큼 읽기
    buf[str_len] = '\0';  // 읽어들인 데이터의 끝에 널 종료 문자 추가 (문자열 완성)

    // 7. 서버로부터 받은 응답 메시지를 출력
    printf("Server Response:\n%s\n", buf);

    // 8. 통신에 사용한 소켓을 닫음
    close(sock);

    return 0;  // 프로그램 정상 종료
}
