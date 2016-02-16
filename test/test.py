#python2.7.6
#coding=utf-8

import socket
import time
import threading

thread_num = 1000

http_request = "POST /htdocs/index.html HTTP/1.1\r\nHost:test.py\r\nConnection:Keep-Alive\r\nContent-Length:5\r\n\r\nHello"

def make_a_request():
    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockfd.connect(('localhost', 8000))
    sockfd.sendall(http_request)
    sockfd.recv(8000)
    #sockfd.sendall(http_request)
    #sockfd.recv(8000)
    #time.sleep(5)
    sockfd.close()

if __name__ == "__main__":
    thread_list = []
        
    start_time = time.time()

    for i in range(0, thread_num):
        thread = threading.Thread(target = make_a_request)
        thread_list.append(thread)

    for thread in thread_list:
        thread.start()

    for thread in thread_list:
        thread.join()
    
    print "Time used for %d request: " %thread_num, time.time() - start_time
