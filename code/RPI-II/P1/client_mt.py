import socket

SERVER = "127.0.0.1"
PORT = 8080

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((SERVER, PORT))
client.sendall(bytes("Hola, soy un cliente!!",'UTF-8'))

while True:
  in_data =  client.recv(1024)

  print("Desde el servidor :" ,in_data.decode())
  out_data = input()
  client.sendall(bytes(out_data,'UTF-8'))

  if out_data=='end':
    break

client.close()
