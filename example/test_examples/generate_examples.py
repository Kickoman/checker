# Generate additions examples (100)
from random import randint
from sys import stdout

for i in range(100):
    stdout.write(f'\rGenerating {i / 100}%')

    a = randint(1, 500)
    b = randint(1, 500)
    c = a + b
    with open(f'addition-{i}.in', 'w') as f:
        f.write(f'{a} {b}')
    with open(f'addition-{i}.out', 'w') as f:
        f.write(f'{c}')
stdout.write('\rGemerated 100%\t\t\t')
