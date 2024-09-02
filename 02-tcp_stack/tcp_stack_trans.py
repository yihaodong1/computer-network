import sys
import string
import socket
from time import sleep
import pdb
import os
data = string.digits + string.ascii_lowercase + string.ascii_uppercase
pkt_size = 1000
def server(port):
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    s.bind(('0.0.0.0', int(port)))
    s.listen(3)
    
    cs, addr = s.accept()
    print(addr)
    
    # while True:
    #     data = cs.recv(1000)
        
    #     if data:
    #         data = "server echoes: " + data.decode()
    #         cs.send(data.encode())
    #     else:
    #         break
    recv_file("server-output.dat", cs)
    s.close()


def client(ip, port):
    s = socket.socket()
    s.connect((ip, int(port)))
    
    # for i in range(3):
    #     new_data = data[i:] + data[:i+1]
    #     s.send(new_data.encode())
    #     print(s.recv(1000).decode())
    #     sleep(1)
    send_file("client-input.dat", s)
    s.close()

def send_file(filepath, s):
    try:
        with open(filepath, 'rb') as file:
            while True:
                data = file.read(pkt_size)  
                if not data:
                    break  
                s.send(data)
                sleep(1)
    finally:
        pass
def recv_file(save_path, s):
    if os.path.exists(save_path):
        os.remove(save_path)
    try:
        with open(save_path, 'ab') as file:
            while True:
                data = s.recv(pkt_size)  
                if not data:
                    break  
                file.write(data)
        print('File received and saved as', save_path)
    finally:
        pass

if __name__ == '__main__':
    if sys.argv[1] == 'server':
        server(sys.argv[2])
    elif sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])

