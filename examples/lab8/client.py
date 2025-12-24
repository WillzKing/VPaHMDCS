import socket


def start_client():
    host = "127.0.0.1"
    port = 12345

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((host, port))


    message = "Hello World от клиента!"
    client.send(message.encode('utf-8'))


    response = client.recv(1024)
    print(f"[*] Ответ от сервера: {response.decode('utf-8')}")

    client.close()


if __name__ == "__main__":
    start_client()