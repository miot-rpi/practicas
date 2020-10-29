import binascii
import socket
import struct
import sys

# Socket TCP
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('localhost', 10001)
sock.connect(server_address)

packed_data = struct.pack("=iif", 1, 4, 2.7)

try:
    # Envio de datos
    print('Enviando "%s"' % binascii.hexlify(packed_data))
    sock.sendall(packed_data)

finally:
    print('Cerrando socket')
    sock.close()
