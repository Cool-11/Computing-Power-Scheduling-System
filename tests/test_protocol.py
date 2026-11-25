"""Small interactive contract test. Run with: python3 tests/test_protocol.py /path/to/binary"""

import heapq
import subprocess
import sys


def encode(text: str, frequencies=(1,) * 9):
    heap = [(weight, i) for i, weight in enumerate(frequencies)]
    heapq.heapify(heap)
    children = {}
    next_id = 9
    while len(heap) > 1:
        wa, a = heapq.heappop(heap)
        wb, b = heapq.heappop(heap)
        children[next_id] = (a, b)
        heapq.heappush(heap, (wa + wb, next_id))
        next_id += 1
    codes = {}

    def visit(node, bits):
        if node < 9:
            codes["abcdefgh_"[node]] = bits
        else:
            left, right = children[node]
            visit(left, bits + "0")
            visit(right, bits + "1")

    visit(heap[0][1], "")
    bits = "".join(codes[c] for c in text)
    padded = bits + "0" * ((-len(bits)) % 4)
    return len(bits), "".join(f"{int(padded[i:i+4], 2):x}" for i in range(0, len(padded), 4))


def exchange(process, input_text, expected_ans, expected_commands=()):
    process.stdin.write(input_text)
    process.stdin.flush()
    answer = process.stdout.readline().strip()
    assert answer == expected_ans, (answer, expected_ans)
    commands = []
    while True:
        line = process.stdout.readline().strip()
        assert line, "missing END or output was not flushed"
        if line == "END":
            break
        commands.append(line)
    if expected_commands is not None:
        assert commands == list(expected_commands), commands


def main(binary):
    process = subprocess.Popen(
        [binary], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1
    )
    init = """4 1 1 1 1 1 1 1 2 2 0
10 10 10 2 3
5
0 0 10
0 4 8 10
-1 4
3
1 0
1 1 1 1 1 1 1 1 1
a 2
aa 3
"""
    process.stdin.write(init)
    process.stdin.flush()
    exchange(
        process,
        """TICK 1 1 7
WF 100000000 0 200 4 1
200000000 2 2 1 0 -1 0 0 0
BILL 0
HEAD 0
ROUTE 0 0
ALARM 0
SLACK 200000000
CACHE 0
RCACHE 0
""",
        "ANS 0 4 0 0 0 0 0",
        ["RUN 200000000 0"],
    )
    bits, hex_text = encode("aa_a")
    exchange(
        process,
        f"TICK 2 1 4\nLOG 0 {bits} {hex_text}\nBILL 0\nHEAD 0\nALARM 0\nCACHE 0\n",
        "ANS 16 2 9 3",
    )
    exchange(
        process,
        "TICK 3 1 4\nDONE 200000000\nBILL 0\nHEAD 0\nALARM 0\nRCACHE 0\n",
        "ANS 26 4 9 3",
    )
    exchange(
        process,
        "TICK 4 1 3\nDCDOWN 0\nROUTE 0 0\nALARM 0\nBILL 0\n",
        "ANS -1 0 26",
    )
    process.stdin.close()
    assert process.wait(timeout=5) == 0
    sample = subprocess.Popen(
        [binary], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1
    )
    sample.stdin.write("""10 1 3 1 2 3 4 2 4 3 2
24 79 12 2 1
3
0 2 3525
0 8 32 562
0 8 64 751
1 0 3
2 0 1
-1 16
0 10
0 35
75 78 40 49
3 0 2 3
2 0 2
3 2 1 3 5 3 1 1 2
gedf 2
fhge 2
bgahbhfchea 79
abbhe 2
""")
    sample.stdin.flush()
    exchange(
        sample,
        """TICK 1 2 4
WF 485263228 2 2613 10 3
786995192 4 16 2 0 0 4 4 0
296820426 1 4 2 1 -1 0 6 1 786995192
316705972 8 8 2 0 0 32 11 1 786995192
WF 515439207 2 2036 16 3
915605826 8 32 3 1 0 9 6 0
311844768 4 4 3 0 -1 0 2 1 915605826
114606478 4 16 2 0 -1 0 11 1 915605826
SLACK 296820426
HEAD 1
ALARM 1
RCACHE 0
""",
        "ANS 0 10 0 0",
        None,
    )
    sample.stdin.close()
    assert sample.wait(timeout=5) == 0

    network = subprocess.Popen(
        [binary], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1
    )
    network.stdin.write("""3 2 2 2 1 2 1 1 1 2 1
10 10 10 1 1
1 2
0 0 0
1 1 0
0 1 1 0
0 1 7
-1 5
0 3
1
1 0
1 1 1 1 1 1 1 1 1
a 1
""")
    network.stdin.flush()
    exchange(network, "TICK 1 0 2\nROUTE 0 1\nHEAD 1\n", "ANS 7 3")
    exchange(
        network,
        "TICK 2 2 2\nLDOWN 0\nQUOTA 1 1\nROUTE 0 1\nHEAD 1\n",
        "ANS -1 1",
    )
    exchange(
        network,
        "TICK 3 3 2\nDCDOWN 1\nLUP 0\nUNDO 1\nROUTE 0 1\nHEAD 1\n",
        "ANS -1 3",
    )
    network.stdin.close()
    assert network.wait(timeout=5) == 0
    print("interactive contract test passed")


if __name__ == "__main__":
    main(sys.argv[1])
