# trrispo25

Сравнение уровней сжатия zstd — консольная утилита на C11  поддержкой HTTP-сервера.

[![ci](https://github.com/isiyo-julikana/trrispo25/actions/workflows/ci.yml/badge.svg)](https://github.com/isiyo-julikana/trrispo25/actions/workflows/ci.yml)

- **Вариант:** 25
- **Программа:** сравнение уровней сжатия
- **Обязательные функции:** сжатие `zstd` с заданным уровнем; таблица «уровень — размер — время»
- **Библиотека:** `libzstd-dev` (runtime — `libzstd1`)
- **Дисциплина:** Технологии распространения, развертывания и сопровождения программного обеспечения
- **Текущая версия:** см. файл [`VERSION`](VERSION)


## Назначение

Программа `trrispo25` сжимает файл алгоритмом zstd на заданном уровне и умеет
строить таблицу «уровень — размер — время», чтобы сравнить, как разные уровни
сжатия влияют на итоговый размер и на затраченное время.

Поддерживаются три подкоманды:

- `compress` — сжать файл на заданном уровне;
- `benchmark` — сжать файл на каждом уровне от 1 до N и напечатать таблицу
  «уровень — размер — время».
- `serve` — запустить HTTP-сервер (по умолчанию порт 8080).

## Требования

- Ubuntu 24.04 LTS (или Debian) — нативная система, виртуальная машина или WSL 2;
- GCC, CMake ≥ 3.20, `pkgconf`, заголовки `libzstd-dev`.

Полный список пакетов, нужных для сборки, хранится в одном месте —
файле [`build-deps.txt`](build-deps.txt). Установить их одной командой:

```bash
sudo apt update
sudo xargs -a build-deps.txt apt install -y
```


## Сборка и тестирование

Из корня репозитория:

```bash
# Релизная сборка
cmake -S . -B build
cmake --build build

# Тестовая сборка с санитайзерами и покрытием
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_SANITIZERS=ON -DENABLE_COVERAGE=ON -DBUILD_TESTING=ON
cmake --build build-test
ctest --test-dir build-test --output-on-failure
gcovr --root . --filter 'src/core' --fail-under-line 70 build-test
```

## Docker

Программа упакована в Docker-образ на базе `ubuntu:24.04`. Образ собирается
из готового `.deb`-пакета (не компилирует программу заново), работает от
непривилегированного пользователя с UID `10001` и содержит настроенный `HEALTHCHECK`.

**Размер образа:** 127 MB

### Сборка образа локально

```bash
# 1. Сначала собираем deb-пакет
./build_deb.sh

# 2. Собираем Docker-образ
docker build -t trrispo25:local .
```

### Запуск контейнера

```bash
docker run -d --name trrispo25-test -p 8080:8080 \
  -e APP_API_TOKEN=secret \
  trrispo25:local
```

**Переменные окружения:**
- `APP_API_TOKEN` — токен для авторизации в API (если не задан, доступ открыт).

**Порты:**
- `8080` — HTTP-сервер.

**Проверка работоспособности:**
```bash
curl http://localhost:8080/health
# OK

curl -X POST -H "X-Api-Token: secret" http://localhost:8080/api/compress?level=5
# Compressed with level 5
```

## Непрерывная интеграция и деплой (CI/CD)

Конвейер GitHub Actions (`.github/workflows/ci.yml`) запускается при пуше в любую ветку,
при создании Pull Request в `master`, а также при отправке тегов `v*`.

Этапы выполняются строго последовательно (ошибка останавливает следующие):

1. **build** — релизная сборка с `-Werror` и статический анализ `cppcheck`.
2. **test** — тестовая сборка с ASan/UBSan, запуск `CTest`, проверка покрытия `gcovr` (порог 70%). JUnit-отчет сохраняется как артефакт.
3. **package** — сборка `.deb` через `build_deb.sh`, проверка `lintian`, сохранение пакета как артефакта.
4. **deploy** *(только для тегов `v*`)* — скачивает `.deb`, собирает Docker-образ, проводит smoke-тест (проверка `/health` и `/api/`) и публикует образ в Docker Hub с двумя тегами: версией и коротким хешем коммита.

Ветка `master` защищена: слияние Pull Request невозможно без успешного прохождения проверок `build`, `test` и `package`.

## Выпуск релиза

Релиз создается автоматически при отправке тега, совпадающего с версией в `VERSION`:

```bash
echo "0.3.0" > VERSION
git add VERSION
git commit -m "Версия 0.3.0"
git push origin master

git tag -a v0.3.0 -m "Релиз 0.3.0: контейнеризация"
git push origin v0.3.0
```

При отправке тега запускается конвейер, который:
1. Проверяет соответствие тега и `VERSION`.
2. Создает GitHub Release и прикрепляет к нему `.deb`.
3. Собирает Docker-образ, тестирует его и публикует в Docker Hub (`ваш_логин/trrispo25:0.3.0` и `ваш_логин/trrispo25:xxxxxxx`).

## Что сделано в каждой работе

### ПР 1 — репозиторий, программа, установочный пакет (тег `v0.1.0`)

- [x] Репозиторий на GitHub, `.gitignore`, `README.md`, `VERSION`, `build-deps.txt`;
- [x] Модель ветвления «main + feature/*», не менее двух влитых pull request;
- [x] Программа на C11: подкоманды `compress` и `benchmark`,
      ключи `--help` и `--version`;
- [x] Логика отделена от ввода-вывода и вынесена в статическую библиотеку
      `trrispo25core` (`src/core.c`);
- [x] Номер версии — только в `VERSION`, попадает в программу через CMake;
- [x] Параметры по умолчанию — из `/etc/trrispo25/trrispo25.conf`,
      ключи CLI имеют приоритет, отсутствие файла не ошибка;
- [x] Сборка без предупреждений `-Wall -Wextra -Wpedantic`;
- [x] Deb-пакет собирается одной командой (`build_deb.sh`), `Architecture`
      и `Depends` определяются автоматически (`dpkg --print-architecture`,
      `dpkg-shlibdeps`), конфигурация объявлена в `DEBIAN/conffiles`;
- [x] Конфигурация объявлена в `DEBIAN/conffiles`; `lintian` — без ошибок.

### ПР 2 — непрерывная интеграция (тег `v0.2.0`)

- [x] Написаны unit-тесты для `core` (12 проверок) и CLI-тесты (2 проверки);
- [x] Тесты зарегистрированы в CTest, собираются и проходят с санитайзерами (ASan, UBSan);
- [x] Покрытие строк `core` превышает 70% (проверяется через `gcovr`);
- [x] Настроен CI-конвейер (GitHub Actions): последовательные этапы `build → test → package`;
- [x] Сборка в CI выполняется с `-Werror`, добавлен статический анализатор `cppcheck`;
- [x] Отчёт о тестах (JUnit) и `.deb`-пакет сохраняются как артефакты CI;
- [x] Включена защита ветки `master` (слияние PR без зелёного CI запрещено);
- [x] Настроен автоматический выпуск: по тегу создается GitHub Release с `.deb` и
      проверкой соответствия `VERSION` и тега.

### ПР 3 — контейнеризация и этап деплоя (тег `v0.3.0`)
- [x] Добавлен HTTP-сервер (`libmicrohttpd`) с эндпоинтами `/health` и `/api/compress`.
- [x] Написан `Dockerfile` (Ubuntu 24.04, UID 10001, `EXPOSE`, `HEALTHCHECK`) и `.dockerignore`.
- [x] Добавлен этап `deploy` в CI для сборки, smoke-теста и публикации образа в Docker Hub.
- [x] Настроены секреты Docker Hub, образ публикуется с двумя тегами (версия и хеш).


### ПР 4 будут отмечены тегами   `v1.0.0` по мере сдачи.

## Структура репозитория

```text
trrispo25/
├── .github/
│   └── workflows/
│       └── ci.yml          # CI/CD-конвейер
├── .gitignore
├── .dockerignore           # исключения для контекста сборки Docker
├── Dockerfile              # инструкция сборки контейнера
├── VERSION                 # номер версии — единственный источник
├── build-deps.txt          # пакеты для сборки и тестов
├── README.md
├── CMakeLists.txt
├── build_deb.sh            # сборка deb-пакета одной командой
├── src/
│   ├── core.h
│   ├── core.c              # логика: сжатие zstd, замер времени
│   ├── server.h
│   ├── server.c            # HTTP-сервер на libmicrohttpd
│   └── main.c              # CLI: разбор аргументов, файловый I/O
├── tests/
│   └── test_core.c         # unit-тесты ядра
└── packaging/
    ├── trrispo25.conf      # /etc/trrispo25/trrispo25.conf
    ├── copyright
    ├── changelog
    └── DEBIAN/
        ├── control         # шаблон с @VERSION@, @ARCH@, @DEPENDS@
        └── conffiles
```
