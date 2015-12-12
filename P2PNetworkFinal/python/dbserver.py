# Echo server program
import socket
import sqlite3
import os
import pdb

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 12345              # Arbitrary non-privileged port

print "Starting server. Please wait."
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
conn, addr = s.accept()
while 1:
    msg = conn.recv(128)
    conn.sendall("hello: "+ msg)
    if msg: break;
conn.close()


#pdb.set_trace()
conn, addr = s.accept()
print 'Connected by', addr
while 1:
    data = conn.recv(128)
    conn.send("hello: " + data)
    #data is the SQL statement
    data = conn.recv(1024)
    localDatabase = sqlite3.connect('localDB.db')
    cursor = localDatabase.cursor()

    #remove the current data table if it already exists
    cursor.execute("drop table if exists DATA")
    cursor.execute("create table DATA(ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, AGE INT NOT NULL)")
    cursor.execute(data)
    localDatabase.commit()
    localDatabase.close()
    conn.sendall("Got it.")
    if data: break
conn.close()

#close the socket
s.close()

#check if database exists already --> if it does, delete it
if(os.path.exists("release\localDB.db")):
    os.system("del release\localDB.db")
#move the database file to the folder where the P2PNetworkFinal.exe is located
os.rename("localDB.db", "release\localDB.db")
#run the P2PNetworkFinal.exe
os.system("release\P2PNetworkFinal.exe")
