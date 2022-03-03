#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_ADDR "192.168.0.31"
#define SERVER_PORT 55555

// レスポンスの値
enum {
  OK,
  NG
};


int main(void) {

  int ret_code = 0;
  char *buf;
  uint32_t buf_len = 0;

  int fd_accept = -1;  // 接続受け付け用のFD
  int fd_other = -1;  // sendとかrecv用のFD
  ssize_t size = 0;
  int flags = 0;  // MSG_WAITALLとかMSG_NOSIGNALをよく使うけど今回はサンプルなのでフラグは無し

  int response = -1;  // レスポンス考えるの面倒だったので0:OK/1:NGで。

  // ソケットアドレス構造体
  struct sockaddr_in sin, sin_client;
  memset(&sin, 0, sizeof(sin));
  memset(&sin_client, 0, sizeof(sin_client));

  socklen_t socklen = sizeof(sin_client);

  // インターネットドメインのTCPソケットを作成
  fd_accept = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_accept == -1) {
    printf("failed to socket(errno=%d:%s)\n", errno, strerror(errno));
    return -1;
  }

  // ソケットアドレス構造体を設定
  sin.sin_family = AF_INET;          // インターネットドメイン(IPv4)
  sin.sin_addr.s_addr = INADDR_ANY;  // 全てのアドレスからの接続を受け入れる(=0.0.0.0)
  sin.sin_port = htons(SERVER_PORT);  // 接続を待ち受けるポート

  // 上記設定をソケットに紐づける
  ret_code = bind(fd_accept, (const struct sockaddr *)&sin, sizeof(sin));
  if (ret_code == -1) {
    printf("failed to bind(errno=%d:%s)\n", errno, strerror(errno));
    close(fd_accept);
    return -1;
  }

  // ソケットに接続待ちを設定する。10はバックログ、同時に何個迄接続要求を受け付けるか。
  ret_code = listen(fd_accept, 10);
  if (ret_code == -1) {
    printf("failed to listen(errno=%d:%s)\n", errno, strerror(errno));
    close(fd_accept);
    return -1;
  }

  // 無限ループのサーバー処理
  while(1) {
    printf("accept wating...\n");
    // 接続受付処理。接続が来るまで無限に待つ。recvのタイムアウト設定時はそれによる。シグナル来ても切れるけど。
    fd_other = accept(fd_accept, (struct sockaddr *)&sin_client, &socklen);
    if (fd_other == -1) {
      printf("failed to accept(errno=%d:%s)\n", errno, strerror(errno));
      continue;
    }

    printf("test00\n");

    // ヘッダーの受信(今回はデータサイズのみだが...)
    size = recv(fd_other, &buf_len, sizeof(buf_len), flags);
    if (size < sizeof(buf_len)) {
      // partial recvパターンも今回はエラーとする
      printf("failed to recv data size(errno=%d:%s)\n", errno, strerror(errno));
      response = NG;
      goto send_response;
    }

    printf("buf_len = %d\n", buf_len);

    // データ本体の受信用にバッファーを確保
    buf = malloc(buf_len);
    if (buf == NULL) {
      printf("failed to malloc");
      response = NG;
      goto send_response;
    }

    // データ本体の受信
    size = recv(fd_other, buf, buf_len, flags);
    if (size < buf_len) {
      // partial recvパターンも今回はエラーとする
      printf("failed to recv data(errno=%d:%s)\n", errno, strerror(errno));
      response = NG;
      free(buf);
      goto send_response;
    }

    printf("received data:%s", buf);
    free(buf);
    response = OK;

send_response:
    // レスポンスの送信
    size = send(fd_other, &response, sizeof(response), flags);
    if (size < sizeof(response)) {
      // partial sendパターンも今回はエラーとする
      printf("failed to send data(errno=%d:%s)\n", errno, strerror(errno));
      printf("test01\n");
    } else {
      printf("success to send responce\n");
    }

    // send/recv用のソケットを閉じてFDを-1に戻しておく
    close(fd_other);
    fd_other = -1;
  }

  return 0;
}

