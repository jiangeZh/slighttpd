#python2.7.6
#coding=utf-8

import socket

if __name__ == "__main__":
    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockfd.connect(('localhost', 8000))
    message = ""
    while 1:
        message = raw_input("Please input:")
        sockfd.send(message)
        message = sockfd.recv(8000)
        print message
    sockfd.close()
