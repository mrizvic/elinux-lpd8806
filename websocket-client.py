import websocket
import time
import sys


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
            
        


def on_error(ws, error):
    print(error)


def on_close(ws):
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
