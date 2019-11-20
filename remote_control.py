from socket import *
from thread import *

server = socket(AF_INET, SOCK_STREAM)
server.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
server.bind(('', 4444))

server.listen(100)

devices = {}

def client_thread(cli, addr):
	d = ord(cli.recv(1))
	if d == 0: # device mode
		d = ord(cli.recv(1))
		name = cli.recv(d)
		devices[name] = cli
		print name
	else: # app
		name = cli.recv(d)
		e = ord(cli.recv(1))
		cmd = cli.recv(e)

		if cmd == "open":
			devices[name].sendall("\x03")
		elif cmd == "close":
			devices[name].sendall("\x04")

while True:
	cli, addr = server.accept()
	start_new_thread(client_thread, (cli, addr))