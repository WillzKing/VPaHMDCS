import socket
import threading


def handle_client(client_socket, addr):
    print(f"[+] Подключение от {addr}")


    data = client_socket.recv(1024)
    print(f"[*] Получено от клиента: {data.decode('utf-8')}")


    response = "Hello World от сервера!"
    client_socket.send(response.encode('utf-8'))

    client_socket.close()
    print(f"[-] Отключение от {addr}")


def start_server():
    host = "127.0.0.1"
    port = 12345

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen(5)

    print(f"[*] Сервер запущен на {host}:{port}")

    while True:
        client, addr = server.accept()
        thread = threading.Thread(target=handle_client, args=(client, addr))
        thread.start()


if __name__ == "__main__":
    start_server()