import pika
import random
import time

time.sleep(5)


rabbitmq_user = "user"
rabbitmq_pass = "password"

credentials = pika.PlainCredentials(rabbitmq_user, rabbitmq_pass)

for i in range(10):  # Retry mehanizam
    try:
        connection = pika.BlockingConnection(pika.ConnectionParameters(
            host='rabbitmq',
            credentials=credentials
        ))
        break
    except pika.exceptions.AMQPConnectionError:
        print("Pokusaj ponovo")
        time.sleep(5)
else:
    print("Kraj programa")
    exit(1)

channel = connection.channel()
channel.queue_declare(queue='brojevi')

brojevi = []

def poruka(ch, method, properties, body):
    brojevi.append(int(body))
    print(" [x] Received", int(body))
    if len(brojevi) >= 2:
        print(brojevi[0]," + ",brojevi[1]," = ",brojevi[0]+brojevi[1])
        brojevi.pop(0)
        brojevi.pop(0)


channel.basic_consume(queue='brojevi', on_message_callback=poruka, auto_ack=True)
channel.start_consuming()

while True:
    continue