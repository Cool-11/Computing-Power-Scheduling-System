"""Generate a large protocol/performance input (not an OJ world)."""

import sys


def main():
    out = sys.stdout
    out.write("600 2 2 2 360 1 1 1 1 20 1\n")
    out.write("10 10 10 1 1\n1 2\n0 0 10\n1 1 10\n")
    for x in range(360):
        out.write(f"{x % 2} 8 8 10\n")
    out.write("0 1 1\n-1 100000\n1\n1 0\n")
    out.write("1 1 1 1 1 1 1 1 1\na 1\n")
    for tick in range(1, 601):
        out.write(f"TICK {tick} 100 0\n")
        for i in range(100):
            n = (tick - 1) * 100 + i
            out.write(f"WF {100000000+n} 0 1000 600 1\n")
            out.write(f"{200000000+n} 1 1 2 0 -1 0 0 0\n")


if __name__ == "__main__":
    main()
