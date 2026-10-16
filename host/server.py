import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(('0.0.0.0', 8080))
server.listen(1)

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("8.8.8.8", 80))
local_ip = s.getsockname()[0]
s.close()

print("IP PC: ", local_ip)
print("Waiting for the connection on the port 8080")

conn, addr = server.accept()
print(f"Connected: {addr}")

while True:
    data = conn.recv(1024)
    if data:
        print(data.decode('utf-8'), end='', flush=True)