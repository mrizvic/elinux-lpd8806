#!/usr/bin/env python

import paho.mqtt.client as mqtt
import os
import signal

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, rc):
    print(str(rc))
    client.subscribe("sys/lpd8806/#")


def on_message(client, userdata, message):

    '''
    message format:

    FROM TO ARGUMENTS

    FROM:=name
    TO:=name
    ARGUMENTS:=object.variable=value ...

    example:
    lpd8806.pid=22350 lpd8806.sparking1=40 lpd8806.sparking2=90 lpd8806.cooling1=160 lpd8806.cooling1=200 lpd8806.command=reload
    '''

    pid=22350

    topic = message.topic
    payload = int(message.payload)
    value = message.payload
    
    msg = message.payload
    params=msg.split(' ')

    if payload < 10:
	value = str(10)
    elif payload > 300:
        value = str(300)

    if topic == 'sys/lpd8806/sparking':
        print 'setting sparking to {}'.format(value)
        f=open('/tmp/lpd8806-fire-sparking1','w')
        f.write(value)
        f.close()
        f=open('/tmp/lpd8806-fire-sparking2','w')
        f.write(value)
        f.close()

    if topic == 'sys/lpd8806/cooling':
        print 'setting cooling to {}'.format(value)
        f=open('/tmp/lpd8806-fire-cooling1','w')
        f.write(value)
        f.close()
        f=open('/tmp/lpd8806-fire-cooling2','w')
        f.write(value)
        f.close()

    os.kill(pid, signal.SIGUSR1)

mqttsrv = os.environ.get('MQTTSRV')
if mqttsrv is None:
    raise ValueError('please set MQTTSRV environment variable')

client = mqtt.Client(client_id=__file__)
client.on_connect = on_connect
client.on_message = on_message

#client.tls_set("/etc/pki/tls/certs/startcom-ca.crt")
client.connect(mqttsrv, 1883, 60)

client.loop_forever()

