zastoj.py svake sekunde ima 20% sanse da izadje s sys.exit(1) i potrebno vrijeme za ponovno pokretanje je izmedju 0.05 i 0.6 sekundi.

posalji.py je proces koji se spaja na rabbitmq i svake pola sekune šalje nasumican broj u kanal.

primi.py je proces koji se spaja na rabbit.mq i osluskuje poruke. Kada primi 2 broja, zbroji ih i ispise.