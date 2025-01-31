# 백업 관리 프로그램 (ssu_backup)

> 숭실대학교 | 컴퓨터학부 | 2024.03.03. ~ 2024.03.31.  
> 리눅스 시스템 프로그래밍 | 개인 프로젝트 | C언어 기반

<br>

`ssu_backup`은 리눅스 환경에서 **파일 및 디렉토리의 백업, 복원, 삭제 기능을 수행하는 백업 관리 프로그램**입니다.

- [명세서](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-backup/docs/backup-project-specification.pdf)
- [결과 보고서](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-backup/docs/backup-report.pdf)

<br>

### 개요

- CLI 기반 백업 관리 시스템 구현
- 리눅스 환경에서 **시스템 콜(system call)** 및 **라이브러리 콜(library call)** 활용  
- **연결 리스트(linked list)** 를 직접 구현하여 백업 목록 관리  
- **로그 파일** 을 통해 수행된 백업 내역 기록 및 관리

<br>

### 구현 기능

사용자 홈 디렉토리(`/home/사용자아이디`) 내 파일 및 디렉토리를 옵션에 따라 백업 디렉토리(`/home/backup`)에 백업하고, 백업된 파일을 다시 복원 및 삭제하는 기능을 구현하였습니다.  
이 때, 백업 디렉토리 내 로그 파일(`/home/backup/ssubak.log`)에 수행된 작업 내역을 기록하였으며, 프로그램 실행 시 백업 디렉토리 내의 모든 백업 파일과 로그 파일 내용을 대조하여 백업 파일들에 대한 백업 상태를 연결 리스트로 관리하였습니다.

| 명령어     | 설명 |
|:--------------:|------------|
| backup  | 지정한 파일 또는 디렉토리를 백업 |
| recover  | 백업된 파일을 원본 경로로 복원 |
| remove   | 백업된 파일 또는 디렉토리를 삭제 |
| help     | 사용 가능한 명령어 및 옵션 출력 |

<br>

### 실행 방법

`make`를 통해 ssu_backup 프로그램 실행 준비를 합니다.

```
git clone https://github.com/junghyun21/linux-system-programming-projects.git
cd linux-system-programming-projects/ssu-backup/src
make
```

백업 디렉토리 접근을 위해 root 권한으로 프로그램을 실행합니다.

```
sudo ./ssu_backup <backup|recover|remove|help> <...>
```

<details>
  <summary><b>backup</b></summary>
  
  ```
  backup <PATH> [OPTION] ...
  ```

  <code>&lt;PATH&gt;</code>에는 백업할 파일이나 디렉토리의 상대경로 또는 절대경로를 입력할 수 있으며,  
  <code>[OPTION]</code>은 여러 개를 동시에 사용할 수도 있고 생략도 가능합니다.
  
  만약 이미 내용이 동일한 백업 파일이 존재한다면, 백업을 진행하지 않습니다.
  
  | 옵션     | 설명 |
  |:--------------:|------------|
  | `-d`  | 지정한 경로가 디렉토리일 때, 해당 디렉토리에 직접 포함된 파일들만 백업 |
  | `-r`  | 지정한 경로가 디렉토리일 때, 해당 디렉토리의 모든 하위 파일 및 디렉토리를 백업 |
  | `-y`  | 동일한 백업 파일이 존재하더라도 덮어쓰기 없이 백업 |

</details>

<details>
  <summary><b>recover</b></summary>
  
  ```
  recover <PATH> [OPTION] ...
  ```

  <code>&lt;PATH&gt;</code>에는 백업할 파일이나 디렉토리의 상대경로 또는 절대경로를 입력할 수 있으며,  
  <code>[OPTION]</code>은 여러 개를 동시에 사용할 수도 있고 생략도 가능합니다.

  만약 백업 파일이 2개 이상인 경우, 복구 가능한 백업 파일들의 목록이 출력되며 이 중 복구할 파일을 선택하면 됩니다.
  
  | 옵션     | 설명 |
  |:--------------:|------------|
  | `-d`  | 지정한 경로가 디렉토리일 때, 해당 디렉토리에 직접 포함된 파일들만 복구 |
  | `-r`  | 지정한 경로가 디렉토리일 때, 해당 디렉토리의 모든 하위 파일 및 디렉토리를 복구 |
  | `-l`  | 백업 파일이 2개 이상인 파일들의 목록 출력 없이, 가장 최근에 백업한 파일을 복구 |

</details>

<details>
  <summary><b>remove</b></summary>
  
  ```
  remove <PATH> [OPTION] ...
  ```

  <code>&lt;PATH&gt;</code>에는 백업할 파일이나 디렉토리의 상대경로 또는 절대경로를 입력할 수 있으며,  
  <code>[OPTION]</code>은 여러 개를 동시에 사용할 수도 있고 생략도 가능합니다.

  만약 백업 파일이 2개 이상인 경우, 삭제 가능한 백업 파일들의 목록이 출력되며 이 중 삭제할 파일을 선택하면 됩니다.
  
  | 옵션     | 설명 |
  |:--------------:|------------|
  | `-d`  | 지정한 경로가 디렉토리일 때, 해당 디렉토리에 직접 포함된 파일들만 삭제 |
  | `-r`  | 지정한 경로가 디렉토리일 때, 해당 디렉토리의 모든 하위 파일 및 디렉토리를 삭제 |
  | `-a`  | 백업 파일이 2개 이상인 파일들의 목록 출력 없이, 모든 백업 파일을 삭제 |

</details>

<details>
  <summary><b>help</b></summary>
  
  ```
  help [COMMAND]
  ```

  <code>[COMMAND]</code>에 해당하는 내장명령어에 대한 설명이 출력되며, 이를 생략한 경우에는 모든 내장 명령어에 대한 설명이 출력됩니다.

</details>

<br>

### 프로젝트 구조

```
ssu-backup/
├── src/
│   ├── usage/
│   │   ├── backup.txt
│   │   ├── recover.txt
│   │   ├── remove.txt
│   │   ├── list.txt
│   │   └── help.txt
│   ├── main.c          # 프로그램 실행 및 내장 명령어 처리 
│   ├── backup.c        # 백업 기능 구현 
│   ├── recover.c       # 파일 복원 기능 
│   ├── remove.c        # 백업 파일 삭제 기능 
│   ├── list.c          # 백업 목록 조회 기능 
│   ├── help.c          # 명령어 사용법 출력 
│   ├── command.c       # 명령어 처리 로직
│   ├── command.h     
│   ├── filepath.c      # 파일 경로 관련 유틸리티 
│   ├── filepath.h    
│   ├── filedata.c      # 파일 데이터 및 백업 리스트 관리 
│   ├── filedata.h 
│   ├── defs.h          # 공통 상수 및 설정 정의
│   └── Makefile        # 프로젝트 빌드 스크립트
├── docs/ 
│   ├── backup-project-spec.pdf     # 프로젝트 명세서
│   └── #P1-report.docxreport.pdf   # 최종 보고서
└── README.md           # 프로젝트 설명 
```