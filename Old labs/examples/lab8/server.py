import zmq
import json
from datetime import datetime
import os


class ZMQServer:
    def __init__(self, port=5556):
        self.port = port
        self.message_counter = 0
        self.data_file = "android_messages.json"
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)

    def start(self):
        """Запуск сервера"""
        self.socket.bind(f"tcp://*:{self.port}")
        print(f"ZMQ Server started on port {self.port}")
        print("Waiting for Android connections...")

        try:
            while True:
                
                message = self.socket.recv_string()
                self.message_counter += 1

                print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]")
                print(f"Received message #{self.message_counter}: {message}")

                self.save_to_file(message)

             
                response = f"Hello from Server! Received: {self.message_counter} messages"

           
                self.socket.send_string(response)

        except KeyboardInterrupt:
            print("\nServer stopped by user")
        except Exception as e:
            print(f"Error: {e}")
        finally:
            self.cleanup()

    def save_to_file(self, message):
        """Сохранение сообщения в файл"""
        data = {
            "timestamp": datetime.now().isoformat(),
            "message": message,
            "counter": self.message_counter
        }

       
        existing_data = []
        if os.path.exists(self.data_file):
            try:
                with open(self.data_file, 'r') as f:
                    existing_data = json.load(f)
            except:
                existing_data = []

  
        existing_data.append(data)

     
        with open(self.data_file, 'w') as f:
            json.dump(existing_data, f, indent=2)

        print(f"Message saved to {self.data_file}")

    def display_all_data(self):
        """Вывод всех сохраненных данных на экран"""
        if not os.path.exists(self.data_file):
            print("No data file found")
            return

        try:
            with open(self.data_file, 'r') as f:
                data = json.load(f)

            print("\n" + "=" * 50)
            print("ALL SAVED MESSAGES:")
            print("=" * 50)

            for item in data:
                print(f"\nTimestamp: {item['timestamp']}")
                print(f"Message #{item['counter']}: {item['message']}")
                print("-" * 30)

            print(f"\nTotal messages: {len(data)}")

        except Exception as e:
            print(f"Error reading data: {e}")

    def cleanup(self):
        """Очистка ресурсов"""
        self.socket.close()
        self.context.term()
        print("Server resources cleaned up")


def main():
    server = ZMQServer(port=5556)

    try:
       
        import threading
        server_thread = threading.Thread(target=server.start)
        server_thread.daemon = True
        server_thread.start()

        print("Commands:")
        print("  'show' - Display all saved messages")
        print("  'exit' - Stop the server")
        print("  'count' - Show message count")

        while True:
            command = input("\nEnter command: ").strip().lower()

            if command == 'show':
                server.display_all_data()
            elif command == 'count':
                print(f"Total messages received: {server.message_counter}")
            elif command == 'exit':
                print("Stopping server...")
                break
            else:
                print("Unknown command")

    except KeyboardInterrupt:
        print("\nServer stopped")
    finally:
        server.cleanup()


if __name__ == "__main__":
    main()