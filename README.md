# TCP 통신을 이용한 다중 채팅 프로그램
- 본 코드는 VEDA 1기 미니 프로젝트 과제용으로 작성되었습니다.
- 자세한 정보는 : https://dynamicseagull.tistory.com/123

---

## 문제 설명

### 구동환경
- **채팅 서버**: 라즈베리파이
- **채팅 클라이언트**: 우분투 (가상머신)

### 구현 기능
- 채팅방 로그인 / 로그아웃 기능
- 채팅 시 로그인 된 ID 뒤에 메세지 표시
- 빌드 시 `make` / `cmake` 사용

### 프로젝트 제약 조건
- 서버는 멀티프로세서 (`fork()`) 사용
- 부모 프로세스와 자식 프로세스들 사이에 IPC(파이프만) 사용
- 채팅 서버를 데몬으로 등록
- `select()` / `epoll()` 함수 사용 불가
- 메시지 큐 / 공유 메모리 사용 불가
- C로만 구현 (C++ 사용 불가)
  
---

## 환경 세팅

### 네트워크 설정
- 가상머신의 네트워크 설정을 브리지 모드로 설정

### 서버 (server folder)
- tcp_server.c
- userList.txt
- CMakeLists.txt
```
git clone https://github.com/Dynamic-Seagull/TCP_multi_chatting.git
cd TCP_multi_chatting
rm -rf client
rm -f README.md
cmake CMakeLists.txt
make
./tcp_server
```


### 클라이언트 (client folder)
- tcp_client.c
- CMakeLists.txt
```
git clone https://github.com/Dynamic-Seagull/TCP_multi_chatting.git
cd TCP_multi_chatting
rm -rf server
rm -f README.md
cmake CMakeLists.txt
make
./tcp_client [server ip address]
```

