# Hajesoft_VPort
하제소프트(HajeSoft) 에서 제공한 윈도우 프린터 포트 드라이버의 샘플입니다.   
강의 링크: https://youtu.be/8qmpy3W1yRs


## 시작하기 전에..
스풀러 관련 Win32 API 함수를 사용하는 것도 가능하며, 이를 이용해 설치하는 것이 일반적인 방식이지만,
해당 프로젝트에서는 레지스트리를 이용해 프린터 포트 드라이버를 설치할 것입니다.  

## 동작 가이드
1. sln 솔루션을 엽니다.
2. 빌드합니다.
3. 빌드한 VPort.dll 파일을 C:\Windows\System32 경로로 복사합니다.
4. 레지스트리 편집기를 열고, HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Print\Monitors 경로에 키를 생성 후   
   Driver 값을 생성하고 이에 대한 데이터를 VPORT.DLL로 설정합니다.
5. 작업 관리자의 서비스에서 Spooler 서비스를 다시 시작합니다.
6. 설정 -> 프린터 및 스캐너에서 아무 프린터 선택 후 프린터 속성을 클릭합니다   
   이후, 속성 대화 상자의 포트 탭에서 VPORT 포트가 추가되었는지 확인합니다.