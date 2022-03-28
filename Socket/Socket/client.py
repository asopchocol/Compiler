from socket import *

s = socket(AF_INET, SOCK_STREAM)
s.connect(('localhost', 8080))
s.send('Send Data\n')
s.close()
