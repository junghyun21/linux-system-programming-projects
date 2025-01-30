# 리눅스 시스템 프로그래밍 프로젝트 모음

> 숭실대학교 | 컴퓨터학부 | 2024년 1학기  
> 리눅스 시스템 프로그래밍 | 개인 프로젝트 | C언어 기반

<br>

이 레포지토리는 리눅스 시스템 프로그래밍 과목에서 수행한 세 개의 프로젝트를 포함하고 있습니다.

각 프로젝트에서 리눅스의 `시스템 콜(system call)` 및 `라이브러리 콜(library call)`을 활용하였으며,  
파일 및 디렉토리의 구조를 `연결 리스트(linked-list)`로 직접 구현하였습니다.

<br>

### 개요

- **언어**: C
- **주요 개념**: 파일 시스템 관리, 프로세스 제어, 데몬 프로세스, 연결 리스트
- **실행 환경**: Ubuntu Server 20.04 LTS (AWS EC2, amd64)
- **개발 기간**: 2024년 1학기 (2024.03. ~ 2024.05.)

<br>

### 핵심 학습 내용

- **리눅스의 시스템 콜 활용**: 파일 및 디렉토리 조작, 프로세스 제어
- **리눅스의 라이브러리 콜 활용**: 파일 경로 처리, 사용자 입력 파싱
- **데몬 프로세스 구현**: 특정 경로를 지속적으로 모니터링하며 변경 사항을 자동 감지
- **연결 리스트 활용**: 파일 및 디렉토리 정보를 동적으로 관리하는 구조 설계 및 구현
- **CLI 기반 인터페이스 구현**: 프로그램 내에서 실행 가능한 내장 명령어 및 옵션 구현
- **데이터 무결성 관리**: MD5 해시를 사용하여 파일 변경 여부 검증
- **예외 처리 및 안정성 강화**: 예상치 못한 사용자 입력에도 프로그램이 정상 동작하도록 예외 처리 개선
- **빌드 자동화**: Makefile을 활용한 소스 코드 컴파일 및 실행 자동화


<br>

### 프로젝트 목록

| 프로젝트     | 설명 |
|--------------|------------|
| [ssu-backup](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-backup)      | 파일 및 디렉토리를 백업하고 복원하는 프로그램 |
| [ssu-repo](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-repo)      | 특정 파일의 변경 사항을 추적하고 버전 관리를 수행하는 프로그램 |
| [ssu-sync](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-sync)  | 데몬 프로세스를 활용한 실시간 동기화 및 백업 프로그램 |

<br>

### 실행 방법

```bash
git clone https://github.com/junghyun21/linux-system-programming-projects.git
cd linux-system-programming-projects/...
make
```

각 프로젝트의 디렉토리로 이동하여 `make`를 통해 실행하면 됩니다.