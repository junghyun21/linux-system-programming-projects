# 파일 버전 관리 프로그램 (ssu_repo)

> 숭실대학교 | 컴퓨터학부 | 2024.04.01. ~ 2024.04.28.
> 리눅스 시스템 프로그래밍 | 개인 프로젝트 | C언어 기반

<br>

`ssu_repo`는 리눅스 환경에서 사용자가 상태 관리를 원하는 **파일 및 디렉토리의 변경 사항을 추적하고, 스테이징 구역과 커밋 시스템을 통해 백업 및 특정 시점의 상태로 복원이 가능한 파일 버전 관리 시스템**입니다.

- [명세서](https://github.com/junghyun21/linux-system-programming-projects/blob/main/ssu-repo/docs/repo-project-spec.pdf)
- [결과 보고서](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-repo/docs/repo-report.pdf)

<br>

### 개요

- CLI 기반 버전 관리 시스템 구현
- 리눅스 환경에서 **시스템 콜(system call)** 및 **라이브러리 콜(library call)** 활용
- **프로세스 생성 및 실행** 기법을 적용하여 내장 명령어 구현  
- **연결 리스트(linked list)** 를 직접 구현하여 버전 관리 정보 저장  
- **로그 파일** 을 통해 변경 내역 추적 및 관리

<br>

### 구현 기능

프로그램 실행 시 현재 작업 경로(`$PWD`)에 레포 디렉토리(`./.repo`)를 생성하고, 하위에 커밋 로그(`./.repo/.commit.log`)와 스테이징 구역(`./.repo/.staging.log`)을 생성합니다. 이미 레포 디렉토리가 존재하면 추가 생성을 수행하지 않습니다.

현재 작업 경로 내 파일 및 디렉토리 중 버전을 관리할 대상은 스테이징 구역을 통해 관리되며, 버전 관리 대상 파일이 추가/수정/삭제되면 자동으로 감지 및 추적됩니다. 백업 및 복구 내역은 커밋 로그를 통해 관리되며, 커밋을 통해 변경 내역을 백업하거나 특정 시점의 상태 복원이 가능합니다.

| 명령어     | 설명 |
|:--------------:|------------|
| add | 버전을 관리할 파일 또는 디렉토리의 경로를 스테이징 구역에 추가 |
| remove | 더 이상 버전을 관리하지 않을 파일 또는 디렉토리의 경로를 스테이징 구역에서 제거 |
| status | 스테이징 구역 내 파일들의 변경 내역을 출력 |
| commit | 스테이징 구역 내 파일 중 변경 내역이 있는 파일을 백업 |
| revert | 특정 커밋 버전으로 파일 복원 |
| log | 커밋 내역 출력 |
| help | 사용 가능한 명령어 출력 |
| exit | ssu_repo 프로그램 종료 |

<br>

### 실행 방법

`make`를 통해 ssu_repo 프로그램의 실행 준비가 완료되면 프로그램을 실행시킵니다.  

```bash
$ git clone https://github.com/junghyun21/linux-system-programming-projects.git
$ cd linux-system-programming-projects/ssu-repo/src
$ make

$ ./ssu_repo
```

프로그램 실행 시 사용자 입력을 기다리는 프롬프트가 출력되며, `exit` 명령어 입력 시 프로그램이 종료됩니다.

```
20201662 > <add|remove|status|commit|revert|log|help|exit> <...>
```

<details>
  <summary><b>add</b></summary>
  
  ```
  add <PATH>
  ```

  <code>&lt;PATH&gt;</code>에는 백업할 파일이나 디렉토리의 상대경로 또는 절대경로를 입력할 수 있으며,  
  만약 해당 경로가 이미 버전 관리 대상이라면, 스테이징 구역에 경로를 추가하지 않습니다.

</details>

<details>
  <summary><b>remove</b></summary>
  
  ```
  remove <PATH>
  ```

  <code>&lt;PATH&gt;</code>에는 백업할 파일이나 디렉토리의 상대경로 또는 절대경로를 입력할 수 있으며,  
  만약 해당 경로가 이미 버전 관리 대상이 아니라면, 스테이징 구역에서 경로를 제거하지 않습니다.

</details>

<details>
  <summary><b>status</b></summary>
  
  ```
  status
  ```

  **스테이징 구역에 추가된 경로 내 파일들** 중 변경 내역이 있는 경우 `Changes to be committed:` 다음 줄에 해당 파일의 상태와 경로를 출력합니다.  
  **스테이징 구역에 추가된 이후, 제거된 파일들**에 대해서는 변경 내역을 추적 및 출력하지 않습니다.  
  **스테이징 구역에 추가된 적이 없는 파일들** 중 변경 내역이 있는 경우 `Untracked files:` 다음 줄에 해당 파일의 상태와 경로를 출력합니다.

  | 변경 내역     | 출력 |
  |:--------------:|------------|
  | 새로 추가 | new file: "현재 작업 경로 내 새로 추가된 파일의 상대경로" |
  | 내용 변경 | modified: "현재 작업 경로 내 수정된 파일의 상대경로" |
  | 삭제 | removed: "현재 작업 경로 내 삭제된 파일의 상대경로" |

  만약 변경 내역이 없는 경우, `Nothing to commit`가 출력됩니다.

</details>

<details>
  <summary><b>commit</b></summary>
  
  ```
  commit <NAME>
  ```

  입력받은 커밋 이름(<code>NAME</code>)으로 레포 디렉토리 하위에 버전 디렉토리가 생성됩니다. 버전 디렉토리에는 스테이징 구역에 추가된 경로 내 파일 중, 마지막 백업 파일과 비교하여 변경된 파일들만 백업됩니다.
  
  해당 명령어 실행 성공 시 몇 개의 파일이 변경되었는지, 몇 줄이 추가 및 삭제되었는지 등의 정보가 출력됩니다. (ex. `3 files changed, 17 insertions(+), 51 deletions(-)`)

</details>

<details>
  <summary><b>revert</b></summary>
  
  ```
  revert <NAME>
  ```

  입력받은 커밋 이름(<code>NAME</code>)의 버전 디렉토리를 찾아 현재 작업 경로 내 파일들을 해당 버전을 백업했던 상태로 되돌립니다.
  
</details>

<details>
  <summary><b>log</b></summary>
  
  ```
  log [NAME]
  ```

  커밋 이름(<code>NAME</code>)과 동일한 커밋 로그를 출력합니다. 커밋 이름을 생략한 경우, 모든 커밋 로그를 출력합니다.
  
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

  현재 실행중인 ssu_repo 프로그램이 종료됩니다.

</details>

<br>

### 프로젝트 구조

```
ssu-repo/
├── src/
│   ├── main.c          # 프로그램 실행 및 내장 명령어 처리 
│   ├── add.c           # 파일 및 디렉토리를 버전 관리 대상에 추가
│   ├── remove.c        # 버전 관리 대상에서 파일 및 디렉토리 제거
│   ├── commit.c        # 커밋 생성 및 파일 백업
│   ├── revert.c        # 특정 커밋으로 복원
│   ├── log.c           # 커밋 내역 출력
│   ├── status.c        # 변경 사항 확인
│   ├── help.c          # 내장명령어 사용법 출력
│   ├── command.c       # 내장명령어 처리 로직
│   ├── command.h     
│   ├── path.c          # 파일 경로 관련 유틸리티 
│   ├── path.h    
│   ├── struct.c        # 데이터 구조 정의 및 관리
│   ├── struct.h    
│   ├── defs.h          # 공통 상수 및 설정 정의
│   ├── Makefile        # 프로젝트 빌드 스크립트
├── docs/ 
│   ├── repo-project-specification.pdf     # 프로젝트 명세서
│   ├── repo-report.pdf                    # 최종 보고서
└── README.md           # 프로젝트 설명 
```