import asyncio
import time
import statistics
import random

HOST = "127.0.0.1"
PORT = 9000

# 동시 접속 클라이언트 수
CLIENTS = 1000

# 각 클라이언트가 FR 요청을 몇 번 보낼지
FR_REQUESTS_PER_CLIENT = 20

# 월드 채팅 포함할지
SEND_WORLD_CHAT = True

# 각 클라이언트 시작 간격
CONNECT_DELAY_SEC = 0.01

# FR 요청 사이 간격
FR_INTERVAL_SEC = 0.02

# 로그인 후 대기시간
POST_LOGIN_DELAY_SEC = 0.05

# 전체 지연시간 저장
fr_latencies_ms = []
fr_failures = 0


# 서버의 [DM_HISTORY_BEGIN] ~ [DM_HISTORY_END]를 읽어서 끝날 때까지 대기하는 함수
async def read_until_dm_history_end(reader, timeout_sec=5.0):
    started = False

    while True:
        line = await asyncio.wait_for(reader.readline(), timeout=timeout_sec)
        if not line:
            raise ConnectionError("서버 연결 종료")

        text = line.decode(errors="replace").strip()

        if text == "[DM_HISTORY_BEGIN]":
            started = True
            continue

        if text == "[DM_HISTORY_END]":
            if started:
                return
            # BEGIN 없이 END만 와도 일단 종료하자
            return


async def client_task(cid: int):
    global fr_failures

    name = f"user{cid}"

    try:
        reader, writer = await asyncio.open_connection(HOST, PORT)

        # 로그인
        writer.write(f"L {name}\n".encode())
        await writer.drain()

        await asyncio.sleep(POST_LOGIN_DELAY_SEC)

        # 월드 채팅 섞기
        if SEND_WORLD_CHAT:
            writer.write(f"W hello_from_{name}\n".encode())
            await writer.drain()

        await asyncio.sleep(0.02)

        # FR 요청 반복
        for _ in range(FR_REQUESTS_PER_CLIENT):
            # 자기 자신 포함 랜덤 선택 가능
            friend_id = random.randint(0, CLIENTS - 1)
            friend_name = f"user{friend_id}"

            start = time.perf_counter()

            writer.write(f"FR {friend_name}\n".encode())
            await writer.drain()

            try:
                await read_until_dm_history_end(reader, timeout_sec=5.0)
                end = time.perf_counter()
                fr_latencies_ms.append((end - start) * 1000.0)
            except Exception:
                fr_failures += 1

            await asyncio.sleep(FR_INTERVAL_SEC)

        writer.close()
        await writer.wait_closed()

    except Exception as e:
        fr_failures += FR_REQUESTS_PER_CLIENT
        print(f"[client {name}] error: {e}")


def calc_p95(values):
    if not values:
        return None
    if len(values) == 1:
        return values[0]

    sorted_vals = sorted(values)
    idx = int(len(sorted_vals) * 0.95) - 1
    idx = max(0, min(idx, len(sorted_vals) - 1))
    return sorted_vals[idx]


async def main():
    print("=== Load Test Start ===")
    print(f"HOST={HOST}, PORT={PORT}")
    print(f"CLIENTS={CLIENTS}")
    print(f"FR_REQUESTS_PER_CLIENT={FR_REQUESTS_PER_CLIENT}")
    print(f"SEND_WORLD_CHAT={SEND_WORLD_CHAT}")
    print()

    tasks = []

    total_start = time.perf_counter()

    for i in range(CLIENTS):
        tasks.append(asyncio.create_task(client_task(i)))
        await asyncio.sleep(CONNECT_DELAY_SEC)

    await asyncio.gather(*tasks)

    total_end = time.perf_counter()
    total_elapsed = total_end - total_start

    total_samples = len(fr_latencies_ms)
    expected_samples = CLIENTS * FR_REQUESTS_PER_CLIENT

    print("\n=== Load Test Result ===")
    print(f"Expected FR samples : {expected_samples}")
    print(f"Collected samples   : {total_samples}")
    print(f"Failures            : {fr_failures}")
    print(f"Total elapsed(sec)  : {total_elapsed:.2f}")

    if fr_latencies_ms:
        avg_ms = statistics.mean(fr_latencies_ms)
        max_ms = max(fr_latencies_ms)
        min_ms = min(fr_latencies_ms)
        p95_ms = calc_p95(fr_latencies_ms)

        print(f"Min latency(ms)     : {min_ms:.2f}")
        print(f"Avg latency(ms)     : {avg_ms:.2f}")
        print(f"P95 latency(ms)     : {p95_ms:.2f}")
        print(f"Max latency(ms)     : {max_ms:.2f}")
    else:
        print("No latency samples collected.")


if __name__ == "__main__":
    asyncio.run(main())