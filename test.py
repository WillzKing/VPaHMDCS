import json
import psycopg2
import os

# НАСТРОЙКИ ПОДКЛЮЧЕНИЯ К БД
DB_CONFIG = {
    'host': 'localhost',
    'port': '5432',
    'dbname': 'backend_control',
    'user': 'postgres',
    'password': '123'  # Ваш пароль от PostgreSQL
}

JSON_FILE = 'drive_test_log — копия (2).txt'  # Имя вашего файла

def main():
    if not os.path.exists(JSON_FILE):
        print(f"❌ Ошибка: Файл '{JSON_FILE}' не найден!")
        return

    print(f"📂 Чтение файла: {JSON_FILE}...")
    
    # Подключение к базе данных
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()
        print("✅ Подключение к PostgreSQL успешно")
        
        # Создаем таблицу кэша вышек, если её нет
        cur.execute("""
            CREATE TABLE IF NOT EXISTS cell_tower_cache (
                id SERIAL PRIMARY KEY,
                mcc VARCHAR(10) NOT NULL,
                mnc VARCHAR(10) NOT NULL,
                cell_id VARCHAR(50) NOT NULL,
                lac_tac VARCHAR(50),
                latitude DOUBLE PRECISION,
                longitude DOUBLE PRECISION,
                source VARCHAR(20) DEFAULT 'local_import',
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(mcc, mnc, cell_id, lac_tac)
            );
            CREATE INDEX IF NOT EXISTS idx_tower_lookup ON cell_tower_cache(mcc, mnc, cell_id);
        """)
        conn.commit()
    except Exception as e:
        print(f"❌ Ошибка подключения к БД: {e}")
        return

    count_inserted = 0
    count_skipped = 0
    count_no_gps = 0
    
    with open(JSON_FILE, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line: continue
            
            try:
                data = json.loads(line)
                
                # 1. Получаем GPS координаты из этой записи
                loc = data.get('location')
                if not loc or loc.get('latitude') == 0 or loc.get('longitude') == 0:
                    count_no_gps += 1
                    continue
                
                lat = loc['latitude']
                lon = loc['longitude']
                
                # 2. Проходим по всем вышкам в этой записи
                telephony = data.get('telephony', [])
                for cell in telephony:
                    identity = None
                    # Поддержка разных типов вышек (LTE, GSM, NR)
                    if 'CellIdentityLte' in cell: identity = cell['CellIdentityLte']
                    elif 'CellIdentityGSM' in cell: identity = cell['CellIdentityGSM']
                    elif 'CellIdentityNr' in cell: identity = cell['CellIdentityNr']
                    
                    if not identity:
                        continue
                    
                    # Извлекаем ключевые поля (учитываем разный регистр букв)
                    mcc = str(identity.get('MCC') or identity.get('mcc') or '')
                    mnc = str(identity.get('MNC') or identity.get('mnc') or '')
                    
                    # Cell ID может называться по-разному
                    cell_id = str(identity.get('CellIdentity') or identity.get('cellIdentity') or 
                                  identity.get('cid') or identity.get('cellId') or '')
                    
                    # LAC/TAC
                    lac = str(identity.get('TAC') or identity.get('tac') or 
                              identity.get('LAC') or identity.get('lac') or '')

                    # Пропускаем, если нет основных идентификаторов
                    if not mcc or not mnc or not cell_id or mcc == 'N/A' or mnc == 'N/A':
                        continue

                    # Вставляем в БД (игнорируем дубликаты)
                    try:
                        cur.execute("""
                            INSERT INTO cell_tower_cache (mcc, mnc, cell_id, lac_tac, latitude, longitude, source)
                            VALUES (%s, %s, %s, %s, %s, %s, 'json_import')
                            ON CONFLICT (mcc, mnc, cell_id, lac_tac) DO UPDATE SET
                                latitude = EXCLUDED.latitude,
                                longitude = EXCLUDED.longitude,
                                source = 'json_import_updated'
                        """, (mcc, mnc, cell_id, lac, lat, lon))
                        count_inserted += 1
                    except Exception as e:
                        pass # Ошибка вставки
                        
            except json.JSONDecodeError:
                continue
            except Exception as e:
                print(f"⚠️ Ошибка в строке {line_num}: {e}")

    conn.commit()
    print(f"\n🎉 ГОТОВО!")
    print(f"✅ Добавлено/Обновлено вышек в базе: {count_inserted}")
    print(f"⏭️ Пропущено (нет GPS или идентификаторов): {count_skipped + count_no_gps}")
    print(f"\nТеперь ваш C++ сервер сможет делать триангуляцию офлайн!")
    
    cur.close()
    conn.close()

if __name__ == "__main__":
    main()