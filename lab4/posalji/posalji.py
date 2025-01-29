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

while True:
    channel.basic_publish(exchange='',
                        routing_key='brojevi',
                        body=str(random.randint(1, 11)))
    print(" [x] Sent random number")

    time.sleep(0.5)

connection.close()