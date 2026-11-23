# trrispo25

Сравнение уровней сжатия zstd — консольная утилита на C11  поддержкой HTTP-сервера, упакованная в Docker и готовная к развертыванию в Kubernetes.

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
сжатия влияют на итоговый размер и на затраченное время, а также может работать как HTTP-сервис
для удаленного сжатия данных.

Поддерживаются три подкоманды:

- `compress` — сжать файл на заданном уровне;
- `benchmark` — сжать файл на каждом уровне от 1 до N и напечатать таблицу
  «уровень — размер — время».
- `serve` — запустить HTTP-сервер (по умолчанию порт 8080).

## Сборка и тестирование

Для установки зависимостей используется файл `build-deps.txt`:
```bash
sudo apt update
sudo xargs -a build-deps.txt apt install -y
```

**Сборка:**
```bash
cmake -S . -B build
cmake --build build
```

**Тестирование (с санитайзерами и покрытием):**
```bash
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_SANITIZERS=ON -DENABLE_COVERAGE=ON -DBUILD_TESTING=ON
cmake --build build-test
ctest --test-dir build-test --output-on-failure
gcovr --root . --filter 'src/core' --fail-under-line 70 build-test
```


## Docker

Программа упакована в Docker-образ на базе `ubuntu:24.04`. Образ собирается
из готового `.deb`-пакета, работает от непривилегированного пользователя с UID `10001` и содержит настроенный `HEALTHCHECK`.

**Размер образа:** ~127 MB

**Сборка и запуск локально:**
```bash
./build_deb.sh
docker build -t trrispo25:local .
docker run -d --name trrispo25-test -p 8080:8080 -e APP_API_TOKEN=secret trrispo25:local
curl http://localhost:8080/health
```

## Развертывание в Kubernetes

Сервис подготовлен для работы в кластере Kubernetes (манифесты находятся в каталоге `k8s/`).

### Создание кластера (kind)
```bash
kind create cluster --name tsp
kubectl get nodes # Убедиться, что узел в статусе Ready
```

### Развертывание приложения
1. Примените Namespace, ConfigMap, Deployment и Service:
```bash
kubectl apply -f k8s/namespace.yaml
kubectl apply -f k8s/
```

2. Создайте Secret с токеном (не хранится в репозитории):
```bash
kubectl -n trrispo25 create secret generic trrispo25-secret \
  --from-literal=APP_API_TOKEN=super-secret-token
```

3. Проверьте, что поды запустились (должно быть 2 реплики):
```bash
kubectl -n trrispo25 get pods
```

4. Для доступа к сервису локально используйте port-forward:
```bash
kubectl -n trrispo25 port-forward svc/trrispo25 8080:80
```
Теперь сервис доступен по адресу `http://localhost:8080`.

---

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


### ПР 4 — развертывание в Kubernetes (тег `v1.0.0`)
- [x] Созданы декларативные манифесты (Namespace, ConfigMap, Deployment, Service) в каталоге `k8s/`.
- [x] Настроены пробы `liveness` (`/health`) и `readiness` (`/ready`), заданы лимиты ресурсов, включен `runAsNonRoot`.
- [x] Настроена стратегия `RollingUpdate` с `maxUnavailable: 0` для обновления без простоя.
- [x] Секреты (токен) передаются через `Secret`, создаваемый командой (нет в репозитории).
- [x] В README добавлен подробный раздел «Эксплуатация» (7 сценариев сопровождения).


## Структура репозитория

```text
trrispo25/
├── .github/
│   └── workflows/
│       └── ci.yml          # CI/CD-конвейер
├── k8s/                    # Манифесты Kubernetes
│   ├── namespace.yaml
│   ├── configmap.yaml
│   ├── deployment.yaml
│   └── service.yaml
├── .gitignore
├── .dockerignore
├── Dockerfile
├── VERSION
├── build-deps.txt
├── README.md
├── CMakeLists.txt
├── build_deb.sh
├── src/
│   ├── core.h
│   ├── core.c
│   ├── server.h
│   ├── server.c
│   └── main.c
├── tests/
│   └── test_core.c
└── packaging/
    ├── trrispo25.conf
    ├── copyright
    ├── changelog
    └── DEBIAN/
        ├── control
        └── conffiles
```
