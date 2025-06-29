import random

sentences = [
    "The sun is shining brightly today.", "Aziz is learning embedded systems.",
    "She walked her dog in the evening.", "This book is really interesting.",
    "We are going to Germany next year.", "The code compiled without errors.",
    "He drinks coffee every morning.", "The cat jumped onto the sofa.",
    "They watched a movie together.", "I forgot my umbrella at home."
]

with open("data.txt", "w") as f:
    for _ in range(10000):
        sentence = random.choice(sentences)
        f.write(sentence + '\n')
