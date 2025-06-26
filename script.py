def main():
    mystr = 'A' * 10000

    with open("data.txt", "w") as f:
        f.write(mystr)


if __name__ == '__main__':
    main()
