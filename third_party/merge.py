import json
import os
from datetime import datetime

def combine_telemetry_compact(input_dir, output_file):
    combined_data = []
    
    # Собираем все json файлы
    files = [f for f in os.listdir(input_dir) if f.endswith('.json')]
    print(f"Найдено файлов: {len(files)}")

    for filename in files:
        if filename == output_file: continue  # Чтобы не сожрать самого себя
        
        file_path = os.path.join(input_dir, filename)
        with open(file_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line: continue
                try:
                    combined_data.append(json.loads(line))
                except json.JSONDecodeError:
                    try:
                        f.seek(0)
                        data = json.load(f)
                        if isinstance(data, list):
                            combined_data.extend(data)
                        else:
                            combined_data.append(data)
                        break
                    except:
                        print(f"Пропущен битый файл: {filename}")
                        break

    # Сортировка (опционально, но полезно для логов)
    combined_data.sort(key=lambda x: x.get('timestamp', 0))

    # Пишем максимально компактно
    with open(output_file, 'w', encoding='utf-8') as out_f:
        json.dump(
            combined_data, 
            out_f, 
            ensure_ascii=False,   # Оставляем нормальные буквы
            separators=(',', ':') # Убираем ВСЕ пробелы (самая важная часть для веса)
        )
    
    final_size = os.path.getsize(output_file) / (1024 * 1024)
    print(f"Готово! Итоговый вес: {final_size:.2f} МБ")

# Настройки
input_folder = './' 
output_filename = 'combined_telemetry.json'

if __name__ == "__main__":
    combine_telemetry_compact(input_folder, output_filename)