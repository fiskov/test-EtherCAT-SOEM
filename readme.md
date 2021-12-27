# Тестирование EtherCAT-контроллеров 
В моём случае есть контроллеры Ethercat (на мк Microchip LAN9252) с параметрами:
- Vendor 0x0B90
- Product 0x01, 0x03
- Name SLE01 и SLE03
- SLE01 = 128+128 uint8 как на input, так и output
- SLE03 = 128 uint8 + 64 uint32 на output, 128 uint8 на input
К контроллерам можно подключиться в TwinCAT или EtherCAT Explorer. 

**Задача:** сделать утилиту для проверки подключенных к EtherCAT-контроллерам датчиков посредством пары кнопок на экране или клавиатуре. Проверка заключается в выполнении "бегущей волны" простым перебором диапазона регистров с установкой им 255 и 0. Также нужны 4 кнопки "зажечь следующий", "зажечь предыдущий", "зажечь всех", "потушить всех" для зоны A, B.

## Попытка сделать на SOEM
Самый верный способ, т.к. решения на C#, Python (или node.js, который вовсе не установился из npm) основаны на библиотеке SOEM (https://github.com/OpenEtherCATsociety/SOEM). 
Библиотека компилируется в набор *.lib-файлов.

Для работы с EtherCAT с надо также установить Npcap (https://nmap.org/npcap/). При установки должна стоять галочка **"WinPcap API-compatible mode"**

![img](img.png)

### Установка 
- Visual Studio 2019, git и CMake уже установлены ранее при попытке сделать на C#
- выполнить `git clone https://github.com/OpenEtherCATsociety/SOEM.git` в подходящую папку
- создать в этой же папке каталог `build` в который необходимо зайти из "x86 Native Tools Command Prompt for VS 2019". Нужна версия именно x86, т.к. библиотека собирается для 32-бит.
- `cmake .. -G "NMake Makefiles"` выполнится генерация make-файла на основе CMakeLists.txt из корневого каталога (если нужны примеры из test\win32, то надо их прописать и в нужных папках добавить свой CMakeLists.txt)
- `nmake` всё скомпилируется MSVS компилятором. 

### Создание проекта
В исходниках SOEM есть *ifdef* на *WIN32* и компилятор от ms, поэтому будет выполняться в MSVS для x86.

- Предполагается, что папка SOEM из предыдущего пункта находится в папке с проектом. Выбрана сборка "Release".
- В проекте для Win C++ в настроках надо добавить скомпилированные библиотеки и хидеры:
  - **C/C++ / General / Additional Include Directories** добавить `SOEM\soem;SOEM\osal;SOEM\osal\win32;SOEM\oshw\win32\wpcap\Include\pcap;SOEM\oshw\win32\wpcap\Include;SOEM\oshw\win32;`
  - **Linker / General / Additional Library Directories** добавить  `SOEM\build;SOEM\oshw\win32\wpcap\Lib;`
  - **Linker / General / Additional Dependencies** добавить в начало `soem.lib;Packet.lib;wpcap.lib;Ws2_32.lib;winmm.lib;msvcmrt.lib;msvcrt.lib;`. Последние 4 библиотеки не из SOEM, но они нужны.
  - **C/C++ / Code Generation / Runtime Library** выбирают `Multi-threaded (/MT)` для статической сборки
  - **Linker / General / Command Line / Additional Options** добавить  `/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:MSVCRTD`. Так как компилируется статически для Release.
  
Первая версия - консольная, на основе примеров из SOEM. 

**КОМПИЛИРОВАТЬ СБОРКУ "x86 Release"**

---

## Попытка сделать на C# (-)
Пробовал воспользоваться пакетом из https://github.com/Apollo3zehn/EtherCAT.NET

**При запуске примеров выдается ошибка, что не может обнаружить Slave**

### Используемое ПО
- Npcap (https://nmap.org/npcap/) - уже установлен, т.к. нужен для TwinCAT или EtherCAT Explorer
- Visual Studio. Потому что делаем ПО на C#
- Git. Для выполнения `Git Clone`. При компиляции библиотек *скрипт* проверяет актуальность репо
- CMake. При установки указать, чтобы дописывалось в **Path**

### Установка
- `git clone https://github.com/Apollo3zehn/EtherCAT.NET.git` 
- Открыть Developer PowerShell от имени администратора и перейти в каталог с репозиторием
- Выполнить `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`, чтобы выполнялись неподписанные (?) скрипты.
- `.\init_solution.ps1`
- `msbuild ./artifacts/bin32/SOEM_wrapper/soem_wrapper.vcxproj p:Configuration=Release`
- `msbuild ./artifacts/bin64/SOEM_wrapper/soem_wrapper.vcxproj p:Configuration=Release`
- `dotnet build ./src/EtherCAT.NET/EtherCAT.NET.csproj`
- Скомпилируются файлы библиотек. Помимо них из репозитория загрузится проект SampleMaster для тестов

---

## Попытка сделать на Python (-)
Существует готовый пакет для Python ( https://github.com/bnjmnp/pysoem )

### Пример не может перевести режим работы из PRE_OP в SAFE_OP

При выполнении функции master.config_map() появлятеся ошибка `pysoem.pysoem.ConfigMapError: [SdoError(1, 7168, 0, 100728832, 'Unsupported access to an object')]`

Возможно это потому, что используется Microchip LAN9252, а не ПЛК от Beckhoff (например, отличается количеством SyncManager)

Если выполнение config_map завернуть в **try-except**, то можно дальше выполнять программу и она даже работает. Проблемы возникают при копировании приложения на другой компьютер. 

### Установка
Для установка достаточно выполнить `pip install pysoem`, но только в Python 3.9, более новые версии не поддерживаются (?).

### Запуск на других компьютерах
Если на демонстрационном скрипте использовать `pyinstaller` с флагом *-F* и UPX.exe в каталоге, то получается одиночный файл размером около 6 МБ. Этот файл можно скопировать и запустить на компьютер без python, но работать **не будет**, т.к. для запуска нужна скомпилированная библиотека SOEM. Но где хранится/вызывется эта библиотека не нашел.

Также следует учесть, что для win7 надо использовать Python 3.8 или старее. 
