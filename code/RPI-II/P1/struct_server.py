import binascii
import socket
import struct
import sys

# Socket TCP
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('localhost', 10001)
sock.bind(server_address)
sock.listen(1)

while True:
    print('Esperando conexiones entrantes')
    connection, client_address = sock.accept()
    try:
        data = connection.recv(1024)
        print('Recibido "%s"' % binascii.hexlify(data))

        unpacked_data = struct.unpack("=iif", data)
        print('Desempaquetado:', unpacked_data)
        
    finally:
        connection.close()
