from random import randint


def main():
    sentences = [
        "The quick brown fox jumps over the lazy dog.",
        "Artificial intelligence is transforming the world rapidly.",
        "Learning Python can open many doors in the tech industry.",
        "He didn't know what to say in that awkward situation.",
        "Tomorrow, we will start working on the new project.",
        "Reading every day improves both vocabulary and comprehension.",
        "Don't forget to commit your changes before pushing the code.",
        "She always drinks coffee before starting her day.",
        "This course is designed to help you master data structures.",
        "Silence sometimes speaks louder than words."
    ]

    with open("data.txt", "w") as f:
        for _ in range(10000):
            sentence = sentences[randint(0, 9)]
            f.write(sentence + '\n')


if __name__ == '__main__':
    main()
