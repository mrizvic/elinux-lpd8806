#!/usr/bin/env python

import websocket
import time
import sys
import os
import signal


def on_message(ws, message):

    '''
    message format: 

    FROM TO ARGUMENTS

    FROM:=name
    TO:=name
    ARGUMENTS:=object.variable=value ...

    example:
    browser raspi lpd8806.sparking1=40 lpd8806.sparking2=90 lpd8806.cooling1=160 lpd8806.cooling1=200 lpd8806.command=reload
    '''

    pid = 17909

    args=message.split(' ')
    sender=args[4]
    rcpt=args[5]
    params=args[6:]
    if rcpt=='raspi':
        for var in params:
            line=var.split('.')
            obj=line[0]
            arg=line[1].split('=')
            var=arg[0]
            value=arg[1]
            print 'object {} variable {} value {}'.format(obj,var,value)

            if obj=='lpd8806' and var=='sparking1':
                print 'setting lpd8806.sparking1 to {}'.format(value)
                f=open('/tmp/lpd8806-fire-sparking1','w')
                f.write(value)
                f.close()
            
            if obj=='lpd8806' and var=='cooling1':
                print 'setting lpd8806.cooling1 to {}'.format(value)
                f=open('/tmp/lpd8806-fire-cooling1','w')
                f.write(value)
                f.close()

            if obj=='lpd8806' and var=='sparking2':
                print 'setting lpd8806.sparking2 to {}'.format(value)
                f=open('/tmp/lpd8806-fire-sparking2','w')
                f.write(value)
                f.close()
            
            if obj=='lpd8806' and var=='cooling2':
                print 'setting lpd8806.cooling2 to {}'.format(value)
                f=open('/tmp/lpd8806-fire-cooling2','w')
                f.write(value)
                f.close()

            if obj=='lpd8806' and var=='command' and value=='reload':
                print 'sending SIGUSR1 to {}'.format(pid)
                os.kill(pid, signal.SIGUSR1)

def on_error(ws, error):
    print(error)


def on_close(ws):
    ws.send('Bye from raspi')
    print("### closed ###")


def on_open(ws):
    ws.send("Hello from raspi")

if __name__ == "__main__":
    websocket.enableTrace(True)
    if len(sys.argv) < 2:
        host = "ws://localhost:8000/"
    else:
        host = sys.argv[1]
    ws = websocket.WebSocketApp(host,
                                on_message = on_message,
                                on_error = on_error,
                                on_close = on_close)
    ws.on_open = on_open
    ws.run_forever()
