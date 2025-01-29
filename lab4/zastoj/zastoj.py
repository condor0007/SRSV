import time
import random
import sys

while True:
    print("Unstable service running...", time.time())
    
    if random.random() < 0.2:  
        print("Unstable service crashed!", time.time())
        sys.exit(1) 
    
    time.sleep(1)
