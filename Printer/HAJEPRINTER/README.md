# HAJEPRINTER
하제소프트(HajeSoft) 에서 제공한 윈도우 프린터 드라이버의 샘플입니다.   
강의 링크: https://youtu.be/8ZttxLRqbcI

## 시작하기 전에..
프린터 드라이버를 설치하는 방법은 아래와 같이 두 가지입니다. 
1. 설치파일을 사용하는 방법과
2. Win32 API를 사용하는 방법

설치파일을 사용하는 경우, WHQL 인증 작업을 받지 않으면 64비트 윈도우에서 드라이버 설치가 불가능하지만   
Win32 API를 사용하면 인증작업 없이 프린터 드라이버를 설치할 수 있습니다.

해당 프로젝트에서는 GPD 파일 및 Win32 API를 사용하여 프린터 드라이버를 설치할 것입니다.  

## 동작 가이드
1. 관리자 권한으로 sln 솔루션을 엽니다.
2. 빌드 후 실행합니다.
3. 프린터가 추가된 것을 확인합니다.
   - (Win11 기준) 설정 -> Bluetooth 및 장치 > 프린터 및 스캐너 에서 HAJESOFT PRINTER 가 설치되어 있는지 확인
   - 프린터 속성 -> 장치 설정 -> 용지함 설정 양식 에서 Hajesoft Form #0 항목이 있는지 확인 (GPD 파일에 기술된 내용)

4. 프린터를 삭제하기 위해서는 아래와 같이 진행합니다
   - (Win11 기준) 설정 -> Bluetooth 및 장치 > 프린터 및 스캐너 에서 HAJESOFT PRINTER 제거
   - C:\Windows\System32\spool\drivers\x64 위치의 UNIDRV.DLL, UNIDRVUI.DLL, HAJEPRINTER.GPD 제거