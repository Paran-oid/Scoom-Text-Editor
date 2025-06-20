mystr = "}}}{{{}}}}}"

mystack = []

wrong = 0
for c in mystr:
    if c == '{':
        mystack.append(c)
    else:
        if mystack and mystack[-1] == '{':
            mystack.pop()
        else:
            wrong += 1

print(wrong)
