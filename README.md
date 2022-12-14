                   
# Технический протокол взаимодействия с ТПО


1. Базовая информация

                   
Программа взаимодействия находится на блоке по следующему пути - **/opt/control/bin/tpoprotocol**. Чтобы взаимодействовать с программой, то нужно слать команды на порт **8889** (дефолтный). Порт можно установить через конфигурационный файл – **tpoprotocol.ini**.
Программа поддерживает опцию **Keep-Alive**, которая по стандарту отключена. Для того, чтобы включить опцию – нужно указать значение для параметра **keep-alive** в конфигурационном файле **tpoprotocol.ini**. Если в течение этого промежутка времени не будет принято ни одного пакета **Keep-Alive**, то программа перестает слать пакеты со статистикой и удаляет все активные устройства и файлы из пула сбора статистики. Для того, чтобы поддержать данную опцию – достаточно отправлять пакет не реже, чем 1 раз в промежуток. Для избежания прекращения сбора статистики из-за сетевых коллизий или задержек - рекомендуется слать пакет **Keep-Alive** 2 раза в промежуток.


2. Конфигурационные файлы

Для настройки поведения программы необходимо 2 конфигурационных файла: **tpoprotocol.ini** и **logger.ini**. Они должны располагаться по адресу - **/opt/control/conf** на блоке. Конфигурационный файл **logger.ini** необходим для настройки логирования, в то время как **tpoprotocol.ini** отвечает за поведение программы.


3. Разделители в пакетах

Программа воспринимает и обрабатывает следующие разделители:
> **“,”** - разделитель для команд и данных, отправляемых в пакетах

> **“/”** - разделитель для устройств и файлов API

> **“@”** - разделитель для файлов API


4. Команды для взаимодействия с программой

Существует 2 вида команд: требующие и не требующие после себя дополнительных данных. Если после команды должны идти вспомогательные данные, то они должны иметь разделитель **“,”** после себя. Пробел и знак переноса новой строки удаляются при получении команд от пользователя.

- Команда **KEEP-ALIVE** необходима для поддержания соединения с программой. Если опция - Keep-Alive включена в программе, то пользователю нужно слать данную команду в пределах таймаута.

Пример команды представлен ниже:
> **keep-alive**



- Команда **GET** необходима для запроса статистики для устройства или файла API. У данной команды могут присутствовать вспомогательные данные: адрес устройства или имя устройства, количество регистров или имя файла API и частота обновления данных. Частота в свою очередь лежит в пределах 0-100 (включая дробные значения), где **0** - считать устройство один раз. Если значение больше 0, то данные будут собираться с данной частотой, пока не будет остановлен сбор. 

Примеры команды представлен ниже:
> **get,0x43c00000/10,10** - считывать 10 раз в секунду 10 регистров от устройства с адресом 0x43c00000

> **get,AD1@/calib_mode,0** - считать один раз содержимое файла calib_mode для устройства AD1@


Команда поддерживает множественный запрос, например:
> **get,0x43c00000/10,0.1,AD1@/calib_mode,2,0x43b00000/1,0** - считывать 1 раз в 10 секунд 10 регистров от устройства с адресом 0x43c00000; считывать 2 раза в секунду содержимое файла calib_mode для устройства AD1@; считать 1 раз 1 регистр для устройства с адресом 0x43b00000

Команду **get** можно использовать без дополнительных параметров - в таком случае возвращаются активные устройства/файлы API с количеством регистров или именем файла и их частотой сбора статистики. При отсутствии активных устройств/файлов API для каждого вида указывается **NULL**, например:
> **ACTIVE,Devs: NULL Files: NULL** - нет активных устройств/файлов API для сбора

> **ACTIVE,Devs: 0x43c00000,10,2 Files: AD1@/calib_mode,3** - считывается 2 раза в секунду 10 регистров для устройства с адресом 0x43c00000; считывается 3 раза в секунду файл calib_mode для AD1@


В случае успеха пользователь получить данные из устройства/файла API с заголовком **GET**. Например, при выполнении такого запроса **get,0x43c00000/2,0** - пользователь получить следующий ответ (данные могут отличаться):
> **GET,0x43c00000,0x00000000,0x43c00004,0x00000000** - ответный пакет на запрос одноразового чтения двух регистров с базовым адресом 0x43c00000

При неправильном запросе программа отсылает пользователю пакет с заголовком **BAD_REQUEST** и телом сообщения. Например, при выполнении пользователем запроса **get,0x43c00000/10** (_отсутствует частота считывания_), то в ответном пакете пользователь получит следующее сообщение:
> **BAD_REQUEST,get,0x43c00000/10** - неправильный запрос для считывания 10 регистров от устройства с адресом 0x43c00000, так-как отсутствует частота считывания

Если пользователь запрашивает устройство по несуществующему адресу или несуществующий файл, то пользователь получает ответный пакет с заголовком **NOT_EXIST** и адресом устройства/именем файла:
> **NOT_EXIST,0x43c90000** - устройства с адресом 0x43c90000 не существует в системе

> **NOT_EXIST,AD1@/calib** - файла calib для AD1@ не существует в системе

> **NOT_EXIST,AD3@/calib_mode** - в системе не существует устройства AD3@


Если произошла какая-либо ошибка при выполнении команды, то пользователь получает ответный пакет с заголовком **ERROR** и адресом устройства/именем файла:
> **ERROR,0x43c00000** - произошла ошибка при выполнении команды get для устройства с адресом 0x43c00000



- Команда **DEL** необходима для удаления устройств/файлов API из списка считываемых. После команды всегда должны следовать дополнительные данные - адрес устройства или имя файла API. 

Пример команды приведен ниже:
> **del,0x43c00000,0x43b00000,AD1@/calib_mode** - удалить устройства с адресами 0x43c00000 и 43b00000 из пула сбора статистики; удалить файл  API calib_mode для устройства AD1@ из пула сбора статистики

При отсутствии устройства или файла API в пуле сбора статистики - пользователь получает в ответном пакете заголовок **NOT_ACTIVE** и адрес устройства/файл API. 
> **NOT_ACTIVE,0x43c00000** - ответный пакет при неудачной попытке удалить устройство с адресом 0x43c0000 из пула сбора статистики.

При успешном удалении устройства/файла API из пула сбора статистики - пользователь получает в ответном пакете заголовок **DELETED** и адрес устройства/файл API.
> **DELETED,0x43c00000** - ответный пакет при успешном удалении устройства с адресом 0x43c00000 из пула сбора статистики



- Команда **SET** служит для записи значения в регистр/файл API. Команда требует после себя указания дополнительных данных - адрес устройства/файла API и значения, которое следует записать.

Пример команды приведен ниже:
> **set,0x43c00000,0x1** - записать 0x1 в регистр по адресу 0x43c00000

> **set,AD1@/calib_mode,manual** - записать manual в файл API  calib_mode для устройства AD1@


При неправильном запросе программа отсылает пользователю пакет с заголовком **BAD_REQUEST** и телом сообщения. Например, при выполнении пользователем запроса **set,0x43c00000** (_отсутствует значение для записи_), то в ответном пакете пользователь получит следующее сообщение:
> **BAD_REQUEST,set,0x43c00000**

При отсутствии устройства или файла API в дереве устройств - пользователь получает в ответном пакете заголовок **NOT_EXIST** и адрес устройства/файл API. 
> **NOT_EXIST,AD1@/calib_moe** - ответный пакет при неудачной попытке записать значение в файл API calib_moe, которого не существует

Если произошла какая-либо ошибка при выполнении команды, то пользователь получает ответный пакет с заголовком **ERROR** и адресом устройства/именем файла:
> **ERROR,0x43c00000** - произошла ошибка при выполнении команды set для устройства с адресом 0x43c00000

При успешной записи в адрес или файл API пользователь получит в ответном письме заголовок **SUCCESS** и адрес устройства/файл API:
> **SUCCESS,AD1@/calib_mode** - при успешной записи в файл API calib_mode для устройства AD1@



- Команда **DTB** предназначена для получения информации из дерева устройств или получения функционала API для конкретного устройства. Данная команда может выполняться как без параметров, так и с ними.

Пример команды приведен ниже:
> **dtb** - получить дерево устройств, состоящее из имен устройств, их compatible, базовых адресов и размеров регионов

Для получения функционала API для конкретного устройства следует после команда **dtb** добавить имя устройства, например:
> **dtb,AD1@** - получить функционал API для устройства AD1@

При отсутствии функционала API или самого устройства, то в ответном письме пользователь получит заголовок **NOT_EXIST** и имя устройства
> **NOT_EXIST,AD3@** - в системе нет устройства с именем AD3@



- Команда **STOP** служит для остановки всех пулов сбора статистики для устройств и файлов API. В ответном пакете пользователь получает заголовок **STOPPED** и список остановленных устройств/файлов API с их частотой считывания. Если в момент отправки команды никакая статистика для устройств/файлов API не собиралась, то будет указано **NULL** для соответствующего типа.

Пример команды приведен ниже:
> **stop** - остановить сбор любой статистики

Пример ответного пакета может выглядеть так:
> **STOPPED,Devs: 0x43c00000,1,1 Files: NULL** - остановлен сбор статистики для устройства с андерсом 0x43c00000 и частотой считывания равной 1Гц; для файлов API никакая статистика в данный момент не собиралась
