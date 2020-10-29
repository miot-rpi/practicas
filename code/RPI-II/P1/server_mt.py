import socket, threading

class ClientThread(threading.Thread):
    def __init__(self,clientAddress,clientsocket):
        threading.Thread.__init__(self)
        self.csocket = clientsocket
        print ("Nueva conexion anyadida: ", clientAddress)
    def run(self):
        print ("Conexion desde: ", clientAddress)
        #self.csocket.send(bytes("Hi, This is from Server..",'utf-8'))
        msg = ''
        while True:
            data = self.csocket.recv(2048)
            msg = data.decode()

            if msg=='bye':
              break

            print ("Desde el cliente", msg)
            self.csocket.send(bytes(msg,'UTF-8'))

        print ("Cliente ", clientAddress , " desconectado...")

LOCALHOST = "127.0.0.1"
PORT = 8080

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((LOCALHOST, PORT))

print("Servidor arrancado...")
print("Esperando petición de clientes...")

server.listen(1)

while True:
    clientsock, clientAddress = server.accept()
    newthread = ClientThread(clientAddress, clientsock)
    newthread.start()
