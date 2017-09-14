import random
import sys

n = int(sys.argv[1])
i = 0
while True:
    if i >= n:
        break;
    print random.randint(-sys.maxint-1, sys.maxint)    
    i += 1
