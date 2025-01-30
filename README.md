# 리눅스 시스템 프로그래밍 프로젝트 모음

> 숭실대학교 | 컴퓨터학부 | 2024년 1학기  
> 리눅스 시스템 프로그래밍 | 개인 프로젝트 | C언어 기반

<br>

이 레포지토리는 리눅스 시스템 프로그래밍 과목에서 수행한 세 개의 프로젝트를 포함하고 있습니다.

각 프로젝트에서 리눅스의 `시스템 콜(system call)` 및 `라이브러리 콜(library call)`을 활용하였으며, 파일 및 디렉토리의 구조를 `연결 리스트(linked-list)`로 직접 구현하였습니다.

이를 통해 **리눅스 환경에서의 시스템 프로그래밍 설계 및 응용 능력을 향상**시켰습니다.

<br>

### 개요

- **언어**: C
- **주요 개념**: 파일 시스템 관리, 프로세스 제어, 데몬 프로세스, 연결 리스트
- **실행 환경**: Ubuntu Server 20.04 LTS (AWS EC2, amd64)
- **개발 기간**: 2024년 1학기 (2024.03. ~ 2024.05.)

<br>

### 프로젝트 목록

| 프로젝트     | 설명 |
|--------------|------------|
| [ssu-backup](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-backup)      | 파일 및 디렉토리를 백업하고 복원하는 프로그램 |
| [ssu-repo](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-repo)      | 특정 파일의 변경 사항을 추적하고 버전 관리를 수행하는 프로그램 |
| [ssu-sync](https://github.com/junghyun21/linux-system-programming-projects/tree/main/ssu-sync)  | 데몬 프로세스를 활용한 실시간 동기화 및 백업 프로그램 |
