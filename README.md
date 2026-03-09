# Boost.Asio & MongoDB를 활용한 비동기 채팅 서버

<br>

## 소개

Linux 환경에서 Boost.Asio 기반 비동기 네트워크 서버를 구현하고  
채팅 로그를 MongoDB에 비동기 저장 및 조회하는 채팅 서버 프로젝트입니다.

이 프로젝트는 기존에 개발했던 Windows IOCP 기반 서버 경험을 Linux 환경으로 확장하고,  
비동기 네트워크 처리와 DB 작업 분리, gdb 디버깅과 테스트 클라이언트를 통해     
병목 지점을 분석하고 개선하는 것을 목표로 개발되었습니다.

<br> 

#### ㅇ 개발 기간 
- 2025.02.25 ~ 2025.03.06 : Boost.Asio & MongoDB를 활용한 비동기 채팅 서버

<br> 

## 기술 스택

### Network
- **Boost.Asio**
  - async_accept / async_read / async_write 기반 비동기 TCP 서버
  - strand 기반 세션 상태 직렬화

### Database
- **MongoDB**
  - mongocxx / bsoncxx C++ driver
  - 채팅 로그와 같은 비정형 데이터를 유연하게 저장

### Concurrency
- multi-thread io_context 실행
- strand 기반 race condition 방지
- DB 작업을 worker thread로 분리

### Test & Debug
- Python 기반 load test client
- gdb stack trace 분석
- 대규모 동시 요청 테스트

<br>


## 프로젝트 목표

#### 1. Linux 환경 비동기 서버 구현

- Windows IOCP 기반 서버 경험을 Linux 환경으로 확장
- Boost.Asio 기반 async 네트워크 서버 설계

#### 2. 채팅 로그 영구 저장 시스템 구현

- 채팅 로그를 MongoDB에 저장
- 채팅 로그 MongoDB에서 불러오기
- NoSQL 기반 유연한 로그 구조 설계

#### 3. 비동기 네트워크 + DB 작업 분리

- 네트워크 IO thread와 DB 작업 thread 분리
- IO blocking 최소화

#### 4. 부하 테스트 기반 성능 개선 경험

- 동시 접속 및 동시 요청 테스트 수행
- 병목 지점 분석 및 구조 개선

<br> 


## 주요 구현 기능

#### 1. Boost.Asio 기반 비동기 채팅 서버

- async_accept 기반 클라이언트 연결 처리
- async_read / async_write 기반 메시지 처리
- session / session manager 구조 분리
- broadcast 채팅 기능 구현

#### 2. 메시지 프레이밍 처리

- async_read_until('\n') 기반 메시지 프레이밍
- streambuf에서 한 줄씩 파싱 후 broadcast

#### 3. Multi-thread 환경 대응

- io_context worker thread 실행
- strand 기반 세션 상태 접근 직렬화
- write queue를 통한 async_write 직렬화

#### 4. MongoDB 채팅 로그 저장

- MongoDB driver (mongocxx / bsoncxx) 연동
- 채팅 로그 저장 스키마 (roomKey, sender, message, cur_ms)
- 친구 이전 채팅 불러오기
- 채팅 로그 비동기 저장 구조 구현

#### 5. DM 채팅 로그 조회 기능

- 친구 채팅방 입장 시 최근 채팅 로그 조회
- MongoDB query 기반 채팅 히스토리 전달

<br>

## Architecture

Boost.Asio 기반 비동기 네트워크 서버 구조로  
네트워크 IO 처리와 MongoDB 작업을 분리하여 동시 요청 처리 성능을 확보했습니다.

<img width="1161" height="101" alt="Arichitecture drawio" src="https://github.com/user-attachments/assets/40a2b31c-ba29-4ff1-8d5c-879e9bb13c26" />

<br>

- **ChatServer**
  - Boost.Asio 기반 TCP 서버
  - 클라이언트 연결 수락 및 세션 생성

- **Session**
  - 클라이언트 연결 단위
  - async_read / async_write 기반 메시지 처리

- **SessionManager**
  - 연결된 세션 관리
  - broadcast 메시지 전달

- **MongoDBManager**
  - 채팅 로그 저장 및 조회
  - Read / Write Queue 기반 MongoDB 작업 처리
  - MongoDB 작업을 worker thread에서 처리


<br>  

## Flow Chart


- #### World Chat
<img width="641" height="561" alt="World Chat drawio" src="https://github.com/user-attachments/assets/abc79f98-880a-4847-b74e-c8ad966035ff" />

<br>

- #### DM History Load
<img width="641" height="531" alt="DM History Load drawio" src="https://github.com/user-attachments/assets/e1de1660-d947-4df2-9525-b0ec3793921f" />


<br>  
<br>  

## Demo

![리눅스 채팅 서버 테스트](https://github.com/user-attachments/assets/9432f886-ab19-426b-9d16-c767ae29f463)


<br>

## Performance Test

### Test Method

Python 기반 load test client를 제작하여 동시 접속 및 동시 요청 환경에서 서버의 안정성과 응답 성능을 테스트


### Test Target

1000 clients / FR 20 requests 환경에서 서버의 안정성과 응답 성능 검증


### Problem

부하 테스트 중 서버 크래시가 발생 (50 clients / FR 20 requests)

<br>

gdb stack trace 분석 결과,  
MongoDB read 작업이 `Session::Read()` 내부에서 동기적으로 수행되며  
IO thread blocking과 MongoDB 접근 경합이 발생하는 것을 확인

---

### Solution

#### Before

기존에는 친구 채팅 로그 조회 요청이 들어오면 `Session::Read()` 내부에서 MongoDB read를 직접 수행했습니다.

Session::Read()  
→ MongoDB read  
→ IO thread blocking

#### After

MongoDB read 작업을 별도의 queue + worker thread 구조로 분리하여 
네트워크 IO 처리와 DB 작업을 분리했습니다.

Session::Read()  
→ Read Queue  
→ MongoDB Read Worker Thread  
→ Callback  
→ Session Strand

<br>

이를 통해 IO thread blocking을 완화하고,  
MongoDB read 접근 경합 가능성을 줄여 서버 안정성을 향상시켰습니다.

---

### Load Test Result

#### Before  
- crash at 50 clients / FR 20 requests

#### After  
- stable at 1000 clients / FR 20 requests

Expected FR samples : 20000  
Collected samples   : 20000  
Failures            : 0  

Avg latency(ms)     : 123.88  
P95 latency(ms)     : 199.65  
Max latency(ms)     : 300.15  

<br>

MongoDB read 경로를 별도의 worker thread로 분리한 이후  
기존 1000명의 동시 접속과 20000건의 친구 채팅 로그 조회 요청을 안정적으로 처리할 수 있었습니다.
