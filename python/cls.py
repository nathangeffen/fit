def outy():
    x = 3

    def inny(y):
        y += 1
        x += 1
        print("In:", x, y)

    y = 0
    inny(y)
    print("Out 1:", x, y)
    inny(y)
    print("Out 2:", x, y)
    inny(x)
    print("Out 3:", x, y)

outy()
