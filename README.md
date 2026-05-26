# Backend Control — визуализация телеметрии Android в реальном времени

Антон Столба ИКС-431

C++/ZMQ backend-приложение, которое принимает данные со смартфона Android (геолокация, параметры сотовых сетей, трафик), сохраняет их в PostgreSQL, отображает в GUI (ImGui + ImPlot), подгружает OpenStreetMap-тайлы и строит тепловую карту качества сигнала.

---

## Содержание
- [Обзор проекта](#обзор-проекта)
- [Архитектура](#архитектура)
- [Структура репозитория](#структура-репозитория)
- [Сборка и запуск](#сборка-и-запуск)
- [Формат входных данных](#формат-входных-данных)
- [Возможности](#возможности)

---

## Обзор проекта

Backend-приложение решает следующие задачи:

- Поднимает **ZMQ PULL-сокет** на порту **5558** и принимает JSON-пакеты от Android-клиента.
- Поднимает **ZMQ PUSH-сокет** на порту **5559** для отправки команд фильтрации на клиент.
- Парсит входящие данные:
  - координаты (`latitude`, `longitude`, `altitude`, `accuracy`, `timestamp`),
  - параметры сотовых сетей LTE/NR/GSM (`rsrp`, `rsrq`, `rssi`, `sinr`, `pci`, `earfcn`, `nrarfcn`, `tac` и др.),
  - статистику сетевого трафика (`networkUsage`, `topApps`).
- Сохраняет сырые пакеты в `build/received_android.json`.
- Опционально записывает нормализованные записи в PostgreSQL.
- Отображает в реальном времени:
  - текущее местоположение и параметры сети,
  - графики уровня сигнала (RSRP, RSRQ, RSSI) по сотам,
  - карту OpenStreetMap с автоматической подгрузкой тайлов,
  - тепловую карту (heatmap) качества покрытия.
- Поддерживает загрузку и визуализацию сохранённых драйв-тестов из JSON-файла.

---

## Архитектура

Проект разделён на логические модули:

- **core** — структуры данных (`RuntimeState`), JSON-парсер, генератор тепловой карты, утилиты OSM-математики.
- **net** — ZMQ-приёмник телеметрии, отправка команд фильтрации.
- **network** — загрузка и декодирование OSM-тайлов через libcurl + stb_image.
- **db** — минимальная обвязка PostgreSQL через libpq.
- **ui** — окна и виджеты интерфейса (Data, Graphs, Map, Telephony).
- **gui_app** — инициализация SDL3/OpenGL, главный цикл рендеринга.
- **utils/logging** — система логирования в консоль и файл.

### Потоки выполнения

1. **Поток сервера**: `RunZmqReceiver(...)` слушает порт 5558, принимает и парсит JSON-пакеты, обновляет общее состояние (`RuntimeState`).
2. **Главный поток GUI**: инициализирует окно SDL3 + OpenGL, рендерит интерфейс через ImGui, читает `RuntimeState` для отображения данных.
3. **Worker pool** (4 потока): асинхронная загрузка OSM-тайлов.
4. **Поток генерации heatmap**: асинхронная генерация тепловой карты по нажатию кнопки.

---

## Структура репозитория

```text
.
├── CMakeLists.txt              # Система сборки CMake
├── README.md                   # Документация
├── setup_db.ps1                # Скрипт создания БД PostgreSQL
├── .gitignore                  # Исключения Git
├── .gitattributes              # Настройки Git
├── .gitmodules                 # Подмодули
├── include/                    # Заголовочные файлы
│   ├── core/                   #   - runtime_state, heatmap_generator, json_parser
│   ├── db/                     #   - pgsql_minimal
│   ├── net/                    #   - zmq_receiver
│   ├── network/                #   - osm_tile_fetcher, osm_tile_texture
│   ├── ui/                     #   - ui_components, ui_widget
│   ├── gui_app.h               #   - GUI lifecycle
│   ├── gui_tabs.h              #   - вкладки интерфейса
│   ├── logging.h               #   - логирование
│   └── osm_math.h              #   - OSM-координаты
├── src/                        # Исходный код
│   ├── core/                   #   - main, json_parser, heatmap_generator
│   ├── db/                     #   - pgsql_minimal
│   ├── net/                    #   - zmq_receiver
│   ├── network/                #   - osm_tile_fetcher, osm_tile_texture
│   ├── ui/                     #   - data_window, graph_window, map_window, menu_bar, ui_widget
│   ├── gui_app.cpp             #   - GUI lifecycle
│   ├── gui_tabs.cpp            #   - вкладки интерфейса
│   └── logging.cpp             #   - логирование
├── third_party/                # Сторонние библиотеки
│   ├── imgui/                  #   - ImGui (Docking branch)
│   ├── implot/                 #   - ImPlot
│   └── stb/                    #   - stb_image, stb_image_write
├── fonts/                      # Шрифты
│   └── jetbrains.ttf           #   - JetBrains Mono
├── drive_test_log.json         # Тестовые данные драйв-теста
└── build/                      # Сборочная директория (не в Git)

---

## Сборка и запуск
Требования:

CMake >= 3.16
Компилятор C++20 (MSVC 2022, GCC 13+, Clang 16+)
vcpkg — менеджер пакетов (рекомендуется)
Библиотеки (устанавливаются через vcpkg):
sdl3 — оконная система и OpenGL-контекст
glew — загрузка OpenGL-расширений
cppzmq, libzmq — ZeroMQ
nlohmann-json — парсинг JSON
curl — загрузка OSM-тайлов
libpqxx — клиент PostgreSQL
PostgreSQL 18 — база данных (опционально)

---

## Быстрый старт (Windows)

# 1. Клонировать репозиторий
git clone <https://github.com/WillzKing/VPaHMDCS.git>
cd imgui_demo

# 2. Установить зависимости через vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install sdl3 glew cppzmq nlohmann-json curl libpqxx
cd ..

# 3. Собрать проект
cmake -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build

# 4. Запустить
.\build\Debug\backend_control.exe

---

## Формат входных данных
Backend принимает JSON-пакеты через ZMQ PULL-сокет на порту 5558. Поддерживаются следующие форматы полей:

Местоположение
{
  "location": {
    "latitude": 55.0084,
    "longitude": 82.9357,
    "altitude": 120.5,
    "accuracy": 5.0,
    "time": 1716800000000
  }
}

Сотовые сети
{
  "telephony": [
    {
      "type": "LTE",
      "pci": 465,
      "rsrp": -95,
      "rsrq": -12,
      "rssi": -70,
      "earfcn": 1300,
      "tac": 12345,
      "cellId": "12345678",
      "mcc": "250",
      "mnc": "01"
    }
  ]
}

Сетевой трафик
{
  "networkUsage": {
    "totalBytesSent": 1048576,
    "totalBytesReceived": 5242880,
    "topApps": [
      {"packageName": "com.example.app", "bytes": 512000}
    ]
  }
}

---

## Возможности

Местоположение
Приём координат (latitude, longitude, altitude, accuracy, timestamp)
Накопление траектории движения
Отображение текущей позиции на карте OSM
Кнопка «Center on GPS» для центрирования карты

Сотовые сети
Отображение всех видимых сот (PCI, RSRP, RSRQ, RSSI, TAC, Cell ID)
Графики изменения сигнала по времени для каждой соты
Фильтрация по PCI и EARFCN для тепловой карты
Команды фильтрации (GPS/LTE/NR) на Android-клиент

OpenStreetMap
Загрузка тайлов с tile.openstreetmap.org
Асинхронная загрузка в 4 потока
Кэширование тайлов на диске (build/<zoom>/<x>/<y>.png)
Поддержка зумов 3-19
Перетаскивание карты мышью и зум колёсиком
Тепловая карта (Heatmap)
Интерполяция методом IDW (Inverse Distance Weighting)
Настраиваемые параметры: радиус, степень, критерий (RSRP/RSRQ/RSSI/SINR/Altitude)
Два режима: общая картинка или по OSM-тайлам
Генерация в отдельном потоке (без зависания GUI)
Наложение на карту с настраиваемой прозрачностью

Драйв-тест
Загрузка сохранённого JSON-файла драйв-теста
Отображение траектории на карте
Графики RSRP по PCI для локальных данных

База данных PostgreSQL
Автоматическое создание таблицы user_equipment
Вставка каждой новой записи телеметрии
Хранение: IMEI, координаты, метка времени, параметры сети, метрики сигнала