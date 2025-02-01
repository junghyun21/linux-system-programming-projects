# 실시간 동기화 및 백업 관리 프로그램 (ssu_sync)

> 숭실대학교 | 컴퓨터학부 | 2024.04.29. ~ 2024.05.26.  
> 리눅스 시스템 프로그래밍 | 개인 프로젝트 | C언어 기반

<br>

`ssu_sync`는 리눅스 환경에서 특정 파일 및 디렉토리를 **실시간으로 감시**하고, 변경이 발생할 경우 **자동으로 백업 및 로그 기록**을 수행하는 프로그램입니다. 

- [명세서](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-sync/docs/sync-project-spec.pdf)
- [결과 보고서](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-sync/docs/sync-report.pdf)


<br>

### 개요

- CLI 기반 실시간 파일 변경 감지 및 백업 시스템 구현
- 리눅스 환경에서 **시스템 콜(system call)** 및 **라이브러리 콜(library call)** 활용
- **데몬 프로세스(daemon process)** 를 생성하여 지속적인 파일 모니터링
- **연결 리스트(linked list)** 를 직접 구현하여 백업 데이터 관리
- **로그 파일**을 통해 변경 이력 추적 및 데몬 프로세스 관리

<br>

### 구현 기능

사용자 홈 디렉토리(`/home/사용자아이디`) 내의 파일 또는 디렉토리 중 모니터링 대상을 지정하면, 모니터링 대상의 변경 상태를 지속적으로 모니터링하는 `데몬 프로세스(daemon process)`가 생성됩니다. 이 때, 생성된 데몬 프로세스의 정보는 전체 백업 디렉토리(`~/backup`) 내 로그 파일(`~/backup/monitor_list.log`)에 기록되고, 해당 데몬 프로세스의 pid를 이름으로 하는 디렉토리(`~/backup/<pid>`) 및 로그 파일(`~/backup/<pid>.log`)이 생성됩니다.

모니터링 중 변경 사항이 감지되면 변경된 파일은 자동으로 특정 데몬 프로세스의 백업 디렉토리(`~/backup/<pid>`)에 백업되고, 백업 내역은 로그 파일(`~/backup/<pid>.log`)에 기록됩니다.

| 명령어     | 설명 |
|:--------------:|------------|
| add  | 특정 파일 또는 디렉토리를 백그라운드에서 모니터링하는 데몬 프로세스 생성 |
| remove  | 데몬 프로세스를 제거하고, 백업 파일을 삭제 |
| list   | 현재 모니터링 중인 데몬 프로세스의 pid와 모니터링 대상 경로 출력 |
| help     | 사용 가능한 명령어 및 옵션 출력 |
| exit     | ssu_sync 프로그램 종료 |

<br>

### 실행 방법

`make`를 통해 ssu_sync 프로그램의 실행 준비가 완료되면 프로그램을 실행시킵니다.  

```bash
$ git clone https://github.com/junghyun21/linux-system-programming-projects.git
$ cd linux-system-programming-projects/ssu-repo/src
$ make

$ ./ssu_sync
```

프로그램 실행 시 사용자 입력을 기다리는 프롬프트가 출력되며, `exit` 명령어 입력 시 프로그램이 종료됩니다.

```
20201662 > <add|remove|list|help|exit> <...>
```

<details>
  <summary><b>add</b></summary>
  
  ```
  add <PATH> [OPTION] ...
  ```

  <code>&lt;PATH&gt;</code>에는 백업할 파일이나 디렉토리의 상대경로 또는 절대경로를 입력할 수 있으며,  
  <code>[OPTION]</code>은 여러 개를 동시에 사용할 수도 있고 생략도 가능합니다.

  만약 이미 입력받은 경로에 대한 모니터링 중이라면, 데몬 프로세스 생성을 진행하지 않습니다.

  | 옵션     | 설명 |
  |:--------------:|------------|
  | `-d`  | 입력받은 경로가 디렉토리일 때, 해당 디렉토리에 직접 포함된 모든 파일들에 대해 모니터링하는 데몬 프로세스 생성 |
  | `-r`  | 입력받은 경로가 디렉토리일 때, 해당 디렉토리 내 하위의 모든 파일들에 대해 모니터링하는 데몬 프로세스 생성 |
  | `-t`  | 옵션 뒤에 반드시 주기(초)를 입력받고, 해당 주기마다 실행되는 데몬 프로세스 생성 |

</details>

<details>
  <summary><b>remove</b></summary>
  
  ```
  remove <DAEMON_PID>
  ```

  <code>&lt;DAEMON_PID&gt;</code>를 PID로 갖는 데몬 프로세스를 제거합니다.

</details>

<details>
  <summary><b>list</b></summary>
  
  ```
  list
  ```

  현재 실행 중인 데몬 프로세스들의 목록을 출력합니다.
  
</details>

<details>
  <summary><b>help</b></summary>
  
  ```
  help [COMMAND]
  ```

  <code>[COMMAND]</code>에 해당하는 내장명령어에 대한 설명이 출력되며, 이를 생략한 경우에는 모든 내장 명령어에 대한 설명이 출력됩니다.

</details>

<details>
  <summary><b>exit</b></summary>
  
  ```
  exit
  ```

  현재 실행중인 ssu_sync 프로그램이 종료됩니다.

</details>

<br>

### 프로젝트 구조

```
ssu-sync/
├── src/
│   ├── main.c          # 프로그램 실행 및 명령어 처리
│   ├── add.c           # 모니터링 대상 추가
│   ├── list.c          # 모니터링 대상 목록 출력
│   ├── remove.c        # 모니터링 대상 제거
│   ├── monitoring.c    # 데몬 프로세스 생성 및 작업 수행
│   ├── monitoring.h    
│   ├── command.c       # 내장 명령어 처리
│   ├── command.h       
│   ├── path.c          # 파일 경로 처리 유틸리티
│   ├── path.h    
│   ├── defs.h          # 공통 상수 및 설정 정의
│   ├── Makefile        # 프로젝트 빌드 스크립트
├── docs/ 
│   ├── sync-project-specification.pdf   # 프로젝트 명세서
│   ├── sync-report.pdf                   # 최종 보고서
└── README.md          # 프로젝트 설명
```
