
#import socket module
from socket import *

serverSocket= socket(AF_INET, SOCK_STREAM)#creates socket

#Prepare a sever socket

#Fill in start

serverPort = 6789 #sets port

serverSocket.bind(('', serverPort)) #associates socket with this port

serverSocket.listen(1) #tells socket to listen for requests

#Fill in end

while True:

#Establish the connection

    print ("Ready to serve…")

    connectionSocket, addr = serverSocket.accept() #creates a socket specifically for this client

    try:

        message = connectionSocket.recv(1024) #receives message from client

        filename = message.split()[1]
        print (filename)
        f = open(filename[1:]) #opens file and reads the contents
        outputdata =f.read()
        print (outputdata)

        #Send one HTTP header line into socket

        #Fill in start
        connectionSocket.send(b'\nHTTP/1.x 200 OK\n') #ok message ready to send
        connectionSocket.send(b'Connection: close\n') #terminate connection after sending
        LengthString = 'Content-Length: '+ str(len(outputdata)) + '\n' #message length
        connectionSocket.send(LengthString.encode()) #send length of the message
        connectionSocket.send(b'Content-Type: text/html\n') #type of message
        connectionSocket.send(b'\n') #blank line
        connectionSocket.send(b'\n') #blank line
        connectionSocket.send(outputdata.encode()) #blank line
        #Fill in end

        #Send the content of the requested file to the client

        connectionSocket.close() #closes the socket for this client

        print ("File Recieved")

    except IOError:

        #Sendresponse message for file not found

        #Fill in start
        connectionSocket.send(b'\n404 File Not Found\n') #sends an error message to be printed on the page
        connectionSocket.close()
        #Fill in end
